#pragma once

#include <Windows.h>

namespace velo::platform::win32 {

inline constexpr UINT kVideoHostMouseMoveMessage = WM_APP + 100;
inline constexpr UINT kVideoHostSingleClickMessage = WM_APP + 101;
inline constexpr UINT kVideoHostDoubleClickMessage = WM_APP + 102;
inline constexpr UINT kVideoHostMouseWheelMessage = WM_APP + 103;
inline constexpr UINT kVideoHostContextMenuMessage = WM_APP + 104;
inline constexpr UINT kVideoHostMouseLeaveMessage = WM_APP + 105;
inline constexpr UINT kVideoHostMButtonUpMessage = WM_APP + 106;
inline constexpr UINT kVideoHostXButtonUpMessage = WM_APP + 107;
inline constexpr UINT kVideoHostKeyDownMessage = WM_APP + 108;
inline constexpr UINT kVideoHostLButtonDownMessage = WM_APP + 109;
inline constexpr UINT kVideoHostLButtonUpMessage = WM_APP + 110;

class WindowHost {
public:
    bool Create(HINSTANCE instance, HWND parent);
    void Resize(const RECT& bounds) const;
    [[nodiscard]] HWND WindowHandle() const noexcept;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static ATOM RegisterClass(HINSTANCE instance);

    HWND hwnd_ = nullptr;
};

}  // namespace velo::platform::win32
