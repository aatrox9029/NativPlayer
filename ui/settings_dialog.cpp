#include "ui/settings_dialog_internal.h"

namespace velo::ui {

namespace {

SettingsDialog* LookupSettingsDialogForTesting(HWND hwnd) {
    if (hwnd == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

}  // namespace

std::optional<velo::config::AppConfig> SettingsDialog::ShowModal(HINSTANCE instance, HWND owner,
                                                                 const velo::config::AppConfig& initialConfig,
                                                                 std::function<void(const velo::config::AppConfig&)> previewCallback) {
    TraceSettingsDialog(L"Opening settings dialog.");
    instance_ = instance;
    owner_ = owner;
    originalConfig_ = initialConfig;
    initialConfig_ = initialConfig;
    previewCallback_ = std::move(previewCallback);
    currentPage_ = 0;
    result_ = Result::None;
    capturingShortcutIndex_ = -1;
    isInitializing_ = true;

    INITCOMMONCONTROLSEX commonControls{};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&commonControls);

    RegisterSettingsClass(instance_);
    hwnd_ = CreateWindowExW(WS_EX_DLGMODALFRAME, kSettingsDialogClassName,
                            (velo::app::AppDisplayName() + L" " +
                             velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsTitle))
                                .c_str(),
                            WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 980, 700, owner_, nullptr,
                            instance_, this);
    if (hwnd_ == nullptr) {
        TraceSettingsDialog(L"CreateWindowExW failed.");
        isInitializing_ = false;
        return std::nullopt;
    }

