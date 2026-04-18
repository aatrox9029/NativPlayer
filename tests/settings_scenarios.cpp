#include "tests/scenario_runner_internal.h"

#include <cstdint>
#include <commctrl.h>

namespace velo::tests {

namespace {

std::vector<uint32_t> RenderSettingsButtonPixels(HWND dialog, HWND button, const std::wstring_view text) {
    std::vector<uint32_t> pixels;
    if (dialog == nullptr || button == nullptr) {
        return pixels;
    }

    RECT client{};
    GetClientRect(button, &client);
    const int width = std::max(1, static_cast<int>(client.right - client.left));
    const int height = std::max(1, static_cast<int>(client.bottom - client.top));

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) {
        return pixels;
    }

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap == nullptr || bits == nullptr) {
        if (bitmap != nullptr) {
            DeleteObject(bitmap);
        }
        DeleteDC(dc);
        return pixels;
    }

    HGDIOBJ previousBitmap = SelectObject(dc, bitmap);
    std::fill_n(static_cast<uint32_t*>(bits), static_cast<size_t>(width) * static_cast<size_t>(height), 0U);
    const std::wstring ownedText{text};
    SetWindowTextW(button, ownedText.c_str());

    DRAWITEMSTRUCT drawItem{};
    drawItem.CtlType = ODT_BUTTON;
    drawItem.CtlID = static_cast<UINT>(GetDlgCtrlID(button));
    drawItem.itemAction = ODA_DRAWENTIRE;
    drawItem.itemState = IsWindowEnabled(button) ? 0U : ODS_DISABLED;
    drawItem.hwndItem = button;
    drawItem.hDC = dc;
    drawItem.rcItem = {0, 0, width, height};

    SendMessageW(dialog, WM_DRAWITEM, static_cast<WPARAM>(drawItem.CtlID), reinterpret_cast<LPARAM>(&drawItem));

    const auto* pixelStart = static_cast<const uint32_t*>(bits);
    pixels.assign(pixelStart, pixelStart + (static_cast<size_t>(width) * static_cast<size_t>(height)));

    SelectObject(dc, previousBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return pixels;
}

std::vector<uint32_t> RenderSettingsButtonPixelsAfterRelabel(HWND dialog, HWND button, const std::wstring_view oldText,
                                                             const std::wstring_view newText) {
    std::vector<uint32_t> pixels;
    if (dialog == nullptr || button == nullptr) {
        return pixels;
    }

    RECT client{};
    GetClientRect(button, &client);
    const int width = std::max(1, static_cast<int>(client.right - client.left));
    const int height = std::max(1, static_cast<int>(client.bottom - client.top));

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) {
        return pixels;
    }

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap == nullptr || bits == nullptr) {
        if (bitmap != nullptr) {
            DeleteObject(bitmap);
        }
        DeleteDC(dc);
        return pixels;
    }

    HGDIOBJ previousBitmap = SelectObject(dc, bitmap);
    std::fill_n(static_cast<uint32_t*>(bits), static_cast<size_t>(width) * static_cast<size_t>(height), 0U);

    DRAWITEMSTRUCT drawItem{};
    drawItem.CtlType = ODT_BUTTON;
    drawItem.CtlID = static_cast<UINT>(GetDlgCtrlID(button));
    drawItem.itemAction = ODA_DRAWENTIRE;
    drawItem.itemState = IsWindowEnabled(button) ? 0U : ODS_DISABLED;
    drawItem.hwndItem = button;
    drawItem.hDC = dc;
    drawItem.rcItem = {0, 0, width, height};

    const std::wstring ownedOldText{oldText};
    SetWindowTextW(button, ownedOldText.c_str());
    SendMessageW(dialog, WM_DRAWITEM, static_cast<WPARAM>(drawItem.CtlID), reinterpret_cast<LPARAM>(&drawItem));

    const std::wstring ownedNewText{newText};
    SetWindowTextW(button, ownedNewText.c_str());
    SendMessageW(dialog, WM_DRAWITEM, static_cast<WPARAM>(drawItem.CtlID), reinterpret_cast<LPARAM>(&drawItem));

    const auto* pixelStart = static_cast<const uint32_t*>(bits);
    pixels.assign(pixelStart, pixelStart + (static_cast<size_t>(width) * static_cast<size_t>(height)));

    SelectObject(dc, previousBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return pixels;
}

}  // namespace

