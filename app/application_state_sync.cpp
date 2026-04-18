#include "app/application_internal.h"

namespace velo::app {

void Application::ApplyConfig(const velo::config::AppConfig& updatedConfig) {
    std::scoped_lock lock(stateMutex_);
    const bool volumeChanged = std::abs(currentConfig_.startupVolume - updatedConfig.startupVolume) > 0.001;
    currentConfig_.rememberVolume = updatedConfig.rememberVolume;
    currentConfig_.startupVolume = updatedConfig.startupVolume;
    currentConfig_.volume = updatedConfig.volume;
    currentConfig_.hwdecPolicy = updatedConfig.hwdecPolicy;
    currentConfig_.audioOutputDevice = updatedConfig.audioOutputDevice;
    currentConfig_.autoplayNextFile = updatedConfig.autoplayNextFile;
    currentConfig_.preservePauseOnOpen = updatedConfig.preservePauseOnOpen;
    currentConfig_.autoLoadSubtitle = updatedConfig.autoLoadSubtitle;
    currentConfig_.rememberPlaybackPosition = updatedConfig.rememberPlaybackPosition;
    currentConfig_.exactSeek = updatedConfig.exactSeek;
    currentConfig_.showPlaylistSidebar = updatedConfig.showPlaylistSidebar;
    currentConfig_.showDebugOverlay = updatedConfig.showDebugOverlay;
    currentConfig_.controlsHideDelayMs = updatedConfig.controlsHideDelayMs;
    currentConfig_.seekStepMode = updatedConfig.seekStepMode;
    currentConfig_.seekStepSeconds = updatedConfig.seekStepSeconds;
    currentConfig_.seekStepPercent = updatedConfig.seekStepPercent;
    currentConfig_.volumeStep = updatedConfig.volumeStep;
    currentConfig_.wheelVolumeStep = updatedConfig.wheelVolumeStep;
    currentConfig_.audioDelaySeconds = updatedConfig.audioDelaySeconds;
    currentConfig_.subtitleDelaySeconds = updatedConfig.subtitleDelaySeconds;
    currentConfig_.subtitleFont = updatedConfig.subtitleFont;
    currentConfig_.subtitleTextColor = updatedConfig.subtitleTextColor;
    currentConfig_.subtitleBorderColor = updatedConfig.subtitleBorderColor;
    currentConfig_.subtitleShadowColor = updatedConfig.subtitleShadowColor;
    currentConfig_.subtitleBackgroundEnabled = updatedConfig.subtitleBackgroundEnabled;
    currentConfig_.subtitleBackgroundColor = updatedConfig.subtitleBackgroundColor;
    currentConfig_.subtitleFontSize = updatedConfig.subtitleFontSize;
    currentConfig_.subtitleBold = updatedConfig.subtitleBold;
    currentConfig_.subtitleItalic = updatedConfig.subtitleItalic;
    currentConfig_.subtitleUnderline = updatedConfig.subtitleUnderline;
    currentConfig_.subtitleStrikeOut = updatedConfig.subtitleStrikeOut;
    currentConfig_.subtitleBorderSize = updatedConfig.subtitleBorderSize;
    currentConfig_.subtitleShadowDepth = updatedConfig.subtitleShadowDepth;
    currentConfig_.subtitleVerticalMargin = updatedConfig.subtitleVerticalMargin;
    currentConfig_.subtitlePositionPreset = updatedConfig.subtitlePositionPreset;
    currentConfig_.subtitleOffsetUp = updatedConfig.subtitleOffsetUp;
    currentConfig_.subtitleOffsetDown = updatedConfig.subtitleOffsetDown;
    currentConfig_.subtitleHorizontalOffset = updatedConfig.subtitleHorizontalOffset;
    currentConfig_.subtitleOffsetLeft = updatedConfig.subtitleOffsetLeft;
    currentConfig_.subtitleOffsetRight = updatedConfig.subtitleOffsetRight;
    currentConfig_.subtitleEncoding = updatedConfig.subtitleEncoding;
    currentConfig_.doubleClickAction = updatedConfig.doubleClickAction;
    currentConfig_.middleClickAction = updatedConfig.middleClickAction;
    currentConfig_.repeatMode = updatedConfig.repeatMode;
    currentConfig_.endOfPlaybackAction = updatedConfig.endOfPlaybackAction;
    currentConfig_.showDebugInfo = updatedConfig.showDebugInfo;
    currentConfig_.preferredAspectRatio = updatedConfig.preferredAspectRatio;
    currentConfig_.videoRotateDegrees = updatedConfig.videoRotateDegrees;
    currentConfig_.mirrorVideo = updatedConfig.mirrorVideo;
    currentConfig_.deinterlaceEnabled = updatedConfig.deinterlaceEnabled;
    currentConfig_.sharpenStrength = updatedConfig.sharpenStrength;
    currentConfig_.denoiseStrength = updatedConfig.denoiseStrength;
    currentConfig_.equalizerProfile = updatedConfig.equalizerProfile;
    currentConfig_.screenshotFormat = updatedConfig.screenshotFormat;
    currentConfig_.screenshotQuality = updatedConfig.screenshotQuality;
    currentConfig_.networkTimeoutMs = updatedConfig.networkTimeoutMs;
    currentConfig_.streamReconnectCount = updatedConfig.streamReconnectCount;
    currentConfig_.keyBindings = updatedConfig.keyBindings;
    configService_.ScheduleSave(currentConfig_);
    playerThread_.ApplyConfig(currentConfig_);
    if (volumeChanged) {
        playerThread_.SetVolume(EffectiveStartupVolume());
    }
}


double Application::EffectiveStartupVolume() const {
    return currentConfig_.startupVolume;
}


void Application::HandlePlayerState(const velo::ui::PlayerState& state) {
    bool hasNext = false;
    bool shouldAutoplayNext = false;
    bool shouldResume = false;
    bool shouldFlushResume = false;
    bool reachedEof = false;
    bool shouldPreservePause = false;
    bool repeatCurrent = false;
    bool shouldCloseWindow = false;
    double resumeSeconds = 0.0;
    std::wstring resumePath;
    std::optional<int> autoplayIndex;

    {
        std::scoped_lock lock(stateMutex_);
        const bool stateMatchesCurrentPath = PlayerStateMatchesCurrentPath(state, currentOpenPath_);
        if (stateMatchesCurrentPath && ShouldClearPendingFileLoad(state)) {
            pendingFileLoad_ = false;
        }
        const bool eofTransition = stateMatchesCurrentPath && !latestState_.eofReached && state.eofReached && !pendingFileLoad_;
        latestState_ = state;
        reachedEof = eofTransition;

        if (stateMatchesCurrentPath && currentConfig_.rememberPlaybackPosition && !currentOpenPath_.empty()) {
            const bool clearResume = state.eofReached || (state.durationSeconds > 0.0 && state.durationSeconds - state.positionSeconds < 10.0);
            if (clearResume || std::abs(state.positionSeconds - lastPersistedPositionSeconds_) >= 20.0) {
                UpdateResumeEntry(currentOpenPath_, state.positionSeconds, clearResume);
                lastPersistedPositionSeconds_ = state.positionSeconds;
                configService_.ScheduleSave(currentConfig_);
                const bool sameFlushPath = SamePathInsensitive(lastFlushedResumePath_, currentOpenPath_);
                shouldFlushResume = clearResume || !sameFlushPath ||
                                    std::abs(state.positionSeconds - lastFlushedResumePositionSeconds_) >= 30.0;
                if (shouldFlushResume) {
                    lastFlushedResumePath_ = clearResume ? std::wstring{} : currentOpenPath_;
                    lastFlushedResumePositionSeconds_ = clearResume ? 0.0 : state.positionSeconds;
                }
            }
        }

        if (stateMatchesCurrentPath && !currentOpenPath_.empty() && state.isLoaded &&
            (std::abs(state.positionSeconds - lastPersistedHistorySeconds_) >= 15.0 || state.eofReached)) {
            RememberHistoryEntry(currentConfig_.historyEntries, currentOpenPath_,
                                state.mediaTitle.empty() ? ShortDisplayName(currentOpenPath_) : Utf8ToWide(state.mediaTitle),
                                state.positionSeconds, IsLikelyUrl(currentOpenPath_), UnixTimestampNow());
            lastPersistedHistorySeconds_ = state.positionSeconds;
            configService_.ScheduleSave(currentConfig_);
        }

        if (stateMatchesCurrentPath && pendingResumeSeconds_.has_value() && pendingResumeSeconds_.value() > 5.0 && state.isLoaded &&
            std::abs(state.positionSeconds - pendingResumeSeconds_.value()) > 1.0) {
            shouldResume = true;
            resumeSeconds = pendingResumeSeconds_.value();
            resumePath = pendingResumePath_;
            pendingResumeSeconds_.reset();
        }

        if (stateMatchesCurrentPath && pendingPauseAfterLoad_ && state.isLoaded) {
            shouldPreservePause = !state.isPaused;
            pendingPauseAfterLoad_ = false;
        }

        const std::optional<int> nextPlaylistIndex = ResolveNextPlaylistIndexLocked(true);
        hasNext = nextPlaylistIndex.has_value();
        if (eofTransition) {
            const EndOfPlaybackDecision decision =
                DecideEndOfPlaybackAction(currentConfig_.endOfPlaybackAction, currentPlaylistIndex_, nextPlaylistIndex);
            autoplayIndex = decision.playbackIndex;
            shouldAutoplayNext = autoplayIndex.has_value();
            repeatCurrent = decision.replayCurrent;
            shouldCloseWindow = decision.closeWindow;
        }

        if (!state.errorState.empty() && state.errorState != lastRecordedError_) {
            ++playbackFailureCounts_[velo::diagnostics::ClassifyPlaybackFailure(state.errorState)];
            lastRecordedError_ = state.errorState;
        } else if (state.errorState.empty()) {
            lastRecordedError_.clear();
        }
    }

    if (shouldResume && !resumePath.empty()) {
        playerThread_.SeekAbsolute(resumeSeconds);
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::ResumedFromPrefix) +
                            velo::ui::FormatTimeLabel(resumeSeconds));
    }

    if (shouldFlushResume) {
        configService_.Flush();
    }

    if (shouldPreservePause) {
        playerThread_.SetPause(true);
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::PauseStatePreserved));
    }

    if (reachedEof) {
        if (autoplayIndex.has_value() && shouldAutoplayNext) {
            mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode,
                                                         repeatCurrent ? velo::localization::TextId::RepeatCurrentItem
                                                                       : velo::localization::TextId::AutoplayingNextItem));
            PlayPlaylistIndex(*autoplayIndex, false, true);
        } else if (shouldCloseWindow) {
            PostMessageW(mainWindow_.WindowHandle(), WM_CLOSE, 0, 0);
        } else {
            mainWindow_.PostEndOfPlayback(hasNext, false);
            mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::PlaybackFinished));
        }
    }

    mainWindow_.PostPlayerState(state);
}


void Application::HandleExternalOpen(std::wstring path) {
    mainWindow_.PostOpenFile(std::move(path));
}


std::wstring Application::BuildPipePayload() const {
    std::wstring payload;
    for (const auto& file : options_.filesToOpen) {
        payload += file;
        payload += L'\n';
    }
    return payload;
}


}  // namespace velo::app

