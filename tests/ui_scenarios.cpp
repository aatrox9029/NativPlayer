#include "tests/scenario_runner_internal.h"

#include "ui/window_placement_utils.h"

namespace velo::tests {

void TestWindowPlacementRestore(ScenarioResult& result) {
    WINDOWPLACEMENT maximizedPlacement{};
    maximizedPlacement.length = sizeof(WINDOWPLACEMENT);
    maximizedPlacement.showCmd = SW_SHOWMAXIMIZED;
    maximizedPlacement.rcNormalPosition = RECT{100, 120, 1700, 1020};

    WINDOWPLACEMENT lastWindowedPlacement{};
    lastWindowedPlacement.length = sizeof(WINDOWPLACEMENT);
    lastWindowedPlacement.showCmd = SW_SHOWNORMAL;
    lastWindowedPlacement.rcNormalPosition = RECT{220, 180, 1420, 860};

    const auto restoredFromLast = velo::ui::ResolveWindowedRestorePlacement(maximizedPlacement, lastWindowedPlacement);
    Expect(restoredFromLast.showCmd == SW_SHOWNORMAL, "windowed restore clears maximized state", result);
    Expect(restoredFromLast.rcNormalPosition.left == 220 && restoredFromLast.rcNormalPosition.top == 180 &&
               restoredFromLast.rcNormalPosition.right == 1420 && restoredFromLast.rcNormalPosition.bottom == 860,
           "windowed restore prefers the last non-maximized bounds", result);

    WINDOWPLACEMENT missingWindowedPlacement{};
    missingWindowedPlacement.length = sizeof(WINDOWPLACEMENT);
    missingWindowedPlacement.showCmd = SW_SHOWMAXIMIZED;

    const auto restoredFromFallback = velo::ui::ResolveWindowedRestorePlacement(maximizedPlacement, missingWindowedPlacement);
    Expect(restoredFromFallback.showCmd == SW_SHOWNORMAL, "windowed restore normalizes fallback show state", result);
    Expect(restoredFromFallback.rcNormalPosition.left == 100 && restoredFromFallback.rcNormalPosition.top == 120 &&
               restoredFromFallback.rcNormalPosition.right == 1700 && restoredFromFallback.rcNormalPosition.bottom == 1020,
           "windowed restore falls back to stored normal bounds when needed", result);

    WINDOWPLACEMENT transientPlacement{};
    transientPlacement.length = sizeof(WINDOWPLACEMENT);
    transientPlacement.showCmd = SW_SHOWNORMAL;
    transientPlacement.rcNormalPosition = RECT{0, 0, 1920, 1040};
    Expect(!velo::ui::ShouldTrackWindowedPlacement(transientPlacement, true, false),
           "windowed tracking ignores transient normal placements while the window is still maximized", result);
}

void TestFriendlyErrors(ScenarioResult& result) {
    const auto missingFile = velo::ui::FriendlyPlaybackError("No such file or directory", L"en-US");
    const auto decoderError = velo::ui::FriendlyPlaybackError("Decoder init failed", L"zh-TW");
    const auto subtitleError = velo::ui::FriendlyPlaybackError("subtitle parse failed", L"zh-CN");
    Expect(!missingFile.empty() && missingFile != L"No such file or directory", "friendly error maps missing file", result);
    Expect(!decoderError.empty() && decoderError != L"Decoder init failed", "friendly error maps decoder failure", result);
    Expect(!subtitleError.empty() && subtitleError != L"subtitle parse failed", "friendly error maps subtitle failure", result);
}

void TestButtonVisuals(ScenarioResult& result) {
    velo::ui::PlayerState pausedState;
    pausedState.isPaused = true;
    const auto playVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::PlayPause, pausedState, false, L"en-US");
    Expect(playVisual.glyph == L"\u25B6", "play button uses solid triangle glyph", result);
    Expect(playVisual.largeGlyph, "play button opts into larger icon size", result);

