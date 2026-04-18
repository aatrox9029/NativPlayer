#include "ui/themed_scrollbar.h"

#include <Windows.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>

#include "ui/design_tokens.h"

namespace velo::ui {
namespace {

constexpr wchar_t kThemedScrollbarClassName[] = L"NativPlayerThemedScrollbar";

ATOM RegisterScrollbarClass(HINSTANCE instance) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = ThemedScrollbar::WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kThemedScrollbarClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_HAND);
    windowClass.hbrBackground = nullptr;
    atom = RegisterClassW(&windowClass);
    return atom;
}

}  // namespace

bool ThemedScrollbar::Create(HINSTANCE instance, HWND parent, const int controlId) {
    instance_ = instance;
    parent_ = parent;
    controlId_ = controlId;

    RegisterScrollbarClass(instance_);
    hwnd_ = CreateWindowExW(0, kThemedScrollbarClassName, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, parent_,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId_)), instance_, this);
    return hwnd_ != nullptr;
}

void ThemedScrollbar::SetMetrics(const int viewportSize, const int contentSize) {
    viewportSize_ = std::max(0, viewportSize);
    contentSize_ = std::max(viewportSize_, contentSize);
    SetValue(value_, false);
}

void ThemedScrollbar::SetValue(const int value, const bool notifyParent) {
    value_ = std::clamp(value, 0, MaxValue());
    UpdateThumbRect();
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
    if (notifyParent) {
        NotifyParent();
    }
}

int ThemedScrollbar::Value() const noexcept {
    return value_;
}

int ThemedScrollbar::MaxValue() const noexcept {
    return std::max(0, contentSize_ - viewportSize_);
}

HWND ThemedScrollbar::WindowHandle() const noexcept {
    return hwnd_;
}

LRESULT CALLBACK ThemedScrollbar::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<ThemedScrollbar*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }

    auto* self = reinterpret_cast<ThemedScrollbar*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->HandleMessage(message, wParam, lParam);
}

LRESULT ThemedScrollbar::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_MOUSEMOVE: {
            hovered_ = true;
            TRACKMOUSEEVENT trackMouse{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd_, 0};
            TrackMouseEvent(&trackMouse);
            if (dragging_) {
                SetValue(ValueFromPoint(GET_Y_LPARAM(lParam) - dragOffsetY_), true);
                return 0;
            }
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;
        }

        case WM_MOUSELEAVE:
            hovered_ = false;
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;

        case WM_LBUTTONDOWN: {
            SetFocus(hwnd_);
            if (!Enabled()) {
                return 0;
            }
            const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (PtInRect(&thumbRect_, point) != FALSE) {
                dragging_ = true;
                dragOffsetY_ = point.y - thumbRect_.top;
                SetCapture(hwnd_);
                return 0;
            }
            SetValue(ValueFromPoint(point.y - (thumbRect_.bottom - thumbRect_.top) / 2), true);
            dragging_ = true;
            dragOffsetY_ = (thumbRect_.bottom - thumbRect_.top) / 2;
            SetCapture(hwnd_);
            return 0;
        }

        case WM_LBUTTONUP:
            if (GetCapture() == hwnd_) {
                ReleaseCapture();
            }
            dragging_ = false;
            return 0;

        case WM_CAPTURECHANGED:
            dragging_ = false;
            return 0;

        case WM_PAINT:
            Paint();
            return 0;

        case WM_ERASEBKGND:
            return 1;

        default:
            break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void ThemedScrollbar::NotifyParent() const {
    if (parent_ != nullptr) {
        SendMessageW(parent_, WM_COMMAND, MAKEWPARAM(controlId_, kNotificationValueChanged), reinterpret_cast<LPARAM>(hwnd_));
    }
}

void ThemedScrollbar::Paint() const {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    RECT client{};
    GetClientRect(hwnd_, &client);

    const auto& palette = tokens::DarkPalette();
    HBRUSH backgroundBrush = CreateSolidBrush(tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.04));
    if (backgroundBrush != nullptr) {
        FillRect(dc, &client, backgroundBrush);
        DeleteObject(backgroundBrush);
    }

    RECT trackRect = client;
    trackRect.left += 4;
    trackRect.right -= 4;
    trackRect.top += 6;
    trackRect.bottom -= 6;
    tokens::FillRoundedRect(dc, trackRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.20), tokens::RadiusSm());

    if (Enabled()) {
        const COLORREF thumbColor = dragging_ ? palette.brandHover
                                              : (hovered_ ? tokens::MixColor(palette.brandPrimary, palette.brandHover, 0.35)
                                                          : palette.brandPrimary);
        tokens::FillRoundedRect(dc, thumbRect_, thumbColor, tokens::RadiusSm());
        tokens::DrawRoundedOutline(dc, thumbRect_, hovered_ || dragging_ ? palette.strokeFocus : palette.strokeSoft,
                                   tokens::RadiusSm(), hovered_ || dragging_ ? 2 : 1);
    }

    EndPaint(hwnd_, &paint);
}

void ThemedScrollbar::UpdateThumbRect() {
    RECT client{};
    if (hwnd_ == nullptr || GetClientRect(hwnd_, &client) == FALSE) {
        thumbRect_ = RECT{};
        return;
    }

    RECT trackRect = client;
    trackRect.left += 4;
    trackRect.right -= 4;
    trackRect.top += 6;
    trackRect.bottom -= 6;
    const int trackHeight = std::max(1L, trackRect.bottom - trackRect.top);
    const int thumbHeight = Enabled()
                                ? std::clamp(static_cast<int>((static_cast<double>(viewportSize_) / contentSize_) * trackHeight), 28, trackHeight)
                                : trackHeight;
    const int travel = std::max(0, trackHeight - thumbHeight);
    const double ratio = MaxValue() == 0 ? 0.0 : static_cast<double>(value_) / MaxValue();
    const int thumbTop = trackRect.top + static_cast<int>(ratio * travel);
    thumbRect_ = RECT{trackRect.left, thumbTop, trackRect.right, thumbTop + thumbHeight};
}

int ThemedScrollbar::ValueFromPoint(const int y) const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    RECT trackRect = client;
    trackRect.left += 4;
    trackRect.right -= 4;
    trackRect.top += 6;
    trackRect.bottom -= 6;
    const int thumbHeight = std::max(1L, thumbRect_.bottom - thumbRect_.top);
    const int travel = std::max(1L, (trackRect.bottom - trackRect.top) - thumbHeight);
    const int clampedTop = std::clamp(y, static_cast<int>(trackRect.top), static_cast<int>(trackRect.bottom) - thumbHeight);
    const double ratio = static_cast<double>(clampedTop - trackRect.top) / travel;
    return static_cast<int>(std::lround(MaxValue() * ratio));
}

bool ThemedScrollbar::Enabled() const noexcept {
    return MaxValue() > 0 && viewportSize_ > 0 && contentSize_ > viewportSize_;
}

}  // namespace velo::ui
