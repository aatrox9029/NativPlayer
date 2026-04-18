#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

#include "config/config_service.h"

namespace velo::playback {

enum class CommandType {
    LoadFile,
    LoadSubtitle,
    ApplyConfig,
    TogglePause,
    SetPause,
    SeekRelative,
    SeekAbsolute,
    SetVolume,
    SetMute,
    SetPlaybackSpeed,
    SetAudioDelay,
    SetSubtitleDelay,
    SetSubtitlePosition,
    ResetSyncDelays,
    StepFrame,
    StepBackwardFrame,
    SetLoopPointA,
    SetLoopPointB,
    ClearLoopPoints,
    TakeScreenshot,
    CycleAudioTrack,
    CycleSubtitleTrack,
    SelectAudioTrack,
    SelectSubtitleTrack,
    SetAudioOutputDevice,
    RecoverAudioOutput,
    Shutdown,
};

struct PlayerCommand {
    CommandType type = CommandType::Shutdown;
    std::wstring text;
    double numberValue = 0.0;
    bool boolValue = false;
    long long intValue = 0;
    std::shared_ptr<velo::config::AppConfig> configValue;
};

class CommandDispatcher {
public:
    void Push(PlayerCommand command);
    std::optional<PlayerCommand> WaitAndPop(int timeoutMilliseconds);
    std::optional<PlayerCommand> TryPop();
    PlayerCommand CollapsePendingSeekCommands(PlayerCommand command);

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<PlayerCommand> queue_;
};

}  // namespace velo::playback