       const auto openVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::OpenFile, pausedState, false, L"en-US");
       Expect(openVisual.glyph == L"\xD83D\xDCC1", "open button uses folder glyph", result);

       const auto speedVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::Speed, pausedState, false, L"en-US");
       Expect(speedVisual.glyph == L"\u23F1\uFE0F", "speed button uses stopwatch glyph", result);

    velo::ui::PlayerState subtitleOff;
    subtitleOff.subtitleTrackId = -1;
    const auto offVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::SubtitleTrack, subtitleOff, false, L"zh-TW");
               Expect(offVisual.imageAsset == L"subtitle", "subtitle off button uses subtitle asset name", result);
    Expect(offVisual.badge.empty(), "subtitle button hides off text badge", result);
    Expect(offVisual.warning, "subtitle button turns warning color when disabled", result);
    Expect(!offVisual.largeGlyph, "subtitle button keeps original icon size", result);

    velo::ui::PlayerState subtitleOn;
    subtitleOn.subtitleTrackId = 2;
    const auto onVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::SubtitleTrack, subtitleOn, false, L"zh-TW");
       Expect(onVisual.imageAsset == L"subtitle", "subtitle on button uses subtitle asset name", result);
    Expect(onVisual.badge.empty(), "subtitle button hides on text badge", result);
    Expect(onVisual.accent, "subtitle button highlights when enabled", result);

    velo::ui::PlayerState mutedVolume;
    mutedVolume.isMuted = true;
    mutedVolume.volume = 42.0;
    const auto muteVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::Mute, mutedVolume, false, L"zh-TW");
    Expect(!muteVisual.label.empty(), "mute button keeps volume label when muted", result);
    Expect(muteVisual.badge.empty() && !muteVisual.glyph.empty(), "mute button keeps muted icon glyph when muted", result);

       const auto settingsVisual = velo::ui::ResolveButtonVisual(velo::ui::ButtonKind::Settings, mutedVolume, false, L"zh-TW");
       Expect(settingsVisual.glyph == L"\xD83D\xDD27", "settings button uses wrench glyph", result);
       Expect(settingsVisual.largeGlyph, "settings button icon is enlarged", result);
}

void TestLanguageMenuLabels(ScenarioResult& result) {
    const auto zhTw = velo::localization::LanguageMenuLabel(velo::localization::AppLanguage::ZhTw);
    const auto zhCn = velo::localization::LanguageMenuLabel(velo::localization::AppLanguage::ZhCn);
    const auto enUs = velo::localization::LanguageMenuLabel(velo::localization::AppLanguage::EnUs);
    Expect(zhTw.find(L"\u7E41") != std::wstring::npos, "traditional Chinese label includes language symbol", result);
    Expect(zhCn.find(L"\u7B80") != std::wstring::npos, "simplified Chinese label includes language symbol", result);
    Expect(enUs.find(L"EN") != std::wstring::npos, "English label includes language symbol", result);
}

void TestMainWindowLayout(ScenarioResult& result) {
    const RECT client{0, 0, 1280, 720};
    const auto windowedVisible = velo::ui::ComputeMainWindowLayout(client, false, false, true);
    Expect(windowedVisible.titleBarRect.bottom == windowedVisible.titleBarRect.top + windowedVisible.captionBarHeight &&
               windowedVisible.titleTextRect.bottom <= windowedVisible.titleBarRect.bottom,
           "windowed layout keeps title content inside the top overlay caption bar", result);
    Expect(windowedVisible.currentTimeLeft + windowedVisible.currentTimeWidth + windowedVisible.seekGap == windowedVisible.seekLeft,
           "windowed layout places seek bar after current time label", result);
    Expect(windowedVisible.durationLeft == windowedVisible.seekLeft + windowedVisible.seekWidth + windowedVisible.seekGap,
           "windowed layout places duration label after seek bar", result);
    Expect(windowedVisible.seekTop <= windowedVisible.timeRowTop + windowedVisible.timeLabelHeight,
           "windowed layout vertically aligns seek bar with time labels", result);
    Expect(windowedVisible.buttonsY >= windowedVisible.panelTop + windowedVisible.panelPaddingTop + windowedVisible.seekRowHeight +
                                      windowedVisible.panelRowGap,
           "windowed layout keeps button row below seek row", result);
    Expect(windowedVisible.buttonsY + windowedVisible.controlHeight <= windowedVisible.panelTop + windowedVisible.panelHeight -
                                                                            windowedVisible.panelPaddingBottom,
           "windowed layout keeps buttons inside control panel", result);

    const auto hiddenControls = velo::ui::ComputeMainWindowLayout(client, false, false, false);
       Expect(hiddenControls.videoRect.top == 0 && hiddenControls.videoRect.bottom == hiddenControls.height,
           "windowed layout keeps the video under the persistent top overlay", result);

    const auto fullscreenVisible = velo::ui::ComputeMainWindowLayout(client, false, true, true);
    Expect(fullscreenVisible.videoRect.top == 0,
           "fullscreen layout overlays the top strip on the video", result);
    Expect(fullscreenVisible.videoRect.bottom == fullscreenVisible.height,
           "fullscreen layout keeps the video fully extended beneath the control panel", result);
    Expect(fullscreenVisible.durationLeft == fullscreenVisible.seekLeft + fullscreenVisible.seekWidth + fullscreenVisible.seekGap,
           "fullscreen layout keeps time row aligned", result);
    Expect(fullscreenVisible.panelHeight * 10 <= fullscreenVisible.height,
           "fullscreen layout keeps the control panel within ten percent of window height", result);
    Expect(fullscreenVisible.buttonsY >= fullscreenVisible.panelTop + fullscreenVisible.panelPaddingTop + fullscreenVisible.seekRowHeight +
                                        fullscreenVisible.panelRowGap,
           "fullscreen layout keeps button row below seek row", result);
    Expect(fullscreenVisible.buttonsY > fullscreenVisible.panelTop + fullscreenVisible.panelPaddingTop,
           "fullscreen layout keeps the control rows lowered inside the panel", result);

    const auto fullscreenHidden = velo::ui::ComputeMainWindowLayout(client, false, true, false);
    Expect(fullscreenHidden.titleBarRect.bottom == 0,
           "fullscreen hidden layout removes the top overlay caption bar", result);
    Expect(fullscreenHidden.videoRect.top == 0 && fullscreenHidden.videoRect.bottom == fullscreenHidden.height,
           "fullscreen hidden layout keeps the video fully extended", result);
    Expect(velo::ui::ComputeFullscreenControlsShowActivationTop(fullscreenVisible) >
               velo::ui::ComputeFullscreenControlsHideActivationTop(fullscreenVisible),
           "fullscreen control activation uses hysteresis between show and hide zones", result);
}

