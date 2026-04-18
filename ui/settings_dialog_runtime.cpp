#include "ui/settings_dialog_internal.h"

namespace velo::ui {

void SettingsDialog::CreateControls() {
    for (size_t index = 0; index < navButtons_.size(); ++index) {
        navButtons_[index] = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                             reinterpret_cast<HMENU>(kControlNavPlayback + static_cast<int>(index)), instance_, nullptr);
    }

    playbackPageControls_.page = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    subtitlePageControls_.page = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    audioPageControls_.page = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    shortcutsPageControls_.page = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    advancedPageControls_.page = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);

    for (HWND page : {playbackPageControls_.page, subtitlePageControls_.page, audioPageControls_.page, shortcutsPageControls_.page,
                      advancedPageControls_.page}) {
        AttachSettingsPageSurface(page, &pageSurfaceTheme_);
    }

    playbackPageControls_.rememberPlaybackCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                     playbackPageControls_.page,
                                                                     reinterpret_cast<HMENU>(kControlRememberPlayback), instance_, nullptr);
    playbackPageControls_.autoplayNextCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                 playbackPageControls_.page,
                                                                 reinterpret_cast<HMENU>(kControlAutoplayNext), instance_, nullptr);
    playbackPageControls_.preservePauseCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                  playbackPageControls_.page,
                                                                  reinterpret_cast<HMENU>(kControlPreservePause), instance_, nullptr);
    playbackPageControls_.seekPreviewCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                playbackPageControls_.page,
                                                                reinterpret_cast<HMENU>(kControlSeekPreview), instance_, nullptr);
    playbackPageControls_.hideDelayLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                           playbackPageControls_.page, nullptr, instance_, nullptr);
    playbackPageControls_.hideDelayBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                         playbackPageControls_.page, reinterpret_cast<HMENU>(kControlHideDelayBar), instance_, nullptr);
    playbackPageControls_.seekStepModeLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                              playbackPageControls_.page, nullptr, instance_, nullptr);
    playbackPageControls_.seekStepModeCombo = CreateWindowExW(
        0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0,
        playbackPageControls_.page, reinterpret_cast<HMENU>(kControlSeekStepModeCombo), instance_, nullptr);
    playbackPageControls_.seekStepLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                          playbackPageControls_.page, nullptr, instance_, nullptr);
    playbackPageControls_.seekStepBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                        playbackPageControls_.page, reinterpret_cast<HMENU>(kControlSeekStepBar), instance_, nullptr);
    playbackPageControls_.repeatModeLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                            playbackPageControls_.page, nullptr, instance_, nullptr);
    playbackPageControls_.repeatModeCombo = CreateWindowExW(
        0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0,
        playbackPageControls_.page, reinterpret_cast<HMENU>(kControlRepeatModeCombo), instance_, nullptr);
    playbackPageControls_.endActionLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                           playbackPageControls_.page, nullptr, instance_, nullptr);
    playbackPageControls_.endActionCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                           WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                           0, 0, 0, 0, playbackPageControls_.page,
                                                           reinterpret_cast<HMENU>(kControlEndActionCombo), instance_, nullptr);

    subtitlePageControls_.autoSubtitleCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                 subtitlePageControls_.page,
                                                                 reinterpret_cast<HMENU>(kControlAutoSubtitle), instance_, nullptr);
    subtitlePageControls_.subtitleFontLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                              subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleFontButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                               subtitlePageControls_.page,
                                                               reinterpret_cast<HMENU>(kControlSubtitleFontButton), instance_, nullptr);
    subtitlePageControls_.subtitleSizeLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                              subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleSizeBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                            subtitlePageControls_.page,
                                                            reinterpret_cast<HMENU>(kControlSubtitleSizeBar), instance_, nullptr);
    subtitlePageControls_.subtitleDelayLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                               subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleDelayBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                             subtitlePageControls_.page,
                                                             reinterpret_cast<HMENU>(kControlSubtitleDelayBar), instance_, nullptr);
    subtitlePageControls_.subtitleTextColorButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                    subtitlePageControls_.page,
                                                                    reinterpret_cast<HMENU>(kControlSubtitleTextColor), instance_, nullptr);
    subtitlePageControls_.subtitleBorderColorButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                      subtitlePageControls_.page,
                                                                      reinterpret_cast<HMENU>(kControlSubtitleBorderColor), instance_, nullptr);
    subtitlePageControls_.subtitleShadowColorButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                      subtitlePageControls_.page,
                                                                      reinterpret_cast<HMENU>(kControlSubtitleShadowColor), instance_, nullptr);
    subtitlePageControls_.subtitleBackgroundCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                       subtitlePageControls_.page,
                                                                       reinterpret_cast<HMENU>(kControlSubtitleBackground), instance_, nullptr);
    subtitlePageControls_.subtitleBackgroundColorButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                          subtitlePageControls_.page,
                                                                          reinterpret_cast<HMENU>(kControlSubtitleBackgroundColor), instance_, nullptr);
    subtitlePageControls_.subtitleBackgroundOpacityLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                           subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleBackgroundOpacityBar = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0, subtitlePageControls_.page,
        reinterpret_cast<HMENU>(kControlSubtitleBackgroundOpacityBar), instance_, nullptr);
    subtitlePageControls_.subtitlePositionLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                  subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitlePositionCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                                  WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                                  0, 0, 0, 0, subtitlePageControls_.page,
                                                                  reinterpret_cast<HMENU>(kControlSubtitlePositionCombo), instance_, nullptr);
    subtitlePageControls_.subtitleOffsetUpLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                  subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleOffsetUpEdit = CreateWindowExW(0, L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER,
                                                                 0, 0, 0, 0, subtitlePageControls_.page,
                                                                 reinterpret_cast<HMENU>(kControlSubtitleOffsetUpEdit), instance_, nullptr);
    subtitlePageControls_.subtitleBorderSizeLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                    subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleBorderSizeBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                                  subtitlePageControls_.page,
                                                                  reinterpret_cast<HMENU>(kControlSubtitleBorderSizeBar), instance_, nullptr);
    subtitlePageControls_.subtitleShadowDepthLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0,
                                                                     subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleShadowDepthBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | TBS_HORZ, 0, 0, 0, 0,
                                                                   subtitlePageControls_.page,
                                                                   reinterpret_cast<HMENU>(kControlSubtitleShadowDepthBar), instance_, nullptr);
    subtitlePageControls_.subtitleMarginLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 0, 0,
                                                                subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleMarginBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | TBS_HORZ, 0, 0, 0, 0,
                                                              subtitlePageControls_.page,
                                                              reinterpret_cast<HMENU>(kControlSubtitleMarginBar), instance_, nullptr);
    subtitlePageControls_.subtitleEncodingLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                  subtitlePageControls_.page, nullptr, instance_, nullptr);
    subtitlePageControls_.subtitleEncodingCombo = CreateWindowExW(
        0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0,
        subtitlePageControls_.page, reinterpret_cast<HMENU>(kControlSubtitleEncodingCombo), instance_, nullptr);

    audioPageControls_.rememberVolumeCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                audioPageControls_.page,
                                                                reinterpret_cast<HMENU>(kControlRememberVolume), instance_, nullptr);
    audioPageControls_.startupVolumeLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                            audioPageControls_.page, nullptr, instance_, nullptr);
    audioPageControls_.startupVolumeBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                          audioPageControls_.page,
                                                          reinterpret_cast<HMENU>(kControlStartupVolumeBar), instance_, nullptr);
    audioPageControls_.volumeStepLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                         audioPageControls_.page, nullptr, instance_, nullptr);
    audioPageControls_.volumeStepBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                       audioPageControls_.page,
                                                       reinterpret_cast<HMENU>(kControlVolumeStepBar), instance_, nullptr);
    audioPageControls_.wheelVolumeStepLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                              audioPageControls_.page, nullptr, instance_, nullptr);
    audioPageControls_.wheelVolumeStepBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                            audioPageControls_.page,
                                                            reinterpret_cast<HMENU>(kControlWheelVolumeStepBar), instance_, nullptr);
    audioPageControls_.audioDelayLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                         audioPageControls_.page, nullptr, instance_, nullptr);
    audioPageControls_.audioDelayBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                       audioPageControls_.page,
                                                       reinterpret_cast<HMENU>(kControlAudioDelayBar), instance_, nullptr);
    audioPageControls_.audioOutputLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                          audioPageControls_.page, nullptr, instance_, nullptr);
    audioPageControls_.audioOutputCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                          0, 0, 0, 0, audioPageControls_.page,
                                                          reinterpret_cast<HMENU>(kControlAudioOutputCombo), instance_, nullptr);

    shortcutsPageControls_.doubleClickActionLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                    shortcutsPageControls_.page, nullptr, instance_, nullptr);
    shortcutsPageControls_.doubleClickActionCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                                    0, 0, 0, 0, shortcutsPageControls_.page,
                                                                    reinterpret_cast<HMENU>(kControlDoubleClickActionCombo), instance_, nullptr);
    shortcutsPageControls_.middleClickActionLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                    shortcutsPageControls_.page, nullptr, instance_, nullptr);
    shortcutsPageControls_.middleClickActionCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                                    0, 0, 0, 0, shortcutsPageControls_.page,
                                                                    reinterpret_cast<HMENU>(kControlMiddleClickActionCombo), instance_, nullptr);
    shortcutsPageControls_.shortcutWarningLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                  shortcutsPageControls_.page, nullptr, instance_, nullptr);

    shortcutLabelControls_.clear();
    shortcutComboControls_.clear();
    shortcutBindingValues_.clear();
    for (size_t index = 0; index < velo::platform::win32::InputBindingDefinitions().size(); ++index) {
        HWND label = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, shortcutsPageControls_.page, nullptr, instance_, nullptr);
        HWND button = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                      shortcutsPageControls_.page, reinterpret_cast<HMENU>(kShortcutButtonBase + index), instance_, nullptr);
        shortcutLabelControls_.push_back(label);
        shortcutComboControls_.push_back(button);
        shortcutBindingValues_.push_back(0);
    }

    advancedPageControls_.languageLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                          advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.languageCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                          WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                          0, 0, 0, 0, advancedPageControls_.page,
                                                          reinterpret_cast<HMENU>(kControlLanguageCombo), instance_, nullptr);
    advancedPageControls_.hwdecLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                       advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.hwdecCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                       WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                       0, 0, 0, 0, advancedPageControls_.page,
                                                       reinterpret_cast<HMENU>(kControlHwdecCombo), instance_, nullptr);
    advancedPageControls_.debugInfoCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                              advancedPageControls_.page,
                                                              reinterpret_cast<HMENU>(kControlDebugInfo), instance_, nullptr);
    advancedPageControls_.aspectRatioLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                             advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.aspectRatioCombo = CreateWindowExW(
        0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0,
        advancedPageControls_.page, reinterpret_cast<HMENU>(kControlAspectRatioCombo), instance_, nullptr);
    advancedPageControls_.rotateLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                        advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.rotateCombo = CreateWindowExW(
        0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0,
        advancedPageControls_.page, reinterpret_cast<HMENU>(kControlRotateCombo), instance_, nullptr);
    advancedPageControls_.mirrorVideoCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                advancedPageControls_.page,
                                                                reinterpret_cast<HMENU>(kControlMirrorVideo), instance_, nullptr);
    advancedPageControls_.deinterlaceCheckbox = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0,
                                                                advancedPageControls_.page,
                                                                reinterpret_cast<HMENU>(kControlDeinterlace), instance_, nullptr);
    advancedPageControls_.sharpenLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                         advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.sharpenBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                       advancedPageControls_.page,
                                                       reinterpret_cast<HMENU>(kControlSharpenBar), instance_, nullptr);
    advancedPageControls_.denoiseLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                         advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.denoiseBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                       advancedPageControls_.page,
                                                       reinterpret_cast<HMENU>(kControlDenoiseBar), instance_, nullptr);
    advancedPageControls_.equalizerLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                           advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.equalizerCombo = CreateWindowExW(
        0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 0, 0,
        advancedPageControls_.page, reinterpret_cast<HMENU>(kControlEqualizerCombo), instance_, nullptr);
    advancedPageControls_.screenshotFormatLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                  advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.screenshotFormatCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                                                  WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                                                  0, 0, 0, 0, advancedPageControls_.page,
                                                                  reinterpret_cast<HMENU>(kControlScreenshotFormatCombo), instance_, nullptr);
    advancedPageControls_.screenshotQualityLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                                   advancedPageControls_.page, nullptr, instance_, nullptr);
    advancedPageControls_.screenshotQualityBar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 0, 0, 0, 0,
                                                                 advancedPageControls_.page,
                                                                 reinterpret_cast<HMENU>(kControlScreenshotQualityBar), instance_, nullptr);
    advancedPageControls_.advancedHintLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                                                              advancedPageControls_.page, nullptr, instance_, nullptr);

    footerControls_.importButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                   reinterpret_cast<HMENU>(kControlImport), instance_, nullptr);
    footerControls_.exportButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                   reinterpret_cast<HMENU>(kControlExport), instance_, nullptr);
    footerControls_.resetButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                  reinterpret_cast<HMENU>(kControlReset), instance_, nullptr);
    footerControls_.okButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                               reinterpret_cast<HMENU>(kControlOk), instance_, nullptr);
    footerControls_.cancelButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                   reinterpret_cast<HMENU>(kControlCancel), instance_, nullptr);

    SendMessageW(playbackPageControls_.hideDelayBar, TBM_SETRANGE, TRUE, MAKELPARAM(10, 80));
    SendMessageW(playbackPageControls_.seekStepBar, TBM_SETRANGE, TRUE, MAKELPARAM(1, 60));
    SendMessageW(subtitlePageControls_.subtitleSizeBar, TBM_SETRANGE, TRUE, MAKELPARAM(12, 72));
    SendMessageW(subtitlePageControls_.subtitleDelayBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));
    SendMessageW(subtitlePageControls_.subtitleBorderSizeBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 12));
    SendMessageW(subtitlePageControls_.subtitleBackgroundOpacityBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageW(subtitlePageControls_.subtitleShadowDepthBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 6));
    SendMessageW(subtitlePageControls_.subtitleMarginBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageW(audioPageControls_.startupVolumeBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessageW(audioPageControls_.volumeStepBar, TBM_SETRANGE, TRUE, MAKELPARAM(1, 20));
    SendMessageW(audioPageControls_.wheelVolumeStepBar, TBM_SETRANGE, TRUE, MAKELPARAM(1, 20));
    SendMessageW(audioPageControls_.audioDelayBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));
    SendMessageW(advancedPageControls_.sharpenBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 20));
    SendMessageW(advancedPageControls_.denoiseBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, 20));
    SendMessageW(advancedPageControls_.screenshotQualityBar, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
    audioOutputValues_.clear();
    for (const auto& output : EnumerateAudioOutputs()) {
        audioOutputValues_.push_back(output.second);
        SendMessageW(audioPageControls_.audioOutputCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(output.first.c_str()));
    }

    PopulateLocalizedCombos();

    for (HWND combo : {playbackPageControls_.seekStepModeCombo,
                       playbackPageControls_.repeatModeCombo,
                       playbackPageControls_.endActionCombo,
                       subtitlePageControls_.subtitlePositionCombo,
                       subtitlePageControls_.subtitleEncodingCombo,
                       shortcutsPageControls_.doubleClickActionCombo,
                       shortcutsPageControls_.middleClickActionCombo,
                       advancedPageControls_.languageCombo,
                       advancedPageControls_.hwdecCombo,
                       advancedPageControls_.aspectRatioCombo,
                       advancedPageControls_.rotateCombo,
                       advancedPageControls_.equalizerCombo,
                       audioPageControls_.audioOutputCombo,
                       advancedPageControls_.screenshotFormatCombo}) {
        SendMessageW(combo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 28);
    }

    for (HWND edit : {subtitlePageControls_.subtitleOffsetUpEdit}) {
        SendMessageW(edit, EM_SETLIMITTEXT, 4, 0);
        SetWindowSubclass(edit, SettingsOffsetEditSubclassProc, 1, 0);
    }

    const std::vector<HWND> allControls = {
        playbackPageControls_.page,
        subtitlePageControls_.page,
        audioPageControls_.page,
        shortcutsPageControls_.page,
        advancedPageControls_.page,
        playbackPageControls_.autoplayNextCheckbox,
        playbackPageControls_.rememberPlaybackCheckbox,
        playbackPageControls_.preservePauseCheckbox,
        playbackPageControls_.seekPreviewCheckbox,
        playbackPageControls_.hideDelayLabel,
        playbackPageControls_.hideDelayBar,
        playbackPageControls_.seekStepModeLabel,
        playbackPageControls_.seekStepModeCombo,
        playbackPageControls_.seekStepLabel,
        playbackPageControls_.seekStepBar,
        playbackPageControls_.repeatModeLabel,
        playbackPageControls_.repeatModeCombo,
        playbackPageControls_.endActionLabel,
        playbackPageControls_.endActionCombo,
        subtitlePageControls_.autoSubtitleCheckbox,
        subtitlePageControls_.subtitleFontLabel,
        subtitlePageControls_.subtitleFontButton,
        subtitlePageControls_.subtitleSizeLabel,
        subtitlePageControls_.subtitleSizeBar,
        subtitlePageControls_.subtitleDelayLabel,
        subtitlePageControls_.subtitleDelayBar,
        subtitlePageControls_.subtitleTextColorButton,
        subtitlePageControls_.subtitleBorderColorButton,
        subtitlePageControls_.subtitleShadowColorButton,
        subtitlePageControls_.subtitleBackgroundCheckbox,
        subtitlePageControls_.subtitleBackgroundColorButton,
        subtitlePageControls_.subtitleBackgroundOpacityLabel,
        subtitlePageControls_.subtitleBackgroundOpacityBar,
        subtitlePageControls_.subtitlePositionLabel,
        subtitlePageControls_.subtitlePositionCombo,
        subtitlePageControls_.subtitleOffsetUpLabel,
        subtitlePageControls_.subtitleOffsetUpEdit,
        subtitlePageControls_.subtitleBorderSizeLabel,
        subtitlePageControls_.subtitleBorderSizeBar,
        subtitlePageControls_.subtitleShadowDepthLabel,
        subtitlePageControls_.subtitleShadowDepthBar,
        subtitlePageControls_.subtitleMarginLabel,
        subtitlePageControls_.subtitleMarginBar,
        subtitlePageControls_.subtitleEncodingLabel,
        subtitlePageControls_.subtitleEncodingCombo,
        audioPageControls_.rememberVolumeCheckbox,
        audioPageControls_.startupVolumeLabel,
        audioPageControls_.startupVolumeBar,
        audioPageControls_.volumeStepLabel,
        audioPageControls_.volumeStepBar,
        audioPageControls_.wheelVolumeStepLabel,
        audioPageControls_.wheelVolumeStepBar,
        audioPageControls_.audioDelayLabel,
        audioPageControls_.audioDelayBar,
        audioPageControls_.audioOutputLabel,
        audioPageControls_.audioOutputCombo,
        shortcutsPageControls_.doubleClickActionLabel,
        shortcutsPageControls_.doubleClickActionCombo,
        shortcutsPageControls_.middleClickActionLabel,
        shortcutsPageControls_.middleClickActionCombo,
        shortcutsPageControls_.shortcutWarningLabel,
        advancedPageControls_.languageLabel,
        advancedPageControls_.languageCombo,
        advancedPageControls_.hwdecLabel,
        advancedPageControls_.hwdecCombo,
        advancedPageControls_.debugInfoCheckbox,
        advancedPageControls_.aspectRatioLabel,
        advancedPageControls_.aspectRatioCombo,
        advancedPageControls_.rotateLabel,
        advancedPageControls_.rotateCombo,
        advancedPageControls_.mirrorVideoCheckbox,
        advancedPageControls_.deinterlaceCheckbox,
        advancedPageControls_.sharpenLabel,
        advancedPageControls_.sharpenBar,
        advancedPageControls_.denoiseLabel,
        advancedPageControls_.denoiseBar,
        advancedPageControls_.equalizerLabel,
        advancedPageControls_.equalizerCombo,
        advancedPageControls_.screenshotFormatLabel,
        advancedPageControls_.screenshotFormatCombo,
        advancedPageControls_.screenshotQualityLabel,
        advancedPageControls_.screenshotQualityBar,
        advancedPageControls_.advancedHintLabel,
        footerControls_.importButton,
        footerControls_.exportButton,
        footerControls_.resetButton,
        footerControls_.okButton,
        footerControls_.cancelButton,
    };
    for (HWND control : allControls) {
        SetControlFont(control, uiFont_);
    }
    for (HWND control : shortcutLabelControls_) {
        SetControlFont(control, uiFont_);
    }
    for (HWND control : shortcutComboControls_) {
        SetControlFont(control, uiFont_);
    }
    for (HWND control : navButtons_) {
        SetControlFont(control, uiFont_);
    }
}

