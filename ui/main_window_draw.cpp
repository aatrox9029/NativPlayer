#include "ui/main_window_internal.h"

#include "ui/button_style_assets.h"

namespace {

std::wstring_view FullscreenCaptionAssetName(const UINT controlId) {
    switch (controlId) {
        case velo::ui::kControlFullscreenDownload:
            return L"download";
        case velo::ui::kControlFullscreenMinimize:
            return L"\u7e2e\u5c0f";
        case velo::ui::kControlFullscreenWindowed:
            return L"\u8996\u7a97\u5316";
        case velo::ui::kControlFullscreenClose:
            return L"\u95dc\u9589";
        default:
            return {};
    }
}

RECT IconImageBounds(const RECT& source, const bool compactControl) {
    RECT bounds = source;
    InflateRect(&bounds, compactControl ? -1 : -2, compactControl ? -1 : -2);
    return bounds;
}

}  // namespace

namespace velo::ui {

double MainWindow::NextPresetSpeed(const double currentSpeed) const {
    constexpr double presets[] = {0.80, 1.00, 1.25, 1.50, 2.00};
    for (const double preset : presets) {
        if (preset > currentSpeed + 0.01) {
            return preset;
        }
    }
    return presets[0];
}

void MainWindow::PaintWindowBackdrop(HDC dc, const RECT& clientRect) const {
    const auto& palette = tokens::DarkPalette();
    const bool effectiveControlsVisible = !uiSuppressed_ && (quickBrowseVisible_ || controlsVisible_);
    const bool captionBarVisible = !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_);
    const MainWindowLayout layout = ComputeMainWindowLayout(clientRect, quickBrowseVisible_, fullscreen_, effectiveControlsVisible || captionBarVisible);

    // Fullscreen transitions should always reveal an app-owned opaque surface.
    FillRect(dc, &clientRect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    if (!fullscreen_) {
        tokens::FillVerticalGradient(dc, clientRect, tokens::MixColor(palette.bgCanvas, palette.bgSurface1, 0.28), palette.bgCanvas);
        RECT topGlow{0, 0, clientRect.right, std::min<LONG>(220, clientRect.bottom)};
        tokens::FillVerticalGradient(dc, topGlow, tokens::MixColor(palette.brandPrimary, palette.bgCanvas, 0.88), palette.bgCanvas);
    }

    if (captionBarVisible && layout.titleBarRect.bottom > layout.titleBarRect.top) {
        FillRect(dc, &layout.titleBarRect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    }

    if (!uiSuppressed_ && mediaInfoVisible_) {
        RECT infoRect{layout.contentLeft + 16, 64, std::min<LONG>(clientRect.right - 16, layout.contentLeft + 16 + kInfoPanelWidth), 210};
        tokens::FillRoundedRect(dc, infoRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.18), tokens::RadiusLg());
        tokens::DrawRoundedOutline(dc, infoRect, palette.strokeSoft, tokens::RadiusLg());
    }

    if (!uiSuppressed_ && !state_.isLoaded) {
        RECT dropRect = EmptyDropZoneRect();
        tokens::FillRoundedRect(dc, dropRect, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.14), tokens::RadiusLg());
        tokens::DrawRoundedOutline(dc, dropRect, palette.strokeFocus, tokens::RadiusLg(), 2);
        RECT hintRect = dropRect;
        hintRect.top += 26;
        hintRect.bottom = hintRect.top + 42;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, palette.textPrimary);
        const auto previousFont = reinterpret_cast<HFONT>(SelectObject(dc, osdFont_));
        const std::wstring primaryHint = velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEmptyHint);
        DrawTextW(dc, primaryHint.c_str(), -1, &hintRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, captionFont_);
        RECT subRect = dropRect;
        subRect.top += 78;
        SetTextColor(dc, palette.textSecondary);
        const std::wstring secondaryHint = velo::localization::Text(config_.languageCode, velo::localization::TextId::MainStatusReady);
        DrawTextW(dc, secondaryHint.c_str(), -1, &subRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, previousFont);
    }

    if (!uiSuppressed_ && effectiveControlsVisible && state_.isLoaded) {
        RECT panelRect = layout.panelRect;
        FillRect(dc, &panelRect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        RECT panelOutline = panelRect;
        panelOutline.top += 1;
        tokens::DrawRoundedOutline(dc, panelOutline, tokens::MixColor(palette.strokeSoft, RGB(0, 0, 0), 0.72), tokens::RadiusSm());
        RECT speedPanelRect = layout.speedPanelRect;
        RECT volumePanelRect = layout.volumePanelRect;
        tokens::FillRoundedRect(dc, speedPanelRect, RGB(0, 0, 0), tokens::RadiusMd());
        tokens::FillRoundedRect(dc, volumePanelRect, RGB(0, 0, 0), tokens::RadiusMd());
        tokens::DrawRoundedOutline(dc, speedPanelRect, tokens::MixColor(palette.strokeSoft, RGB(0, 0, 0), 0.66), tokens::RadiusMd());
        tokens::DrawRoundedOutline(dc, volumePanelRect, tokens::MixColor(palette.strokeSoft, RGB(0, 0, 0), 0.66), tokens::RadiusMd());
    }

    if (endPromptVisible_) {
        const int promptCenterX = fullscreen_ ? layout.currentTimeCenterX : static_cast<int>(clientRect.right) / 2;
        RECT promptRect{std::max(24, promptCenterX - (fullscreen_ ? 190 : 240)),
                        fullscreen_ ? std::max(28, layout.panelTop - 106) : std::max(40, static_cast<int>(clientRect.bottom) / 2 - 120),
                        std::min(static_cast<int>(clientRect.right) - 24, promptCenterX + (fullscreen_ ? 190 : 240)),
                        fullscreen_ ? std::max(28, layout.panelTop - 106) + 126
                                    : std::min(static_cast<int>(clientRect.bottom) - 90, static_cast<int>(clientRect.bottom) / 2 + 70)};
        tokens::FillRoundedRect(dc, promptRect, tokens::MixColor(palette.bgSurface1, palette.bgOverlay, 0.24), tokens::RadiusLg());
        tokens::DrawRoundedOutline(dc, promptRect, palette.strokeSoft, tokens::RadiusLg());
    }
}

