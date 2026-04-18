#pragma once

#include "ui/settings_dialog.h"

#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <mmdeviceapi.h>
#include <objbase.h>
#include <propsys.h>
#include <propvarutil.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "app/app_metadata.h"
#include "common/hex_color.h"
#include "config/subtitle_position_utils.h"
#include "localization/localization.h"
#include "platform/win32/input_profile.h"
#include "ui/combo_box_utils.h"
#include "ui/design_tokens.h"
#include "ui/settings_dialog_state.h"
#include "ui/shortcut_capture.h"

namespace velo::ui {

namespace shared_testing_state {

inline std::optional<std::vector<std::pair<std::wstring, std::wstring>>> g_audioOutputOverride;
inline std::optional<COLORREF> g_colorDialogOverride;
inline std::optional<COLORREF> g_lastColorDialogInitialColor;

}  // namespace shared_testing_state

namespace testing_internal {

inline constexpr UINT kSettingsDialogAutomationMessage = WM_APP + 41;
inline std::function<void(HWND)> g_settingsDialogAutomationCallback;

}  // namespace testing_internal

namespace {

constexpr wchar_t kSettingsDialogClassName[] = L"NativPlayerSettingsDialog";
constexpr wchar_t kSettingsDialogReadyProperty[] = L"NativPlayerSettingsDialogReady";
const PROPERTYKEY kDeviceFriendlyNameKey = {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}, 14};

enum ControlId {
    kControlNavPlayback = 6001,
    kControlNavSubtitle = 6002,
    kControlNavAudio = 6003,
    kControlNavShortcuts = 6004,
    kControlNavAdvanced = 6005,
    kControlAutoplayNext = 6020,
    kControlRememberPlayback = 6021,
    kControlHideDelayBar = 6022,
    kControlAutoSubtitle = 6023,
    kControlPreservePause = 6027,
    kControlSeekStepBar = 6028,
    kControlEndActionCombo = 6029,
    kControlSeekStepModeCombo = 6074,
    kControlSubtitleSizeBar = 6030,
    kControlScreenshotQualityBar = 6031,
    kControlDoubleClickActionCombo = 6032,
    kControlMiddleClickActionCombo = 6033,
    kControlDebugInfo = 6034,
    kControlImport = 6035,
    kControlExport = 6036,
    kControlReset = 6037,
    kControlSeekPreview = 6038,
    kControlSubtitleFontButton = 6039,
    kControlSubtitleTextColor = 6040,
    kControlSubtitleBorderColor = 6041,
    kControlSubtitleShadowColor = 6042,
    kControlSubtitleBackground = 6066,
    kControlSubtitleBackgroundColor = 6067,
    kControlSubtitlePositionCombo = 6068,
    kControlSubtitleOffsetUpEdit = 6069,
    kControlSubtitleOffsetDownEdit = 6070,
    kControlSubtitleOffsetLeftEdit = 6071,
    kControlSubtitleOffsetRightEdit = 6072,
    kControlAudioOutputCombo = 6043,
    kControlScreenshotFormatCombo = 6044,
    kControlHwdecCombo = 6045,
    kControlLanguageCombo = 6073,
    kControlRepeatModeCombo = 6046,
    kControlSubtitleBackgroundOpacityBar = 6075,
    kControlRememberVolume = 6047,
    kControlStartupVolumeBar = 6048,
    kControlVolumeStepBar = 6049,
    kControlWheelVolumeStepBar = 6050,
    kControlAudioDelayBar = 6051,
    kControlSubtitleDelayBar = 6052,
    kControlSubtitleBorderSizeBar = 6053,
    kControlSubtitleShadowDepthBar = 6054,
    kControlSubtitleMarginBar = 6055,
    kControlSubtitleEncodingCombo = 6056,
    kControlAspectRatioCombo = 6057,
    kControlRotateCombo = 6058,
    kControlMirrorVideo = 6059,
    kControlDeinterlace = 6060,
    kControlSharpenBar = 6061,
    kControlDenoiseBar = 6062,
    kControlEqualizerCombo = 6063,
    kControlNetworkTimeoutBar = 6064,
    kControlReconnectBar = 6065,
    kShortcutButtonBase = 6200,
    kControlOk = IDOK,
    kControlCancel = IDCANCEL,
};

ATOM RegisterSettingsClass(HINSTANCE instance) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = SettingsDialog::WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kSettingsDialogClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    atom = RegisterClassW(&windowClass);
    return atom;
}

