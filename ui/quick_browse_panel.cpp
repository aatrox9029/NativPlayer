#include "ui/quick_browse_panel.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <utility>
#include <windowsx.h>

#include "localization/localization.h"
#include "ui/design_tokens.h"

namespace velo::ui {
namespace {

constexpr wchar_t kQuickBrowsePanelClassName[] = L"NativPlayerQuickBrowsePanel";
constexpr int kPanelHeaderHeight = 74;
constexpr int kPanelPadding = 14;
constexpr int kEntryHeight = 112;
constexpr int kEntryGap = 10;
constexpr int kScrollbarWidth = 14;
constexpr int kPreviewBarHeight = 14;

enum ControlId {
    kControlScrollbar = 1,
};

ATOM RegisterPanelClass(HINSTANCE instance) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = QuickBrowsePanel::WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kQuickBrowsePanelClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_HAND);
    windowClass.hbrBackground = nullptr;
    atom = RegisterClassW(&windowClass);
    return atom;
}

std::wstring FolderMeta(const std::wstring_view languageCode, const velo::app::QuickBrowseEntry& entry) {
    if (entry.kind == velo::app::QuickBrowseEntryKind::NavigateUp) {
        return velo::localization::Text(languageCode, velo::localization::TextId::QuickBrowseNavigateUp);
    }
    if (entry.videoCount > 0) {
        return std::to_wstring(entry.videoCount) +
               velo::localization::Text(languageCode, velo::localization::TextId::QuickBrowseVideoCountSuffix);
    }
    return velo::localization::Text(languageCode, velo::localization::TextId::QuickBrowseFolder);
}

}  // namespace

