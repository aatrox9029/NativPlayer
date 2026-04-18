#include "app/application_internal.h"

#include "app/replay_selection_policy.h"

namespace velo::app {

void Application::PlayNext() {
    std::optional<int> nextIndex;
    bool forcePauseAfterLoad = false;
    {
        std::scoped_lock lock(stateMutex_);
        nextIndex = ResolveNextPlaylistIndexLocked(false);
        forcePauseAfterLoad = latestState_.eofReached;
    }
    if (nextIndex.has_value()) {
        PlayPlaylistIndex(*nextIndex, forcePauseAfterLoad);
    } else {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::NoNextItem));
    }
}


void Application::PlayPrevious() {
    {
        std::scoped_lock lock(stateMutex_);
        if (latestState_.positionSeconds > 5.0 && currentPlaylistIndex_ >= 0) {
            playerThread_.SeekAbsolute(0.0);
            mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::RestartedCurrentItem));
            return;
        }
    }

    int previousIndex = -1;
    {
        std::scoped_lock lock(stateMutex_);
        if (currentPlaylistIndex_ > 0) {
            previousIndex = currentPlaylistIndex_ - 1;
        }
    }
    if (previousIndex >= 0) {
        PlayPlaylistIndex(previousIndex);
    } else {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::NoPreviousItem));
    }
}


void Application::ReplayCurrent() {
    int playlistIndex = -1;
    std::wstring currentPath;
    {
        std::scoped_lock lock(stateMutex_);
        currentPath = currentOpenPath_;
        playlistIndex = ResolveReplayPlaylistIndex(currentPlaylist_, currentPlaylistIndex_, currentPath);
    }

    if (playlistIndex >= 0) {
        PlayPlaylistIndex(playlistIndex, false, false, true);
    } else if (!currentPath.empty()) {
        OpenFile(currentPath);
    }
}

void Application::PlayPlaylistIndex(const int index) {
    PlayPlaylistIndex(index, false, false, false);
}

void Application::PlayPlaylistIndex(const int index, const bool forcePauseAfterLoad) {
    PlayPlaylistIndex(index, forcePauseAfterLoad, false, false);
}

void Application::PlayPlaylistIndex(const int index, const bool forcePauseAfterLoad, const bool forcePlayAfterLoad) {
    PlayPlaylistIndex(index, forcePauseAfterLoad, forcePlayAfterLoad, false);
}

void Application::PlayPlaylistIndex(const int index, const bool forcePauseAfterLoad, const bool forcePlayAfterLoad,
                                    const bool suppressResumeRestore) {
    std::wstring pathToOpen;
    bool autoLoadSubtitle = false;
    bool rememberPlaybackPosition = false;
    bool preservePause = false;
    {
        std::scoped_lock lock(stateMutex_);
        if (index < 0 || index >= static_cast<int>(currentPlaylist_.Size())) {
            return;
        }
        currentPlaylistIndex_ = index;
        pendingFileLoad_ = true;
        pathToOpen = currentPlaylist_.PathAt(index);
        if (pathToOpen.empty()) {
            return;
        }
        currentOpenPath_ = pathToOpen;
        if (IsLikelyUrl(pathToOpen)) {
            currentConfig_.lastOpenUrl = pathToOpen;
        } else {
            currentConfig_.lastFile = pathToOpen;
        }
        autoLoadSubtitle = currentConfig_.autoLoadSubtitle;
        rememberPlaybackPosition = currentConfig_.rememberPlaybackPosition;
        preservePause =
            ShouldPauseAfterOpeningItem(currentConfig_.preservePauseOnOpen, latestState_.isPaused, latestState_.eofReached, forcePauseAfterLoad);
        pendingResumePath_ = pathToOpen;
        pendingResumeSeconds_ = ResolvePendingResumeSeconds(rememberPlaybackPosition, suppressResumeRestore, LookupResumeEntry(pathToOpen));
        pendingPauseAfterLoad_ = preservePause;
        lastPersistedPositionSeconds_ = 0.0;
        lastPersistedHistorySeconds_ = 0.0;
        latestState_.currentPath.clear();
        latestState_.isLoaded = false;
        latestState_.eofReached = false;
        latestState_.errorState.clear();
        configService_.ScheduleSave(currentConfig_);
    }

    RememberRecentFile(pathToOpen);
    SyncPlaylistUi();
    mainWindow_.PrepareForIncomingMedia();
    {
        std::scoped_lock lock(stateMutex_);
        latestState_.eofReached = false;
    }
    playerThread_.LoadFile(pathToOpen);
    if (forcePlayAfterLoad) {
        playerThread_.SetPause(false);
    }
    mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::OpeningPrefix) + ShortDisplayName(pathToOpen));

    if (autoLoadSubtitle && !IsLikelyUrl(pathToOpen)) {
        const auto subtitle = FindSidecarSubtitle(pathToOpen);
        if (subtitle.has_value()) {
            playerThread_.LoadSubtitle(subtitle->wstring());
            mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::AutoLoadedSidecarSubtitle));
        }
    }
}