void TestSettingsDialogStateAudit(ScenarioResult& result) {
    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    config.languageCode = L"en-US";
    config.autoplayNextFile = false;
    config.rememberPlaybackPosition = false;
    config.preservePauseOnOpen = false;
    config.showSeekPreview = false;
    config.controlsHideDelayMs = 3600;
    config.seekStepMode = velo::config::SeekStepMode::Percent;
    config.seekStepSeconds = 17;
    config.seekStepPercent = 22;
    config.repeatMode = velo::config::RepeatMode::All;
    config.endOfPlaybackAction = velo::config::EndOfPlaybackAction::Stop;
    config.autoLoadSubtitle = false;
    config.subtitleDelaySeconds = 1.2;
    config.subtitleFont = L"Audit Font";
    config.subtitleTextColor = L"ABCDEF12";
    config.subtitleBorderColor = L"123456FF";
    config.subtitleShadowColor = L"876543AA";
    config.subtitleBackgroundEnabled = true;
    config.subtitleBackgroundColor = L"102030B3";
    config.subtitleFontSize = 33;
    config.subtitleBorderSize = 4;
    config.subtitleShadowDepth = 3;
    config.subtitleVerticalMargin = 71;
    config.subtitlePositionPreset = L"top";
    config.subtitleOffsetUp = 6;
    config.subtitleOffsetDown = 2;
    config.subtitleEncoding = L"big5";
    config.rememberVolume = false;
    config.startupVolume = 55.0;
    config.volumeStep = 11;
    config.wheelVolumeStep = 13;
    config.audioDelaySeconds = -0.8;
    config.audioOutputDevice = L"wasapi/test-device";
    config.doubleClickAction = L"none";
    config.middleClickAction = L"play_next";
    config.hwdecPolicy = L"auto-copy";
    config.showDebugInfo = true;
    config.preferredAspectRatio = L"21:9";
    config.videoRotateDegrees = 270;
    config.mirrorVideo = true;
    config.deinterlaceEnabled = true;
    config.sharpenStrength = 9;
    config.denoiseStrength = 6;
    config.equalizerProfile = L"music";
    config.screenshotFormat = L"jpg";
    config.screenshotQuality = 87;
    config.networkTimeoutMs = 14000;
    config.streamReconnectCount = 5;
    velo::platform::win32::SetVirtualKey(config, L"toggle_pause", VK_RETURN);

    const auto state = velo::ui::BuildSettingsDialogState(config);
    Expect(state.languageCode == L"en-US", "settings audit captures language code", result);
    Expect(!state.autoplayNextFile, "settings audit captures autoplay toggle", result);
    Expect(!state.rememberPlaybackPosition, "settings audit captures resume toggle", result);
    Expect(state.seekStepMode == velo::config::SeekStepMode::Percent, "settings audit captures seek step mode", result);
    Expect(state.seekStepPercent == 22, "settings audit captures seek step percent", result);
    Expect(state.repeatMode == velo::config::RepeatMode::All, "settings audit captures repeat mode", result);
    Expect(state.endOfPlaybackAction == velo::config::EndOfPlaybackAction::Stop, "settings audit captures end action", result);
    Expect(std::abs(state.subtitleDelaySeconds - 1.2) < 0.001, "settings audit captures subtitle delay", result);
    Expect(state.subtitleBorderSize == 4, "settings audit captures subtitle border size", result);
    Expect(state.subtitleShadowDepth == 3, "settings audit captures subtitle shadow depth", result);
    Expect(state.subtitleVerticalMargin == 71, "settings audit captures subtitle margin", result);
    Expect(state.subtitleBackgroundEnabled, "settings audit captures subtitle background toggle", result);
    Expect(state.subtitleBackgroundColor == L"102030B3", "settings audit captures subtitle background color", result);
    Expect(state.subtitlePositionPreset == L"top", "settings audit captures subtitle position preset", result);
    Expect(state.subtitleOffsetUp == 6 && state.subtitleOffsetDown == 2, "settings audit captures subtitle vertical offsets", result);
    Expect(state.subtitleHorizontalOffset == 0, "settings audit clears subtitle horizontal offset", result);
    Expect(state.subtitleOffsetLeft == 0 && state.subtitleOffsetRight == 0,
           "settings audit clears legacy subtitle horizontal offsets", result);
    Expect(state.subtitleEncoding == L"big5", "settings audit captures subtitle encoding", result);
    Expect(!state.rememberVolume, "settings audit captures remember volume toggle", result);
    Expect(state.volumeStep == 11, "settings audit captures keyboard volume step", result);
    Expect(state.wheelVolumeStep == 13, "settings audit captures wheel volume step", result);
    Expect(std::abs(state.audioDelaySeconds + 0.8) < 0.001, "settings audit captures audio delay", result);
    Expect(state.audioOutputDevice == L"wasapi/test-device", "settings audit captures audio output", result);
    Expect(state.preferredAspectRatio == L"21:9", "settings audit captures aspect ratio", result);
    Expect(state.videoRotateDegrees == 270, "settings audit captures rotation", result);
    Expect(state.mirrorVideo, "settings audit captures mirror toggle", result);
    Expect(state.deinterlaceEnabled, "settings audit captures deinterlace toggle", result);
    Expect(state.sharpenStrength == 9, "settings audit captures sharpen strength", result);
    Expect(state.denoiseStrength == 6, "settings audit captures denoise strength", result);
    Expect(state.equalizerProfile == L"music", "settings audit captures equalizer profile", result);
    Expect(state.screenshotFormat == L"jpg", "settings audit captures screenshot format", result);
    Expect(state.screenshotQuality == 87, "settings audit captures screenshot quality", result);
    Expect(state.networkTimeoutMs == 14000, "settings audit captures network timeout", result);
    Expect(state.streamReconnectCount == 5, "settings audit captures reconnect count", result);
    Expect(state.shortcutBindingValues.size() == velo::platform::win32::InputBindingDefinitions().size(),
           "settings audit captures shortcut bindings", result);

    const auto rebuilt = velo::ui::ApplySettingsDialogState(velo::config::DefaultAppConfig(), state);
    Expect(rebuilt.languageCode == L"en-US", "settings audit rebuilds language code", result);
    Expect(rebuilt.seekStepMode == velo::config::SeekStepMode::Percent, "settings audit rebuilds seek step mode", result);
    Expect(rebuilt.seekStepPercent == 22, "settings audit rebuilds seek step percent", result);
    Expect(rebuilt.repeatMode == velo::config::RepeatMode::All, "settings audit rebuilds repeat mode", result);
    Expect(std::abs(rebuilt.subtitleDelaySeconds - 1.2) < 0.001, "settings audit rebuilds subtitle delay", result);
    Expect(rebuilt.subtitleBackgroundEnabled, "settings audit rebuilds subtitle background toggle", result);
    Expect(rebuilt.subtitleBackgroundColor == L"102030B3", "settings audit rebuilds subtitle background color", result);
    Expect(rebuilt.subtitlePositionPreset == L"top", "settings audit rebuilds subtitle position preset", result);
    Expect(rebuilt.subtitleOffsetUp == 6 && rebuilt.subtitleOffsetDown == 2, "settings audit rebuilds subtitle vertical offsets", result);
    Expect(rebuilt.subtitleHorizontalOffset == 0, "settings audit rebuilds cleared subtitle horizontal offset", result);
    Expect(rebuilt.subtitleOffsetLeft == 0 && rebuilt.subtitleOffsetRight == 0,
           "settings audit rebuilds cleared legacy subtitle horizontal offsets", result);
    Expect(rebuilt.subtitleEncoding == L"big5", "settings audit rebuilds subtitle encoding", result);
    Expect(rebuilt.startupVolume == 55.0, "settings audit rebuilds startup volume", result);
    Expect(rebuilt.volumeStep == 11 && rebuilt.wheelVolumeStep == 13, "settings audit rebuilds volume steps", result);
    Expect(std::abs(rebuilt.audioDelaySeconds + 0.8) < 0.001, "settings audit rebuilds audio delay", result);
    Expect(rebuilt.audioOutputDevice == L"wasapi/test-device", "settings audit rebuilds audio output", result);
    Expect(rebuilt.preferredAspectRatio == L"21:9", "settings audit rebuilds aspect ratio", result);
    Expect(rebuilt.videoRotateDegrees == 270, "settings audit rebuilds rotation", result);
    Expect(rebuilt.mirrorVideo && rebuilt.deinterlaceEnabled, "settings audit rebuilds video toggles", result);
    Expect(rebuilt.sharpenStrength == 9 && rebuilt.denoiseStrength == 6, "settings audit rebuilds filter strengths", result);
    Expect(rebuilt.equalizerProfile == L"music", "settings audit rebuilds equalizer profile", result);
    Expect(rebuilt.networkTimeoutMs == 14000 && rebuilt.streamReconnectCount == 5, "settings audit rebuilds network settings", result);
}


