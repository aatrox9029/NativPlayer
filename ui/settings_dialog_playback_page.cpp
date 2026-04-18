#include "ui/settings_dialog_internal.h"

namespace velo::ui {

namespace {

constexpr int kSeekStepSecondsMin = 1;
constexpr int kSeekStepSecondsMax = 60;
constexpr int kSeekStepPercentMin = 1;
constexpr int kSeekStepPercentMax = 100;

int SeekStepMin(const velo::config::SeekStepMode mode) {
    return mode == velo::config::SeekStepMode::Percent ? kSeekStepPercentMin : kSeekStepSecondsMin;
}

int SeekStepMax(const velo::config::SeekStepMode mode) {
    return mode == velo::config::SeekStepMode::Percent ? kSeekStepPercentMax : kSeekStepSecondsMax;
}

}

void SettingsDialog::UpdateSliderLabels() {
    const int hideDelayMs = SliderToDelayMs(static_cast<int>(SendMessageW(playbackPageControls_.hideDelayBar, TBM_GETPOS, 0, 0)));
    const int seekStep = static_cast<int>(SendMessageW(playbackPageControls_.seekStepBar, TBM_GETPOS, 0, 0));
    const auto seekStepMode = SelectedSeekStepMode();
    const int subtitleSize = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleSizeBar, TBM_GETPOS, 0, 0));
    const double subtitleDelay = SliderToSignedTenthSeconds(static_cast<int>(SendMessageW(subtitlePageControls_.subtitleDelayBar, TBM_GETPOS, 0, 0)));
    const int subtitleBorderSize = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleBorderSizeBar, TBM_GETPOS, 0, 0));
    const int subtitleBackgroundOpacity = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleBackgroundOpacityBar, TBM_GETPOS, 0, 0));
    const int startupVolume = static_cast<int>(SendMessageW(audioPageControls_.startupVolumeBar, TBM_GETPOS, 0, 0));
    const int volumeStep = static_cast<int>(SendMessageW(audioPageControls_.volumeStepBar, TBM_GETPOS, 0, 0));
    const int wheelVolumeStep = static_cast<int>(SendMessageW(audioPageControls_.wheelVolumeStepBar, TBM_GETPOS, 0, 0));
    const double audioDelay = SliderToSignedTenthSeconds(static_cast<int>(SendMessageW(audioPageControls_.audioDelayBar, TBM_GETPOS, 0, 0)));
    const int sharpen = static_cast<int>(SendMessageW(advancedPageControls_.sharpenBar, TBM_GETPOS, 0, 0));
    const int denoise = static_cast<int>(SendMessageW(advancedPageControls_.denoiseBar, TBM_GETPOS, 0, 0));
    const int screenshotQuality = static_cast<int>(SendMessageW(advancedPageControls_.screenshotQualityBar, TBM_GETPOS, 0, 0));

    velo::config::AppConfig subtitleFontPreview = initialConfig_;
    subtitleFontPreview.subtitleFontSize = subtitleSize;
    if (!isInitializing_) {
        initialConfig_.subtitleFontSize = subtitleSize;
    }
    SetWindowTextW(subtitlePageControls_.subtitleFontButton, SubtitleFontButtonText(subtitleFontPreview).c_str());

    std::wostringstream hideDelay;
    hideDelay << std::fixed << std::setprecision(1) << (static_cast<double>(hideDelayMs) / 1000.0);
    SetWindowTextW(playbackPageControls_.hideDelayLabel,
                   (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsControlHideDelay) + L" " +
                    hideDelay.str())
                       .c_str());
    SetWindowTextW(playbackPageControls_.seekStepModeLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSeekStepMode).c_str());
    SetWindowTextW(playbackPageControls_.seekStepLabel,
                   (velo::localization::Text(initialConfig_.languageCode,
                                             seekStepMode == velo::config::SeekStepMode::Percent
                                                 ? velo::localization::TextId::SettingsSeekStepPercent
                                                 : velo::localization::TextId::SettingsSeekStepSeconds) +
                    L" " + std::to_wstring(seekStep) +
                    (seekStepMode == velo::config::SeekStepMode::Percent ? L"%" : L" s"))
                       .c_str());
     SetWindowTextW(subtitlePageControls_.subtitleSizeLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleSize) + L" " +
                          std::to_wstring(subtitleSize) + L" pt")
                              .c_str());
    std::wostringstream subtitleDelayText;
    subtitleDelayText << std::fixed << std::setprecision(1) << subtitleDelay;
     SetWindowTextW(subtitlePageControls_.subtitleDelayLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleDelay) + L" " +
                          subtitleDelayText.str() + L" s")
                              .c_str());
    SetWindowTextW(subtitlePageControls_.subtitleBorderSizeLabel,
                   (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleBorderSize) + L" " +
                    std::to_wstring(subtitleBorderSize))
                       .c_str());
    SetWindowTextW(subtitlePageControls_.subtitleBackgroundOpacityLabel,
                   (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleBackgroundOpacity) + L" " +
                    std::to_wstring(subtitleBackgroundOpacity) + L"%")
                       .c_str());
     SetWindowTextW(audioPageControls_.startupVolumeLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsCurrentVolume) + L" " +
                          std::to_wstring(startupVolume) + L"%")
                              .c_str());
     SetWindowTextW(audioPageControls_.volumeStepLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsKeyboardVolumeStep) + L" " +
                          std::to_wstring(volumeStep))
                              .c_str());
     SetWindowTextW(audioPageControls_.wheelVolumeStepLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsWheelVolumeStep) + L" " +
                          std::to_wstring(wheelVolumeStep))
                              .c_str());
    std::wostringstream audioDelayText;
    audioDelayText << std::fixed << std::setprecision(1) << audioDelay;
     SetWindowTextW(audioPageControls_.audioDelayLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsAudioDelay) + L" " +
                          audioDelayText.str() + L" s")
                              .c_str());
     SetWindowTextW(advancedPageControls_.sharpenLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSharpenStrength) + L" " +
                          std::to_wstring(sharpen))
                              .c_str());
     SetWindowTextW(advancedPageControls_.denoiseLabel,
                         (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsDenoiseStrength) + L" " +
                          std::to_wstring(denoise))
                              .c_str());
    SetWindowTextW(advancedPageControls_.screenshotQualityLabel,
                   (velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsJpegQuality) + L" " +
                    std::to_wstring(screenshotQuality) + L"%")
                       .c_str());
}

