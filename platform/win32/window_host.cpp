#include "platform/win32/window_host.h"

namespace velo::platform::win32 {
namespace {

constexpr wchar_t kWindowClassName[] = L"NativPlayerVideoHostWindow";

void ForwardToParent(HWND hwnd, UINT message, WPARAM wParam = 0, LPARAM lParam = 0) {
    const HWND parent = GetParent(hwnd);
    if (parent != nullptr) {
        PostMessageW(parent, message, wParam, lParam);
    }
}

}  // namespace

bool WindowHost::Create(HINSTANCE instance, HWND parent) {
    RegisterClass(instance);
    hwnd_ = CreateWindowExW(0, kWindowClassName, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0,
                            0, 0, 0, parent, nullptr, instance, nullptr);
    return hwnd_ != nullptr;
}

void WindowHost::Resize(const RECT& bounds) const {
    MoveWindow(hwnd_, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, FALSE);
}

HWND WindowHost::WindowHandle() const noexcept {
    return hwnd_;
}

LRESULT CALLBACK WindowHost::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_NCHITTEST: {
            const HWND parent = GetParent(hwnd);
            if (parent != nullptr) {
                return SendMessageW(parent, WM_NCHITTEST, wParam, lParam);
            }
            break;
        }
        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT track{};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            TrackMouseEvent(&track);
            ForwardToParent(hwnd, kVideoHostMouseMoveMessage, wParam, lParam);
            return 0;
        }
        case WM_MOUSELEAVE:
            ForwardToParent(hwnd, kVideoHostMouseLeaveMessage, wParam, lParam);
            return 0;
        case WM_MOUSEWHEEL:
            ForwardToParent(hwnd, kVideoHostMouseWheelMessage, wParam, lParam);
            return 0;
        case WM_RBUTTONUP:
            ForwardToParent(hwnd, kVideoHostContextMenuMessage, wParam, lParam);
            return 0;
        case WM_LBUTTONDOWN:
            ForwardToParent(hwnd, kVideoHostLButtonDownMessage, wParam, lParam);
            return 0;
        case WM_MBUTTONUP:
            ForwardToParent(hwnd, kVideoHostMButtonUpMessage, wParam, lParam);
            return 0;
        case WM_XBUTTONUP:
            ForwardToParent(hwnd, kVideoHostXButtonUpMessage, wParam, lParam);
            return 0;
        case WM_LBUTTONUP:
            ForwardToParent(hwnd, kVideoHostLButtonUpMessage, wParam, lParam);
            ForwardToParent(hwnd, kVideoHostSingleClickMessage, wParam, lParam);
            return 0;
        case WM_LBUTTONDBLCLK:
            ForwardToParent(hwnd, kVideoHostDoubleClickMessage, wParam, lParam);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            ForwardToParent(hwnd, kVideoHostKeyDownMessage, wParam, lParam);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            RECT rect{};
            GetClientRect(hwnd, &rect);
            FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            EndPaint(hwnd, &paint);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

ATOM WindowHost::RegisterClass(HINSTANCE instance) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    atom = ::RegisterClassW(&windowClass);
    return atom;
}

}  // namespace velo::platform::win32