bool QuickBrowsePanel::Create(HINSTANCE instance, HWND parent, Callbacks callbacks) {
    instance_ = instance;
    parent_ = parent;
    callbacks_ = std::move(callbacks);
    RegisterPanelClass(instance_);
    hwnd_ = CreateWindowExW(0, kQuickBrowsePanelClassName, L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            0, 0, 0, 0, parent_, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

void QuickBrowsePanel::SetBounds(const RECT& bounds) const {
    if (hwnd_ == nullptr) {
        return;
    }
    MoveWindow(hwnd_, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top, TRUE);
}

void QuickBrowsePanel::SetCatalog(velo::app::QuickBrowseCatalog catalog) {
    StopPreview(true);
    thumbnailProvider_.Clear();
    catalog_ = std::move(catalog);
    hoveredIndex_ = -1;
    ScrollToActiveEntry();
    UpdateScrollbar();
    UpdateThumbnailBudget();
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void QuickBrowsePanel::SetLanguageCode(std::wstring languageCode) {
    languageCode_ = std::move(languageCode);
    if (hwnd_ != nullptr) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void QuickBrowsePanel::SetVisible(const bool visible) {
    if (visible_ == visible) {
        return;
    }
    visible_ = visible;
    if (!visible_) {
        StopPreview(true);
        thumbnailProvider_.Clear();
    }
    if (hwnd_ != nullptr) {
        ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    }
}

bool QuickBrowsePanel::Visible() const noexcept {
    return visible_;
}

bool QuickBrowsePanel::IsPointInside(const POINT parentClientPoint) const {
    if (hwnd_ == nullptr || !visible_) {
        return false;
    }
    RECT rect{};
    GetWindowRect(hwnd_, &rect);
    MapWindowPoints(HWND_DESKTOP, parent_, reinterpret_cast<POINT*>(&rect), 2);
    return PtInRect(&rect, parentClientPoint) != FALSE;
}

bool QuickBrowsePanel::IsScreenPointInside(const POINT screenPoint) const {
    if (hwnd_ == nullptr || !visible_) {
        return false;
    }
    RECT rect{};
    return GetWindowRect(hwnd_, &rect) != FALSE && PtInRect(&rect, screenPoint) != FALSE;
}

HWND QuickBrowsePanel::WindowHandle() const noexcept {
    return hwnd_;
}

LRESULT CALLBACK QuickBrowsePanel::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<QuickBrowsePanel*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }

    auto* self = reinterpret_cast<QuickBrowsePanel*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->HandleMessage(message, wParam, lParam);
}

LRESULT QuickBrowsePanel::HandleMessage(const UINT message, const WPARAM wParam, const LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            titleFont_ = tokens::CreateAppFont(tokens::FontRole::H2);
            bodyFont_ = tokens::CreateAppFont(tokens::FontRole::BodyStrong);
            captionFont_ = tokens::CreateAppFont(tokens::FontRole::Caption);
            scrollbar_.Create(instance_, hwnd_, kControlScrollbar);
            previewWindow_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                             0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
            EnableWindow(previewWindow_, FALSE);
            UpdateScrollbar();
            return 0;

        case WM_SIZE: {
            RECT client{};
            GetClientRect(hwnd_, &client);
            MoveWindow(scrollbar_.WindowHandle(), client.right - kScrollbarWidth - 4, kPanelHeaderHeight, kScrollbarWidth,
                       std::max(0L, client.bottom - kPanelHeaderHeight - kPanelPadding), TRUE);
            UpdatePreviewBounds();
            UpdateScrollbar();
            UpdateThumbnailBudget();
            return 0;
        }

        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd_, 0};
            TrackMouseEvent(&track);
            if (previewProgressDragging_) {
                UpdatePreviewProgressFromPoint({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
                return 0;
            }
            UpdateHoverIndex({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;
        }

        case WM_MOUSELEAVE:
            if (previewProgressDragging_) {
                return 0;
            }
            {
                POINT cursor{};
                if (GetCursorPos(&cursor) != FALSE && IsScreenPointInside(cursor)) {
                    TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd_, 0};
                    TrackMouseEvent(&track);
                    return 0;
                }
            }
            hoveredIndex_ = -1;
            StopPreview();
            InvalidateRect(hwnd_, nullptr, FALSE);
            return 0;

        case WM_MOUSEWHEEL: {
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            SetScrollOffset(scrollOffset_ - (delta / WHEEL_DELTA) * std::max(28, EntryStride() / 2));
            return 0;
        }

        case WM_TIMER:
            if (wParam == kPreviewTimer) {
                if (previewIndex_ < 0) {
                    KillTimer(hwnd_, kPreviewTimer);
                    return 0;
                }
                previewSession_.Pump(0.0);
                if (previewSession_.State().eofReached && !previewPath_.empty()) {
                    previewSession_.Open(previewPath_, false, true);
                }
                InvalidateEntry(previewIndex_);
                return 0;
            }
            if (wParam == kPreviewShutdownTimer) {
                KillTimer(hwnd_, kPreviewShutdownTimer);
                if (previewInitialized_) {
                    previewSession_.Shutdown();
                    previewInitialized_ = false;
                    previewPath_.clear();
                }
                return 0;
            }
            break;

        case WM_LBUTTONDOWN: {
            const POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            const RECT closeButtonRect = CloseButtonRect();
            if (PtInRect(&closeButtonRect, clientPoint) != FALSE) {
                suppressActivateOnRelease_ = true;
                return 0;
            }
            if (IsPreviewProgressHit(clientPoint)) {
                previewProgressDragging_ = true;
                suppressActivateOnRelease_ = true;
                SetCapture(hwnd_);
                UpdatePreviewProgressFromPoint(clientPoint);
                return 0;
            }
            StopPreview();
            suppressActivateOnRelease_ = false;
            return 0;
        }

        case WM_LBUTTONUP:
            if (previewProgressDragging_) {
                previewProgressDragging_ = false;
                if (GetCapture() == hwnd_) {
                    ReleaseCapture();
                }
                UpdatePreviewProgressFromPoint({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
                return 0;
            }
            {
                const RECT closeButtonRect = CloseButtonRect();
                const POINT releasePoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (PtInRect(&closeButtonRect, releasePoint) != FALSE) {
                suppressActivateOnRelease_ = false;
                if (callbacks_.closePanel != nullptr) {
                    callbacks_.closePanel();
                }
                return 0;
                }
            }
            if (suppressActivateOnRelease_) {
                suppressActivateOnRelease_ = false;
                return 0;
            }
            ActivateEntry(HitTestEntry({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}));
            return 0;

        case WM_COMMAND:
            if (reinterpret_cast<HWND>(lParam) == scrollbar_.WindowHandle() &&
                HIWORD(wParam) == ThemedScrollbar::kNotificationValueChanged) {
                SetScrollOffset(scrollbar_.Value());
                return 0;
            }
            break;

        case WM_CAPTURECHANGED:
            previewProgressDragging_ = false;
            return 0;

        case WM_PAINT:
            Paint();
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_NCDESTROY:
            if (titleFont_ != nullptr) {
                DeleteObject(titleFont_);
            }
            if (bodyFont_ != nullptr) {
                DeleteObject(bodyFont_);
            }
            if (captionFont_ != nullptr) {
                DeleteObject(captionFont_);
            }
            StopPreview(true);
            previewSession_.Shutdown();
            thumbnailProvider_.Clear();
            return DefWindowProcW(hwnd_, message, wParam, lParam);

        default:
            break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void QuickBrowsePanel::Paint() {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    RECT client{};
    GetClientRect(hwnd_, &client);
    const auto& palette = tokens::DarkPalette();

    tokens::FillVerticalGradient(dc, client, tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.18), palette.bgCanvas);

    RECT contentRect = ContentRect();
    if (catalog_.Empty()) {
        RECT headerRect = HeaderRect();
        tokens::FillRoundedRect(dc, headerRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.16), tokens::RadiusLg());
        tokens::DrawRoundedOutline(dc, headerRect, palette.strokeSoft, tokens::RadiusLg());
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, palette.textPrimary);
        HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(dc, titleFont_));
        RECT titleRect = headerRect;
        titleRect.left += kPanelPadding;
        titleRect.top += 10;
        const std::wstring quickBrowseTitle = velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowseTitle);
        DrawTextW(dc, quickBrowseTitle.c_str(), -1, &titleRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
        SelectObject(dc, captionFont_);
        SetTextColor(dc, palette.textSecondary);
        RECT subtitleRect = headerRect;
        subtitleRect.left += kPanelPadding;
        subtitleRect.top += 40;
          const std::wstring currentFolderLabel = catalog_.currentFolder.empty()
                                        ? velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowseNotLoadedLocalMedia)
                                                    : std::filesystem::path(catalog_.currentFolder).filename().wstring().empty()
                                                          ? catalog_.currentFolder
                                                          : std::filesystem::path(catalog_.currentFolder).filename().wstring();
        DrawTextW(dc, currentFolderLabel.c_str(), -1, &subtitleRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, previousFont);
        DrawCloseButton(dc);
        SetTextColor(dc, palette.textSecondary);
        SelectObject(dc, bodyFont_);
        const std::wstring emptyText = velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowseNoSwitchableVideos);
        DrawTextW(dc, emptyText.c_str(), -1, &contentRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        SelectObject(dc, previousFont);
        EndPaint(hwnd_, &paint);
        return;
    }

    const int savedDc = SaveDC(dc);
    IntersectClipRect(dc, contentRect.left, contentRect.top, contentRect.right, contentRect.bottom);

    for (int index = 0; index < static_cast<int>(catalog_.EntryCount()); ++index) {
        RECT entryRect = EntryRect(index);
        if (entryRect.bottom < contentRect.top || entryRect.top > contentRect.bottom) {
            continue;
        }

        const auto entry = catalog_.EntryAt(index);
        const bool hovered = index == hoveredIndex_;
        const bool previewActive = index == previewIndex_ && previewWindow_ != nullptr && IsWindowVisible(previewWindow_) != FALSE;
        const COLORREF fillColor = entry.active
                                       ? tokens::MixColor(palette.brandPrimary, palette.bgSurface2, 0.88)
                                       : (hovered ? palette.bgSurface2 : tokens::MixColor(palette.bgSurface2, palette.bgCanvas, 0.24));
        const COLORREF outlineColor = entry.active
                                          ? tokens::MixColor(palette.brandHover, palette.strokeSoft, 0.36)
                                          : (hovered ? palette.strokeFocus : palette.strokeSoft);
        tokens::FillRoundedRect(dc, entryRect, fillColor, tokens::RadiusLg());
        tokens::DrawRoundedOutline(dc, entryRect, outlineColor, tokens::RadiusLg(), previewActive ? 2 : (hovered ? 2 : 1));

        DrawThumbnail(dc, entryRect, entry, previewActive);

        RECT titleTextRect = TextRect(entryRect);
        titleTextRect.bottom = entryRect.bottom - 14;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, palette.textPrimary);
        SelectObject(dc, bodyFont_);
        DrawTextW(dc, entry.label.c_str(), -1, &titleTextRect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);

        RECT metaRect = TextRect(entryRect);
        metaRect.top = entryRect.bottom - 34;
        metaRect.bottom = entryRect.bottom - 12;
        SetTextColor(dc, entry.active ? palette.bgCanvas : palette.textSecondary);
        SelectObject(dc, captionFont_);
        std::wstring metaText;
        switch (entry.kind) {
            case velo::app::QuickBrowseEntryKind::NavigateUp:
            case velo::app::QuickBrowseEntryKind::Folder:
                metaText = FolderMeta(languageCode_, entry);
                break;
            case velo::app::QuickBrowseEntryKind::Video:
                metaText.clear();
                break;
        }
        if (!metaText.empty()) {
            DrawTextW(dc, metaText.c_str(), -1, &metaRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }

        if (previewActive && previewSession_.State().durationSeconds > 0.0) {
            RECT barRect = ProgressBarRect(entryRect);
            tokens::FillRoundedRect(dc, barRect, tokens::MixColor(palette.bgCanvas, palette.bgOverlay, 0.16), tokens::RadiusSm());
            RECT fillRect = barRect;
            const double ratio = std::clamp(previewSession_.State().positionSeconds / previewSession_.State().durationSeconds, 0.0, 1.0);
            fillRect.right = fillRect.left + static_cast<LONG>(std::lround((fillRect.right - fillRect.left) * ratio));
            if (fillRect.right > fillRect.left) {
                tokens::FillRoundedRect(dc, fillRect, palette.brandHover, tokens::RadiusSm());
            }
        }
    }

    RestoreDC(dc, savedDc);

    RECT headerRect = HeaderRect();
    tokens::FillRoundedRect(dc, headerRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.16), tokens::RadiusLg());
    tokens::DrawRoundedOutline(dc, headerRect, palette.strokeSoft, tokens::RadiusLg());

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, palette.textPrimary);
    HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(dc, titleFont_));
    RECT titleRect = headerRect;
    titleRect.left += kPanelPadding;
    titleRect.top += 10;
    const std::wstring quickBrowseTitle = velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowseTitle);
    DrawTextW(dc, quickBrowseTitle.c_str(), -1, &titleRect, DT_LEFT | DT_TOP | DT_SINGLELINE);

    SelectObject(dc, captionFont_);
    SetTextColor(dc, palette.textSecondary);
    RECT subtitleRect = headerRect;
    subtitleRect.left += kPanelPadding;
    subtitleRect.top += 40;
    const std::wstring currentFolderLabel = catalog_.currentFolder.empty()
                                                ? velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowseNotLoadedLocalMedia)
                                                : std::filesystem::path(catalog_.currentFolder).filename().wstring().empty()
                                                      ? catalog_.currentFolder
                                                      : std::filesystem::path(catalog_.currentFolder).filename().wstring();
    DrawTextW(dc, currentFolderLabel.c_str(), -1, &subtitleRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(dc, previousFont);
    DrawCloseButton(dc);

    SelectObject(dc, previousFont);
    EndPaint(hwnd_, &paint);
}