void SettingsDialog::PopulateLocalizedCombos() {
    const auto language = velo::localization::ResolveLanguage(initialConfig_.languageCode);
    const auto resetCombo = [](HWND combo) {
        SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    };
    const auto selectionOf = [](HWND combo) {
        return combo != nullptr ? static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0)) : CB_ERR;
    };

    const int seekStepModeSelection = selectionOf(playbackPageControls_.seekStepModeCombo);
    const int repeatModeSelection = selectionOf(playbackPageControls_.repeatModeCombo);
    const int endActionSelection = selectionOf(playbackPageControls_.endActionCombo);
    const int subtitlePositionSelection = selectionOf(subtitlePageControls_.subtitlePositionCombo);
    const int subtitleEncodingSelection = selectionOf(subtitlePageControls_.subtitleEncodingCombo);
    const int doubleClickSelection = selectionOf(shortcutsPageControls_.doubleClickActionCombo);
    const int middleClickSelection = selectionOf(shortcutsPageControls_.middleClickActionCombo);
    const int languageSelection = selectionOf(advancedPageControls_.languageCombo);
    const int hwdecSelection = selectionOf(advancedPageControls_.hwdecCombo);
    const int aspectRatioSelection = selectionOf(advancedPageControls_.aspectRatioCombo);
    const int rotateSelection = selectionOf(advancedPageControls_.rotateCombo);
    const int equalizerSelection = selectionOf(advancedPageControls_.equalizerCombo);
    const int screenshotFormatSelection = selectionOf(advancedPageControls_.screenshotFormatCombo);

    resetCombo(playbackPageControls_.seekStepModeCombo);
    SendMessageW(playbackPageControls_.seekStepModeCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsSeekStepModeSeconds).c_str()));
    SendMessageW(playbackPageControls_.seekStepModeCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsSeekStepModePercent).c_str()));

    resetCombo(playbackPageControls_.repeatModeCombo);
    for (const auto id : {velo::localization::TextId::SettingsRepeatModeOff,
                          velo::localization::TextId::SettingsRepeatModeOne,
                          velo::localization::TextId::SettingsRepeatModeAll}) {
        const auto label = velo::localization::Text(language, id);
        SendMessageW(playbackPageControls_.repeatModeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    resetCombo(playbackPageControls_.endActionCombo);
    SendMessageW(playbackPageControls_.endActionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsEndActionPlayNext).c_str()));
    SendMessageW(playbackPageControls_.endActionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsEndActionReplay).c_str()));
    SendMessageW(playbackPageControls_.endActionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsEndActionStop).c_str()));
    SendMessageW(playbackPageControls_.endActionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsEndActionCloseWindow).c_str()));

    resetCombo(subtitlePageControls_.subtitlePositionCombo);
    SendMessageW(subtitlePageControls_.subtitlePositionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsSubtitlePositionTop).c_str()));
    SendMessageW(subtitlePageControls_.subtitlePositionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsSubtitlePositionMiddle).c_str()));
    SendMessageW(subtitlePageControls_.subtitlePositionCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsSubtitlePositionBottom).c_str()));
    resetCombo(subtitlePageControls_.subtitleEncodingCombo);
    for (const wchar_t* label : {L"Auto", L"UTF-8", L"Big5", L"GB18030", L"Shift-JIS", L"Windows-1252"}) {
        SendMessageW(subtitlePageControls_.subtitleEncodingCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }

    resetCombo(shortcutsPageControls_.doubleClickActionCombo);
    resetCombo(shortcutsPageControls_.middleClickActionCombo);
    for (const auto id : {velo::localization::TextId::SettingsMouseActionFullscreen, velo::localization::TextId::SettingsMouseActionPauseResume,
                          velo::localization::TextId::SettingsMouseActionPlayNext, velo::localization::TextId::SettingsMouseActionNone}) {
        const auto label = velo::localization::Text(language, id);
        SendMessageW(shortcutsPageControls_.doubleClickActionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        SendMessageW(shortcutsPageControls_.middleClickActionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    resetCombo(advancedPageControls_.languageCombo);
    for (const auto code : {L"zh-TW", L"zh-CN", L"en-US"}) {
        SendMessageW(advancedPageControls_.languageCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(velo::localization::LanguageMenuLabel(velo::localization::ResolveLanguage(code)).c_str()));
    }

    resetCombo(advancedPageControls_.hwdecCombo);
    SendMessageW(advancedPageControls_.hwdecCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsHwdecAutoSafe).c_str()));
    SendMessageW(advancedPageControls_.hwdecCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsHwdecSoftwareOnly).c_str()));
    SendMessageW(advancedPageControls_.hwdecCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsHwdecAutoCopy).c_str()));

    resetCombo(advancedPageControls_.aspectRatioCombo);
    const auto defaultAspectRatio = velo::localization::Text(language, velo::localization::TextId::SettingsAspectRatioDefault);
    SendMessageW(advancedPageControls_.aspectRatioCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(defaultAspectRatio.c_str()));
    for (const wchar_t* label : {L"16:9", L"4:3", L"21:9", L"1:1"}) {
        SendMessageW(advancedPageControls_.aspectRatioCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }

    resetCombo(advancedPageControls_.rotateCombo);
    for (const wchar_t* label : {L"0°", L"90°", L"180°", L"270°"}) {
        SendMessageW(advancedPageControls_.rotateCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
    }

    resetCombo(advancedPageControls_.equalizerCombo);
    for (const auto id : {velo::localization::TextId::SettingsEqualizerOff,
                          velo::localization::TextId::SettingsEqualizerVoice,
                          velo::localization::TextId::SettingsEqualizerCinema,
                          velo::localization::TextId::SettingsEqualizerMusic}) {
        const auto label = velo::localization::Text(language, id);
        SendMessageW(advancedPageControls_.equalizerCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    resetCombo(advancedPageControls_.screenshotFormatCombo);
    SendMessageW(advancedPageControls_.screenshotFormatCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsScreenshotFormatPng).c_str()));
    SendMessageW(advancedPageControls_.screenshotFormatCombo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(velo::localization::Text(language, velo::localization::TextId::SettingsScreenshotFormatJpg).c_str()));

    if (seekStepModeSelection != CB_ERR) {
        SendMessageW(playbackPageControls_.seekStepModeCombo, CB_SETCURSEL, seekStepModeSelection, 0);
    }
    if (repeatModeSelection != CB_ERR) {
        SendMessageW(playbackPageControls_.repeatModeCombo, CB_SETCURSEL, repeatModeSelection, 0);
    }
    if (endActionSelection != CB_ERR) {
        SendMessageW(playbackPageControls_.endActionCombo, CB_SETCURSEL, endActionSelection, 0);
    }
    if (subtitlePositionSelection != CB_ERR) {
        SendMessageW(subtitlePageControls_.subtitlePositionCombo, CB_SETCURSEL, subtitlePositionSelection, 0);
    }
    if (subtitleEncodingSelection != CB_ERR) {
        SendMessageW(subtitlePageControls_.subtitleEncodingCombo, CB_SETCURSEL, subtitleEncodingSelection, 0);
    }
    if (doubleClickSelection != CB_ERR) {
        SendMessageW(shortcutsPageControls_.doubleClickActionCombo, CB_SETCURSEL, doubleClickSelection, 0);
    }
    if (middleClickSelection != CB_ERR) {
        SendMessageW(shortcutsPageControls_.middleClickActionCombo, CB_SETCURSEL, middleClickSelection, 0);
    }
    if (languageSelection != CB_ERR) {
        SendMessageW(advancedPageControls_.languageCombo, CB_SETCURSEL, languageSelection, 0);
    }
    if (hwdecSelection != CB_ERR) {
        SendMessageW(advancedPageControls_.hwdecCombo, CB_SETCURSEL, hwdecSelection, 0);
    }
    if (aspectRatioSelection != CB_ERR) {
        SendMessageW(advancedPageControls_.aspectRatioCombo, CB_SETCURSEL, aspectRatioSelection, 0);
    }
    if (rotateSelection != CB_ERR) {
        SendMessageW(advancedPageControls_.rotateCombo, CB_SETCURSEL, rotateSelection, 0);
    }
    if (equalizerSelection != CB_ERR) {
        SendMessageW(advancedPageControls_.equalizerCombo, CB_SETCURSEL, equalizerSelection, 0);
    }
    if (screenshotFormatSelection != CB_ERR) {
        SendMessageW(advancedPageControls_.screenshotFormatCombo, CB_SETCURSEL, screenshotFormatSelection, 0);
    }
}

void SettingsDialog::RefreshLocalizedText() {
    SetWindowTextW(hwnd_, (velo::app::AppDisplayName() + L" " +
                           velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsTitle))
                              .c_str());
    SetWindowTextW(navButtons_[0], velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsPagePlayback).c_str());
    SetWindowTextW(navButtons_[1], velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsPageSubtitle).c_str());
    SetWindowTextW(navButtons_[2], velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsPageAudio).c_str());
    SetWindowTextW(navButtons_[3], velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsPageShortcuts).c_str());
    SetWindowTextW(navButtons_[4], velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsPageAdvanced).c_str());

    SetWindowTextW(playbackPageControls_.autoplayNextCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsAutoplayNextFile).c_str());
    SetWindowTextW(playbackPageControls_.rememberPlaybackCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsRememberPlaybackPosition).c_str());
    SetWindowTextW(playbackPageControls_.preservePauseCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsPreservePauseOnOpen).c_str());
    SetWindowTextW(playbackPageControls_.seekPreviewCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSeekPreview).c_str());
    SetWindowTextW(playbackPageControls_.seekStepModeLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSeekStepMode).c_str());
    SetWindowTextW(playbackPageControls_.repeatModeLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsRepeatMode).c_str());
    SetWindowTextW(playbackPageControls_.endActionLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsEndOfPlayback).c_str());

    SetWindowTextW(subtitlePageControls_.autoSubtitleCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsAutoLoadSubtitle).c_str());
    SetWindowTextW(subtitlePageControls_.subtitleFontLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleFont).c_str());
    SetWindowTextW(subtitlePageControls_.subtitleBackgroundCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsShowSubtitleBackground).c_str());
    SetWindowTextW(subtitlePageControls_.subtitleBackgroundOpacityLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleBackgroundOpacity).c_str());
    SetWindowTextW(subtitlePageControls_.subtitlePositionLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitlePosition).c_str());
    SetWindowTextW(subtitlePageControls_.subtitleOffsetUpLabel, SubtitleVerticalDirectionLabel(initialConfig_.languageCode).c_str());
    SetWindowTextW(subtitlePageControls_.subtitleEncodingLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsSubtitleEncoding).c_str());

    SetWindowTextW(audioPageControls_.rememberVolumeCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsRememberCurrentVolume).c_str());
    SetWindowTextW(audioPageControls_.audioOutputLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsAudioOutput).c_str());
    SetWindowTextW(shortcutsPageControls_.doubleClickActionLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsDoubleClickAction).c_str());
    SetWindowTextW(shortcutsPageControls_.middleClickActionLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsMiddleClickAction).c_str());

    const auto language = velo::localization::ResolveLanguage(initialConfig_.languageCode);
    for (size_t index = 0; index < shortcutLabelControls_.size() && index < velo::platform::win32::InputBindingDefinitions().size(); ++index) {
        SetWindowTextW(shortcutLabelControls_[index],
                       velo::localization::ActionLabel(language, velo::platform::win32::InputBindingDefinitions()[index].actionId).c_str());
    }

    SetWindowTextW(advancedPageControls_.languageLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsLanguage).c_str());
    SetWindowTextW(advancedPageControls_.hwdecLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsHardwareDecode).c_str());
    SetWindowTextW(advancedPageControls_.debugInfoCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsShowDebugInfo).c_str());
    SetWindowTextW(advancedPageControls_.aspectRatioLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsAspectRatio).c_str());
    SetWindowTextW(advancedPageControls_.rotateLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsRotate).c_str());
    SetWindowTextW(advancedPageControls_.mirrorVideoCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsMirrorVideo).c_str());
    SetWindowTextW(advancedPageControls_.deinterlaceCheckbox,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsEnableDeinterlace).c_str());
    SetWindowTextW(advancedPageControls_.equalizerLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsEqualizer).c_str());
    SetWindowTextW(advancedPageControls_.screenshotFormatLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsScreenshotFormat).c_str());
    SetWindowTextW(advancedPageControls_.advancedHintLabel,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsAdvancedHint).c_str());

    SetWindowTextW(footerControls_.importButton,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsImport).c_str());
    SetWindowTextW(footerControls_.exportButton,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsExport).c_str());
    SetWindowTextW(footerControls_.resetButton,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsResetDefaults).c_str());
    SetWindowTextW(footerControls_.okButton,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsOk).c_str());
    SetWindowTextW(footerControls_.cancelButton,
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsCancel).c_str());

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

    UpdateSliderLabels();
    UpdateShortcutWarning();
}