void TraceSettingsDialog(const std::wstring& message) {
    OutputDebugStringW((L"[NativPlayer][SettingsDialog] " + message + L"\n").c_str());
}

void SetControlFont(HWND control, HFONT font) {
    if (control != nullptr) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

int SliderToDelayMs(const int position) {
    return position * 100;
}

int DelayMsToSlider(const int milliseconds) {
    return milliseconds / 100;
}

int SignedTenthSecondsToSlider(const double seconds) {
    return std::clamp(static_cast<int>(std::lround(seconds * 10.0)) + 100, 0, 200);
}

double SliderToSignedTenthSeconds(const int position) {
    return static_cast<double>(std::clamp(position, 0, 200) - 100) / 10.0;
}

int SecondsToTenthSlider(const double seconds) {
    return static_cast<int>(seconds * 10.0);
}

double TenthSliderToSeconds(const int position) {
    return static_cast<double>(position) / 10.0;
}

int SubtitleHorizontalOffsetFromLegacyOffsets(const int leftOffset, const int rightOffset) {
    return std::clamp(rightOffset - leftOffset, -200, 200);
}

constexpr int kSubtitleOffsetInputMax = 999;
constexpr int kSubtitleVerticalDirectionMin = velo::config::kSubtitleVerticalDirectionMin;
constexpr int kSubtitleVerticalDirectionMax = velo::config::kSubtitleVerticalDirectionMax;

int SubtitleVerticalDirectionFromConfig(const velo::config::AppConfig& config) {
    return velo::config::SubtitleVerticalDirectionFromMargin(config.subtitlePositionPreset, config.subtitleVerticalMargin);
}

int SubtitleBackgroundOpacityPercent(const std::wstring& rgbaHex) {
    const auto color = velo::common::TryParseRgbaColor(rgbaHex);
    const int alpha = color.has_value() ? static_cast<int>(color->alpha) : 0xFF;
    return std::clamp(static_cast<int>(std::lround((static_cast<double>(alpha) / 255.0) * 100.0)), 0, 100);
}

void ApplySubtitleBackgroundOpacity(std::wstring* rgbaHex, const int opacityPercent) {
    if (rgbaHex == nullptr) {
        return;
    }

    velo::common::RgbaColor color = velo::common::TryParseRgbaColor(*rgbaHex).value_or(velo::common::RgbaColor{
        0x10, 0x10, 0x10, 0xFF
    });
    color.alpha = static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(std::lround((static_cast<double>(std::clamp(opacityPercent, 0, 100)) / 100.0) * 255.0)), 0, 255));
    *rgbaHex = velo::common::FormatRgbaHexColor(color);
}

void ApplySubtitleVerticalDirection(velo::config::AppConfig* config, const int direction) {
    if (config == nullptr) {
        return;
    }

    const int clampedDirection = velo::config::ClampSubtitleVerticalDirection(direction);
    config->subtitleOffsetUp = std::max(0, clampedDirection);
    config->subtitleOffsetDown = std::max(0, -clampedDirection);
    config->subtitleVerticalMargin = velo::config::SubtitleVerticalMarginFromDirection(config->subtitlePositionPreset, clampedDirection);
}

std::wstring SubtitleVerticalDirectionLabel(const std::wstring_view languageCode) {
    if (languageCode == L"en-US" || languageCode == L"en") {
        return L"Vertical";
    }
    return L"\u5782\u76f4\u65b9\u5411";
}

void ApplyCanonicalSubtitleHorizontalOffset(velo::config::AppConfig* config) {
    if (config == nullptr) {
        return;
    }
    config->subtitleHorizontalOffset = 0;
    config->subtitleOffsetLeft = 0;
    config->subtitleOffsetRight = 0;
}

int TimeoutMsToSlider(const int milliseconds) {
    return milliseconds / 1000;
}