void QuickBrowsePanel::UpdateScrollbar() {
    if (scrollbar_.WindowHandle() == nullptr) {
        return;
    }

    RECT contentRect = ContentRect();
    scrollbar_.SetMetrics(std::max(0L, contentRect.bottom - contentRect.top), ContentHeight());
    scrollbar_.SetValue(std::clamp(scrollOffset_, 0, MaxScrollOffset()));
}

void QuickBrowsePanel::UpdateThumbnailBudget() {
    const RECT contentRect = ContentRect();
    const int visibleHeight = std::max(0L, contentRect.bottom - contentRect.top);
    const int visibleEntries = std::max(1, visibleHeight / std::max(1, EntryStride()) + 2);
    thumbnailBudget_ = std::clamp(visibleEntries * 2, 8, 48);
    thumbnailProvider_.SetMaxEntries(static_cast<size_t>(thumbnailBudget_));
}

void QuickBrowsePanel::SetScrollOffset(const int offset) {
    const int nextOffset = std::clamp(offset, 0, MaxScrollOffset());
    if (nextOffset == scrollOffset_) {
        return;
    }
    StopPreview();
    scrollOffset_ = nextOffset;
    UpdateScrollbar();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void QuickBrowsePanel::ScrollToActiveEntry() {
    const RECT contentRect = ContentRect();
    const int visibleHeight = std::max(0L, contentRect.bottom - contentRect.top);
    if (catalog_.activeEntryIndex < 0 || visibleHeight <= 0) {
        scrollOffset_ = 0;
        return;
    }

    const int entryTop = catalog_.activeEntryIndex * EntryStride();
    scrollOffset_ = std::clamp(entryTop - visibleHeight / 3, 0, MaxScrollOffset());
}

void QuickBrowsePanel::UpdateHoverIndex(const POINT clientPoint) {
    const int nextIndex = HitTestEntry(clientPoint);
    if (nextIndex == hoveredIndex_) {
        return;
    }
    InvalidateEntry(hoveredIndex_);
    hoveredIndex_ = nextIndex;
    if (hoveredIndex_ >= 0 && hoveredIndex_ < static_cast<int>(catalog_.EntryCount()) &&
        catalog_.EntryAt(hoveredIndex_).kind == velo::app::QuickBrowseEntryKind::Video) {
        StartPreview(hoveredIndex_);
    } else {
        StopPreview();
    }
    InvalidateEntry(hoveredIndex_);
}

void QuickBrowsePanel::InvalidateEntry(const int index) const {
    if (hwnd_ == nullptr || index < 0 || index >= static_cast<int>(catalog_.EntryCount())) {
        return;
    }
    RECT rect = EntryRect(index);
    InvalidateRect(hwnd_, &rect, FALSE);
}

void QuickBrowsePanel::StartPreview(const int index) {
    if (index < 0 || index >= static_cast<int>(catalog_.EntryCount())) {
        StopPreview();
        return;
    }

    const auto entry = catalog_.EntryAt(index);
    if (entry.kind != velo::app::QuickBrowseEntryKind::Video || entry.path.empty()) {
        StopPreview();
        return;
    }
    if (previewIndex_ == index && previewPath_ == entry.path) {
        return;
    }

    KillTimer(hwnd_, kPreviewShutdownTimer);
    previewInitialized_ = previewSession_.EnsureInitialized(previewWindow_, L"auto");
    if (!previewInitialized_) {
        previewIndex_ = -1;
        previewPath_.clear();
        return;
    }

    InvalidateEntry(previewIndex_);
    previewIndex_ = index;
    previewPath_ = entry.path;
    previewSession_.Open(entry.path, false, true);
    previewSession_.SetVolume(0.0);
    UpdatePreviewBounds();
    ShowWindow(previewWindow_, SW_SHOWNA);
    SetTimer(hwnd_, kPreviewTimer, 45, nullptr);
}

void QuickBrowsePanel::StopPreview(const bool releaseSession) {
    KillTimer(hwnd_, kPreviewTimer);
    if (previewInitialized_) {
        previewSession_.Stop();
    }
    if (releaseSession) {
        KillTimer(hwnd_, kPreviewShutdownTimer);
        if (previewInitialized_) {
            previewSession_.Shutdown();
            previewInitialized_ = false;
        }
    } else {
        SchedulePreviewShutdown();
    }
    if (previewWindow_ != nullptr) {
        ShowWindow(previewWindow_, SW_HIDE);
    }
    InvalidateEntry(previewIndex_);
    previewIndex_ = -1;
    previewPath_.clear();
    previewProgressDragging_ = false;
    suppressActivateOnRelease_ = false;
}

void QuickBrowsePanel::SchedulePreviewShutdown() {
    if (hwnd_ == nullptr || !previewInitialized_) {
        return;
    }
    SetTimer(hwnd_, kPreviewShutdownTimer, 600, nullptr);
}

void QuickBrowsePanel::UpdatePreviewBounds() {
    if (previewWindow_ == nullptr || previewIndex_ < 0 || previewIndex_ >= static_cast<int>(catalog_.EntryCount())) {
        return;
    }

    RECT entryRect = EntryRect(previewIndex_);
    RECT contentRect = ContentRect();
    if (entryRect.bottom < contentRect.top || entryRect.top > contentRect.bottom || entryRect.top < contentRect.top || entryRect.bottom > contentRect.bottom) {
        StopPreview();
        return;
    }

    RECT previewRect = PreviewRect(entryRect);
    MoveWindow(previewWindow_, previewRect.left, previewRect.top, std::max(0L, previewRect.right - previewRect.left),
               std::max(0L, previewRect.bottom - previewRect.top), TRUE);
    SetWindowPos(previewWindow_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

bool QuickBrowsePanel::UpdatePreviewProgressFromPoint(const POINT clientPoint) {
    int hitIndex = -1;
    if (!IsPreviewProgressHit(clientPoint, &hitIndex) || hitIndex != previewIndex_ || previewSession_.State().durationSeconds <= 0.0) {
        return false;
    }

    const RECT barRect = ProgressBarRect(EntryRect(hitIndex));
    const int width = std::max(1L, barRect.right - barRect.left);
    const double ratio = std::clamp(static_cast<double>(clientPoint.x - barRect.left) / static_cast<double>(width), 0.0, 1.0);
    const double targetSeconds = previewSession_.State().durationSeconds * ratio;
    previewSession_.SeekAbsolute(targetSeconds);
    InvalidateEntry(hitIndex);
    return true;
}

void QuickBrowsePanel::DrawThumbnail(HDC dc, const RECT& entryRect, const velo::app::QuickBrowseEntry& entry, const bool previewActive) {
    const auto& palette = tokens::DarkPalette();
    RECT thumbnailRect = ThumbnailRect(entryRect);
    tokens::FillRoundedRect(dc, thumbnailRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.14), tokens::RadiusMd());
    tokens::DrawRoundedOutline(dc, thumbnailRect, palette.strokeSoft, tokens::RadiusMd());

    if (!previewActive) {
        SIZE requestedSize{thumbnailRect.right - thumbnailRect.left, thumbnailRect.bottom - thumbnailRect.top};
        const HBITMAP bitmap = thumbnailProvider_.GetThumbnail(entry.path, requestedSize);
        if (bitmap != nullptr) {
            DrawBitmap(dc, bitmap, thumbnailRect);
        } else {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, palette.textSecondary);
            const auto previousFont = reinterpret_cast<HFONT>(SelectObject(dc, captionFont_));
            const std::wstring fallback = entry.kind == velo::app::QuickBrowseEntryKind::Video
                                              ? velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowsePreview)
                                              : velo::localization::Text(languageCode_, velo::localization::TextId::QuickBrowseFolder);
            DrawTextW(dc, fallback.c_str(), -1, &thumbnailRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, previousFont);
        }
    }
}

