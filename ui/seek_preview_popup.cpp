#include "ui/seek_preview_popup.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>

#include "ui/design_tokens.h"

namespace velo::ui {
namespace {

constexpr wchar_t kSeekPreviewPopupClassName[] = L"NativPlayerSeekPreviewPopup";
constexpr int kPopupWidth = 224;
constexpr int kPopupHeight = 154;
constexpr int kLabelHeight = 28;

HBRUSH PopupFooterBrush() {
    static HBRUSH brush = CreateSolidBrush(tokens::MixColor(tokens::DarkPalette().bgOverlay, tokens::DarkPalette().bgSurface1, 0.20));
    return brush;
}

ATOM RegisterSeekPreviewPopupClass(HINSTANCE instance) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = SeekPreviewPopup::WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kSeekPreviewPopupClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    atom = RegisterClassW(&windowClass);
    return atom;
}

}  // namespace

bool SeekPreviewPopup::Create(HINSTANCE instance, HWND parent) {
    instance_ = instance;
    parent_ = parent;
    RegisterSeekPreviewPopupClass(instance_);
    hwnd_ = CreateWindowExW(0, kSeekPreviewPopupClassName, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, kPopupWidth,
                            kPopupHeight, parent_, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

void SeekPreviewPopup::SetEnabled(const bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        Hide();
    }
}

void SeekPreviewPopup::SetMedia(const std::wstring& path, const std::wstring& hwdecPolicy) {
    currentPath_ = path;
    hwdecPolicy_ = hwdecPolicy.empty() ? L"auto" : hwdecPolicy;
    if (loadedPath_ != currentPath_) {
        loadedPath_.clear();
        lastSeekSeconds_ = -1.0;
        seekPending_ = false;
    }
}

void SeekPreviewPopup::ShowAt(const RECT& anchorRect, const double sliderRatio, const double targetSeconds, const double durationSeconds) {
    if (!enabled_ || hwnd_ == nullptr || currentPath_.empty()) {
        Hide();
        return;
    }

    durationSeconds_ = durationSeconds;
    EnsurePreviewPlayer();
    QueueSeek(targetSeconds);

    RECT parentClient{};
    GetClientRect(parent_, &parentClient);
    const int anchorWidth = std::max(1L, anchorRect.right - anchorRect.left);
    const int xCenter = anchorRect.left + static_cast<int>(std::lround(anchorWidth * std::clamp(sliderRatio, 0.0, 1.0)));
    const int x = std::clamp(xCenter - kPopupWidth / 2, static_cast<int>(parentClient.left) + 8,
                             static_cast<int>(parentClient.right) - kPopupWidth - 8);
    const int y = std::max(8, static_cast<int>(anchorRect.top) - kPopupHeight - 14);

    SetWindowPos(hwnd_, HWND_TOP, x, y, kPopupWidth, kPopupHeight, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    visible_ = true;
    SetWindowTextW(timeLabel_, FormatTimeLabel(targetSeconds).c_str());
    SetTimer(hwnd_, kPreviewTimer, 45, nullptr);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void SeekPreviewPopup::Hide() {
    if (hwnd_ == nullptr) {
        return;
    }
    KillTimer(hwnd_, kPreviewTimer);
    previewSession_.Shutdown();
    previewInitialized_ = false;
    loadedPath_.clear();
    seekPending_ = false;
    lastSeekSeconds_ = -1.0;
    visible_ = false;
    ShowWindow(hwnd_, SW_HIDE);
}

bool SeekPreviewPopup::Visible() const noexcept {
    return visible_;
}

HWND SeekPreviewPopup::WindowHandle() const noexcept {
    return hwnd_;
}

LRESULT CALLBACK SeekPreviewPopup::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<SeekPreviewPopup*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }

    auto* self = reinterpret_cast<SeekPreviewPopup*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->HandleMessage(message, wParam, lParam);
}

LRESULT SeekPreviewPopup::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            bodyFont_ = tokens::CreateAppFont(tokens::FontRole::BodyStrong);
            videoHost_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                         0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
            timeLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, 0, 0, 0, 0, hwnd_,
                                         nullptr, instance_, nullptr);
            SendMessageW(timeLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(bodyFont_), TRUE);
            UpdateLayout();
            return 0;

        case WM_SIZE:
            UpdateLayout();
            return 0;

        case WM_TIMER:
            if (wParam == kPreviewTimer) {
                previewSession_.Pump(0.0);
                if (seekPending_ && previewSession_.State().isLoaded) {
                    previewSession_.SetPause(true);
                    previewSession_.SeekAbsolute(queuedSeekSeconds_);
                    lastSeekSeconds_ = queuedSeekSeconds_;
                    seekPending_ = false;
                }
                return 0;
            }
            break;

        case WM_PAINT:
            Paint();
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            const auto& palette = tokens::DarkPalette();
            SetBkMode(dc, OPAQUE);
            SetTextColor(dc, palette.textPrimary);
            SetBkColor(dc, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.20));
            return reinterpret_cast<LRESULT>(PopupFooterBrush());
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_NCDESTROY:
            Hide();
            previewSession_.Shutdown();
            if (bodyFont_ != nullptr) {
                DeleteObject(bodyFont_);
            }
            hwnd_ = nullptr;
            return DefWindowProcW(hwnd_, message, wParam, lParam);

        default:
            break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void SeekPreviewPopup::EnsurePreviewPlayer() {
    if (previewInitialized_ || videoHost_ == nullptr) {
        return;
    }
    previewInitialized_ = previewSession_.EnsureInitialized(videoHost_, hwdecPolicy_);
}

void SeekPreviewPopup::UpdateLayout() {
    RECT client{};
    GetClientRect(hwnd_, &client);
    MoveWindow(videoHost_, 10, 10, std::max(0L, client.right - 20), std::max(0L, client.bottom - kLabelHeight - 18), TRUE);
    MoveWindow(timeLabel_, 10, client.bottom - kLabelHeight - 4, std::max(0L, client.right - 20), kLabelHeight, TRUE);
}

void SeekPreviewPopup::Paint() const {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    RECT client{};
    GetClientRect(hwnd_, &client);
    const auto& palette = tokens::DarkPalette();

    tokens::FillRoundedRect(dc, client, tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.06), tokens::RadiusLg());
    tokens::DrawRoundedOutline(dc, client, palette.strokeSoft, tokens::RadiusLg());

    RECT footerRect = client;
    footerRect.top = footerRect.bottom - kLabelHeight - 8;
    tokens::FillRoundedRect(dc, footerRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.20), tokens::RadiusMd());

    EndPaint(hwnd_, &paint);
}

void SeekPreviewPopup::QueueSeek(const double targetSeconds) {
    if (!previewInitialized_) {
        queuedSeekSeconds_ = std::max(0.0, targetSeconds);
        seekPending_ = true;
        return;
    }

    if (loadedPath_ != currentPath_ || previewSession_.LoadedPath() != currentPath_) {
        loadedPath_ = currentPath_;
        previewSession_.Open(currentPath_, true, true);
        queuedSeekSeconds_ = std::max(0.0, targetSeconds);
        seekPending_ = true;
        return;
    }

    queuedSeekSeconds_ = std::max(0.0, targetSeconds);
    if (previewSession_.State().isLoaded && std::abs(lastSeekSeconds_ - queuedSeekSeconds_) > 0.20) {
        previewSession_.SetPause(true);
        previewSession_.SeekAbsolute(queuedSeekSeconds_);
        lastSeekSeconds_ = queuedSeekSeconds_;
        seekPending_ = false;
    } else {
        seekPending_ = true;
    }
}

}  // namespace velo::ui