void TestSettingsPageSurfaceForwarding(ScenarioResult& result) {
    constexpr wchar_t kProbeClassName[] = L"NativPlayerForwardingProbe";
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = ForwardingProbeProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = kProbeClassName;
        RegisterClassW(&windowClass);
        classRegistered = true;
    }

    ForwardingProbeState probe{};
    HWND owner = CreateWindowExW(0, kProbeClassName, L"", WS_OVERLAPPED, 0, 0, 10, 10, nullptr, nullptr, GetModuleHandleW(nullptr), &probe);
    HWND page = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 0, 0, 10, 10, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
    HBRUSH pageBrush = CreateSolidBrush(RGB(1, 2, 3));
    HBRUSH controlBrush = CreateSolidBrush(RGB(4, 5, 6));
    velo::ui::SettingsPageSurfaceTheme theme{};
    theme.owner = owner;
    theme.pageBrush = pageBrush;
    theme.controlBrush = controlBrush;
    velo::ui::AttachSettingsPageSurface(page, &theme);

    const LRESULT commandResult = SendMessageW(page, WM_COMMAND, MAKEWPARAM(6200, BN_CLICKED), reinterpret_cast<LPARAM>(page));
    DRAWITEMSTRUCT drawItem{};
    drawItem.CtlType = ODT_BUTTON;
    drawItem.CtlID = 6200;
    drawItem.hwndItem = page;
    const LRESULT drawResult = SendMessageW(page, WM_DRAWITEM, 0, reinterpret_cast<LPARAM>(&drawItem));
    MEASUREITEMSTRUCT measureItem{};
    measureItem.CtlType = ODT_BUTTON;
    const LRESULT measureResult = SendMessageW(page, WM_MEASUREITEM, 0, reinterpret_cast<LPARAM>(&measureItem));

    Expect(commandResult == 101 && probe.commandCount == 1, "settings page surface forwards command messages", result);
    Expect(drawResult == 102 && probe.drawItemCount == 1, "settings page surface forwards draw item messages", result);
    Expect(measureResult == 103 && probe.measureItemCount == 1, "settings page surface forwards measure item messages", result);

    DeleteObject(pageBrush);
    DeleteObject(controlBrush);
    DestroyWindow(page);
    DestroyWindow(owner);
}

