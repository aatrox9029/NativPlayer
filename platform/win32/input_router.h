#pragma once

#include <Windows.h>

#include <functional>

#include "config/config_service.h"

namespace velo::platform::win32 {

struct InputCommandCallbacks {
    std::function<void()> togglePause;
    std::function<void(double)> seekRelative;
    std::function<void(bool)> seekConfigured;
    std::function<void(int)> adjustVolume;
    std::function<void()> toggleMute;
    std::function<void()> openFile;
    std::function<void()> showRecentFiles;
    std::function<void()> cycleAudioTrack;
    std::function<void()> cycleSubtitleTrack;
    std::function<void()> takeScreenshot;
    std::function<void(double)> adjustPlaybackSpeed;
    std::function<void()> resetPlaybackSpeed;
    std::function<void()> toggleFullscreen;
    std::function<void()> exitFullscreen;
    std::function<void()> playPrevious;
    std::function<void()> playNext;
    std::function<void()> showMediaInfo;
};

bool HandleKeyDown(WPARAM key, const velo::config::AppConfig& config, const InputCommandCallbacks& callbacks);
bool HandlePointerBinding(unsigned int virtualKey, const velo::config::AppConfig& config, const InputCommandCallbacks& callbacks);

}  // namespace velo::platform::win32