void SettingsDialog::LoadConfigToControls() {
    PopulateLocalizedCombos();
    SetCheckboxState(playbackPageControls_.autoplayNextCheckbox, initialConfig_.autoplayNextFile);
    SetCheckboxState(playbackPageControls_.rememberPlaybackCheckbox, initialConfig_.rememberPlaybackPosition);
    SetCheckboxState(playbackPageControls_.preservePauseCheckbox, initialConfig_.preservePauseOnOpen);
    SetCheckboxState(playbackPageControls_.seekPreviewCheckbox, initialConfig_.showSeekPreview);
    SetCheckboxState(subtitlePageControls_.autoSubtitleCheckbox, initialConfig_.autoLoadSubtitle);
    SetCheckboxState(subtitlePageControls_.subtitleBackgroundCheckbox, initialConfig_.subtitleBackgroundEnabled);
    SetCheckboxState(audioPageControls_.rememberVolumeCheckbox, initialConfig_.rememberVolume);
    SetCheckboxState(advancedPageControls_.debugInfoCheckbox, initialConfig_.showDebugInfo);
    SetCheckboxState(advancedPageControls_.mirrorVideoCheckbox, initialConfig_.mirrorVideo);
    SetCheckboxState(advancedPageControls_.deinterlaceCheckbox, initialConfig_.deinterlaceEnabled);

    SendMessageW(playbackPageControls_.hideDelayBar, TBM_SETPOS, TRUE,
                 std::clamp(DelayMsToSlider(initialConfig_.controlsHideDelayMs), 10, 80));
    SendMessageW(playbackPageControls_.seekStepModeCombo, CB_SETCURSEL,
                 initialConfig_.seekStepMode == velo::config::SeekStepMode::Percent ? 1 : 0, 0);
    UpdateSeekStepModeControls(false);
    SendMessageW(subtitlePageControls_.subtitleSizeBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.subtitleFontSize, 12, 72));
    SendMessageW(subtitlePageControls_.subtitleDelayBar, TBM_SETPOS, TRUE, SignedTenthSecondsToSlider(initialConfig_.subtitleDelaySeconds));
    SendMessageW(subtitlePageControls_.subtitleBorderSizeBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.subtitleBorderSize, 0, 12));
    SendMessageW(subtitlePageControls_.subtitleBackgroundOpacityBar, TBM_SETPOS, TRUE,
                 SubtitleBackgroundOpacityPercent(initialConfig_.subtitleBackgroundColor));
    SendMessageW(audioPageControls_.startupVolumeBar, TBM_SETPOS, TRUE, std::clamp(static_cast<int>(std::lround(initialConfig_.startupVolume)), 0, 100));
    SendMessageW(audioPageControls_.volumeStepBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.volumeStep, 1, 20));
    SendMessageW(audioPageControls_.wheelVolumeStepBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.wheelVolumeStep, 1, 20));
    SendMessageW(audioPageControls_.audioDelayBar, TBM_SETPOS, TRUE, SignedTenthSecondsToSlider(initialConfig_.audioDelaySeconds));
    SendMessageW(advancedPageControls_.sharpenBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.sharpenStrength, 0, 20));
    SendMessageW(advancedPageControls_.denoiseBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.denoiseStrength, 0, 20));
    SendMessageW(advancedPageControls_.screenshotQualityBar, TBM_SETPOS, TRUE, std::clamp(initialConfig_.screenshotQuality, 10, 100));
    SendMessageW(playbackPageControls_.repeatModeCombo, CB_SETCURSEL, RepeatModeIndex(initialConfig_.repeatMode), 0);
    SendMessageW(playbackPageControls_.endActionCombo, CB_SETCURSEL, EndActionIndex(initialConfig_.endOfPlaybackAction), 0);
    SendMessageW(subtitlePageControls_.subtitlePositionCombo, CB_SETCURSEL, SubtitlePositionIndex(initialConfig_.subtitlePositionPreset), 0);
    SendMessageW(subtitlePageControls_.subtitleEncodingCombo, CB_SETCURSEL, SubtitleEncodingIndex(initialConfig_.subtitleEncoding), 0);
    SendMessageW(shortcutsPageControls_.doubleClickActionCombo, CB_SETCURSEL, MouseActionIndex(initialConfig_.doubleClickAction), 0);
    SendMessageW(shortcutsPageControls_.middleClickActionCombo, CB_SETCURSEL, MouseActionIndex(initialConfig_.middleClickAction), 0);
    SendMessageW(advancedPageControls_.languageCombo, CB_SETCURSEL,
                 initialConfig_.languageCode == L"zh-CN" ? 1 : (initialConfig_.languageCode == L"en-US" ? 2 : 0), 0);
    SendMessageW(advancedPageControls_.hwdecCombo, CB_SETCURSEL,
                 initialConfig_.hwdecPolicy == L"no" ? 1 : (initialConfig_.hwdecPolicy == L"auto-copy" ? 2 : 0), 0);
    SendMessageW(advancedPageControls_.aspectRatioCombo, CB_SETCURSEL, AspectRatioIndex(initialConfig_.preferredAspectRatio), 0);
    SendMessageW(advancedPageControls_.rotateCombo, CB_SETCURSEL, RotateIndex(initialConfig_.videoRotateDegrees), 0);
    SendMessageW(advancedPageControls_.equalizerCombo, CB_SETCURSEL, EqualizerIndex(initialConfig_.equalizerProfile), 0);
    SendMessageW(advancedPageControls_.screenshotFormatCombo, CB_SETCURSEL, initialConfig_.screenshotFormat == L"jpg" ? 1 : 0, 0);

    SetWindowTextW(subtitlePageControls_.subtitleFontButton, SubtitleFontButtonText(initialConfig_).c_str());
    RefreshSubtitleColorButtons();
    SetWindowTextW(subtitlePageControls_.subtitleOffsetUpEdit, std::to_wstring(SubtitleVerticalDirectionFromConfig(initialConfig_)).c_str());

    int audioOutputIndex = 0;
    for (size_t index = 0; index < audioOutputValues_.size(); ++index) {
        if (audioOutputValues_[index] == initialConfig_.audioOutputDevice) {
            audioOutputIndex = static_cast<int>(index);
            break;
        }
    }
    SendMessageW(audioPageControls_.audioOutputCombo, CB_SETCURSEL, audioOutputIndex, 0);

    shortcutBindingValues_.resize(velo::platform::win32::InputBindingDefinitions().size());
    for (size_t index = 0; index < velo::platform::win32::InputBindingDefinitions().size(); ++index) {
        shortcutBindingValues_[index] =
            velo::platform::win32::ResolveVirtualKey(initialConfig_, velo::platform::win32::InputBindingDefinitions()[index].actionId);
        RefreshShortcutBindingButton(index);
    }

    SelectPage(0);
    RefreshLocalizedText();
    OnSubtitleControlsChanged();
}

