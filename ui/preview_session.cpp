#include "ui/preview_session.h"

namespace velo::ui {

bool PreviewSession::EnsureInitialized(HWND videoWindow, const std::wstring& hwdecPolicy) {
    if (initialized_) {
        return true;
    }

    player_.SetStateCallback([this](const PlayerState& state) { state_ = state; });
    initialized_ = player_.Initialize(videoWindow, hwdecPolicy, nullptr, nullptr, true);
    return initialized_;
}

void PreviewSession::Open(const std::wstring& path, const bool paused, const bool muted) {
    if (!initialized_ || path.empty()) {
        return;
    }

    loadedPath_ = path;
    state_ = PlayerState{};
    player_.SetMute(muted);
    player_.SetVolume(muted ? 0.0 : state_.volume);
    player_.LoadFile(path);
    player_.SetPause(paused);
}

void PreviewSession::Stop() {
    if (!initialized_) {
        return;
    }
    player_.StopPlayback();
    loadedPath_.clear();
    state_ = PlayerState{};
}

void PreviewSession::Pump(const double timeoutSeconds) {
    if (!initialized_) {
        return;
    }
    player_.PumpEvents(timeoutSeconds);
}

void PreviewSession::SeekAbsolute(const double seconds) {
    if (!initialized_) {
        return;
    }
    player_.SeekAbsolute(seconds);
}

void PreviewSession::SetPause(const bool paused) {
    if (!initialized_) {
        return;
    }
    player_.SetPause(paused);
}

void PreviewSession::SetMute(const bool muted) {
    if (!initialized_) {
        return;
    }
    player_.SetMute(muted);
}

void PreviewSession::SetVolume(const double volume) {
    if (!initialized_) {
        return;
    }
    player_.SetVolume(volume);
}

void PreviewSession::Shutdown() {
    if (!initialized_) {
        return;
    }
    player_.Shutdown();
    initialized_ = false;
    loadedPath_.clear();
    state_ = PlayerState{};
}

bool PreviewSession::Initialized() const noexcept {
    return initialized_;
}

const std::wstring& PreviewSession::LoadedPath() const noexcept {
    return loadedPath_;
}

const PlayerState& PreviewSession::State() const noexcept {
    return state_;
}

}  // namespace velo::ui
