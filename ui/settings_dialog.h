#pragma once

#include <Windows.h>

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "config/config_service.h"
#include "ui/settings_page_surface.h"

namespace velo::ui {

class SettingsDialog {
public:
    std::optional<velo::config::AppConfig> ShowModal(HINSTANCE instance, HWND owner,
                                                     const velo::config::AppConfig& initialConfig,
                                                     std::function<void(const velo::config::AppConfig&)> previewCallback = {});

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    friend bool SettingsDialogCancelForTesting(HWND hwnd);
    friend bool SettingsDialogAcceptForTesting(HWND hwnd);
    friend bool SettingsDialogSelectSubtitlePageForTesting(HWND hwnd);
    friend bool SettingsDialogSetSubtitlePositionForTesting(HWND hwnd, int index);
    friend bool SettingsDialogChooseSubtitleTextColorForTesting(HWND hwnd);

private:
    struct PlaybackPageControls {
        HWND page = nullptr;
        HWND autoplayNextCheckbox = nullptr;
        HWND rememberPlaybackCheckbox = nullptr;
        HWND preservePauseCheckbox = nullptr;
        HWND seekPreviewCheckbox = nullptr;
        HWND hideDelayLabel = nullptr;
        HWND hideDelayBar = nullptr;
        HWND seekStepModeLabel = nullptr;
        HWND seekStepModeCombo = nullptr;
        HWND seekStepLabel = nullptr;
        HWND seekStepBar = nullptr;
        HWND repeatModeLabel = nullptr;
        HWND repeatModeCombo = nullptr;
        HWND endActionLabel = nullptr;
        HWND endActionCombo = nullptr;
    };

    struct SubtitlePageControls {
        HWND page = nullptr;
        HWND autoSubtitleCheckbox = nullptr;
        HWND subtitleFontLabel = nullptr;
        HWND subtitleFontButton = nullptr;
        HWND subtitleSizeLabel = nullptr;
        HWND subtitleSizeBar = nullptr;
        HWND subtitleDelayLabel = nullptr;
        HWND subtitleDelayBar = nullptr;
        HWND subtitleTextColorButton = nullptr;
        HWND subtitleBorderColorButton = nullptr;
        HWND subtitleShadowColorButton = nullptr;
        HWND subtitleBackgroundCheckbox = nullptr;
        HWND subtitleBackgroundColorButton = nullptr;
        HWND subtitleBackgroundOpacityLabel = nullptr;
        HWND subtitleBackgroundOpacityBar = nullptr;
        HWND subtitlePositionLabel = nullptr;
        HWND subtitlePositionCombo = nullptr;
        HWND subtitleOffsetUpLabel = nullptr;
        HWND subtitleOffsetUpEdit = nullptr;
        HWND subtitleOffsetDownLabel = nullptr;
        HWND subtitleOffsetDownEdit = nullptr;
        HWND subtitleOffsetLeftLabel = nullptr;
        HWND subtitleOffsetLeftEdit = nullptr;
        HWND subtitleOffsetRightLabel = nullptr;
        HWND subtitleOffsetRightEdit = nullptr;
        HWND subtitleBorderSizeLabel = nullptr;
        HWND subtitleBorderSizeBar = nullptr;
        HWND subtitleShadowDepthLabel = nullptr;
        HWND subtitleShadowDepthBar = nullptr;
        HWND subtitleMarginLabel = nullptr;
        HWND subtitleMarginBar = nullptr;
        HWND subtitleEncodingLabel = nullptr;
        HWND subtitleEncodingCombo = nullptr;
    };

    struct AudioPageControls {
        HWND page = nullptr;
        HWND rememberVolumeCheckbox = nullptr;
        HWND startupVolumeLabel = nullptr;
        HWND startupVolumeBar = nullptr;
        HWND volumeStepLabel = nullptr;
        HWND volumeStepBar = nullptr;
        HWND wheelVolumeStepLabel = nullptr;
        HWND wheelVolumeStepBar = nullptr;
        HWND audioDelayLabel = nullptr;
        HWND audioDelayBar = nullptr;
        HWND audioOutputLabel = nullptr;
        HWND audioOutputCombo = nullptr;
    };

    struct ShortcutsPageControls {
        HWND page = nullptr;
        HWND doubleClickActionLabel = nullptr;
        HWND doubleClickActionCombo = nullptr;
        HWND middleClickActionLabel = nullptr;
        HWND middleClickActionCombo = nullptr;
        HWND shortcutWarningLabel = nullptr;
    };

    struct AdvancedPageControls {
        HWND page = nullptr;
        HWND hwdecLabel = nullptr;
        HWND hwdecCombo = nullptr;
        HWND languageLabel = nullptr;
        HWND languageCombo = nullptr;
        HWND debugInfoCheckbox = nullptr;
        HWND aspectRatioLabel = nullptr;
        HWND aspectRatioCombo = nullptr;
        HWND rotateLabel = nullptr;
        HWND rotateCombo = nullptr;
        HWND mirrorVideoCheckbox = nullptr;
        HWND deinterlaceCheckbox = nullptr;
        HWND sharpenLabel = nullptr;
        HWND sharpenBar = nullptr;
        HWND denoiseLabel = nullptr;
        HWND denoiseBar = nullptr;
        HWND equalizerLabel = nullptr;
        HWND equalizerCombo = nullptr;
        HWND advancedHintLabel = nullptr;
        HWND screenshotFormatLabel = nullptr;
        HWND screenshotFormatCombo = nullptr;
        HWND screenshotQualityLabel = nullptr;
        HWND screenshotQualityBar = nullptr;
        HWND networkTimeoutLabel = nullptr;
        HWND networkTimeoutBar = nullptr;
        HWND reconnectLabel = nullptr;
        HWND reconnectBar = nullptr;
    };