velo::config::SeekStepMode SettingsDialog::SelectedSeekStepMode() const {
    const LRESULT selection = playbackPageControls_.seekStepModeCombo != nullptr
                                  ? SendMessageW(playbackPageControls_.seekStepModeCombo, CB_GETCURSEL, 0, 0)
                                  : 0;
    return selection == 1 ? velo::config::SeekStepMode::Percent : velo::config::SeekStepMode::Seconds;
}

void SettingsDialog::UpdateSeekStepModeControls(const bool preserveCurrentValue) {
    if (playbackPageControls_.seekStepBar == nullptr || playbackPageControls_.seekStepModeCombo == nullptr) {
        return;
    }

    if (preserveCurrentValue) {
        const int currentValue = static_cast<int>(SendMessageW(playbackPageControls_.seekStepBar, TBM_GETPOS, 0, 0));
        if (SelectedSeekStepMode() == velo::config::SeekStepMode::Percent) {
            initialConfig_.seekStepSeconds = std::clamp(currentValue, kSeekStepSecondsMin, kSeekStepSecondsMax);
        } else {
            initialConfig_.seekStepPercent = std::clamp(currentValue, kSeekStepPercentMin, kSeekStepPercentMax);
        }
    }

    const auto mode = SelectedSeekStepMode();
    const int minimum = SeekStepMin(mode);
    const int maximum = SeekStepMax(mode);
    const int value = mode == velo::config::SeekStepMode::Percent
                          ? std::clamp(initialConfig_.seekStepPercent, minimum, maximum)
                          : std::clamp(initialConfig_.seekStepSeconds, minimum, maximum);

    initialConfig_.seekStepMode = mode;
    SendMessageW(playbackPageControls_.seekStepBar, TBM_SETRANGE, TRUE, MAKELPARAM(minimum, maximum));
    SendMessageW(playbackPageControls_.seekStepBar, TBM_SETPOS, TRUE, value);
    UpdateSliderLabels();
}

}  // namespace velo::ui
