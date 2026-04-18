#pragma once

#include <Windows.h>

#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include "app/app_metadata.h"
#include "app/command_line.h"
#include "app/media_history.h"
#include "app/media_playlist.h"
#include "app/media_source.h"
#include "app/playlist_storage.h"
#include "app/runtime_playlist.h"
#include "app/single_instance.h"
#include "config/config_service.h"
#include "diagnostics/logger.h"
#include "diagnostics/performance_report.h"
#include "diagnostics/release_gate.h"
#include "diagnostics/session_recovery.h"
#include "diagnostics/startup_metrics.h"
#include "diagnostics/system_report.h"
#include "playback/player_thread.h"
#include "ui/main_window.h"

namespace velo::app {

class Application {
public:
    Application(HINSTANCE instance, CommandLineOptions options);
    int Run(int showCommand);

private:
    void OpenFile(const std::wstring& path);
    void OpenFiles(const std::vector<std::wstring>& paths);
    void OpenFolder(const std::wstring& folderPath);
    void LoadSubtitle(const std::wstring& path);
    void TakeScreenshot();
    void PlayNext();
    void PlayPrevious();
    void ReplayCurrent();
    void PlayPlaylistIndex(int index);
    void PlayPlaylistIndex(int index, bool forcePauseAfterLoad);
    void PlayPlaylistIndex(int index, bool forcePauseAfterLoad, bool forcePlayAfterLoad);
    void PlayPlaylistIndex(int index, bool forcePauseAfterLoad, bool forcePlayAfterLoad, bool suppressResumeRestore);
    void RememberRecentFile(const std::wstring& path);
    void RemoveRecentFile(const std::wstring& path);
    void UpdateResumeEntry(const std::wstring& path, double positionSeconds, bool clearEntry);
    double LookupResumeEntry(const std::wstring& path) const;
    void ApplyConfig(const velo::config::AppConfig& updatedConfig);
    double EffectiveStartupVolume() const;
    void HandlePlayerState(const velo::ui::PlayerState& state);
    void HandleExternalOpen(std::wstring path);
    std::wstring BuildPipePayload() const;
    std::wstring ExportDiagnosticsBundle() const;
    std::wstring BuildAboutText() const;
    [[nodiscard]] std::wstring BuildScreenshotOutputPath() const;
    void SyncPlaylistUi();
    std::optional<int> ResolveNextPlaylistIndexLocked(bool forAutoplay) const;
    void StartReleaseUpdateCheck();
    void OpenUpdateDownload() const;
    void OpenUpdateDownload(const std::wstring& url) const;

    HINSTANCE instance_ = nullptr;
    CommandLineOptions options_;
    velo::config::ConfigService configService_;
    velo::diagnostics::Logger logger_;
    velo::diagnostics::SessionRecoveryTracker sessionRecovery_;
    velo::diagnostics::StartupMetrics startupMetrics_;
    velo::ui::MainWindow mainWindow_;
    velo::playback::PlayerThread playerThread_;
    std::unique_ptr<SingleInstanceManager> singleInstance_;
    velo::config::AppConfig currentConfig_;
    mutable std::mutex stateMutex_;
    velo::ui::PlayerState latestState_;
    RuntimePlaylist currentPlaylist_;
    int currentPlaylistIndex_ = -1;
    bool temporaryPlaylist_ = false;
    std::wstring currentOpenPath_;
    std::wstring pendingResumePath_;
    std::optional<double> pendingResumeSeconds_;
    bool pendingPauseAfterLoad_ = false;
    double lastPersistedPositionSeconds_ = 0.0;
    double lastFlushedResumePositionSeconds_ = 0.0;
    double lastPersistedHistorySeconds_ = 0.0;
    std::wstring lastFlushedResumePath_;
    std::unordered_map<std::string, int> playbackFailureCounts_;
    std::string lastRecordedError_;
    bool pendingFileLoad_ = false;
    std::jthread updateCheckThread_;
};

}  // namespace velo::app