    EnableWindow(owner_, FALSE);
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    MSG message{};
    while (result_ == Result::None && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (hwnd_ != nullptr && IsDialogMessageW(hwnd_, &message)) {
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    EnableWindow(owner_, TRUE);
    SetForegroundWindow(owner_);
    TraceSettingsDialog(result_ == Result::Accepted ? L"Dialog closed with accept." : L"Dialog closed without accept.");
    if (result_ != Result::Accepted) {
        return std::nullopt;
    }
    return initialConfig_;
}

LRESULT CALLBACK SettingsDialog::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<SettingsDialog*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    auto* self = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->HandleMessage(message, wParam, lParam);
}

LRESULT SettingsDialog::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            TraceSettingsDialog(L"WM_CREATE start.");
            backgroundBrush_ = CreateSolidBrush(tokens::DarkPalette().bgSurface1);
            pageBrush_ = CreateSolidBrush(tokens::MixColor(tokens::DarkPalette().bgSurface2, tokens::DarkPalette().bgCanvas, 0.10));
            controlBrush_ = CreateSolidBrush(tokens::MixColor(tokens::DarkPalette().bgSurface1, tokens::DarkPalette().bgCanvas, 0.22));
            pageSurfaceTheme_ = {.owner = hwnd_, .pageBrush = pageBrush_, .controlBrush = controlBrush_};
            uiFont_ = tokens::CreateAppFont(tokens::FontRole::Body);
            titleFont_ = tokens::CreateAppFont(tokens::FontRole::H3);
            CreateControls();
            ApplyVisualTheme();
            LayoutControls();
            LoadConfigToControls();
            isInitializing_ = false;
            SetPropW(hwnd_, kSettingsDialogReadyProperty, reinterpret_cast<HANDLE>(1));
            if (testing_internal::g_settingsDialogAutomationCallback) {
                PostMessageW(hwnd_, testing_internal::kSettingsDialogAutomationMessage, 0, 0);
            }
            TraceSettingsDialog(L"WM_CREATE finished.");
            return 0;
        case WM_SIZE:
            LayoutControls();
            return 0;
        case testing_internal::kSettingsDialogAutomationMessage:
            if (testing_internal::g_settingsDialogAutomationCallback) {
                testing_internal::g_settingsDialogAutomationCallback(hwnd_);
            }
            return 0;
        case WM_COMMAND: {
            const UINT controlId = LOWORD(wParam);
            switch (controlId) {
                case kControlNavPlayback:
                case kControlNavSubtitle:
                case kControlNavAudio:
                case kControlNavShortcuts:
                case kControlNavAdvanced:
                    SelectPage(NavPageFromControlId(controlId));
                    return 0;
                case kControlRememberPlayback:
                    ToggleCheckbox(playbackPageControls_.rememberPlaybackCheckbox);
                    return 0;
                case kControlAutoplayNext:
                    ToggleCheckbox(playbackPageControls_.autoplayNextCheckbox);
                    return 0;
                case kControlPreservePause:
                    ToggleCheckbox(playbackPageControls_.preservePauseCheckbox);
                    return 0;
                case kControlSeekPreview:
                    ToggleCheckbox(playbackPageControls_.seekPreviewCheckbox);
                    return 0;
                case kControlSeekStepModeCombo:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        UpdateSeekStepModeControls(true);
                        return 0;
                    }
                    break;
                case kControlAutoSubtitle:
                    ToggleCheckbox(subtitlePageControls_.autoSubtitleCheckbox);
                    OnSubtitleControlsChanged();
                    return 0;
                case kControlSubtitleBackground:
                    ToggleCheckbox(subtitlePageControls_.subtitleBackgroundCheckbox);
                    OnSubtitleControlsChanged();
                    return 0;
                case kControlRememberVolume:
                    ToggleCheckbox(audioPageControls_.rememberVolumeCheckbox);
                    return 0;
                case kControlDebugInfo:
                    ToggleCheckbox(advancedPageControls_.debugInfoCheckbox);
                    UpdateShortcutWarning();
                    return 0;
                case kControlLanguageCombo:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        const int index = static_cast<int>(SendMessageW(advancedPageControls_.languageCombo, CB_GETCURSEL, 0, 0));
                        initialConfig_.languageCode = index == 1 ? L"zh-CN" : (index == 2 ? L"en-US" : L"zh-TW");
                        PopulateLocalizedCombos();
                        RefreshLocalizedText();
                        LayoutControls();
                        RefreshSubtitleColorButtons();
                        OnSubtitleControlsChanged();
                        RedrawWindow(hwnd_, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                        return 0;
                    }
                    break;
                case kControlMirrorVideo:
                    ToggleCheckbox(advancedPageControls_.mirrorVideoCheckbox);
                    return 0;
                case kControlDeinterlace:
                    ToggleCheckbox(advancedPageControls_.deinterlaceCheckbox);
                    return 0;
                case kControlSubtitleFontButton:
                    ChooseSubtitleFont();
                    return 0;
                case kControlSubtitleTextColor:
                    ChooseSubtitleColor(&initialConfig_.subtitleTextColor);
                    return 0;
                case kControlSubtitleBorderColor:
                    ChooseSubtitleColor(&initialConfig_.subtitleBorderColor);
                    return 0;
                case kControlSubtitleBackgroundColor:
                    ChooseSubtitleColor(&initialConfig_.subtitleBackgroundColor);
                    return 0;
                case kControlImport:
                    ImportSettings();
                    return 0;
                case kControlExport:
                    ExportSettings();
                    return 0;
                case kControlReset:
                    ResetDefaults();
                    return 0;
                case kControlOk:
                    CommitAndClose();
                    return 0;
                case kControlCancel:
                    Close(Result::Cancelled);
                    return 0;
                default:
                    if ((controlId == kControlSubtitlePositionCombo && HIWORD(wParam) == CBN_SELCHANGE) ||
                        (controlId == kControlSubtitleEncodingCombo && HIWORD(wParam) == CBN_SELCHANGE) ||
                        (controlId == kControlSubtitleOffsetUpEdit && HIWORD(wParam) == EN_CHANGE)) {
                        OnSubtitleControlsChanged();
                        return 0;
                    }
                    if (controlId >= kShortcutButtonBase &&
                        controlId < kShortcutButtonBase + static_cast<UINT>(shortcutComboControls_.size())) {
                        BeginShortcutCapture(controlId - kShortcutButtonBase);
                        return 0;
                    }
                    UpdateShortcutWarning();
                    return 0;
            }
            break;
        }
        case WM_HSCROLL:
            UpdateSliderLabels();
            if (reinterpret_cast<HWND>(lParam) == subtitlePageControls_.subtitleSizeBar ||
                reinterpret_cast<HWND>(lParam) == subtitlePageControls_.subtitleDelayBar ||
                reinterpret_cast<HWND>(lParam) == subtitlePageControls_.subtitleBorderSizeBar ||
                reinterpret_cast<HWND>(lParam) == subtitlePageControls_.subtitleBackgroundOpacityBar) {
                OnSubtitleControlsChanged();
            }
            return 0;
        case WM_MOUSEWHEEL: {
            const POINT screenPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            HWND hovered = WindowFromPoint(screenPoint);
            if (hovered != nullptr && GetAncestor(hovered, GA_ROOT) == hwnd_ &&
                HandleSubtitleOffsetWheel(hovered, GET_WHEEL_DELTA_WPARAM(wParam))) {
                return 0;
            }
            break;
        }
        case WM_KEYDOWN:
            if (capturingShortcutIndex_ >= 0) {
                if (wParam == VK_ESCAPE) {
                    CancelShortcutCapture();
                    return 0;
                }
                const unsigned int resolvedKey = ResolveShortcutCaptureKey(static_cast<unsigned int>(wParam), lParam);
                if (!IsAllowedShortcutCaptureKey(resolvedKey)) {
                    return 0;
                }
                if (CaptureShortcutInput(resolvedKey)) {
                    return 0;
                }
            }
            break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if (capturingShortcutIndex_ >= 0) {
                return 0;
            }
            break;
        case WM_MBUTTONUP:
            if (capturingShortcutIndex_ >= 0 && IsAllowedShortcutCapturePointer(VK_MBUTTON) && CaptureShortcutInput(VK_MBUTTON)) {
                return 0;
            }
            break;
        case WM_XBUTTONUP:
            if (capturingShortcutIndex_ >= 0) {
                const unsigned int virtualKey = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
                if (IsAllowedShortcutCapturePointer(virtualKey) && CaptureShortcutInput(virtualKey)) {
                    return 0;
                }
            }
            break;
        case WM_CLOSE:
            Close(Result::Cancelled);
            return 0;
        case WM_DRAWITEM: {
            const auto* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (drawItem->CtlType == ODT_BUTTON) {
                DrawButton(*drawItem);
                return TRUE;
            }
            if (drawItem->CtlType == ODT_COMBOBOX) {
                DrawComboItem(*drawItem);
                return TRUE;
            }
            break;
        }
        case WM_MEASUREITEM: {
            auto* measureItem = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            if (measureItem->CtlType == ODT_COMBOBOX) {
                measureItem->itemHeight = 28;
                measureItem->itemWidth = 260;
                return TRUE;
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            const auto& palette = tokens::DarkPalette();
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkMode(dc, OPAQUE);
            SetBkColor(dc, palette.bgSurface1);
            SetTextColor(dc, palette.textPrimary);
            return reinterpret_cast<LRESULT>(backgroundBrush_);
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            const auto& palette = tokens::DarkPalette();
            HDC dc = reinterpret_cast<HDC>(wParam);
            SetBkMode(dc, OPAQUE);
            SetBkColor(dc, tokens::MixColor(palette.bgSurface2, palette.bgCanvas, 0.18));
            SetTextColor(dc, palette.textPrimary);
            return reinterpret_cast<LRESULT>(controlBrush_);
        }
        case WM_ERASEBKGND: {
            RECT client{};
            GetClientRect(hwnd_, &client);
            FillRect(reinterpret_cast<HDC>(wParam), &client, backgroundBrush_);
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd_, &paint);
            RECT client{};
            GetClientRect(hwnd_, &client);
            const auto& palette = tokens::DarkPalette();
            FillRect(dc, &client, backgroundBrush_);
            RECT navRect{20, 20, 240, client.bottom - 92};
            tokens::FillRoundedRect(dc, navRect, tokens::MixColor(palette.bgSurface2, palette.bgCanvas, 0.18), tokens::RadiusLg());
            tokens::DrawRoundedOutline(dc, navRect, palette.strokeSoft, tokens::RadiusLg());
            RECT pageRect{260, 20, client.right - 20, client.bottom - 92};
            tokens::FillRoundedRect(dc, pageRect, tokens::MixColor(palette.bgSurface1, palette.bgCanvas, 0.14), tokens::RadiusLg());
            tokens::DrawRoundedOutline(dc, pageRect, palette.strokeSoft, tokens::RadiusLg());
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, palette.textPrimary);
            HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(dc, titleFont_));
            RECT titleRect{40, 34, 220, 64};
            const std::wstring title = velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsTitle);
            DrawTextW(dc, title.c_str(), -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, previousFont);
            EndPaint(hwnd_, &paint);
            return 0;
        }
        case WM_NCDESTROY: {
            TraceSettingsDialog(L"WM_NCDESTROY.");
            isInitializing_ = false;
            HWND dialogHandle = hwnd_;
            if (dialogHandle != nullptr) {
                RemovePropW(dialogHandle, kSettingsDialogReadyProperty);
            }
            if (backgroundBrush_ != nullptr) {
                DeleteObject(backgroundBrush_);
                backgroundBrush_ = nullptr;
            }
            if (pageBrush_ != nullptr) {
                DeleteObject(pageBrush_);
                pageBrush_ = nullptr;
            }
            if (controlBrush_ != nullptr) {
                DeleteObject(controlBrush_);
                controlBrush_ = nullptr;
            }
            if (uiFont_ != nullptr) {
                DeleteObject(uiFont_);
                uiFont_ = nullptr;
            }
            if (titleFont_ != nullptr) {
                DeleteObject(titleFont_);
                titleFont_ = nullptr;
            }
            if (dialogHandle != nullptr) {
                SetWindowLongPtrW(dialogHandle, GWLP_USERDATA, 0);
            }
            hwnd_ = nullptr;
            return DefWindowProcW(dialogHandle, message, wParam, lParam);
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void SettingsDialog::UpdateShortcutWarning() {
    if (!HasStableControlsForConfig()) {
        hasShortcutConflict_ = false;
        if (shortcutsPageControls_.shortcutWarningLabel != nullptr) {
            SetWindowTextW(shortcutsPageControls_.shortcutWarningLabel, L"");
        }
        if (footerControls_.okButton != nullptr) {
            EnableWindow(footerControls_.okButton, TRUE);
        }
        return;
    }

    const auto conflicts = velo::platform::win32::FindBindingConflicts(BuildConfig());
    hasShortcutConflict_ = !conflicts.empty();
    if (capturingShortcutIndex_ >= 0) {
        SetWindowTextW(shortcutsPageControls_.shortcutWarningLabel,
                       velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsShortcutWarningCapturing).c_str());
    } else if (conflicts.empty()) {
        SetWindowTextW(shortcutsPageControls_.shortcutWarningLabel,
                       velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsShortcutWarningIdle).c_str());
    } else {
        SetWindowTextW(shortcutsPageControls_.shortcutWarningLabel, conflicts.front().c_str());
    }
    EnableWindow(footerControls_.okButton, hasShortcutConflict_ ? FALSE : TRUE);
}