void TestSettingsDialogSmokeCloseWithoutInitPreview(ScenarioResult& result) {
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<int> previewCount = 0;
    std::atomic<bool> closeIssued = false;
    std::atomic<bool> previewBeforeClose = false;
    std::atomic<bool> dialogSeen = false;

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }
        closeIssued = true;
        velo::ui::SettingsDialogCancelForTesting(dialog);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, velo::config::DefaultAppConfig(),
                                          [&](const velo::config::AppConfig&) {
                                              if (!closeIssued.load()) {
                                                  previewBeforeClose = true;
                                              }
                                              ++previewCount;
                                          });
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    Expect(dialogSeen.load(), "settings dialog smoke opens window", result);
    Expect(!updated.has_value(), "settings dialog smoke close returns no update", result);
    Expect(!previewBeforeClose.load(), "settings dialog smoke suppresses preview during initialization", result);
    Expect(previewCount.load() <= 1, "settings dialog smoke only previews on cancel rollback", result);
    DestroyWindow(owner);
}

void TestSettingsDialogSmokeAcceptAfterUserChange(ScenarioResult& result) {
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<int> previewCount = 0;
    std::atomic<bool> actionIssued = false;
    std::atomic<bool> previewBeforeAction = false;
    std::atomic<bool> dialogSeen = false;

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }
        actionIssued = true;
        if (!velo::ui::SettingsDialogSetSubtitlePositionForTesting(dialog, 1)) {
            actionIssued = false;
        }
        velo::ui::SettingsDialogAcceptForTesting(dialog);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, velo::config::DefaultAppConfig(),
                                          [&](const velo::config::AppConfig&) {
                                              if (!actionIssued.load()) {
                                                  previewBeforeAction = true;
                                              }
                                              ++previewCount;
                                          });
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    Expect(dialogSeen.load(), "settings dialog accept smoke opens window", result);
    Expect(updated.has_value(), "settings dialog accept smoke returns updated config", result);
    Expect(!previewBeforeAction.load(), "settings dialog accept smoke avoids init preview", result);
    Expect(previewCount.load() >= 1, "settings dialog accept smoke previews after user change", result);
    if (updated.has_value()) {
        Expect(updated->subtitlePositionPreset == L"middle", "settings dialog accept smoke applies user change", result);
    }
    DestroyWindow(owner);
}