void QuickBrowsePanel::DrawBitmap(HDC dc, HBITMAP bitmap, const RECT& targetRect) const {
    if (bitmap == nullptr) {
        return;
    }

    BITMAP bitmapInfo{};
    if (GetObjectW(bitmap, sizeof(bitmapInfo), &bitmapInfo) == 0 || bitmapInfo.bmWidth <= 0 || bitmapInfo.bmHeight <= 0) {
        return;
    }

    const int targetWidth = targetRect.right - targetRect.left;
    const int targetHeight = targetRect.bottom - targetRect.top;
    if (targetWidth <= 0 || targetHeight <= 0) {
        return;
    }

    HDC memoryDc = CreateCompatibleDC(dc);
    if (memoryDc == nullptr) {
        return;
    }
    const int savedDc = SaveDC(dc);
    const auto previousBitmap = SelectObject(memoryDc, bitmap);
    SetStretchBltMode(dc, HALFTONE);
    IntersectClipRect(dc, targetRect.left, targetRect.top, targetRect.right, targetRect.bottom);

    const double scale = std::max(static_cast<double>(targetWidth) / static_cast<double>(bitmapInfo.bmWidth),
                                  static_cast<double>(targetHeight) / static_cast<double>(bitmapInfo.bmHeight));
    const int drawWidth = static_cast<int>(std::lround(bitmapInfo.bmWidth * scale));
    const int drawHeight = static_cast<int>(std::lround(bitmapInfo.bmHeight * scale));
    const int drawLeft = targetRect.left + (targetWidth - drawWidth) / 2;
    const int drawTop = targetRect.top + (targetHeight - drawHeight) / 2;
    StretchBlt(dc, drawLeft, drawTop, drawWidth, drawHeight, memoryDc, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight, SRCCOPY);

    SelectObject(memoryDc, previousBitmap);
    RestoreDC(dc, savedDc);
    DeleteDC(memoryDc);
}