const wchar_t* SettingsDialogClassNameForTesting() {
    return kSettingsDialogClassName;
}

UINT SettingsDialogAudioOutputComboIdForTesting() {
    return kControlAudioOutputCombo;
}

UINT SettingsDialogSubtitleNavControlIdForTesting() {
    return kControlNavSubtitle;
}

UINT SettingsDialogSubtitlePositionComboIdForTesting() {
    return kControlSubtitlePositionCombo;
}

UINT SettingsDialogSubtitleTextColorControlIdForTesting() {
    return kControlSubtitleTextColor;
}

UINT SettingsDialogSubtitleFontButtonControlIdForTesting() {
    return kControlSubtitleFontButton;
}

UINT SettingsDialogSubtitleBackgroundOpacityControlIdForTesting() {
    return kControlSubtitleBackgroundOpacityBar;
}

UINT SettingsDialogOkControlIdForTesting() {
    return kControlOk;
}

void SetSettingsDialogAudioOutputsOverrideForTesting(
    std::optional<std::vector<std::pair<std::wstring, std::wstring>>> outputs) {
    shared_testing_state::g_audioOutputOverride = std::move(outputs);
}

void SetSettingsDialogColorOverrideForTesting(std::optional<COLORREF> color) {
    shared_testing_state::g_colorDialogOverride = color;
    if (color.has_value()) {
        shared_testing_state::g_lastColorDialogInitialColor.reset();
    }
}

