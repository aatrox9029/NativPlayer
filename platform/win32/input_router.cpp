#include "platform/win32/input_router.h"

#include "platform/win32/input_profile.h"

namespace velo::platform::win32 {

bool HandleKeyDown(const WPARAM key, const velo::config::AppConfig& config, const InputCommandCallbacks& callbacks) {
    if (MatchBinding(key, config, L"toggle_pause")) {
        callbacks.togglePause();
        return true;
    }
    if (MatchBinding(key, config, L"seek_backward")) {
        if (callbacks.seekConfigured) {
            callbacks.seekConfigured(false);
        }
        return true;
    }
    if (MatchBinding(key, config, L"seek_forward")) {
        if (callbacks.seekConfigured) {
            callbacks.seekConfigured(true);
        }
        return true;
    }
    if (MatchBinding(key, config, L"volume_up")) {
        callbacks.adjustVolume(config.volumeStep);
        return true;
    }
    if (MatchBinding(key, config, L"volume_down")) {
        callbacks.adjustVolume(-config.volumeStep);
        return true;
    }
    if (MatchBinding(key, config, L"toggle_mute")) {
        callbacks.toggleMute();
        return true;
    }
    if (MatchBinding(key, config, L"open_file")) {
        callbacks.openFile();
        return true;
    }
    if (MatchBinding(key, config, L"cycle_audio")) {
        callbacks.cycleAudioTrack();
        return true;
    }
    if (MatchBinding(key, config, L"cycle_subtitle")) {
        callbacks.cycleSubtitleTrack();
        return true;
    }
    if (MatchBinding(key, config, L"take_screenshot")) {
        callbacks.takeScreenshot();
        return true;
    }
    if (MatchBinding(key, config, L"slower_speed")) {
        callbacks.adjustPlaybackSpeed(-0.10);
        return true;
    }
    if (MatchBinding(key, config, L"faster_speed")) {
        callbacks.adjustPlaybackSpeed(0.10);
        return true;
    }
    if (MatchBinding(key, config, L"reset_speed")) {
        callbacks.resetPlaybackSpeed();
        return true;
    }
    if (MatchBinding(key, config, L"toggle_fullscreen")) {
        callbacks.toggleFullscreen();
        return true;
    }
    if (MatchBinding(key, config, L"play_previous")) {
        callbacks.playPrevious();
        return true;
    }
    if (MatchBinding(key, config, L"play_next")) {
        callbacks.playNext();
        return true;
    }
    if (key == VK_ESCAPE) {
        callbacks.exitFullscreen();
        return true;
    }
    return false;
}

bool HandlePointerBinding(const unsigned int virtualKey, const velo::config::AppConfig& config, const InputCommandCallbacks& callbacks) {
    if (MatchBinding(virtualKey, config, L"toggle_pause")) {
        callbacks.togglePause();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"seek_backward")) {
        if (callbacks.seekConfigured) {
            callbacks.seekConfigured(false);
        }
        return true;
    }
    if (MatchBinding(virtualKey, config, L"seek_forward")) {
        if (callbacks.seekConfigured) {
            callbacks.seekConfigured(true);
        }
        return true;
    }
    if (MatchBinding(virtualKey, config, L"volume_up")) {
        callbacks.adjustVolume(config.volumeStep);
        return true;
    }
    if (MatchBinding(virtualKey, config, L"volume_down")) {
        callbacks.adjustVolume(-config.volumeStep);
        return true;
    }
    if (MatchBinding(virtualKey, config, L"toggle_mute")) {
        callbacks.toggleMute();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"open_file")) {
        callbacks.openFile();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"cycle_audio")) {
        callbacks.cycleAudioTrack();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"cycle_subtitle")) {
        callbacks.cycleSubtitleTrack();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"take_screenshot")) {
        callbacks.takeScreenshot();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"slower_speed")) {
        callbacks.adjustPlaybackSpeed(-0.10);
        return true;
    }
    if (MatchBinding(virtualKey, config, L"faster_speed")) {
        callbacks.adjustPlaybackSpeed(0.10);
        return true;
    }
    if (MatchBinding(virtualKey, config, L"reset_speed")) {
        callbacks.resetPlaybackSpeed();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"toggle_fullscreen")) {
        callbacks.toggleFullscreen();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"play_previous")) {
        callbacks.playPrevious();
        return true;
    }
    if (MatchBinding(virtualKey, config, L"play_next")) {
        callbacks.playNext();
        return true;
    }
    return false;
}

}  // namespace velo::platform::win32