void SettingsDialog::ApplyVisualTheme() {
    for (HWND page : {playbackPageControls_.page, subtitlePageControls_.page, audioPageControls_.page, shortcutsPageControls_.page,
                      advancedPageControls_.page}) {
        SetWindowTheme(page, L"", L"");
    }
    for (HWND combo : {playbackPageControls_.seekStepModeCombo,
                       playbackPageControls_.repeatModeCombo,
                       playbackPageControls_.endActionCombo,
                       subtitlePageControls_.subtitlePositionCombo,
                       subtitlePageControls_.subtitleEncodingCombo,
                       shortcutsPageControls_.doubleClickActionCombo,
                       shortcutsPageControls_.middleClickActionCombo,
                       advancedPageControls_.hwdecCombo,
                       advancedPageControls_.aspectRatioCombo,
                       advancedPageControls_.rotateCombo,
                       advancedPageControls_.equalizerCombo,
                       audioPageControls_.audioOutputCombo,
                       advancedPageControls_.screenshotFormatCombo,
                       advancedPageControls_.languageCombo}) {
        SetWindowTheme(combo, L"", L"");
    }
    for (HWND slider : {playbackPageControls_.hideDelayBar,
                        playbackPageControls_.seekStepBar,
                        subtitlePageControls_.subtitleSizeBar,
                        subtitlePageControls_.subtitleDelayBar,
                        subtitlePageControls_.subtitleBorderSizeBar,
                        subtitlePageControls_.subtitleBackgroundOpacityBar,
                        audioPageControls_.startupVolumeBar,
                        audioPageControls_.volumeStepBar,
                        audioPageControls_.wheelVolumeStepBar,
                        audioPageControls_.audioDelayBar,
                        advancedPageControls_.sharpenBar,
                        advancedPageControls_.denoiseBar,
                        advancedPageControls_.screenshotQualityBar}) {
        SetWindowTheme(slider, L"", L"");
    }
}