void TestSettingsDialogAudioOutputFallback(ScenarioResult& result) {
    velo::ui::SetSettingsDialogAudioOutputsOverrideForTesting(std::vector<std::pair<std::wstring, std::wstring>>{});
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<int> comboCount = -1;
    std::atomic<int> comboSelection = -2;
    std::atomic<bool> dialogSeen = false;

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }
        HWND combo = FindDescendantById(dialog, static_cast<int>(velo::ui::SettingsDialogAudioOutputComboIdForTesting()));
        if (combo != nullptr) {
            comboCount = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
            comboSelection = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
        }
        velo::ui::SettingsDialogCancelForTesting(dialog);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, velo::config::DefaultAppConfig());
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    velo::ui::SetSettingsDialogAudioOutputsOverrideForTesting(std::nullopt);
    Expect(dialogSeen.load(), "settings dialog audio fallback opens window", result);
    Expect(!updated.has_value(), "settings dialog audio fallback closes without update", result);
    Expect(comboCount.load() >= 1, "settings dialog audio fallback keeps safe default entry", result);
    Expect(comboSelection.load() == 0, "settings dialog audio fallback selects safe default", result);
    DestroyWindow(owner);
}

void TestSettingsDialogColorPreviewAndCancelRollback(ScenarioResult& result) {
    const std::wstring originalColor = velo::config::DefaultAppConfig().subtitleTextColor;
    velo::ui::SetSettingsDialogColorOverrideForTesting(RGB(0x12, 0x34, 0x56));
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<int> previewCount = 0;
    std::atomic<bool> actionIssued = false;
    std::atomic<bool> dialogSeen = false;
    std::atomic<bool> changedPreviewSeen = false;
    std::atomic<bool> rollbackPreviewSeen = false;

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }
        actionIssued = true;
        if (!velo::ui::SettingsDialogChooseSubtitleTextColorForTesting(dialog)) {
            actionIssued = false;
        }
        PostMessageW(dialog, WM_CLOSE, 0, 0);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, velo::config::DefaultAppConfig(),
                                          [&](const velo::config::AppConfig& previewConfig) {
                                              if (!actionIssued.load()) {
                                                  return;
                                              }
                                              ++previewCount;
                                              if (previewConfig.subtitleTextColor == L"123456FF") {
                                                  changedPreviewSeen = true;
                                              }
                                              if (previewConfig.subtitleTextColor == originalColor) {
                                                  rollbackPreviewSeen = true;
                                              }
                                          });
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    velo::ui::SetSettingsDialogColorOverrideForTesting(std::nullopt);
    const auto initialColor = velo::ui::SettingsDialogLastColorDialogInitialColorForTesting();
    Expect(dialogSeen.load(), "settings dialog color preview opens window", result);
    Expect(!updated.has_value(), "settings dialog color preview cancel returns no update", result);
    Expect(initialColor.has_value() && GetRValue(*initialColor) == 0xFF && GetGValue(*initialColor) == 0xFF && GetBValue(*initialColor) == 0xFF,
           "settings dialog color picker initializes from existing subtitle color", result);
    Expect(changedPreviewSeen.load(), "settings dialog color preview fires after user color change", result);
    Expect(rollbackPreviewSeen.load(), "settings dialog cancel restores original subtitle colors", result);
    Expect(previewCount.load() >= 2, "settings dialog color preview emits change and rollback previews", result);
    DestroyWindow(owner);
}