    struct FooterControls {
        HWND importButton = nullptr;
        HWND exportButton = nullptr;
        HWND resetButton = nullptr;
        HWND okButton = nullptr;
        HWND cancelButton = nullptr;
    };

    enum class Result {
        None,
        Accepted,
        Cancelled,
    };

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void CreateControls();
    void LayoutControls();
    void UpdatePageVisibility();
    void SelectPage(int pageIndex);
    void UpdateSliderLabels();
    void UpdateSeekStepModeControls(bool preserveCurrentValue);
    void UpdateShortcutWarning();
    void RefreshLocalizedText();
    void PopulateLocalizedCombos();
    void LoadConfigToControls();
    void RefreshShortcutBindingButton(size_t index);
    void BeginShortcutCapture(size_t index);
    void CancelShortcutCapture();
    bool CaptureShortcutInput(unsigned int virtualKey);
    void ToggleCheckbox(HWND checkbox) const;
    bool HandleSubtitleOffsetWheel(HWND control, short wheelDelta);
    [[nodiscard]] velo::config::SeekStepMode SelectedSeekStepMode() const;
    void ChooseSubtitleFont();
    void ChooseSubtitleColor(std::wstring* colorTarget);
    void RefreshSubtitleColorButtons();
    void OnSubtitleControlsChanged();
    void PreviewSubtitleConfig();
    bool TryBuildPreviewConfig(velo::config::AppConfig* previewConfig) const;
    void ImportSettings();
    void ExportSettings();
    void ResetDefaults();
    void CommitAndClose();
    void Close(Result result);
    void ApplyVisualTheme();
    void DrawButton(const DRAWITEMSTRUCT& drawItem) const;
    void DrawComboItem(const DRAWITEMSTRUCT& drawItem) const;
    bool HasStableControlsForConfig() const;
    velo::config::AppConfig BuildConfig() const;

    HINSTANCE instance_ = nullptr;
    HWND owner_ = nullptr;
    HWND hwnd_ = nullptr;
    std::array<HWND, 5> navButtons_{};
    PlaybackPageControls playbackPageControls_{};
    SubtitlePageControls subtitlePageControls_{};
    AudioPageControls audioPageControls_{};
    ShortcutsPageControls shortcutsPageControls_{};
    AdvancedPageControls advancedPageControls_{};
    FooterControls footerControls_{};
    HBRUSH backgroundBrush_ = nullptr;
    HBRUSH pageBrush_ = nullptr;
    HBRUSH controlBrush_ = nullptr;
    SettingsPageSurfaceTheme pageSurfaceTheme_{};
    HFONT uiFont_ = nullptr;
    HFONT titleFont_ = nullptr;
    velo::config::AppConfig originalConfig_;
    velo::config::AppConfig initialConfig_;
    std::function<void(const velo::config::AppConfig&)> previewCallback_;
    std::vector<HWND> shortcutLabelControls_;
    std::vector<HWND> shortcutComboControls_;
    std::vector<unsigned int> shortcutBindingValues_;
    std::vector<std::wstring> audioOutputValues_;
    int currentPage_ = 0;
    int capturingShortcutIndex_ = -1;
    Result result_ = Result::None;
    bool hasShortcutConflict_ = false;
    bool isInitializing_ = false;
};

const wchar_t* SettingsDialogClassNameForTesting();
UINT SettingsDialogAudioOutputComboIdForTesting();
UINT SettingsDialogSubtitleNavControlIdForTesting();
UINT SettingsDialogSubtitlePositionComboIdForTesting();
UINT SettingsDialogSubtitleTextColorControlIdForTesting();
UINT SettingsDialogSubtitleFontButtonControlIdForTesting();
UINT SettingsDialogSubtitleBackgroundOpacityControlIdForTesting();
UINT SettingsDialogOkControlIdForTesting();
void SetSettingsDialogAudioOutputsOverrideForTesting(
    std::optional<std::vector<std::pair<std::wstring, std::wstring>>> outputs);
void SetSettingsDialogColorOverrideForTesting(std::optional<COLORREF> color);
std::optional<COLORREF> SettingsDialogLastColorDialogInitialColorForTesting();
bool SettingsDialogWindowReadyForTesting(HWND hwnd);
void SetSettingsDialogAutomationCallbackForTesting(std::function<void(HWND)> callback);
bool SettingsDialogCancelForTesting(HWND hwnd);
bool SettingsDialogAcceptForTesting(HWND hwnd);
bool SettingsDialogSelectSubtitlePageForTesting(HWND hwnd);
bool SettingsDialogSetSubtitlePositionForTesting(HWND hwnd, int index);
bool SettingsDialogChooseSubtitleTextColorForTesting(HWND hwnd);

}  // namespace velo::ui
