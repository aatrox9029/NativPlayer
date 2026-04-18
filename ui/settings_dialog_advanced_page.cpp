#include "ui/settings_dialog_internal.h"

namespace velo::ui {

void SettingsDialog::ImportSettings() {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = L"Settings (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&dialog) == FALSE) {
        return;
    }

    velo::config::AppConfig imported;
    if (!velo::config::ImportConfigSnapshot(filePath, imported)) {
        MessageBoxW(hwnd_, velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsImportFailed).c_str(),
                    velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsTitle).c_str(),
                    MB_OK | MB_ICONWARNING);
        return;
    }
    initialConfig_ = imported;
    LoadConfigToControls();
}

void SettingsDialog::ExportSettings() {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = L"Settings (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = L"ini";
    if (GetSaveFileNameW(&dialog) == FALSE) {
        return;
    }
    if (!velo::config::ExportConfigSnapshot(BuildConfig(), filePath)) {
        MessageBoxW(hwnd_, velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsExportFailed).c_str(),
                    velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsTitle).c_str(),
                    MB_OK | MB_ICONWARNING);
    }
}

void SettingsDialog::ResetDefaults() {
    initialConfig_ = velo::config::DefaultAppConfig();
    LoadConfigToControls();
}

void SettingsDialog::CommitAndClose() {
    if (!HasStableControlsForConfig()) {
        TraceSettingsDialog(L"Commit skipped because controls are not stable.");
        Close(Result::Cancelled);
        return;
    }
    UpdateShortcutWarning();
    if (hasShortcutConflict_) {
        return;
    }
    initialConfig_ = BuildConfig();
    Close(Result::Accepted);
}

void SettingsDialog::Close(const Result result) {
    result_ = result;
    if (result == Result::Cancelled && previewCallback_) {
        previewCallback_(originalConfig_);
    }
    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
    }
}

