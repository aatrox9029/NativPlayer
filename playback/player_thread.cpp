#include "playback/player_thread.h"

#include "playback/mpv_player.h"

namespace velo::playback {

PlayerThread::~PlayerThread() {
    Stop();
}

void PlayerThread::SetStateCallback(StateCallback callback) {
    std::scoped_lock lock(mutex_);
    stateCallback_ = std::move(callback);
}

bool PlayerThread::Start(HWND videoWindow, std::wstring hwdecPolicy, velo::diagnostics::Logger* logger,
                         velo::diagnostics::StartupMetrics* metrics) {
    if (running_) {
        return true;
    }
    running_ = true;
    worker_ = std::thread([this, videoWindow, hwdecPolicy = std::move(hwdecPolicy), logger, metrics]() mutable {
        ThreadMain(videoWindow, std::move(hwdecPolicy), logger, metrics);
    });
    return true;
}

void PlayerThread::Stop() {
    if (!running_) {
        return;
    }

    PushCommand({CommandType::Shutdown});
    if (worker_.joinable()) {
        worker_.join();
    }
    running_ = false;
}

void PlayerThread::LoadFile(const std::wstring& path) {
    PushCommand({CommandType::LoadFile, path});
}

void PlayerThread::LoadSubtitle(const std::wstring& path) {
    PushCommand({CommandType::LoadSubtitle, path});
}

void PlayerThread::ApplyConfig(const velo::config::AppConfig& config) {
    PlayerCommand command{CommandType::ApplyConfig};
    command.configValue = std::make_shared<velo::config::AppConfig>(config);
    PushCommand(std::move(command));
}

void PlayerThread::TogglePause() {
    PushCommand({CommandType::TogglePause});
}

void PlayerThread::SetPause(const bool value) {
    PushCommand({CommandType::SetPause, L"", 0.0, value});
}

void PlayerThread::SeekRelative(const double seconds) {
    PushCommand({CommandType::SeekRelative, L"", seconds, false});
}

void PlayerThread::SeekAbsolute(const double seconds, const bool exact) {
    PushCommand({CommandType::SeekAbsolute, L"", seconds, exact});
}

void PlayerThread::SetVolume(const double volume) {
    PushCommand({CommandType::SetVolume, L"", volume, false});
}

void PlayerThread::SetMute(const bool value) {
    PushCommand({CommandType::SetMute, L"", 0.0, value});
}

void PlayerThread::SetPlaybackSpeed(const double speed) {
    PushCommand({CommandType::SetPlaybackSpeed, L"", speed, false});
}

void PlayerThread::SetAudioDelay(const double seconds) {
    PushCommand({CommandType::SetAudioDelay, L"", seconds, false});
}

void PlayerThread::SetSubtitleDelay(const double seconds) {
    PushCommand({CommandType::SetSubtitleDelay, L"", seconds, false});
}

void PlayerThread::SetSubtitlePosition(const int positionPercent) {
    PushCommand({CommandType::SetSubtitlePosition, L"", static_cast<double>(positionPercent), false});
}

void PlayerThread::ResetSyncDelays() {
    PushCommand({CommandType::ResetSyncDelays});
}

void PlayerThread::StepFrame() {
    PushCommand({CommandType::StepFrame});
}

void PlayerThread::StepBackwardFrame() {
    PushCommand({CommandType::StepBackwardFrame});
}

void PlayerThread::SetLoopPointA() {
    PushCommand({CommandType::SetLoopPointA});
}

void PlayerThread::SetLoopPointB() {
    PushCommand({CommandType::SetLoopPointB});
}

void PlayerThread::ClearLoopPoints() {
    PushCommand({CommandType::ClearLoopPoints});
}

void PlayerThread::TakeScreenshot(const std::wstring& outputPath) {
    PushCommand({CommandType::TakeScreenshot, outputPath});
}

void PlayerThread::CycleAudioTrack() {
    PushCommand({CommandType::CycleAudioTrack});
}

void PlayerThread::CycleSubtitleTrack() {
    PushCommand({CommandType::CycleSubtitleTrack});
}

void PlayerThread::SelectAudioTrack(const long long trackId) {
    PlayerCommand command{CommandType::SelectAudioTrack};
    command.intValue = trackId;
    PushCommand(std::move(command));
}

void PlayerThread::SelectSubtitleTrack(const long long trackId) {
    PlayerCommand command{CommandType::SelectSubtitleTrack};
    command.intValue = trackId;
    PushCommand(std::move(command));
}

void PlayerThread::SetAudioOutputDevice(const std::wstring& device) {
    PushCommand({CommandType::SetAudioOutputDevice, device});
}

void PlayerThread::RecoverAudioOutput() {
    PushCommand({CommandType::RecoverAudioOutput});
}

void PlayerThread::ThreadMain(HWND videoWindow, std::wstring hwdecPolicy, velo::diagnostics::Logger* logger,
                              velo::diagnostics::StartupMetrics* metrics) {
    MpvPlayer player;
    {
        std::scoped_lock lock(mutex_);
        player.SetStateCallback(stateCallback_);
    }
    player.Initialize(videoWindow, hwdecPolicy, logger, metrics);

    bool shuttingDown = false;
    while (!shuttingDown) {
        player.PumpEvents(0.01);

        auto command = dispatcher_.WaitAndPop(10);
        if (!command.has_value()) {
            continue;
        }

        do {
            if ((command->type == CommandType::SeekRelative || command->type == CommandType::SeekAbsolute) &&
                player.ShouldCollapseQueuedSeekCommands()) {
                *command = dispatcher_.CollapsePendingSeekCommands(std::move(*command));
            }
            switch (command->type) {
                case CommandType::LoadFile:
                    player.LoadFile(command->text);
                    break;
                case CommandType::LoadSubtitle:
                    player.LoadSubtitle(command->text);
                    break;
                case CommandType::ApplyConfig:
                    if (command->configValue != nullptr) {
                        player.ApplyConfig(*command->configValue);
                    }
                    break;
                case CommandType::TogglePause:
                    player.TogglePause();
                    break;
                case CommandType::SetPause:
                    player.SetPause(command->boolValue);
                    break;
                case CommandType::SeekRelative:
                    player.SeekRelative(command->numberValue);
                    break;
                case CommandType::SeekAbsolute:
                    player.SeekAbsolute(command->numberValue, command->boolValue);
                    break;
                case CommandType::SetVolume:
                    player.SetVolume(command->numberValue);
                    break;
                case CommandType::SetMute:
                    player.SetMute(command->boolValue);
                    break;
                case CommandType::SetPlaybackSpeed:
                    player.SetPlaybackSpeed(command->numberValue);
                    break;
                case CommandType::SetAudioDelay:
                    player.SetAudioDelay(command->numberValue);
                    break;
                case CommandType::SetSubtitleDelay:
                    player.SetSubtitleDelay(command->numberValue);
                    break;
                case CommandType::SetSubtitlePosition:
                    player.SetSubtitlePosition(static_cast<int>(command->numberValue));
                    break;
                case CommandType::ResetSyncDelays:
                    player.ResetSyncDelays();
                    break;
                case CommandType::StepFrame:
                    player.StepFrame();
                    break;
                case CommandType::StepBackwardFrame:
                    player.StepBackwardFrame();
                    break;
                case CommandType::SetLoopPointA:
                    player.SetLoopPointA();
                    break;
                case CommandType::SetLoopPointB:
                    player.SetLoopPointB();
                    break;
                case CommandType::ClearLoopPoints:
                    player.ClearLoopPoints();
                    break;
                case CommandType::TakeScreenshot:
                    player.TakeScreenshot(command->text);
                    break;
                case CommandType::CycleAudioTrack:
                    player.CycleAudioTrack();
                    break;
                case CommandType::CycleSubtitleTrack:
                    player.CycleSubtitleTrack();
                    break;
                case CommandType::SelectAudioTrack:
                    player.SelectAudioTrack(command->intValue);
                    break;
                case CommandType::SelectSubtitleTrack:
                    player.SelectSubtitleTrack(command->intValue);
                    break;
                case CommandType::SetAudioOutputDevice:
                    player.SetAudioOutputDevice(command->text);
                    break;
                case CommandType::RecoverAudioOutput:
                    player.RecoverAudioOutput();
                    break;
                case CommandType::Shutdown:
                    shuttingDown = true;
                    break;
            }
            command = dispatcher_.TryPop();
        } while (!shuttingDown && command.has_value());
    }

    player.Shutdown();
}

void PlayerThread::PushCommand(PlayerCommand command) {
    dispatcher_.Push(std::move(command));
}

}  // namespace velo::playback