void QuickBrowsePanel::DrawCloseButton(HDC dc) const {
    const auto& palette = tokens::DarkPalette();
    RECT closeRect = CloseButtonRect();
    tokens::FillRoundedRect(dc, closeRect, tokens::MixColor(palette.bgSurface2, palette.bgCanvas, 0.10), tokens::RadiusMd());
    tokens::DrawRoundedOutline(dc, closeRect, palette.strokeSoft, tokens::RadiusMd());
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, palette.textPrimary);
    const auto previousFont = reinterpret_cast<HFONT>(SelectObject(dc, bodyFont_));
    DrawTextW(dc, L"X", -1, &closeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, previousFont);
}

void QuickBrowsePanel::ActivateEntry(const int index) const {
    if (index < 0 || index >= static_cast<int>(catalog_.EntryCount())) {
        return;
    }

    const auto entry = catalog_.EntryAt(index);
    if (entry.path.empty()) {
        return;
    }

    if (entry.kind == velo::app::QuickBrowseEntryKind::Video) {
        if (callbacks_.openFile != nullptr) {
            callbacks_.openFile(entry.path);
        }
        return;
    }

    if (callbacks_.navigateFolder != nullptr) {
        callbacks_.navigateFolder(entry.path);
    }
}

RECT QuickBrowsePanel::HeaderRect() const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    return RECT{kPanelPadding, kPanelPadding, client.right - kPanelPadding, kPanelPadding + kPanelHeaderHeight - 10};
}

