#include "ui/themed_slider.h"

#include <Windows.h>
#include <windowsx.h>

#include <algorithm>

#include "ui/design_tokens.h"

namespace velo::ui {
namespace {

constexpr wchar_t kThemedSliderClassName[] = L"NativPlayerThemedSlider";

ATOM RegisterSliderClass(HINSTANCE instance) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = ThemedSlider::WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kThemedSliderClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_HAND);
    windowClass.hbrBackground = nullptr;
    atom = RegisterClassW(&windowClass);
    return atom;
}

}  // namespace

bool ThemedSlider::Create(HINSTANCE instance, HWND parent, const int controlId) {
    instance_ = instance;
    parent_ = parent;
    controlId_ = controlId;

    RegisterSliderClass(instance_);
    hwnd_ = CreateWindowExW(0, kThemedSliderClassName, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, parent_,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId_)), instance_, this);
    return hwnd_ != nullptr;
}

void ThemedSlider::SetRange(const int minimum, const int maximum) {
    minimum_ = minimum;
    maximum_ = std::max(minimum + 1, maximum);
    SetValue(value_, false);
}

void ThemedSlider::SetValue(const int value, const bool notifyParent) {
    value_ = std::clamp(value, minimum_, maximum_);
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
    if (notifyParent) {
        NotifyParent(kNotificationValueChanged);
    }
}

int ThemedSlider::Value() const noexcept {
    return value_;
}

int ThemedSlider::HoverValue() const noexcept {
    return hoverValue_;
}

bool ThemedSlider::IsDragging() const noexcept {
    return dragging_;
}

HWND ThemedSlider::WindowHandle() const noexcept {
    return hwnd_;
}

LRESULT CALLBACK ThemedSlider::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<ThemedSlider*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }

    auto* self = reinterpret_cast<ThemedSlider*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->HandleMessage(message, wParam, lParam);
}

LRESULT ThemedSlider::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_MOUSEMOVE: {
            hovered_ = true;
            const int hoverValue = ValueFromPoint(GET_X_LPARAM(lParam));
            if (hoverValue_ != hoverValue) {
                hoverValue_ = hoverValue;
                NotifyParent(kNotificationHoverChanged);
            }
            if ((wParam & MK_LBUTTON) != 0) {
                dragging_ = true;
                SetValue(hoverValue_, false);
            }
            TRACKMOUSEEVENT trackMouse{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd_, 0};
            TrackMouseEvent(&trackMouse);
            return 0;
        }

        case WM_MOUSELEAVE:
            hovered_ = false;
            if (GetCapture() != hwnd_) {
                dragging_ = false;
            }
            NotifyParent(kNotificationHoverEnded);
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;

        case WM_LBUTTONDOWN:
            SetFocus(hwnd_);
            SetCapture(hwnd_);
            dragging_ = true;
            hoverValue_ = ValueFromPoint(GET_X_LPARAM(lParam));
            NotifyParent(kNotificationHoverChanged);
            SetValue(hoverValue_, false);
            return 0;

        case WM_LBUTTONUP:
            if (GetCapture() == hwnd_) {
                ReleaseCapture();
            }
            dragging_ = false;
            hoverValue_ = ValueFromPoint(GET_X_LPARAM(lParam));
            NotifyParent(kNotificationHoverChanged);
            SetValue(hoverValue_, true);
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_HOME) {
                SetValue(minimum_, true);
                return 0;
            }
            if (wParam == VK_END) {
                SetValue(maximum_, true);
                return 0;
            }
            if (parent_ != nullptr) {
                const LRESULT handled = SendMessageW(parent_, WM_KEYDOWN, wParam, lParam);
                if (handled == 0) {
                    return 0;
                }
            }
            break;

        case WM_SYSKEYDOWN:
            if (parent_ != nullptr) {
                const LRESULT handled = SendMessageW(parent_, WM_SYSKEYDOWN, wParam, lParam);
                if (handled == 0) {
                    return 0;
                }
            }
            break;

        case WM_PAINT:
            Paint();
            return 0;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        default:
            break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void ThemedSlider::NotifyParent(const WORD notificationCode) const {
    if (parent_ != nullptr) {
        SendMessageW(parent_, WM_COMMAND, MAKEWPARAM(controlId_, notificationCode), reinterpret_cast<LPARAM>(hwnd_));
    }
}

void ThemedSlider::Paint() const {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    RECT client{};
    GetClientRect(hwnd_, &client);

    const auto& palette = tokens::DarkPalette();
    HBRUSH background = CreateSolidBrush(RGB(0, 0, 0));
    if (background != nullptr) {
        FillRect(dc, &client, background);
        DeleteObject(background);
    }

    RECT trackRect = client;
    trackRect.top = client.top + (client.bottom - client.top) / 2 - (hovered_ || dragging_ ? 3 : 2);
    trackRect.bottom = trackRect.top + (hovered_ || dragging_ ? 6 : 4);
    trackRect.left += 6;
    trackRect.right -= 6;

    tokens::FillRoundedRect(dc, trackRect, tokens::MixColor(palette.textPrimary, palette.bgSurface1, 0.82), tokens::RadiusSm());

    RECT valueRect = trackRect;
    const double progress = maximum_ == minimum_ ? 0.0 : static_cast<double>(value_ - minimum_) / (maximum_ - minimum_);
    valueRect.right = valueRect.left + static_cast<LONG>((valueRect.right - valueRect.left) * progress);
    if (valueRect.right < valueRect.left + 4) {
        valueRect.right = valueRect.left + 4;
    }
    tokens::FillRoundedRect(dc, valueRect, hovered_ ? palette.brandHover : palette.brandPrimary, tokens::RadiusSm());

    const int thumbRadius = hovered_ || dragging_ ? 7 : 6;
    const int thumbX = valueRect.right;
    const int thumbY = (client.bottom - client.top) / 2;
    HBRUSH thumbBrush = CreateSolidBrush(palette.textPrimary);
    HPEN thumbPen = CreatePen(PS_SOLID, 1, GetFocus() == hwnd_ ? palette.strokeFocus : palette.bgSurface2);
    const HGDIOBJ oldBrush = thumbBrush != nullptr ? SelectObject(dc, thumbBrush) : nullptr;
    const HGDIOBJ oldPen = thumbPen != nullptr ? SelectObject(dc, thumbPen) : nullptr;
    Ellipse(dc, thumbX - thumbRadius, thumbY - thumbRadius, thumbX + thumbRadius, thumbY + thumbRadius);
    if (oldBrush != nullptr) {
        SelectObject(dc, oldBrush);
    }
    if (oldPen != nullptr) {
        SelectObject(dc, oldPen);
    }
    if (thumbBrush != nullptr) {
        DeleteObject(thumbBrush);
    }
    if (thumbPen != nullptr) {
        DeleteObject(thumbPen);
    }

    EndPaint(hwnd_, &paint);
}

int ThemedSlider::ValueFromPoint(const int x) const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const int left = client.left + 6;
    const int right = client.right - 6;
    const int clamped = std::clamp(x, left, right);
    const double ratio = right == left ? 0.0 : static_cast<double>(clamped - left) / static_cast<double>(right - left);
    return minimum_ + static_cast<int>((maximum_ - minimum_) * ratio);
}

}  // namespace velo::ui