int SliderToTimeoutMs(const int position) {
    return position * 1000;
}

int ParseEditValue(HWND edit, const int fallback, const int minValue, const int maxValue, bool* valid = nullptr) {
    wchar_t text[16] = {};
    GetWindowTextW(edit, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    const std::wstring value = text;
    if (value.empty()) {
        if (valid != nullptr) {
            *valid = false;
        }
        return fallback;
    }
    try {
        const int parsed = std::stoi(value);
        if (valid != nullptr) {
            *valid = true;
        }
        return std::clamp(parsed, minValue, maxValue);
    } catch (...) {
        if (valid != nullptr) {
            *valid = false;
        }
        return fallback;
    }
}

std::wstring MouseActionIdFromIndex(const int index) {
    switch (index) {
        case 1:
            return L"toggle_pause";
        case 2:
            return L"play_next";
        case 3:
            return L"none";
        default:
            return L"fullscreen";
    }
}

int MouseActionIndex(const std::wstring& actionId) {
    if (actionId == L"toggle_pause") {
        return 1;
    }
    if (actionId == L"play_next") {
        return 2;
    }
    if (actionId == L"show_info" || actionId == L"none") {
        return 3;
    }
    return 0;
}

velo::config::EndOfPlaybackAction EndActionFromIndex(const int index) {
    switch (index) {
        case 1:
            return velo::config::EndOfPlaybackAction::Replay;
        case 2:
            return velo::config::EndOfPlaybackAction::Stop;
        case 3:
            return velo::config::EndOfPlaybackAction::CloseWindow;
        default:
            return velo::config::EndOfPlaybackAction::PlayNext;
    }
}

int EndActionIndex(const velo::config::EndOfPlaybackAction action) {
    switch (action) {
        case velo::config::EndOfPlaybackAction::Replay:
            return 1;
        case velo::config::EndOfPlaybackAction::Stop:
            return 2;
        case velo::config::EndOfPlaybackAction::CloseWindow:
            return 3;
        case velo::config::EndOfPlaybackAction::PlayNext:
        default:
            return 0;
    }
}

velo::config::RepeatMode RepeatModeFromIndex(const int index) {
    switch (index) {
        case 1:
            return velo::config::RepeatMode::One;
        case 2:
            return velo::config::RepeatMode::All;
        default:
            return velo::config::RepeatMode::Off;
    }
}

int RepeatModeIndex(const velo::config::RepeatMode repeatMode) {
    switch (repeatMode) {
        case velo::config::RepeatMode::One:
            return 1;
        case velo::config::RepeatMode::All:
            return 2;
        case velo::config::RepeatMode::Off:
        default:
            return 0;
    }
}

std::wstring SubtitleFontButtonText(const velo::config::AppConfig& config) {
    std::wstring text = config.subtitleFont.empty()
                            ? velo::localization::Text(config.languageCode, velo::localization::TextId::SettingsChooseFont)
                            : config.subtitleFont;
    text += L" ";
    text += std::to_wstring(std::max(12, config.subtitleFontSize));
    text += L"pt";
    if (config.subtitleBold) {
        text += L" B";
    }
    if (config.subtitleItalic) {
        text += L" I";
    }
    if (config.subtitleUnderline) {
        text += L" U";
    }
    if (config.subtitleStrikeOut) {
        text += L" S";
    }
    return text;
}

std::wstring SubtitlePositionIdFromIndex(const int index) {
    switch (index) {
        case 1:
            return L"middle";
        case 2:
            return L"bottom";
        default:
            return L"top";
    }
}

int SubtitlePositionIndex(const std::wstring& preset) {
    if (preset == L"middle") {
        return 1;
    }
    if (preset == L"bottom") {
        return 2;
    }
    return 0;
}

std::wstring SubtitleEncodingValueFromIndex(const int index) {
    switch (index) {
        case 1:
            return L"utf8";
        case 2:
            return L"big5";
        case 3:
            return L"gb18030";
        case 4:
            return L"shift_jis";
        case 5:
            return L"windows-1252";
        default:
            return L"auto";
    }
}

int SubtitleEncodingIndex(const std::wstring& value) {
    if (value == L"utf8" || value == L"utf-8") {
        return 1;
    }
    if (value == L"big5") {
        return 2;
    }
    if (value == L"gb18030") {
        return 3;
    }
    if (value == L"shift_jis" || value == L"shift-jis") {
        return 4;
    }
    if (value == L"windows-1252") {
        return 5;
    }
    return 0;
}

std::wstring AspectRatioValueFromIndex(const int index) {
    switch (index) {
        case 1:
            return L"16:9";
        case 2:
            return L"4:3";
        case 3:
            return L"21:9";
        case 4:
            return L"1:1";
        default:
            return L"default";
    }
}

int AspectRatioIndex(const std::wstring& value) {
    if (value == L"16:9") {
        return 1;
    }
    if (value == L"4:3") {
        return 2;
    }
    if (value == L"21:9") {
        return 3;
    }
    if (value == L"1:1") {
        return 4;
    }
    return 0;
}

int RotateDegreesFromIndex(const int index) {
    switch (index) {
        case 1:
            return 90;
        case 2:
            return 180;
        case 3:
            return 270;
        default:
            return 0;
    }
}

int RotateIndex(const int degrees) {
    switch (((degrees % 360) + 360) % 360) {
        case 90:
            return 1;
        case 180:
            return 2;
        case 270:
            return 3;
        default:
            return 0;
    }
}

std::wstring EqualizerValueFromIndex(const int index) {
    switch (index) {
        case 1:
            return L"voice";
        case 2:
            return L"cinema";
        case 3:
            return L"music";
        default:
            return L"off";
    }
}

int EqualizerIndex(const std::wstring& value) {
    if (value == L"voice") {
        return 1;
    }
    if (value == L"cinema") {
        return 2;
    }
    if (value == L"music") {
        return 3;
    }
    return 0;
}

std::wstring ColorButtonText(const std::wstring_view languageCode, const wchar_t* label, const std::wstring& color) {
    struct NamedColor {
        COLORREF value;
        velo::localization::TextId textId;
    };
    constexpr NamedColor kNamedColors[] = {
        {RGB(255, 255, 255), velo::localization::TextId::SettingsColorWhite},
        {RGB(0, 0, 0), velo::localization::TextId::SettingsColorBlack},
        {RGB(255, 0, 0), velo::localization::TextId::SettingsColorRed},
        {RGB(255, 128, 0), velo::localization::TextId::SettingsColorOrange},
        {RGB(255, 255, 0), velo::localization::TextId::SettingsColorYellow},
        {RGB(0, 176, 80), velo::localization::TextId::SettingsColorGreen},
        {RGB(0, 112, 192), velo::localization::TextId::SettingsColorBlue},
        {RGB(112, 48, 160), velo::localization::TextId::SettingsColorPurple},
        {RGB(128, 128, 128), velo::localization::TextId::SettingsColorGray},
        {RGB(255, 192, 203), velo::localization::TextId::SettingsColorPink},
    };

    const COLORREF parsed = velo::common::TryParseRgbColor(color).value_or(RGB(255, 255, 255));
    const auto distance = [&](const COLORREF target) {
        const int dr = static_cast<int>(GetRValue(parsed)) - static_cast<int>(GetRValue(target));
        const int dg = static_cast<int>(GetGValue(parsed)) - static_cast<int>(GetGValue(target));
        const int db = static_cast<int>(GetBValue(parsed)) - static_cast<int>(GetBValue(target));
        return dr * dr + dg * dg + db * db;
    };
    const auto closest = std::min_element(std::begin(kNamedColors), std::end(kNamedColors), [&](const NamedColor& left, const NamedColor& right) {
        return distance(left.value) < distance(right.value);
    });
    return std::wstring(label) + L" " +
           (closest != std::end(kNamedColors)
                ? velo::localization::Text(languageCode, closest->textId)
                : velo::localization::Text(languageCode, velo::localization::TextId::SettingsCustomColor));
}

constexpr wchar_t kCheckboxStateProp[] = L"NativPlayerSettingsCheckboxState";

void SetCheckboxState(HWND checkbox, const bool checked) {
    if (checkbox == nullptr) {
        return;
    }
    SetPropW(checkbox, kCheckboxStateProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(checked ? BST_CHECKED : BST_UNCHECKED)));
    SendMessageW(checkbox, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool GetCheckboxState(HWND checkbox) {
    if (checkbox == nullptr) {
        return false;
    }
    const auto stored = reinterpret_cast<INT_PTR>(GetPropW(checkbox, kCheckboxStateProp));
    if (stored == BST_CHECKED || stored == BST_UNCHECKED) {
        return stored == BST_CHECKED;
    }
    return SendMessageW(checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool IsSubtitleOffsetEditControl(HWND hwnd, const std::span<const HWND> edits) {
    return std::find(edits.begin(), edits.end(), hwnd) != edits.end();
}

LRESULT CALLBACK SettingsOffsetEditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData) {
    static_cast<void>(refData);
    if (message == WM_MOUSEWHEEL) {
        HWND root = GetAncestor(hwnd, GA_ROOT);
        if (root != nullptr) {
            return SendMessageW(root, message, wParam, lParam);
        }
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

std::vector<std::pair<std::wstring, std::wstring>> EnumerateAudioOutputs() {
    std::vector<std::pair<std::wstring, std::wstring>> outputs;
    const bool useOverride = shared_testing_state::g_audioOutputOverride.has_value();
    if (useOverride) {
        outputs = *shared_testing_state::g_audioOutputOverride;
        std::erase_if(outputs, [](const auto& output) { return output.first.empty() || output.second.empty(); });
        if (outputs.empty()) {
            outputs.emplace_back(velo::localization::Text(velo::localization::AppLanguage::EnUs, velo::localization::TextId::SettingsAudioDefault), L"auto");
        }
        return outputs;
    }
    outputs.emplace_back(velo::localization::Text(velo::localization::AppLanguage::ZhTw, velo::localization::TextId::SettingsAudioDefault), L"auto");
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDeviceCollection* collection = nullptr;
    if (!useOverride &&
        SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator))) &&
        enumerator != nullptr &&
        SUCCEEDED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection)) &&
        collection != nullptr) {
        UINT count = 0;
        collection->GetCount(&count);
        for (UINT index = 0; index < count; ++index) {
            IMMDevice* device = nullptr;
            if (FAILED(collection->Item(index, &device)) || device == nullptr) {
                continue;
            }
            std::wstring deviceName = velo::localization::Text(velo::localization::AppLanguage::ZhTw, velo::localization::TextId::SettingsAudioSpeakerPrefix) +
                                      std::to_wstring(index + 1);
            IPropertyStore* propertyStore = nullptr;
            PROPVARIANT friendlyName;
            PropVariantInit(&friendlyName);
            if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &propertyStore)) && propertyStore != nullptr) {
                if (SUCCEEDED(propertyStore->GetValue(kDeviceFriendlyNameKey, &friendlyName)) && friendlyName.vt == VT_LPWSTR &&
                    friendlyName.pwszVal != nullptr && friendlyName.pwszVal[0] != L'\0') {
                    deviceName = friendlyName.pwszVal;
                }
                PropVariantClear(&friendlyName);
                propertyStore->Release();
            }
            LPWSTR deviceId = nullptr;
            if (SUCCEEDED(device->GetId(&deviceId)) && deviceId != nullptr) {
                outputs.emplace_back(
                    velo::localization::Text(velo::localization::AppLanguage::ZhTw, velo::localization::TextId::SettingsAudioOutputPrefix) +
                        std::to_wstring(index + 1),
                    std::wstring(L"wasapi/") + deviceId);
                outputs.back().first = deviceName;
                CoTaskMemFree(deviceId);
            }
            device->Release();
        }
    }
    if (collection != nullptr) {
        collection->Release();
    }
    if (enumerator != nullptr) {
        enumerator->Release();
    }
    if (SUCCEEDED(init)) {
        CoUninitialize();
    }
    std::erase_if(outputs, [](const auto& output) { return output.first.empty() || output.second.empty(); });
    if (outputs.empty()) {
        outputs.emplace_back(velo::localization::Text(velo::localization::AppLanguage::EnUs, velo::localization::TextId::SettingsAudioDefault), L"auto");
    }
    return outputs;
}

