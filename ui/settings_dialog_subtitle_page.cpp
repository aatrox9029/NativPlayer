#include "ui/settings_dialog_internal.h"

namespace velo::ui {

bool SettingsDialog::HandleSubtitleOffsetWheel(HWND control, const short wheelDelta) {
    const std::array<HWND, 1> edits{subtitlePageControls_.subtitleOffsetUpEdit};
    if (!IsSubtitleOffsetEditControl(control, edits)) {
        return false;
    }

    bool valid = true;
    const int currentValue = ParseEditValue(control, 0, kSubtitleVerticalDirectionMin, kSubtitleVerticalDirectionMax, &valid);
    const int direction = wheelDelta > 0 ? 1 : -1;
    const int stepCount = std::max(1, std::abs(static_cast<int>(wheelDelta)) / WHEEL_DELTA);
    const int updatedValue = std::clamp(currentValue + direction * stepCount, kSubtitleVerticalDirectionMin, kSubtitleVerticalDirectionMax);
    SetWindowTextW(control, std::to_wstring(updatedValue).c_str());
    OnSubtitleControlsChanged();
    return true;
}

void SettingsDialog::ChooseSubtitleFont() {
    LOGFONTW logFont{};
    wcsncpy_s(logFont.lfFaceName, initialConfig_.subtitleFont.c_str(), LF_FACESIZE - 1);
    logFont.lfWeight = initialConfig_.subtitleBold ? FW_BOLD : FW_NORMAL;
    logFont.lfItalic = initialConfig_.subtitleItalic ? TRUE : FALSE;
    logFont.lfUnderline = initialConfig_.subtitleUnderline ? TRUE : FALSE;
    logFont.lfStrikeOut = initialConfig_.subtitleStrikeOut ? TRUE : FALSE;

    HDC windowDc = GetDC(hwnd_);
    const int dpiY = windowDc != nullptr ? GetDeviceCaps(windowDc, LOGPIXELSY) : 96;
    if (windowDc != nullptr) {
        ReleaseDC(hwnd_, windowDc);
    }
    logFont.lfHeight = -MulDiv(std::max(12, initialConfig_.subtitleFontSize), dpiY, 72);

    CHOOSEFONTW chooseFont{};
    chooseFont.lStructSize = sizeof(chooseFont);
    chooseFont.hwndOwner = hwnd_;
    chooseFont.lpLogFont = &logFont;
    chooseFont.iPointSize = std::max(120, initialConfig_.subtitleFontSize * 10);
    chooseFont.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
    if (ChooseFontW(&chooseFont) == FALSE) {
        return;
    }

    initialConfig_.subtitleFont = logFont.lfFaceName;
    if (!initialConfig_.subtitleFont.empty() && initialConfig_.subtitleFont.front() == L'@') {
        initialConfig_.subtitleFont.erase(initialConfig_.subtitleFont.begin());
    }
    initialConfig_.subtitleBold = logFont.lfWeight >= FW_BOLD;
    initialConfig_.subtitleItalic = logFont.lfItalic != FALSE;
    initialConfig_.subtitleUnderline = logFont.lfUnderline != FALSE;
    initialConfig_.subtitleStrikeOut = logFont.lfStrikeOut != FALSE;
    if (chooseFont.iPointSize > 0) {
        initialConfig_.subtitleFontSize = std::max(12, chooseFont.iPointSize / 10);
        SendMessageW(subtitlePageControls_.subtitleSizeBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.subtitleFontSize, 12, 72));
    }

    SetWindowTextW(subtitlePageControls_.subtitleFontButton, SubtitleFontButtonText(initialConfig_).c_str());
    UpdateSliderLabels();
    OnSubtitleControlsChanged();
}

