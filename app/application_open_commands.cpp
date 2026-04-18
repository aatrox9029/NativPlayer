#include "app/application_internal.h"

namespace velo::app {

void Application::OpenFile(const std::wstring& path) {
    if (path.empty()) {
        return;
    }

    const auto sourceKind = DetectMediaSourceKind(path);
    if (sourceKind == MediaSourceKind::NetworkUrl || sourceKind == MediaSourceKind::DiscProtocol) {
        logger_.Info("Open streaming or disc protocol source");
        {
            std::scoped_lock lock(stateMutex_);
            currentPlaylist_.SetExplicit({path});
            currentPlaylistIndex_ = 0;
            temporaryPlaylist_ = false;
            if (sourceKind == MediaSourceKind::NetworkUrl) {
                currentConfig_.lastOpenUrl = path;
            }
        }
        SyncPlaylistUi();
        PlayPlaylistIndex(0);
        return;
    }

    const std::filesystem::path fsPath(path);
    if (sourceKind == MediaSourceKind::PlaylistContainer && std::filesystem::exists(fsPath)) {
        const auto loadedItems = LoadPlaylist(fsPath);
        if (loadedItems.empty()) {
            mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::PlaylistContainerIsEmpty));
            return;
        }
        {
            std::scoped_lock lock(stateMutex_);
            currentPlaylist_.SetExplicit(loadedItems);
            currentPlaylistIndex_ = 0;
            temporaryPlaylist_ = false;
        }
        SyncPlaylistUi();
        PlayPlaylistIndex(0);
        return;
    }
    if (sourceKind == MediaSourceKind::DiscImage) {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::DiscImageDetected));
        return;
    }
    if (!std::filesystem::exists(fsPath)) {
        std::scoped_lock lock(stateMutex_);
        RemoveRecentFile(path);
        configService_.ScheduleSave(currentConfig_);
        mainWindow_.SetRecentFiles(currentConfig_.recentFiles);
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::MediaSourceNotFound));
        return;
    }
    if (std::filesystem::is_directory(fsPath)) {
        OpenFolder(path);
        return;
    }
    if (IsSubtitleFile(fsPath)) {
        LoadSubtitle(path);
        return;
    }
    if (!IsPlayableMediaFile(fsPath)) {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::UnsupportedMediaFile));
        return;
    }

    logger_.Info("Open file request received");
    const auto playlist = BuildPlaylistFromFile(fsPath);
    {
        std::scoped_lock lock(stateMutex_);
        if (playlist.isFolderBacked) {
            currentPlaylist_.SetFolderBacked(playlist.compactRootFolder, playlist.compactItemNames);
        } else {
            currentPlaylist_.SetExplicit(playlist.items);
        }
        currentPlaylistIndex_ = playlist.currentIndex;
        temporaryPlaylist_ = false;
    }
    SyncPlaylistUi();
    PlayPlaylistIndex(playlist.currentIndex >= 0 ? playlist.currentIndex : 0);
}


void Application::OpenFiles(const std::vector<std::wstring>& paths) {
    const auto playlist = BuildTemporaryPlaylist(paths);
    if (playlist.items.empty()) {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::NoSupportedFilesInSelection));
        return;
    }

    {
        std::scoped_lock lock(stateMutex_);
        currentPlaylist_.SetExplicit(playlist.items);
        currentPlaylistIndex_ = playlist.currentIndex;
        temporaryPlaylist_ = true;
    }
    mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::TemporaryPlaylistReady));
    SyncPlaylistUi();
    PlayPlaylistIndex(playlist.currentIndex >= 0 ? playlist.currentIndex : 0);
}


void Application::OpenFolder(const std::wstring& folderPath) {
    if (folderPath.empty()) {
        return;
    }

    const auto playlist = BuildPlaylistFromFolder(folderPath);
    if (playlist.items.empty()) {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::NoPlayableFilesFoundInFolder));
        return;
    }

    {
        std::scoped_lock lock(stateMutex_);
        if (playlist.isFolderBacked) {
            currentPlaylist_.SetFolderBacked(playlist.compactRootFolder, playlist.compactItemNames);
        } else {
            currentPlaylist_.SetExplicit(playlist.items);
        }
        currentPlaylistIndex_ = playlist.currentIndex;
        temporaryPlaylist_ = false;
    }
    mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::FolderPlaylistReady));
    SyncPlaylistUi();
    PlayPlaylistIndex(playlist.currentIndex >= 0 ? playlist.currentIndex : 0);
}


void Application::LoadSubtitle(const std::wstring& path) {
    if (path.empty()) {
        return;
    }
    if (!std::filesystem::exists(path)) {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::SubtitleFileNotFound));
        return;
    }
    playerThread_.LoadSubtitle(path);
    mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::SubtitleLoadedPrefix) + ShortDisplayName(path));
}


}  // namespace velo::app