void SettingsDialog::LayoutControls() {
    RECT client{};
    GetClientRect(hwnd_, &client);

    const int navLeft = 20;
    const int navTop = 20;
    const int navWidth = 220;
    const int footerHeight = 72;
    const int navButtonHeight = 40;
    for (size_t index = 0; index < navButtons_.size(); ++index) {
        MoveWindow(navButtons_[index], navLeft + 12, navTop + 18 + static_cast<int>(index) * (navButtonHeight + 10), navWidth - 24,
                   navButtonHeight, TRUE);
    }

    RECT pageRect{navLeft + navWidth + 20, navTop, client.right - 20, client.bottom - footerHeight - 20};
    for (HWND page : {playbackPageControls_.page, subtitlePageControls_.page, audioPageControls_.page, shortcutsPageControls_.page,
                      advancedPageControls_.page}) {
        MoveWindow(page, pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top, TRUE);
    }

    const int pageWidth = pageRect.right - pageRect.left;
    const int insetLeft = 28;
    const int contentWidth = pageWidth - insetLeft * 2;
    const int columnGap = 28;
    const int columnWidth = (contentWidth - columnGap) / 2;
    const int leftColumn = insetLeft;
    const int rightColumn = insetLeft + columnWidth + columnGap;

    MoveWindow(playbackPageControls_.autoplayNextCheckbox, insetLeft, 28, contentWidth, 24, TRUE);
    MoveWindow(playbackPageControls_.rememberPlaybackCheckbox, insetLeft, 60, contentWidth, 24, TRUE);
    MoveWindow(playbackPageControls_.preservePauseCheckbox, insetLeft, 92, contentWidth, 24, TRUE);
    MoveWindow(playbackPageControls_.seekPreviewCheckbox, insetLeft, 124, contentWidth, 24, TRUE);
    MoveWindow(playbackPageControls_.hideDelayLabel, insetLeft, 168, columnWidth, 22, TRUE);
    MoveWindow(playbackPageControls_.hideDelayBar, insetLeft, 196, columnWidth, 32, TRUE);
    MoveWindow(playbackPageControls_.seekStepModeLabel, rightColumn, 168, columnWidth, 22, TRUE);
    MoveWindow(playbackPageControls_.seekStepModeCombo, rightColumn, 196, 180, 240, TRUE);
    MoveWindow(playbackPageControls_.seekStepLabel, rightColumn, 248, columnWidth, 22, TRUE);
    MoveWindow(playbackPageControls_.seekStepBar, rightColumn, 276, columnWidth, 32, TRUE);
    MoveWindow(playbackPageControls_.repeatModeLabel, insetLeft, 328, 180, 22, TRUE);
    MoveWindow(playbackPageControls_.repeatModeCombo, insetLeft, 356, 220, 240, TRUE);
    MoveWindow(playbackPageControls_.endActionLabel, rightColumn, 328, columnWidth, 22, TRUE);
    MoveWindow(playbackPageControls_.endActionCombo, rightColumn, 356, 240, 240, TRUE);

    MoveWindow(subtitlePageControls_.autoSubtitleCheckbox, insetLeft, 24, contentWidth, 24, TRUE);
    MoveWindow(subtitlePageControls_.subtitleFontLabel, leftColumn, 60, 110, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleFontButton, leftColumn + 118, 56, columnWidth - 118, 30, TRUE);
    MoveWindow(subtitlePageControls_.subtitleSizeLabel, leftColumn, 98, columnWidth, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleSizeBar, leftColumn, 124, columnWidth, 32, TRUE);
    MoveWindow(subtitlePageControls_.subtitleDelayLabel, rightColumn, 98, columnWidth, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleDelayBar, rightColumn, 124, columnWidth, 32, TRUE);
    MoveWindow(subtitlePageControls_.subtitlePositionLabel, leftColumn, 166, 110, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitlePositionCombo, leftColumn + 118, 162, 120, 240, TRUE);
    MoveWindow(subtitlePageControls_.subtitleOffsetUpLabel, leftColumn + 252, 166, 76, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleOffsetUpEdit, leftColumn + 334, 162, 86, 28, TRUE);
    MoveWindow(subtitlePageControls_.subtitleTextColorButton, leftColumn, 208, 150, 32, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBorderColorButton, leftColumn + 162, 208, 150, 32, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBorderSizeLabel, leftColumn, 252, columnWidth, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBorderSizeBar, leftColumn, 278, columnWidth, 32, TRUE);
    MoveWindow(subtitlePageControls_.subtitleEncodingLabel, rightColumn, 252, 140, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleEncodingCombo, rightColumn, 278, 220, 240, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBackgroundCheckbox, leftColumn, 322, columnWidth, 24, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBackgroundColorButton, leftColumn, 352, 180, 32, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBackgroundOpacityLabel, rightColumn, 322, columnWidth, 22, TRUE);
    MoveWindow(subtitlePageControls_.subtitleBackgroundOpacityBar, rightColumn, 352, columnWidth, 32, TRUE);

    MoveWindow(audioPageControls_.rememberVolumeCheckbox, insetLeft, 28, contentWidth, 24, TRUE);
    MoveWindow(audioPageControls_.startupVolumeLabel, leftColumn, 68, columnWidth, 22, TRUE);
    MoveWindow(audioPageControls_.startupVolumeBar, leftColumn, 96, columnWidth, 32, TRUE);
    MoveWindow(audioPageControls_.volumeStepLabel, rightColumn, 68, columnWidth, 22, TRUE);
    MoveWindow(audioPageControls_.volumeStepBar, rightColumn, 96, columnWidth, 32, TRUE);
    MoveWindow(audioPageControls_.wheelVolumeStepLabel, leftColumn, 148, columnWidth, 22, TRUE);
    MoveWindow(audioPageControls_.wheelVolumeStepBar, leftColumn, 176, columnWidth, 32, TRUE);
    MoveWindow(audioPageControls_.audioDelayLabel, rightColumn, 148, columnWidth, 22, TRUE);
    MoveWindow(audioPageControls_.audioDelayBar, rightColumn, 176, columnWidth, 32, TRUE);
    MoveWindow(audioPageControls_.audioOutputLabel, insetLeft, 232, contentWidth, 22, TRUE);
    MoveWindow(audioPageControls_.audioOutputCombo, insetLeft, 260, 360, 260, TRUE);

    MoveWindow(shortcutsPageControls_.doubleClickActionLabel, insetLeft, 24, 120, 22, TRUE);
    MoveWindow(shortcutsPageControls_.doubleClickActionCombo, insetLeft + 136, 20, 110, 240, TRUE);
    MoveWindow(shortcutsPageControls_.middleClickActionLabel, insetLeft + 300, 24, 96, 22, TRUE);
    MoveWindow(shortcutsPageControls_.middleClickActionCombo, insetLeft + 404, 20, 110, 240, TRUE);
    MoveWindow(shortcutsPageControls_.shortcutWarningLabel, insetLeft, 64, contentWidth, 22, TRUE);
    const int shortcutTop = 104;
    const int shortcutGapY = 36;
    const int shortcutColumnGap = 28;
    const int shortcutColumnWidth = (contentWidth - shortcutColumnGap) / 2;
    const int shortcutButtonWidth = 164;
    const int shortcutLabelWidth = std::max(96, shortcutColumnWidth - shortcutButtonWidth - 12);
    const size_t rowsPerColumn = (shortcutLabelControls_.size() + 1) / 2;
    for (size_t index = 0; index < shortcutLabelControls_.size() && index < shortcutComboControls_.size(); ++index) {
        const bool rightColumn = index >= rowsPerColumn;
        const size_t row = rightColumn ? (index - rowsPerColumn) : index;
        const int columnLeft = insetLeft + (rightColumn ? shortcutColumnWidth + shortcutColumnGap : 0);
        const int top = shortcutTop + static_cast<int>(row) * shortcutGapY;
        MoveWindow(shortcutLabelControls_[index], columnLeft, top, shortcutLabelWidth, 24, TRUE);
        MoveWindow(shortcutComboControls_[index], columnLeft + shortcutLabelWidth + 12, top - 2, shortcutButtonWidth, 28, TRUE);
    }

    MoveWindow(advancedPageControls_.languageLabel, leftColumn, 28, 100, 22, TRUE);
    MoveWindow(advancedPageControls_.languageCombo, leftColumn + 108, 24, 180, 240, TRUE);
    MoveWindow(advancedPageControls_.hwdecLabel, rightColumn, 28, 110, 22, TRUE);
    MoveWindow(advancedPageControls_.hwdecCombo, rightColumn + 118, 24, 180, 240, TRUE);
    MoveWindow(advancedPageControls_.debugInfoCheckbox, insetLeft, 68, contentWidth, 24, TRUE);
    MoveWindow(advancedPageControls_.aspectRatioLabel, leftColumn, 104, 110, 22, TRUE);
    MoveWindow(advancedPageControls_.aspectRatioCombo, leftColumn + 118, 100, 160, 240, TRUE);
    MoveWindow(advancedPageControls_.rotateLabel, rightColumn, 104, 110, 22, TRUE);
    MoveWindow(advancedPageControls_.rotateCombo, rightColumn + 118, 100, 160, 240, TRUE);
    MoveWindow(advancedPageControls_.mirrorVideoCheckbox, leftColumn, 144, columnWidth, 24, TRUE);
    MoveWindow(advancedPageControls_.deinterlaceCheckbox, rightColumn, 144, columnWidth, 24, TRUE);
    MoveWindow(advancedPageControls_.sharpenLabel, leftColumn, 184, columnWidth, 22, TRUE);
    MoveWindow(advancedPageControls_.sharpenBar, leftColumn, 210, columnWidth, 32, TRUE);
    MoveWindow(advancedPageControls_.denoiseLabel, rightColumn, 184, columnWidth, 22, TRUE);
    MoveWindow(advancedPageControls_.denoiseBar, rightColumn, 210, columnWidth, 32, TRUE);
    MoveWindow(advancedPageControls_.equalizerLabel, leftColumn, 254, 110, 22, TRUE);
    MoveWindow(advancedPageControls_.equalizerCombo, leftColumn + 118, 250, 160, 240, TRUE);
    MoveWindow(advancedPageControls_.screenshotFormatLabel, rightColumn, 254, 120, 22, TRUE);
    MoveWindow(advancedPageControls_.screenshotFormatCombo, rightColumn + 128, 250, 140, 240, TRUE);
    MoveWindow(advancedPageControls_.screenshotQualityLabel, insetLeft, 302, contentWidth, 22, TRUE);
    MoveWindow(advancedPageControls_.screenshotQualityBar, insetLeft, 328, contentWidth, 32, TRUE);
    MoveWindow(advancedPageControls_.advancedHintLabel, insetLeft, 376, contentWidth, 42, TRUE);

    const int footerTop = client.bottom - footerHeight + 12;
    MoveWindow(footerControls_.importButton, 280, footerTop, 116, 36, TRUE);
    MoveWindow(footerControls_.exportButton, 408, footerTop, 116, 36, TRUE);
    MoveWindow(footerControls_.resetButton, 536, footerTop, 116, 36, TRUE);
    MoveWindow(footerControls_.cancelButton, client.right - 264, footerTop, 104, 36, TRUE);
    MoveWindow(footerControls_.okButton, client.right - 144, footerTop, 104, 36, TRUE);
}

void SettingsDialog::UpdatePageVisibility() {
    ShowWindow(playbackPageControls_.page, currentPage_ == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(subtitlePageControls_.page, currentPage_ == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(audioPageControls_.page, currentPage_ == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(shortcutsPageControls_.page, currentPage_ == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(advancedPageControls_.page, currentPage_ == 4 ? SW_SHOW : SW_HIDE);
    for (HWND navButton : navButtons_) {
        InvalidateRect(navButton, nullptr, FALSE);
    }
}

void SettingsDialog::SelectPage(const int pageIndex) {
    currentPage_ = std::clamp(pageIndex, 0, static_cast<int>(navButtons_.size()) - 1);
    UpdatePageVisibility();
}

}  // namespace velo::ui
