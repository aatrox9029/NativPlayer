#pragma once

#include <Windows.h>

#include <string>

#include "playback/mpv_player.h"
#include "ui/player_state.h"

namespace velo::ui {

class PreviewSession {
public:
    bool EnsureInitialized(HWND videoWindow, const std::wstring& hwdecPolicy);
    void Open(const std::wstring& path, bool paused, bool muted);
    void Stop();
    void Pump(double timeoutSeconds);
    void SeekAbsolute(double seconds);
    void SetPause(bool paused);
    void SetMute(bool muted);
    void SetVolume(double volume);
    void Shutdown();

    [[nodiscard]] bool Initialized() const noexcept;
    [[nodiscard]] const std::wstring& LoadedPath() const noexcept;
    [[nodiscard]] const PlayerState& State() const noexcept;

private:
    velo::playback::MpvPlayer player_;
    PlayerState state_;
    std::wstring loadedPath_;
    bool initialized_ = false;
};

}  // namespace velo::ui