std::optional<COLORREF> SettingsDialogLastColorDialogInitialColorForTesting() {
    return shared_testing_state::g_lastColorDialogInitialColor;
}

bool SettingsDialogWindowReadyForTesting(HWND hwnd) {
    return hwnd != nullptr && GetPropW(hwnd, kSettingsDialogReadyProperty) != nullptr;
}

void SetSettingsDialogAutomationCallbackForTesting(std::function<void(HWND)> callback) {
    testing_internal::g_settingsDialogAutomationCallback = std::move(callback);
}

bool SettingsDialogCancelForTesting(HWND hwnd) {
    auto* self = LookupSettingsDialogForTesting(hwnd);
    if (self == nullptr) {
        return false;
    }
    self->Close(SettingsDialog::Result::Cancelled);
    return true;
}

bool SettingsDialogAcceptForTesting(HWND hwnd) {
    auto* self = LookupSettingsDialogForTesting(hwnd);
    if (self == nullptr) {
        return false;
    }
    self->CommitAndClose();
    return self->result_ == SettingsDialog::Result::Accepted;
}

bool SettingsDialogSelectSubtitlePageForTesting(HWND hwnd) {
    auto* self = LookupSettingsDialogForTesting(hwnd);
    if (self == nullptr) {
        return false;
    }
    self->SelectPage(1);
    return true;
}

bool SettingsDialogSetSubtitlePositionForTesting(HWND hwnd, const int index) {
    auto* self = LookupSettingsDialogForTesting(hwnd);
    if (self == nullptr || self->subtitlePageControls_.subtitlePositionCombo == nullptr) {
        return false;
    }
    SendMessageW(self->subtitlePageControls_.subtitlePositionCombo, CB_SETCURSEL, index, 0);
    self->OnSubtitleControlsChanged();
    return true;
}

bool SettingsDialogChooseSubtitleTextColorForTesting(HWND hwnd) {
    auto* self = LookupSettingsDialogForTesting(hwnd);
    if (self == nullptr) {
        return false;
    }
    self->ChooseSubtitleColor(&self->initialConfig_.subtitleTextColor);
    return true;
}

}  // namespace velo::ui