RECT QuickBrowsePanel::CloseButtonRect() const {
    RECT headerRect = HeaderRect();
    return RECT{headerRect.right - 34, headerRect.top + 10, headerRect.right - 4, headerRect.top + 40};
}

RECT QuickBrowsePanel::ContentRect() const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    return RECT{kPanelPadding, kPanelHeaderHeight + kPanelPadding + 4, client.right - kScrollbarWidth - kPanelPadding - 8,
                client.bottom - kPanelPadding};
}

RECT QuickBrowsePanel::EntryRect(const int index) const {
    RECT contentRect = ContentRect();
    const int top = contentRect.top + index * EntryStride() - scrollOffset_;
    return RECT{contentRect.left, top, contentRect.right, top + kEntryHeight};
}

RECT QuickBrowsePanel::ThumbnailRect(const RECT& entryRect) const {
    const int entryWidth = static_cast<int>(entryRect.right - entryRect.left);
    const int width = std::clamp(entryWidth / 2, 104, 146);
    return RECT{entryRect.left + 10, entryRect.top + 10, entryRect.left + 10 + width, entryRect.bottom - 10};
}

RECT QuickBrowsePanel::ProgressBarRect(const RECT& entryRect) const {
    RECT rect = ThumbnailRect(entryRect);
    rect.left += 8;
    rect.right -= 8;
    rect.top = rect.bottom - (kPreviewBarHeight + 10);
    rect.bottom = rect.top + kPreviewBarHeight;
    return rect;
}