velo::config::AppConfig SettingsDialog::BuildConfig() const {
    velo::config::AppConfig updated = initialConfig_;
    updated.languageCode = initialConfig_.languageCode;
    updated.autoplayNextFile = GetCheckboxState(playbackPageControls_.autoplayNextCheckbox);
    updated.rememberPlaybackPosition = GetCheckboxState(playbackPageControls_.rememberPlaybackCheckbox);
    updated.preservePauseOnOpen = GetCheckboxState(playbackPageControls_.preservePauseCheckbox);
    updated.showSeekPreview = GetCheckboxState(playbackPageControls_.seekPreviewCheckbox);
    updated.controlsHideDelayMs = SliderToDelayMs(static_cast<int>(SendMessageW(playbackPageControls_.hideDelayBar, TBM_GETPOS, 0, 0)));
    updated.seekStepMode = SelectedSeekStepMode();
    if (updated.seekStepMode == velo::config::SeekStepMode::Percent) {
        updated.seekStepPercent = static_cast<int>(SendMessageW(playbackPageControls_.seekStepBar, TBM_GETPOS, 0, 0));
    } else {
        updated.seekStepSeconds = static_cast<int>(SendMessageW(playbackPageControls_.seekStepBar, TBM_GETPOS, 0, 0));
    }
    updated.repeatMode = RepeatModeFromIndex(static_cast<int>(SendMessageW(playbackPageControls_.repeatModeCombo, CB_GETCURSEL, 0, 0)));
    updated.endOfPlaybackAction = EndActionFromIndex(static_cast<int>(SendMessageW(playbackPageControls_.endActionCombo, CB_GETCURSEL, 0, 0)));

    updated.autoLoadSubtitle = GetCheckboxState(subtitlePageControls_.autoSubtitleCheckbox);
    updated.subtitleBackgroundEnabled = GetCheckboxState(subtitlePageControls_.subtitleBackgroundCheckbox);
    ApplySubtitleBackgroundOpacity(
        &updated.subtitleBackgroundColor,
        static_cast<int>(SendMessageW(subtitlePageControls_.subtitleBackgroundOpacityBar, TBM_GETPOS, 0, 0)));
    updated.subtitleFontSize = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleSizeBar, TBM_GETPOS, 0, 0));
    updated.subtitleDelaySeconds = SliderToSignedTenthSeconds(static_cast<int>(SendMessageW(subtitlePageControls_.subtitleDelayBar, TBM_GETPOS, 0, 0)));
    updated.subtitleBorderSize = static_cast<int>(SendMessageW(subtitlePageControls_.subtitleBorderSizeBar, TBM_GETPOS, 0, 0));
    updated.subtitlePositionPreset =
        SubtitlePositionIdFromIndex(static_cast<int>(SendMessageW(subtitlePageControls_.subtitlePositionCombo, CB_GETCURSEL, 0, 0)));
    ApplySubtitleVerticalDirection(
        &updated,
        ParseEditValue(subtitlePageControls_.subtitleOffsetUpEdit, SubtitleVerticalDirectionFromConfig(initialConfig_),
                       kSubtitleVerticalDirectionMin, kSubtitleVerticalDirectionMax));
    updated.subtitleHorizontalOffset = 0;
    ApplyCanonicalSubtitleHorizontalOffset(&updated);
    updated.subtitleEncoding =
        SubtitleEncodingValueFromIndex(static_cast<int>(SendMessageW(subtitlePageControls_.subtitleEncodingCombo, CB_GETCURSEL, 0, 0)));

    updated.rememberVolume = GetCheckboxState(audioPageControls_.rememberVolumeCheckbox);
    updated.startupVolume = static_cast<double>(SendMessageW(audioPageControls_.startupVolumeBar, TBM_GETPOS, 0, 0));
    updated.volumeStep = static_cast<int>(SendMessageW(audioPageControls_.volumeStepBar, TBM_GETPOS, 0, 0));
    updated.wheelVolumeStep = static_cast<int>(SendMessageW(audioPageControls_.wheelVolumeStepBar, TBM_GETPOS, 0, 0));
    updated.audioDelaySeconds = SliderToSignedTenthSeconds(static_cast<int>(SendMessageW(audioPageControls_.audioDelayBar, TBM_GETPOS, 0, 0)));
    const LRESULT audioOutputIndex = SendMessageW(audioPageControls_.audioOutputCombo, CB_GETCURSEL, 0, 0);
    updated.audioOutputDevice = (audioOutputIndex >= 0 && audioOutputIndex < static_cast<LRESULT>(audioOutputValues_.size()))
                                    ? audioOutputValues_[static_cast<size_t>(audioOutputIndex)]
                                    : L"auto";

    updated.doubleClickAction =
        MouseActionIdFromIndex(static_cast<int>(SendMessageW(shortcutsPageControls_.doubleClickActionCombo, CB_GETCURSEL, 0, 0)));
    updated.middleClickAction =
        MouseActionIdFromIndex(static_cast<int>(SendMessageW(shortcutsPageControls_.middleClickActionCombo, CB_GETCURSEL, 0, 0)));

    const int hwdecSelection = static_cast<int>(SendMessageW(advancedPageControls_.hwdecCombo, CB_GETCURSEL, 0, 0));
    updated.hwdecPolicy = hwdecSelection == 1 ? L"no" : (hwdecSelection == 2 ? L"auto-copy" : L"auto");
    updated.showDebugInfo = GetCheckboxState(advancedPageControls_.debugInfoCheckbox);
    updated.preferredAspectRatio =
        AspectRatioValueFromIndex(static_cast<int>(SendMessageW(advancedPageControls_.aspectRatioCombo, CB_GETCURSEL, 0, 0)));
    updated.videoRotateDegrees = RotateDegreesFromIndex(static_cast<int>(SendMessageW(advancedPageControls_.rotateCombo, CB_GETCURSEL, 0, 0)));
    updated.mirrorVideo = GetCheckboxState(advancedPageControls_.mirrorVideoCheckbox);
    updated.deinterlaceEnabled = GetCheckboxState(advancedPageControls_.deinterlaceCheckbox);
    updated.sharpenStrength = static_cast<int>(SendMessageW(advancedPageControls_.sharpenBar, TBM_GETPOS, 0, 0));
    updated.denoiseStrength = static_cast<int>(SendMessageW(advancedPageControls_.denoiseBar, TBM_GETPOS, 0, 0));
    updated.equalizerProfile =
        EqualizerValueFromIndex(static_cast<int>(SendMessageW(advancedPageControls_.equalizerCombo, CB_GETCURSEL, 0, 0)));
    updated.screenshotFormat = SendMessageW(advancedPageControls_.screenshotFormatCombo, CB_GETCURSEL, 0, 0) == 1 ? L"jpg" : L"png";
    updated.screenshotQuality = static_cast<int>(SendMessageW(advancedPageControls_.screenshotQualityBar, TBM_GETPOS, 0, 0));

    updated.keyBindings.clear();
    const auto& definitions = velo::platform::win32::InputBindingDefinitions();
    for (size_t index = 0; index < definitions.size(); ++index) {
        const unsigned int virtualKey =
            index < shortcutBindingValues_.size()
                ? shortcutBindingValues_[index]
                : velo::platform::win32::ResolveVirtualKey(initialConfig_, definitions[index].actionId);
        velo::platform::win32::SetVirtualKey(updated, definitions[index].actionId, virtualKey);
    }
    return updated;
}

