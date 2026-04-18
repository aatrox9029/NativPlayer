#include "app/application_internal.h"
#include "app/mpv_runtime_bootstrap.h"

namespace velo::app {

int Application::Run(const int showCommand) {
    const auto configRoot = velo::config::DefaultConfigRoot();
    velo::diagnostics::InstallCrashDumpHandler(configRoot / "dumps");
    sessionRecovery_.PrepareForStartup();

    configService_.Load();
    currentConfig_ = configService_.Current();
    startupMetrics_.Mark("config_loaded");

    const auto runtimeBootstrap = EnsureManagedMpvRuntimeAvailable(logger_, currentConfig_.networkTimeoutMs, currentConfig_.languageCode);
    if (!runtimeBootstrap.effectiveLanguageCode.empty() && currentConfig_.languageCode != runtimeBootstrap.effectiveLanguageCode) {
        currentConfig_.languageCode = runtimeBootstrap.effectiveLanguageCode;
        configService_.ScheduleSave(currentConfig_);
    }
    if (!runtimeBootstrap.ready) {
        logger_.Warn("Managed libmpv runtime bootstrap did not complete: " + Narrow(runtimeBootstrap.detail));
        return 0;
    }
    startupMetrics_.Mark("runtime_bootstrap");

    if (!options_.noSingleInstance) {
        singleInstance_ = std::make_unique<SingleInstanceManager>(L"NativPlayer", [this](std::wstring payload) {
            HandleExternalOpen(std::move(payload));
        });
        if (!singleInstance_->AcquirePrimary()) {
            if (!options_.filesToOpen.empty()) {
                singleInstance_->SendToPrimary(BuildPipePayload());
            }
            return 0;
        }
    }

    velo::ui::MainWindow::Callbacks callbacks{
        .openFile = [this](const std::wstring& path) { OpenFile(path); },
        .openFiles = [this](const std::vector<std::wstring>& paths) { OpenFiles(paths); },
        .openFolder = [this](const std::wstring& path) { OpenFolder(path); },
        .loadSubtitle = [this](const std::wstring& path) { LoadSubtitle(path); },
        .togglePause = [this]() {
            std::scoped_lock lock(stateMutex_);
            if (latestState_.eofReached) {
                if (latestState_.durationSeconds > 0.0 && latestState_.positionSeconds + 0.5 < latestState_.durationSeconds) {
                    playerThread_.SetPause(false);
                } else {
                    playerThread_.SeekAbsolute(0.0);
                    playerThread_.SetPause(false);
                }
                return;
            }
            playerThread_.TogglePause();
        },
        .seekRelative = [this](const double seconds) {
            {
                std::scoped_lock lock(stateMutex_);
                latestState_.eofReached = false;
            }
            playerThread_.SeekRelative(seconds);
        },
        .seekAbsolutePreview = [this](const double seconds) {
            {
                std::scoped_lock lock(stateMutex_);
                latestState_.eofReached = false;
            }
            playerThread_.SeekAbsolute(seconds, false);
        },
        .seekAbsolute = [this](const double seconds) {
            {
                std::scoped_lock lock(stateMutex_);
                latestState_.eofReached = false;
            }
            playerThread_.SeekAbsolute(seconds, true);
        },
        .setVolume = [this](const double volume) {
            std::scoped_lock lock(stateMutex_);
            currentConfig_.volume = volume;
            currentConfig_.startupVolume = volume;
            configService_.ScheduleSave(currentConfig_);
            playerThread_.SetVolume(volume);
        },
        .setMute = [this](const bool muted) {
            std::scoped_lock lock(stateMutex_);
            currentConfig_.muted = muted;
            configService_.ScheduleSave(currentConfig_);
            playerThread_.SetMute(muted);
        },
        .setPlaybackSpeed = [this](const double speed) { playerThread_.SetPlaybackSpeed(speed); },
        .adjustPlaybackSpeed = [this](const double delta) {
            double currentSpeed = 1.0;
            {
                std::scoped_lock lock(stateMutex_);
                currentSpeed = latestState_.playbackSpeed > 0.0 ? latestState_.playbackSpeed : 1.0;
            }
            playerThread_.SetPlaybackSpeed(std::clamp(currentSpeed + delta, 0.25, 3.0));
        },
        .resetPlaybackSpeed = [this]() { playerThread_.SetPlaybackSpeed(1.0); },
        .cycleAudioTrack = [this]() { playerThread_.CycleAudioTrack(); },
        .cycleSubtitleTrack = [this]() { playerThread_.CycleSubtitleTrack(); },
        .selectAudioTrack = [this](const long long trackId) { playerThread_.SelectAudioTrack(trackId); },
        .selectSubtitleTrack = [this](const long long trackId) { playerThread_.SelectSubtitleTrack(trackId); },
        .setAudioOutputDevice = [this](const std::wstring& device) {
            std::wstring resolvedDevice;
            {
                std::scoped_lock lock(stateMutex_);
                currentConfig_.audioOutputDevice = device.empty() ? L"auto" : device;
                resolvedDevice = currentConfig_.audioOutputDevice;
                configService_.ScheduleSave(currentConfig_);
            }
            playerThread_.SetAudioOutputDevice(resolvedDevice);
        },
        .takeScreenshot = [this]() { TakeScreenshot(); },
        .recoverAudioOutput = [this]() { playerThread_.RecoverAudioOutput(); },
        .playNext = [this]() { PlayNext(); },
        .playPrevious = [this]() { PlayPrevious(); },
        .replayCurrent = [this]() { ReplayCurrent(); },
        .applyConfig = [this](const velo::config::AppConfig& updatedConfig) { ApplyConfig(updatedConfig); },
        .exportDiagnostics = [this]() { return ExportDiagnosticsBundle(); },
        .buildAboutText = [this]() { return BuildAboutText(); },
        .openUpdateDownload = [this](const std::wstring& url) { OpenUpdateDownload(url); },
    };

    if (!mainWindow_.Create(instance_, currentConfig_, std::move(callbacks))) {
        return 1;
    }
    mainWindow_.Show(currentConfig_.startMaximized ? SW_SHOWMAXIMIZED : showCommand);
    mainWindow_.SetRecentFiles(currentConfig_.recentFiles);
    mainWindow_.SetPlaylistContext(-1, 0, false);
    if (sessionRecovery_.HadPreviousUncleanExit()) {
        mainWindow_.PostOsd(velo::localization::Text(currentConfig_.languageCode, velo::localization::TextId::RecoveredAfterUncleanShutdown));
    }
    StartReleaseUpdateCheck();
    startupMetrics_.Mark("window_created");

    playerThread_.SetStateCallback([this](const velo::ui::PlayerState& state) { HandlePlayerState(state); });
    playerThread_.Start(mainWindow_.VideoHostWindow(), currentConfig_.hwdecPolicy, &logger_, &startupMetrics_);
    startupMetrics_.Mark("player_thread_started");
    playerThread_.ApplyConfig(currentConfig_);
    playerThread_.SetVolume(EffectiveStartupVolume());
    playerThread_.SetMute(currentConfig_.muted);

    if (options_.filesToOpen.size() > 1) {
        OpenFiles(options_.filesToOpen);
    } else {
        for (const auto& path : options_.filesToOpen) {
            OpenFile(path);
        }
    }

    const int exitCode = mainWindow_.RunMessageLoop();
    updateCheckThread_.request_stop();
    if (updateCheckThread_.joinable()) {
        updateCheckThread_.join();
    }

    {
        std::scoped_lock lock(stateMutex_);
        if (!currentOpenPath_.empty()) {
            const bool clearResume = latestState_.eofReached ||
                                     (latestState_.durationSeconds > 0.0 &&
                                      latestState_.durationSeconds - latestState_.positionSeconds < 10.0);
            UpdateResumeEntry(currentOpenPath_, latestState_.positionSeconds, clearResume);
            RememberHistoryEntry(currentConfig_.historyEntries, currentOpenPath_,
                                latestState_.mediaTitle.empty() ? ShortDisplayName(currentOpenPath_) : Utf8ToWide(latestState_.mediaTitle),
                                latestState_.positionSeconds, IsLikelyUrl(currentOpenPath_), UnixTimestampNow());
        }
        auto capturedConfig = mainWindow_.CaptureConfig();
        capturedConfig.lastFile = currentConfig_.lastFile;
        capturedConfig.lastOpenUrl = currentConfig_.lastOpenUrl;
        capturedConfig.recentFiles = currentConfig_.recentFiles;
        capturedConfig.recentItems = currentConfig_.recentItems;
        capturedConfig.resumeEntries = currentConfig_.resumeEntries;
        capturedConfig.historyEntries = currentConfig_.historyEntries;
        capturedConfig.bookmarkEntries = currentConfig_.bookmarkEntries;
        currentConfig_ = std::move(capturedConfig);
        configService_.ScheduleSave(currentConfig_);
    }
    configService_.Flush();
    playerThread_.Stop();
    const bool shouldWriteExitDiagnostics = currentConfig_.showDebugInfo || !playbackFailureCounts_.empty() ||
                                            !latestState_.errorState.empty() || sessionRecovery_.HadPreviousUncleanExit();
    if (shouldWriteExitDiagnostics) {
        startupMetrics_.WriteReport(velo::diagnostics::DefaultLogRoot() / "startup-latest.log");
        const auto benchmark = velo::diagnostics::BuildBenchmarkReport(startupMetrics_, latestState_);
        velo::diagnostics::WriteBenchmarkReport(velo::diagnostics::DefaultLogRoot() / "benchmark-latest.log", benchmark);
    }
    sessionRecovery_.MarkCleanExit();
    return exitCode;
}

}  // namespace velo::app