void TestSettingsDialogFooterButtonsRedrawCleanlyAfterRelabel(ScenarioResult& result) {
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<bool> dialogSeen = false;
    std::atomic<bool> okButtonRepaintedCleanly = false;
    std::atomic<bool> cancelButtonRepaintedCleanly = false;

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }

        HWND okButton = WaitForDescendantById(dialog, static_cast<int>(velo::ui::SettingsDialogOkControlIdForTesting()));
        HWND cancelButton = WaitForDescendantById(dialog, IDCANCEL);
        if (okButton != nullptr) {
            const auto relabeledPixels = RenderSettingsButtonPixelsAfterRelabel(dialog, okButton, L"確定", L"OK");
            const auto cleanPixels = RenderSettingsButtonPixels(dialog, okButton, L"OK");
            okButtonRepaintedCleanly = !relabeledPixels.empty() && relabeledPixels == cleanPixels;
        }
        if (cancelButton != nullptr) {
            const auto relabeledPixels = RenderSettingsButtonPixelsAfterRelabel(dialog, cancelButton, L"取消", L"Cancel");
            const auto cleanPixels = RenderSettingsButtonPixels(dialog, cancelButton, L"Cancel");
            cancelButtonRepaintedCleanly = !relabeledPixels.empty() && relabeledPixels == cleanPixels;
        }
        velo::ui::SettingsDialogCancelForTesting(dialog);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, velo::config::DefaultAppConfig());
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    Expect(dialogSeen.load(), "settings dialog footer redraw test opens window", result);
    Expect(!updated.has_value(), "settings dialog footer redraw test closes without update", result);
    Expect(okButtonRepaintedCleanly.load(), "settings dialog footer OK button clears stale text before redraw", result);
    Expect(cancelButtonRepaintedCleanly.load(), "settings dialog footer Cancel button clears stale text before redraw", result);
    DestroyWindow(owner);
}

void TestSettingsDialogPreservesSubtitleSizeDisplayOnReopen(ScenarioResult& result) {
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<bool> dialogSeen = false;
    std::wstring fontButtonText;

    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    config.subtitleFont = L"Reopen Font";
    config.subtitleFontSize = 30;

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }
        HWND fontButton = WaitForDescendantById(dialog, static_cast<int>(velo::ui::SettingsDialogSubtitleFontButtonControlIdForTesting()));
        if (fontButton != nullptr) {
            wchar_t text[256] = {};
            GetWindowTextW(fontButton, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
            fontButtonText = text;
        }
        velo::ui::SettingsDialogCancelForTesting(dialog);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, config);
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    Expect(dialogSeen.load(), "settings dialog subtitle size reopen test opens window", result);
    Expect(!updated.has_value(), "settings dialog subtitle size reopen test closes without update", result);
    Expect(fontButtonText.find(L"30pt") != std::wstring::npos,
           "settings dialog keeps subtitle font button size text when reopening", result);
    DestroyWindow(owner);
}