void Application::RememberRecentFile(const std::wstring& path) {
    if (path.empty()) {
        return;
    }
    std::vector<std::wstring> recentFiles;
    {
        std::scoped_lock lock(stateMutex_);
        RememberRecentItem(currentConfig_.recentItems, path, UnixTimestampNow());
        currentConfig_.recentFiles = BuildRecentPaths(currentConfig_.recentItems);
        recentFiles = currentConfig_.recentFiles;
        configService_.ScheduleSave(currentConfig_);
    }
    mainWindow_.SetRecentFiles(std::move(recentFiles));
}


void Application::RemoveRecentFile(const std::wstring& path) {
    if (path.empty()) {
        return;
    }
    std::vector<std::wstring> recentFiles;
    {
        std::scoped_lock lock(stateMutex_);
        velo::app::RemoveRecentItem(currentConfig_.recentItems, path);
        currentConfig_.recentFiles = BuildRecentPaths(currentConfig_.recentItems);
        recentFiles = currentConfig_.recentFiles;
        configService_.ScheduleSave(currentConfig_);
    }
    mainWindow_.SetRecentFiles(std::move(recentFiles));
}


void Application::UpdateResumeEntry(const std::wstring& path, const double positionSeconds, const bool clearEntry) {
    auto& entries = currentConfig_.resumeEntries;
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const velo::config::ResumeEntry& entry) {
                      return SamePathInsensitive(entry.path, path);
                  }),
                  entries.end());
    if (!clearEntry && positionSeconds >= 5.0) {
        entries.insert(entries.begin(), velo::config::ResumeEntry{path, positionSeconds});
        if (entries.size() > 20) {
            entries.resize(20);
        }
    }
}


double Application::LookupResumeEntry(const std::wstring& path) const {
    for (const auto& entry : currentConfig_.resumeEntries) {
        if (SamePathInsensitive(entry.path, path)) {
            return entry.positionSeconds;
        }
    }
    return 0.0;
}


void Application::SyncPlaylistUi() {
    std::scoped_lock lock(stateMutex_);
    const int totalCount = static_cast<int>(currentPlaylist_.Size());
    mainWindow_.SetPlaylistContext(currentPlaylistIndex_, totalCount, temporaryPlaylist_);
}


std::optional<int> Application::ResolveNextPlaylistIndexLocked(const bool forAutoplay) const {
    if (currentPlaylist_.Empty() || currentPlaylistIndex_ < 0) {
        return std::nullopt;
    }
    if (currentPlaylistIndex_ + 1 < static_cast<int>(currentPlaylist_.Size())) {
        return currentPlaylistIndex_ + 1;
    }
    if (currentConfig_.repeatMode == velo::config::RepeatMode::All &&
        (forAutoplay || temporaryPlaylist_ || currentPlaylist_.Size() > 1)) {
        return 0;
    }
    return std::nullopt;
}

}  // namespace velo::app