void SettingsDialog::ChooseSubtitleColor(std::wstring* colorTarget) {
    if (colorTarget == nullptr) {
        return;
    }

    const auto currentColor = velo::common::TryParseRgbaColor(*colorTarget);
    COLORREF customColors[16] = {};
    CHOOSECOLORW chooseColor{};
    chooseColor.lStructSize = sizeof(chooseColor);
    chooseColor.hwndOwner = hwnd_;
    chooseColor.lpCustColors = customColors;
    chooseColor.Flags = CC_RGBINIT | CC_FULLOPEN;
    chooseColor.rgbResult = currentColor.has_value() ? RGB(currentColor->red, currentColor->green, currentColor->blue) : RGB(255, 255, 255);
    shared_testing_state::g_lastColorDialogInitialColor = chooseColor.rgbResult;
    if (shared_testing_state::g_colorDialogOverride.has_value()) {
        chooseColor.rgbResult = *shared_testing_state::g_colorDialogOverride;
    } else if (ChooseColorW(&chooseColor) == FALSE) {
        return;
    }

    const velo::common::RgbaColor updatedColor{static_cast<std::uint8_t>(GetRValue(chooseColor.rgbResult)),
                                               static_cast<std::uint8_t>(GetGValue(chooseColor.rgbResult)),
                                               static_cast<std::uint8_t>(GetBValue(chooseColor.rgbResult)),
                                               currentColor.has_value() ? currentColor->alpha : static_cast<std::uint8_t>(0xFF)};
    *colorTarget = velo::common::FormatRgbaHexColor(updatedColor);

    RefreshSubtitleColorButtons();
    OnSubtitleControlsChanged();
}

void SettingsDialog::RefreshSubtitleColorButtons() {
    SetWindowTextW(subtitlePageControls_.subtitleTextColorButton,
                   ColorButtonText(initialConfig_.languageCode,
                                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsColorPrimary).c_str(),
                                   initialConfig_.subtitleTextColor)
                       .c_str());
    SetWindowTextW(subtitlePageControls_.subtitleBorderColorButton,
                   ColorButtonText(initialConfig_.languageCode,
                                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsColorBorder).c_str(),
                                   initialConfig_.subtitleBorderColor)
                       .c_str());
    SetWindowTextW(subtitlePageControls_.subtitleBackgroundColorButton,
                   ColorButtonText(initialConfig_.languageCode,
                                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsColorBackground).c_str(),
                                   initialConfig_.subtitleBackgroundColor)
                       .c_str());
}

void SettingsDialog::OnSubtitleControlsChanged() {
    PreviewSubtitleConfig();
}

void SettingsDialog::PreviewSubtitleConfig() {
    if (!previewCallback_ || isInitializing_ || !HasStableControlsForConfig()) {
        return;
    }
    velo::config::AppConfig previewConfig;
    if (TryBuildPreviewConfig(&previewConfig)) {
        previewCallback_(previewConfig);
    }
}

bool SettingsDialog::TryBuildPreviewConfig(velo::config::AppConfig* previewConfig) const {
    if (previewConfig == nullptr || isInitializing_ || !HasStableControlsForConfig()) {
        return false;
    }

    velo::config::AppConfig config = BuildConfig();
    bool valid = true;
    const int subtitleVerticalDirection =
        ParseEditValue(subtitlePageControls_.subtitleOffsetUpEdit, SubtitleVerticalDirectionFromConfig(config),
                       kSubtitleVerticalDirectionMin, kSubtitleVerticalDirectionMax, &valid);
    if (!valid) {
        return false;
    }
    ApplySubtitleVerticalDirection(&config, subtitleVerticalDirection);
    config.subtitleHorizontalOffset = 0;
    ApplyCanonicalSubtitleHorizontalOffset(&config);
    config.subtitleFontSize = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleSizeBar, TBM_GETPOS, 0, 0));
    config.subtitleDelaySeconds = SliderToSignedTenthSeconds(static_cast<int>(SendMessageW(subtitlePageControls_.subtitleDelayBar, TBM_GETPOS, 0, 0)));
    config.subtitleBorderSize = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleBorderSizeBar, TBM_GETPOS, 0, 0));
    config.subtitleEncoding = SubtitleEncodingValueFromIndex(static_cast<int>(SendMessageW(subtitlePageControls_.subtitleEncodingCombo, CB_GETCURSEL, 0, 0)));
    *previewConfig = std::move(config);
    return true;
}

}  // namespace velo::ui