RECT QuickBrowsePanel::PreviewRect(const RECT& entryRect) const {
    RECT rect = ThumbnailRect(entryRect);
    rect.left += 1;
    rect.top += 1;
    rect.right -= 1;
    rect.bottom = ProgressBarRect(entryRect).top - 6;
    return rect;
}

RECT QuickBrowsePanel::TextRect(const RECT& entryRect) const {
    RECT thumbnailRect = ThumbnailRect(entryRect);
    return RECT{thumbnailRect.right + 12, entryRect.top + 12, entryRect.right - 12, entryRect.bottom - 16};
}

int QuickBrowsePanel::EntryStride() const {
    return kEntryHeight + kEntryGap;
}

int QuickBrowsePanel::ContentHeight() const {
    return static_cast<int>(catalog_.EntryCount()) * EntryStride();
}

int QuickBrowsePanel::MaxScrollOffset() const {
    const RECT contentRect = ContentRect();
    const int visibleHeight = std::max(0L, contentRect.bottom - contentRect.top);
    return std::max(0, ContentHeight() - visibleHeight);
}

int QuickBrowsePanel::HitTestEntry(const POINT clientPoint) const {
    const RECT contentRect = ContentRect();
    if (PtInRect(&contentRect, clientPoint) == FALSE) {
        return -1;
    }

    for (int index = 0; index < static_cast<int>(catalog_.EntryCount()); ++index) {
        const RECT entryRect = EntryRect(index);
        if (PtInRect(&entryRect, clientPoint) != FALSE) {
            return index;
        }
    }
    return -1;
}

bool QuickBrowsePanel::IsPreviewProgressHit(const POINT clientPoint, int* hitIndex) const {
    if (previewIndex_ < 0 || previewIndex_ >= static_cast<int>(catalog_.EntryCount()) || previewSession_.State().durationSeconds <= 0.0) {
        return false;
    }

    const RECT entryRect = EntryRect(previewIndex_);
    const RECT progressRect = ProgressBarRect(entryRect);
    if (PtInRect(&progressRect, clientPoint) == FALSE) {
        return false;
    }
    if (hitIndex != nullptr) {
        *hitIndex = previewIndex_;
    }
    return true;
}

}  // namespace velo::ui
