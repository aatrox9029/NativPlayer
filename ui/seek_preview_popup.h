#pragma once

#include <Windows.h>

#include <string>

#include "ui/preview_session.h"
#include "ui/player_state.h"

namespace velo::ui {

class SeekPreviewPopup {
public:
    bool Create(HINSTANCE instance, HWND parent);
    void SetEnabled(bool enabled);
    void SetMedia(const std::wstring& path, const std::wstring& hwdecPolicy);
    void ShowAt(const RECT& anchorRect, double sliderRatio, double targetSeconds, double durationSeconds);
    void Hide();
    [[nodiscard]] bool Visible() const noexcept;
    [[nodiscard]] HWND WindowHandle() const noexcept;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void EnsurePreviewPlayer();
    void UpdateLayout();
    void Paint() const;
    void QueueSeek(double targetSeconds);

    static constexpr UINT_PTR kPreviewTimer = 1;

    HINSTANCE instance_ = nullptr;
    HWND parent_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND videoHost_ = nullptr;
    HWND timeLabel_ = nullptr;
    HFONT bodyFont_ = nullptr;
    bool enabled_ = true;
    bool visible_ = false;
    bool previewInitialized_ = false;
    bool seekPending_ = false;
    std::wstring currentPath_;
    std::wstring loadedPath_;
    std::wstring hwdecPolicy_ = L"auto";
    double queuedSeekSeconds_ = 0.0;
    double durationSeconds_ = 0.0;
    double lastSeekSeconds_ = -1.0;
    PreviewSession previewSession_;
};

}  // namespace velo::ui
