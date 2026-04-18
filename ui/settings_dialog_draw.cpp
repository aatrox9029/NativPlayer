#include "ui/settings_dialog_internal.h"

namespace velo::ui {

void SettingsDialog::DrawButton(const DRAWITEMSTRUCT& drawItem) const {
    const auto& palette = tokens::DarkPalette();
    const UINT controlId = static_cast<UINT>(drawItem.CtlID);
    const bool navButton = controlId >= kControlNavPlayback && controlId <= kControlNavAdvanced;
    const bool checkbox = controlId == kControlRememberPlayback || controlId == kControlPreservePause || controlId == kControlSeekPreview ||
                          controlId == kControlAutoSubtitle || controlId == kControlSubtitleBackground || controlId == kControlDebugInfo;
    const bool colorButton = controlId == kControlSubtitleTextColor || controlId == kControlSubtitleBorderColor ||
                             controlId == kControlSubtitleShadowColor || controlId == kControlSubtitleBackgroundColor;
    const bool primaryAction = controlId == kControlOk;
    const bool destructiveAction = controlId == kControlReset;
    const bool selectedNav = navButton && currentPage_ == NavPageFromControlId(controlId);
    const bool disabled = (drawItem.itemState & ODS_DISABLED) != 0;
    const bool pressed = (drawItem.itemState & ODS_SELECTED) != 0;
    const bool focus = (drawItem.itemState & ODS_FOCUS) != 0;

    COLORREF fill = tokens::MixColor(palette.bgSurface2, palette.bgCanvas, pressed ? 0.02 : 0.10);
    if (primaryAction) {
        fill = tokens::MixColor(palette.brandPrimary, palette.bgSurface2, pressed ? 0.76 : 0.88);
    } else if (destructiveAction) {
        fill = tokens::MixColor(palette.error, palette.bgSurface2, pressed ? 0.84 : 0.90);
    } else if (selectedNav) {
        fill = tokens::MixColor(palette.brandPrimary, palette.bgSurface1, 0.84);
    } else if (disabled) {
        fill = tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.12);
    }

    if (checkbox) {
        RECT boxRect = drawItem.rcItem;
        boxRect.right = boxRect.left + 20;
        boxRect.top += 2;
        boxRect.bottom = boxRect.top + 20;
        FillRect(drawItem.hDC, &drawItem.rcItem, pageBrush_);
        const bool checked = GetCheckboxState(drawItem.hwndItem);
        tokens::FillRoundedRect(drawItem.hDC, boxRect,
                                checked ? RGB(48, 226, 104) : tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.10),
                                tokens::RadiusSm());
        tokens::DrawRoundedOutline(drawItem.hDC, boxRect, focus ? palette.strokeFocus : palette.strokeSoft, tokens::RadiusSm(),
                                   focus ? 2 : 1);
        if (checked) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            HGDIOBJ previousPen = SelectObject(drawItem.hDC, pen);
            HGDIOBJ previousBrush = SelectObject(drawItem.hDC, GetStockObject(NULL_BRUSH));
            MoveToEx(drawItem.hDC, boxRect.left + 5, boxRect.top + 10, nullptr);
            LineTo(drawItem.hDC, boxRect.left + 9, boxRect.bottom - 5);
            LineTo(drawItem.hDC, boxRect.right - 4, boxRect.top + 5);
            SelectObject(drawItem.hDC, previousBrush);
            SelectObject(drawItem.hDC, previousPen);
            DeleteObject(pen);
        }
    } else if (colorButton) {
        tokens::FillRoundedRect(drawItem.hDC, drawItem.rcItem, fill, tokens::RadiusMd());
        tokens::DrawRoundedOutline(drawItem.hDC, drawItem.rcItem, focus ? palette.strokeFocus : palette.strokeSoft, tokens::RadiusMd(),
                                   focus ? 2 : 1);
    } else {
        tokens::FillRoundedRect(drawItem.hDC, drawItem.rcItem, fill, tokens::RadiusMd());
        tokens::DrawRoundedOutline(drawItem.hDC, drawItem.rcItem, focus ? palette.strokeFocus : palette.strokeSoft, tokens::RadiusMd(),
                                   focus ? 2 : 1);
    }

    wchar_t text[256] = {};
    GetWindowTextW(drawItem.hwndItem, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    RECT textRect = drawItem.rcItem;
    if (checkbox) {
        textRect.left += 30;
    } else if (colorButton) {
        RECT swatchRect = drawItem.rcItem;
        swatchRect.left += 10;
        swatchRect.right = swatchRect.left + 28;
        swatchRect.top += 6;
        swatchRect.bottom -= 6;
        const std::wstring* colorValue = nullptr;
        if (controlId == kControlSubtitleTextColor) {
            colorValue = &initialConfig_.subtitleTextColor;
        } else if (controlId == kControlSubtitleBorderColor) {
            colorValue = &initialConfig_.subtitleBorderColor;
        } else if (controlId == kControlSubtitleShadowColor) {
            colorValue = &initialConfig_.subtitleShadowColor;
        } else if (controlId == kControlSubtitleBackgroundColor) {
            colorValue = &initialConfig_.subtitleBackgroundColor;
        }
        const COLORREF swatchColor =
            colorValue != nullptr ? velo::common::TryParseRgbColor(*colorValue).value_or(RGB(255, 255, 255)) : RGB(255, 255, 255);
        tokens::FillRoundedRect(drawItem.hDC, swatchRect, swatchColor, tokens::RadiusSm());
        tokens::DrawRoundedOutline(drawItem.hDC, swatchRect, palette.strokeSoft, tokens::RadiusSm());
        textRect.left = swatchRect.right + 10;
    } else if (selectedNav) {
        textRect.left += 10;
    }

    SetBkMode(drawItem.hDC, TRANSPARENT);
    SetTextColor(drawItem.hDC, disabled ? palette.textTertiary : palette.textPrimary);
    HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(drawItem.hDC, uiFont_));
    DrawTextW(drawItem.hDC, text, -1, &textRect, (navButton || checkbox ? DT_LEFT : DT_CENTER) | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(drawItem.hDC, previousFont);
}

void SettingsDialog::DrawComboItem(const DRAWITEMSTRUCT& drawItem) const {
    const auto& palette = tokens::DarkPalette();
    std::wstring text;
    bool selected = (drawItem.itemState & ODS_SELECTED) != 0;
    if (drawItem.itemID == static_cast<UINT>(-1)) {
        const int currentSelection = static_cast<int>(SendMessageW(drawItem.hwndItem, CB_GETCURSEL, 0, 0));
        if (currentSelection >= 0) {
            text = GetComboBoxItemText(drawItem.hwndItem, currentSelection);
        }
        selected = false;
    } else {
        text = GetComboBoxItemText(drawItem.hwndItem, static_cast<int>(drawItem.itemID));
    }

    HBRUSH fillBrush = CreateSolidBrush(selected ? tokens::MixColor(palette.brandPrimary, palette.bgSurface2, 0.80)
                                                 : tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.18));
    FillRect(drawItem.hDC, &drawItem.rcItem, fillBrush);
    DeleteObject(fillBrush);

    RECT textRect = drawItem.rcItem;
    textRect.left += 12;
    textRect.right -= 12;
    SetBkMode(drawItem.hDC, TRANSPARENT);
    SetTextColor(drawItem.hDC, palette.textPrimary);
    HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(drawItem.hDC, uiFont_));
    DrawTextW(drawItem.hDC, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(drawItem.hDC, previousFont);
}

}  // namespace velo::ui