int NavPageFromControlId(const UINT controlId) {
    switch (controlId) {
        case kControlNavPlayback:
            return 0;
        case kControlNavSubtitle:
            return 1;
        case kControlNavAudio:
            return 2;
        case kControlNavShortcuts:
            return 3;
        case kControlNavAdvanced:
            return 4;
        default:
            return 0;
    }
}

}  // namespace


#define playbackPage_ playbackPageControls_.page
#define subtitlePage_ subtitlePageControls_.page
#define audioPage_ audioPageControls_.page
#define shortcutsPage_ shortcutsPageControls_.page
#define advancedPage_ advancedPageControls_.page
#define autoplayNextCheckbox_ playbackPageControls_.autoplayNextCheckbox
#define rememberPlaybackCheckbox_ playbackPageControls_.rememberPlaybackCheckbox
#define preservePauseCheckbox_ playbackPageControls_.preservePauseCheckbox
#define seekPreviewCheckbox_ playbackPageControls_.seekPreviewCheckbox
#define hideDelayLabel_ playbackPageControls_.hideDelayLabel
#define hideDelayBar_ playbackPageControls_.hideDelayBar
#define seekStepLabel_ playbackPageControls_.seekStepLabel
#define seekStepBar_ playbackPageControls_.seekStepBar
#define repeatModeLabel_ playbackPageControls_.repeatModeLabel
#define repeatModeCombo_ playbackPageControls_.repeatModeCombo
#define autoSubtitleCheckbox_ subtitlePageControls_.autoSubtitleCheckbox
#define endActionLabel_ playbackPageControls_.endActionLabel
#define endActionCombo_ playbackPageControls_.endActionCombo
#define subtitleFontLabel_ subtitlePageControls_.subtitleFontLabel
#define subtitleFontButton_ subtitlePageControls_.subtitleFontButton
#define subtitleSizeLabel_ subtitlePageControls_.subtitleSizeLabel
#define subtitleSizeBar_ subtitlePageControls_.subtitleSizeBar
#define subtitleDelayLabel_ subtitlePageControls_.subtitleDelayLabel
#define subtitleDelayBar_ subtitlePageControls_.subtitleDelayBar
#define subtitleTextColorButton_ subtitlePageControls_.subtitleTextColorButton
#define subtitleBorderColorButton_ subtitlePageControls_.subtitleBorderColorButton
#define subtitleShadowColorButton_ subtitlePageControls_.subtitleShadowColorButton
#define subtitleBackgroundCheckbox_ subtitlePageControls_.subtitleBackgroundCheckbox
#define subtitleBackgroundColorButton_ subtitlePageControls_.subtitleBackgroundColorButton
#define subtitleBackgroundOpacityLabel_ subtitlePageControls_.subtitleBackgroundOpacityLabel
#define subtitleBackgroundOpacityBar_ subtitlePageControls_.subtitleBackgroundOpacityBar
#define subtitlePositionLabel_ subtitlePageControls_.subtitlePositionLabel
#define subtitlePositionCombo_ subtitlePageControls_.subtitlePositionCombo
#define subtitleOffsetUpLabel_ subtitlePageControls_.subtitleOffsetUpLabel
#define subtitleOffsetUpEdit_ subtitlePageControls_.subtitleOffsetUpEdit
#define subtitleOffsetDownLabel_ subtitlePageControls_.subtitleOffsetDownLabel
#define subtitleOffsetDownEdit_ subtitlePageControls_.subtitleOffsetDownEdit
#define subtitleOffsetLeftLabel_ subtitlePageControls_.subtitleOffsetLeftLabel
#define subtitleOffsetLeftEdit_ subtitlePageControls_.subtitleOffsetLeftEdit
#define subtitleOffsetRightLabel_ subtitlePageControls_.subtitleOffsetRightLabel
#define subtitleOffsetRightEdit_ subtitlePageControls_.subtitleOffsetRightEdit
#define subtitleBorderSizeLabel_ subtitlePageControls_.subtitleBorderSizeLabel
#define subtitleBorderSizeBar_ subtitlePageControls_.subtitleBorderSizeBar
#define subtitleShadowDepthLabel_ subtitlePageControls_.subtitleShadowDepthLabel
#define subtitleShadowDepthBar_ subtitlePageControls_.subtitleShadowDepthBar
#define subtitleMarginLabel_ subtitlePageControls_.subtitleMarginLabel
#define subtitleMarginBar_ subtitlePageControls_.subtitleMarginBar
#define subtitleEncodingLabel_ subtitlePageControls_.subtitleEncodingLabel
#define subtitleEncodingCombo_ subtitlePageControls_.subtitleEncodingCombo
#define rememberVolumeCheckbox_ audioPageControls_.rememberVolumeCheckbox
#define startupVolumeLabel_ audioPageControls_.startupVolumeLabel
#define startupVolumeBar_ audioPageControls_.startupVolumeBar
#define volumeStepLabel_ audioPageControls_.volumeStepLabel
#define volumeStepBar_ audioPageControls_.volumeStepBar
#define wheelVolumeStepLabel_ audioPageControls_.wheelVolumeStepLabel
#define wheelVolumeStepBar_ audioPageControls_.wheelVolumeStepBar
#define audioDelayLabel_ audioPageControls_.audioDelayLabel
#define audioDelayBar_ audioPageControls_.audioDelayBar
#define audioOutputLabel_ audioPageControls_.audioOutputLabel
#define audioOutputCombo_ audioPageControls_.audioOutputCombo
#define doubleClickActionLabel_ shortcutsPageControls_.doubleClickActionLabel
#define doubleClickActionCombo_ shortcutsPageControls_.doubleClickActionCombo
#define middleClickActionLabel_ shortcutsPageControls_.middleClickActionLabel
#define middleClickActionCombo_ shortcutsPageControls_.middleClickActionCombo
#define shortcutWarningLabel_ shortcutsPageControls_.shortcutWarningLabel
#define hwdecLabel_ advancedPageControls_.hwdecLabel
#define hwdecCombo_ advancedPageControls_.hwdecCombo
#define languageLabel_ advancedPageControls_.languageLabel
#define languageCombo_ advancedPageControls_.languageCombo
#define debugInfoCheckbox_ advancedPageControls_.debugInfoCheckbox
#define aspectRatioLabel_ advancedPageControls_.aspectRatioLabel
#define aspectRatioCombo_ advancedPageControls_.aspectRatioCombo
#define rotateLabel_ advancedPageControls_.rotateLabel
#define rotateCombo_ advancedPageControls_.rotateCombo
#define mirrorVideoCheckbox_ advancedPageControls_.mirrorVideoCheckbox
#define deinterlaceCheckbox_ advancedPageControls_.deinterlaceCheckbox
#define sharpenLabel_ advancedPageControls_.sharpenLabel
#define sharpenBar_ advancedPageControls_.sharpenBar
#define denoiseLabel_ advancedPageControls_.denoiseLabel
#define denoiseBar_ advancedPageControls_.denoiseBar
#define equalizerLabel_ advancedPageControls_.equalizerLabel
#define equalizerCombo_ advancedPageControls_.equalizerCombo
#define advancedHintLabel_ advancedPageControls_.advancedHintLabel
#define screenshotFormatLabel_ advancedPageControls_.screenshotFormatLabel
#define screenshotFormatCombo_ advancedPageControls_.screenshotFormatCombo
#define screenshotQualityLabel_ advancedPageControls_.screenshotQualityLabel
#define screenshotQualityBar_ advancedPageControls_.screenshotQualityBar
#define networkTimeoutLabel_ advancedPageControls_.networkTimeoutLabel
#define networkTimeoutBar_ advancedPageControls_.networkTimeoutBar
#define reconnectLabel_ advancedPageControls_.reconnectLabel
#define reconnectBar_ advancedPageControls_.reconnectBar
#define importButton_ footerControls_.importButton
#define exportButton_ footerControls_.exportButton
#define resetButton_ footerControls_.resetButton
#define okButton_ footerControls_.okButton
#define cancelButton_ footerControls_.cancelButton

}  // namespace velo::ui