void TestSettingsDialogSubtitleBackgroundOpacitySlider(ScenarioResult& result) {
    HWND owner = CreateSettingsDialogTestOwner();
    std::atomic<bool> dialogSeen = false;
    std::atomic<int> initialOpacity = -1;

    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    config.subtitleBackgroundEnabled = true;
    config.subtitleBackgroundColor = L"102030B3";

    velo::ui::SetSettingsDialogAutomationCallbackForTesting([&](HWND dialog) {
        dialogSeen = dialog != nullptr;
        if (dialog == nullptr) {
            return;
        }

        HWND opacityBar =
            WaitForDescendantById(dialog, static_cast<int>(velo::ui::SettingsDialogSubtitleBackgroundOpacityControlIdForTesting()));
        if (opacityBar != nullptr) {
            initialOpacity = static_cast<int>(SendMessageW(opacityBar, TBM_GETPOS, 0, 0));
            SendMessageW(opacityBar, TBM_SETPOS, TRUE, 25);
            SendMessageW(dialog, WM_HSCROLL, MAKEWPARAM(TB_THUMBPOSITION, 25), reinterpret_cast<LPARAM>(opacityBar));
        }
        velo::ui::SettingsDialogAcceptForTesting(dialog);
    });

    velo::ui::SettingsDialog dialog;
    const auto updated = dialog.ShowModal(GetModuleHandleW(nullptr), owner, config);
    velo::ui::SetSettingsDialogAutomationCallbackForTesting({});
    Expect(dialogSeen.load(), "settings dialog background opacity test opens window", result);
    Expect(updated.has_value(), "settings dialog background opacity test returns updated config", result);
    Expect(initialOpacity.load() == 70, "settings dialog derives background opacity slider from subtitle alpha", result);
    if (updated.has_value()) {
        Expect(updated->subtitleBackgroundColor == L"10203040", "settings dialog saves background opacity slider back into subtitle alpha", result);
    }
    DestroyWindow(owner);
}

void TestComboBoxItemTextHandlesLongStrings(ScenarioResult& result) {
    HWND combo = CreateWindowExW(0, L"COMBOBOX", L"", WS_OVERLAPPED | CBS_DROPDOWNLIST | WS_VISIBLE, 0, 0, 320, 160, nullptr, nullptr,
                                 GetModuleHandleW(nullptr), nullptr);
    const std::wstring longLabel(600, L'A');
    const LRESULT index = SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(longLabel.c_str()));
    const std::wstring loaded = velo::ui::GetComboBoxItemText(combo, static_cast<int>(index));
    Expect(loaded == longLabel, "combo box text helper preserves long entries without truncation", result);
    DestroyWindow(combo);
}


void RunSettingsScenarios(ScenarioResult& result) {
    AppendHeadlessTrace("settings:state_audit:start");
    TestSettingsDialogStateAudit(result);
    AppendHeadlessTrace("settings:state_audit:end");
    AppendHeadlessTrace("settings:surface_forwarding:start");
    TestSettingsPageSurfaceForwarding(result);
    AppendHeadlessTrace("settings:surface_forwarding:end");
    AppendHeadlessTrace("settings:smoke_close:start");
    TestSettingsDialogSmokeCloseWithoutInitPreview(result);
    AppendHeadlessTrace("settings:smoke_close:end");
    AppendHeadlessTrace("settings:accept_smoke:start");
    TestSettingsDialogSmokeAcceptAfterUserChange(result);
    AppendHeadlessTrace("settings:accept_smoke:end");
    AppendHeadlessTrace("settings:audio_fallback:start");
    TestSettingsDialogAudioOutputFallback(result);
    AppendHeadlessTrace("settings:audio_fallback:end");
    AppendHeadlessTrace("settings:color_preview:start");
    TestSettingsDialogColorPreviewAndCancelRollback(result);
    AppendHeadlessTrace("settings:color_preview:end");
    AppendHeadlessTrace("settings:footer_redraw:start");
    TestSettingsDialogFooterButtonsRedrawCleanlyAfterRelabel(result);
    AppendHeadlessTrace("settings:footer_redraw:end");
    AppendHeadlessTrace("settings:subtitle_size_reopen:start");
    TestSettingsDialogPreservesSubtitleSizeDisplayOnReopen(result);
    AppendHeadlessTrace("settings:subtitle_size_reopen:end");
    AppendHeadlessTrace("settings:subtitle_background_opacity:start");
    TestSettingsDialogSubtitleBackgroundOpacitySlider(result);
    AppendHeadlessTrace("settings:subtitle_background_opacity:end");
    AppendHeadlessTrace("settings:combo_text:start");
    TestComboBoxItemTextHandlesLongStrings(result);
    AppendHeadlessTrace("settings:combo_text:end");
}

}  // namespace velo::tests