void TestInputProfiles(ScenarioResult& result) {
    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    velo::platform::win32::SetVirtualKey(config, L"toggle_pause", VK_RETURN);
    velo::platform::win32::SetVirtualKey(config, L"play_next", VK_RETURN);
    Expect(velo::platform::win32::ResolveVirtualKey(config, L"toggle_pause") == VK_RETURN,
           "input profile resolves overridden binding", result);
    const auto conflicts = velo::platform::win32::FindBindingConflicts(config);
    Expect(!conflicts.empty(), "input profile detects binding conflicts", result);
    Expect(velo::platform::win32::VirtualKeyDisplayName(VK_RETURN) == L"Enter", "input profile formats key name", result);
}

void TestShortcutDisplay(ScenarioResult& result) {
    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    config.languageCode = L"zh-TW";
    velo::platform::win32::SetVirtualKey(config, L"toggle_pause", VK_RETURN);
    config.doubleClickAction = L"none";
    config.middleClickAction = L"play_next";

    const auto pauseText = velo::ui::ShortcutDisplayName(config, L"toggle_pause");
    Expect(pauseText == L"Enter", "shortcut display uses custom pause binding", result);

    const auto helpText = velo::ui::BuildShortcutHelpText(config);
    Expect(helpText.find(L"Enter") != std::wstring::npos, "shortcut help uses custom keyboard binding", result);
    Expect(helpText.find(L"雙擊  無") != std::wstring::npos, "shortcut help uses configured double click action", result);
    Expect(helpText.find(L"中鍵  播放下一個") != std::wstring::npos, "shortcut help uses configured middle click action", result);
}

void TestShortcutCaptureRules(ScenarioResult& result) {
    Expect(!velo::ui::IsAllowedShortcutCaptureKey(VK_LEFT), "shortcut capture rejects left key", result);
    Expect(!velo::ui::IsAllowedShortcutCaptureKey(VK_RIGHT), "shortcut capture rejects right key", result);
    Expect(velo::ui::IsAllowedShortcutCaptureKey(VK_UP), "shortcut capture allows other keyboard keys", result);
    Expect(velo::ui::IsAllowedShortcutCaptureKey('K'), "shortcut capture allows letter keys", result);
    Expect(velo::ui::ResolveShortcutCaptureKey(VK_PROCESSKEY, 0x001E0001) == 'A',
           "shortcut capture resolves process key to concrete virtual key", result);
    Expect(!velo::ui::IsAllowedShortcutCapturePointer(VK_LBUTTON), "shortcut capture rejects left mouse button", result);
    Expect(!velo::ui::IsAllowedShortcutCapturePointer(VK_RBUTTON), "shortcut capture rejects right mouse button", result);
    Expect(velo::ui::IsAllowedShortcutCapturePointer(VK_MBUTTON), "shortcut capture allows middle mouse button", result);
    Expect(velo::ui::IsAllowedShortcutCapturePointer(VK_XBUTTON1), "shortcut capture allows X1 mouse button", result);
    Expect(velo::ui::IsAllowedShortcutCapturePointer(VK_XBUTTON2), "shortcut capture allows X2 mouse button", result);
}


void RunUiScenarios(ScenarioResult& result) {
    TestFriendlyErrors(result);
    TestButtonVisuals(result);
    TestMainWindowLayout(result);
    TestWindowPlacementRestore(result);
    TestInputProfiles(result);
    TestShortcutDisplay(result);
    TestShortcutCaptureRules(result);
    TestLanguageMenuLabels(result);
}

}  // namespace velo::tests