void MainWindow::DrawButton(const DRAWITEMSTRUCT& drawItem) const {
    if (drawItem.CtlType != ODT_BUTTON) {
        return;
    }

    const auto& palette = tokens::DarkPalette();
    const bool captionButton = drawItem.CtlID == kControlFullscreenDownload || drawItem.CtlID == kControlFullscreenMinimize ||
                               drawItem.CtlID == kControlFullscreenWindowed || drawItem.CtlID == kControlFullscreenClose;
    const bool flatBottomControl = drawItem.CtlID == kControlOpen || drawItem.CtlID == kControlQuickBrowse ||
                                   drawItem.CtlID == kControlPreviousTrack || drawItem.CtlID == kControlPlayPause || drawItem.CtlID == kControlNextTrack ||
                                   drawItem.CtlID == kControlVolumeToggle || drawItem.CtlID == kControlSpeed || drawItem.CtlID == kControlSubtitle ||
                                   drawItem.CtlID == kControlSettings || drawItem.CtlID == kControlFullscreen || drawItem.CtlID == kControlMore;
    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);
    RECT controlRect{};
    GetWindowRect(drawItem.hwndItem, &controlRect);
    MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<POINT*>(&controlRect), 2);
    const int savedState = SaveDC(drawItem.hDC);
    SetViewportOrgEx(drawItem.hDC, -controlRect.left, -controlRect.top, nullptr);
    PaintWindowBackdrop(drawItem.hDC, clientRect);
    RestoreDC(drawItem.hDC, savedState);

    if (captionButton) {
        wchar_t text[8] = {};
        GetWindowTextW(drawItem.hwndItem, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
        RECT imageBounds = IconImageBounds(drawItem.rcItem, false);
        if (!DrawButtonStyleIcon(instance_, FullscreenCaptionAssetName(drawItem.CtlID), drawItem.hDC, imageBounds,
                                 (drawItem.itemState & ODS_DISABLED) != 0)) {
            SetBkMode(drawItem.hDC, TRANSPARENT);
            SetTextColor(drawItem.hDC,
                         drawItem.CtlID == kControlFullscreenClose ? palette.error
                                                                   : (drawItem.CtlID == kControlFullscreenDownload ? palette.brandHover
                                                                                                                   : palette.textPrimary));
            HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(drawItem.hDC, largeTextGlyphFont_));
            DrawTextW(drawItem.hDC, text, -1, const_cast<RECT*>(&drawItem.rcItem), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(drawItem.hDC, previousFont);
        }
        return;
    }

    const auto visual = ResolveButtonVisual(ButtonKindFromControlId(drawItem.CtlID), state_, fullscreen_, config_.languageCode);
    const bool disabled = (drawItem.itemState & ODS_DISABLED) != 0;
    const bool compactControl = flatBottomControl || (drawItem.rcItem.bottom - drawItem.rcItem.top) <= 32 || (drawItem.rcItem.right - drawItem.rcItem.left) <= 36;

    RECT contentRect = drawItem.rcItem;
    InflateRect(&contentRect, compactControl ? -1 : -2, compactControl ? -1 : -2);
    SetBkMode(drawItem.hDC, TRANSPARENT);
    RECT imageBounds = IconImageBounds(contentRect, compactControl);
    if (!DrawButtonStyleIcon(instance_, visual.imageAsset, drawItem.hDC, imageBounds, disabled)) {
        SetTextColor(drawItem.hDC, disabled ? palette.textTertiary
                                            : (visual.warning ? palette.error
                                                              : (visual.accent ? palette.textPrimary : palette.brandHover)));
        HFONT iconFont = reinterpret_cast<HFONT>(SelectObject(drawItem.hDC, visual.largeGlyph ? (visual.textGlyph ? largeTextGlyphFont_ : largeIconFont_)
                                                                                               : (visual.textGlyph ? uiFont_ : iconFont_)));
        DrawTextW(drawItem.hDC, visual.glyph.c_str(), -1, &contentRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(drawItem.hDC, iconFont);
    }
}

}  // namespace velo::ui