bool SettingsDialog::HasStableControlsForConfig() const {
    return hwnd_ != nullptr && playbackPageControls_.autoplayNextCheckbox != nullptr &&
           playbackPageControls_.rememberPlaybackCheckbox != nullptr &&
           playbackPageControls_.preservePauseCheckbox != nullptr && playbackPageControls_.seekPreviewCheckbox != nullptr &&
           playbackPageControls_.hideDelayBar != nullptr && playbackPageControls_.seekStepModeLabel != nullptr &&
           playbackPageControls_.seekStepModeCombo != nullptr && playbackPageControls_.seekStepBar != nullptr &&
           playbackPageControls_.repeatModeCombo != nullptr && playbackPageControls_.endActionCombo != nullptr &&
           subtitlePageControls_.autoSubtitleCheckbox != nullptr &&
           subtitlePageControls_.subtitleBackgroundCheckbox != nullptr && subtitlePageControls_.subtitlePositionCombo != nullptr &&
           subtitlePageControls_.subtitleBackgroundOpacityBar != nullptr && subtitlePageControls_.subtitleOffsetUpEdit != nullptr &&
           subtitlePageControls_.subtitleSizeBar != nullptr && subtitlePageControls_.subtitleDelayBar != nullptr &&
           subtitlePageControls_.subtitleBorderSizeBar != nullptr && subtitlePageControls_.subtitleEncodingCombo != nullptr &&
           audioPageControls_.rememberVolumeCheckbox != nullptr && audioPageControls_.startupVolumeBar != nullptr &&
           audioPageControls_.volumeStepBar != nullptr && audioPageControls_.wheelVolumeStepBar != nullptr &&
           audioPageControls_.audioDelayBar != nullptr && audioPageControls_.audioOutputCombo != nullptr &&
           shortcutsPageControls_.doubleClickActionCombo != nullptr &&
           shortcutsPageControls_.middleClickActionCombo != nullptr && advancedPageControls_.hwdecCombo != nullptr &&
           advancedPageControls_.debugInfoCheckbox != nullptr && advancedPageControls_.aspectRatioCombo != nullptr &&
           advancedPageControls_.rotateCombo != nullptr && advancedPageControls_.mirrorVideoCheckbox != nullptr &&
           advancedPageControls_.deinterlaceCheckbox != nullptr && advancedPageControls_.sharpenBar != nullptr &&
           advancedPageControls_.denoiseBar != nullptr && advancedPageControls_.equalizerCombo != nullptr &&
           advancedPageControls_.screenshotFormatCombo != nullptr && advancedPageControls_.screenshotQualityBar != nullptr &&
           shortcutBindingValues_.size() == velo::platform::win32::InputBindingDefinitions().size();
}

}  // namespace velo::ui
