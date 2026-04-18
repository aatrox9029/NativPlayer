#pragma once

#include <Windows.h>

#include <chrono>
#include <functional>
#include <string>

#include "config/config_service.h"
#include "diagnostics/logger.h"
#include "diagnostics/startup_metrics.h"
#include "playback/low_latency_seek.h"
#include "playback/mpv_loader.h"
#include "playback/seek_optimization_profile.h"
#include "playback/state_normalizer.h"
#include "ui/player_state.h"

namespace velo::playback {

class MpvPlayer {
public:
    using StateCallback = std::function<void(const velo::ui::PlayerState&)>;

    void SetStateCallback(StateCallback callback);
    bool Initialize(HWND videoWindow, const std::wstring& hwdecPolicy, velo::diagnostics::Logger* logger,
                    velo::diagnostics::StartupMetrics* metrics, bool lightweightMode = false);
    void Shutdown();

    void LoadFile(const std::wstring& path);
    void LoadSubtitle(const std::wstring& path);
    void StopPlayback();
    void ApplyConfig(const velo::config::AppConfig& config);
    void TogglePause();
    void SetPause(bool value);
    void SeekRelative(double seconds);
    void SeekAbsolute(double seconds, bool exact = true);
    void SetVolume(double volume);
    void SetMute(bool value);
    void SetAudioOutputDevice(const std::wstring& device);
    void SetPlaybackSpeed(double speed);
    void SetAudioDelay(double seconds);
    void SetSubtitleDelay(double seconds);
    void SetSubtitlePosition(int positionPercent);
    void ResetSyncDelays();
    void StepFrame();
    void StepBackwardFrame();
    void SetLoopPointA();
    void SetLoopPointB();
    void ClearLoopPoints();
    void TakeScreenshot(const std::wstring& outputPath);
    void CycleAudioTrack();
    void CycleSubtitleTrack();
    void SelectAudioTrack(long long trackId);
    void SelectSubtitleTrack(long long trackId);
    void RecoverAudioOutput();
    void PumpEvents(double timeoutSeconds);
    bool ShouldCollapseQueuedSeekCommands() const noexcept;

private:
    void EmitState();
    void HandleEvent(const mpv_event& event);
    void HandlePropertyChange(const mpv_event_property& property);
    void SyncSelectionProperties();
    std::string ErrorText(int errorCode) const;
    bool ExecuteCommand(const char* const command[], const char* context);
    void ReleasePlaybackMemory(bool clearPlaylist);
    void DropBufferedPlaybackState(const char* context);
    bool SetInitialOption(const char* name, const std::string& value);
    bool ApplyDoubleProperty(const char* name, double value);
    bool ApplyInt64Property(const char* name, int64_t value);
    bool ApplyStringProperty(const char* name, const std::string& value);
    bool ApplyCommandStringProperty(const char* name, const std::string& value);
    void ResumePlaybackAfterSeek(const char* context);
    void BeginSeekMeasurement(const char* context);
    void CompleteSeekMeasurement(const char* context);
    void ApplyRuntimeCacheProfile(const std::wstring& path);
    void RefreshSeekOptimizationProfile();
    void ApplySeekOptimizationProfile();
    bool TryRecoverHardwareFallback(const std::string& message);

    MpvLoader loader_;
    mpv_handle* handle_ = nullptr;
    StateNormalizer normalizer_;
    velo::ui::PlayerState state_;
    StateCallback stateCallback_;
    velo::diagnostics::Logger* logger_ = nullptr;
    velo::diagnostics::StartupMetrics* metrics_ = nullptr;
    bool firstFrameMarked_ = false;
    bool lightweightMode_ = false;
    std::wstring activeHwdecPolicy_;
    std::wstring lastRequestedPath_;
    bool exactSeekEnabled_ = false;
    bool hardwareFallbackAttempted_ = false;
    bool seekMeasurementPending_ = false;
    std::chrono::steady_clock::time_point seekMeasurementStart_{};
    std::string pendingSeekContext_;
    SeekOptimizationProfile seekOptimizationProfile_;
};

}  // namespace velo::playback
