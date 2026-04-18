#pragma once

#include <Windows.h>

#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "config/config_service.h"
#include "diagnostics/logger.h"
#include "diagnostics/startup_metrics.h"
#include "playback/command_dispatcher.h"
#include "ui/player_state.h"

namespace velo::playback {

class PlayerThread {
public:
    using StateCallback = std::function<void(const velo::ui::PlayerState&)>;

    ~PlayerThread();

    void SetStateCallback(StateCallback callback);
    bool Start(HWND videoWindow, std::wstring hwdecPolicy, velo::diagnostics::Logger* logger,
               velo::diagnostics::StartupMetrics* metrics);
    void Stop();

    void LoadFile(const std::wstring& path);
    void LoadSubtitle(const std::wstring& path);
    void ApplyConfig(const velo::config::AppConfig& config);
    void TogglePause();
    void SetPause(bool value);
    void SeekRelative(double seconds);
    void SeekAbsolute(double seconds, bool exact = true);
    void SetVolume(double volume);
    void SetMute(bool value);
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
    void SetAudioOutputDevice(const std::wstring& device);
    void RecoverAudioOutput();

private:
    void ThreadMain(HWND videoWindow, std::wstring hwdecPolicy, velo::diagnostics::Logger* logger,
                    velo::diagnostics::StartupMetrics* metrics);
    void PushCommand(PlayerCommand command);

    std::mutex mutex_;
    StateCallback stateCallback_;
    CommandDispatcher dispatcher_;
    std::thread worker_;
    bool running_ = false;
};

}  // namespace velo::playback
