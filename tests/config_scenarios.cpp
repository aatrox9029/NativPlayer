#include "tests/scenario_runner_internal.h"

#include "config/config_service_internal.h"
#include "config/subtitle_position_utils.h"

namespace velo::tests {

void TestCommandLine(ScenarioResult& result) {
    wchar_t arg0[] = L"nativplayer.exe";
    wchar_t arg1[] = L"--no-single-instance";
    wchar_t arg2[] = L"sample.mp4";
    wchar_t* argv[] = {arg0, arg1, arg2};
    const auto options = velo::app::ParseCommandLine(3, argv);
    Expect(options.noSingleInstance, "command line parses no-single-instance", result);
    Expect(options.filesToOpen.size() == 1 && options.filesToOpen.front() == L"sample.mp4",
           "command line preserves file path", result);
}

void TestConfigRoundTrip(ScenarioResult& result) {
    const auto root = std::filesystem::temp_directory_path() / "nativplayer-tests" / "config";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    velo::config::ConfigService service(root);
    velo::config::AppConfig config;
    config.windowWidth = 1440;
    config.windowHeight = 900;
    config.windowPosX = 100;
    config.windowPosY = 140;
    config.hasSavedWindowPlacement = true;
    config.volume = 62.5;
    config.startupVolume = 48.0;
    config.rememberVolume = false;
    config.languageCode = L"en-US";
    config.muted = true;
    config.autoplayNextFile = false;
    config.preservePauseOnOpen = false;
    config.autoLoadSubtitle = false;
    config.rememberPlaybackPosition = true;
    config.exactSeek = false;
    config.controlsHideDelayMs = 2400;
    config.seekStepMode = velo::config::SeekStepMode::Percent;
    config.seekStepSeconds = 12;
    config.seekStepPercent = 20;
    config.showSeekPreview = false;
    config.volumeStep = 7;
    config.wheelVolumeStep = 9;
    config.audioDelaySeconds = 0.35;
    config.subtitleDelaySeconds = -0.15;
    config.subtitleFont = L"Test Font";
    config.subtitleBackgroundEnabled = true;
    config.subtitleBackgroundColor = L"202020B3";
    config.subtitleFontSize = 36;
    config.subtitleBorderSize = 9;
    config.subtitlePositionPreset = L"middle";
    config.subtitleOffsetUp = 0;
    config.subtitleOffsetDown = 6;
    config.subtitleEncoding = L"utf8";
    config.doubleClickAction = L"none";
    config.middleClickAction = L"toggle_pause";
    config.repeatMode = velo::config::RepeatMode::All;
    config.showDebugInfo = true;
    config.showDebugOverlay = true;
    config.preferredAspectRatio = L"16:9";
    config.videoRotateDegrees = 90;
    config.mirrorVideo = true;
    config.deinterlaceEnabled = true;
    config.sharpenStrength = 8;
    config.denoiseStrength = 5;
    config.equalizerProfile = L"voice";
    config.lastFile = L"movie.mkv";
    config.lastOpenUrl = L"https://example.com/live.m3u8";
    config.recentItems = {{L"movie.mkv", true, 111}, {L"https://example.com/live.m3u8", false, 112}};
    config.recentFiles = {L"movie.mkv", L"https://example.com/live.m3u8"};
    config.resumeEntries = {velo::config::ResumeEntry{L"movie.mkv", 91.0}};
    config.historyEntries = {{L"movie.mkv", L"Movie", 88.0, false, 333}};
    config.bookmarkEntries = {{L"movie.mkv", L"Intro", 12.0}};
    config.keyBindings = {{L"toggle_pause", VK_RETURN}};
    service.ScheduleSave(config);
    service.Flush();

    velo::config::ConfigService loaded(root);
    loaded.Load();
    Expect(loaded.Current().windowWidth == 1440, "config saves width", result);
    Expect(loaded.Current().muted, "config saves mute flag", result);
    Expect(loaded.Current().lastFile == L"movie.mkv", "config saves last file", result);
    Expect(loaded.Current().hasSavedWindowPlacement, "config saves window placement flag", result);
    Expect(loaded.Current().windowPosX == 100 && loaded.Current().windowPosY == 140,
           "config saves window position", result);
    Expect(loaded.Current().startupVolume == 48.0, "config saves startup volume", result);
    Expect(loaded.Current().languageCode == L"en-US", "config saves language code", result);
    Expect(!loaded.Current().rememberVolume, "config saves remember volume toggle", result);
    Expect(!loaded.Current().autoplayNextFile, "config saves autoplay next toggle", result);
    Expect(!loaded.Current().preservePauseOnOpen, "config saves preserve pause toggle", result);
    Expect(!loaded.Current().autoLoadSubtitle, "config saves auto subtitle toggle", result);
    Expect(!loaded.Current().exactSeek, "config saves exact seek toggle", result);
    Expect(loaded.Current().controlsHideDelayMs == 2400, "config saves controls hide delay", result);
    Expect(loaded.Current().seekStepMode == velo::config::SeekStepMode::Percent, "config saves seek step mode", result);
    Expect(loaded.Current().seekStepSeconds == 12, "config saves seek step seconds", result);
    Expect(loaded.Current().seekStepPercent == 20, "config saves seek step percent", result);
    Expect(!loaded.Current().showSeekPreview, "config saves seek preview toggle", result);
    Expect(loaded.Current().volumeStep == 7, "config saves keyboard volume step", result);
    Expect(loaded.Current().wheelVolumeStep == 9, "config saves wheel volume step", result);
    Expect(loaded.Current().repeatMode == velo::config::RepeatMode::All, "config saves repeat mode", result);
    Expect(loaded.Current().showDebugInfo, "config saves debug info toggle", result);
    Expect(std::abs(loaded.Current().audioDelaySeconds - 0.35) < 0.001, "config saves audio delay", result);
    Expect(std::abs(loaded.Current().subtitleDelaySeconds + 0.15) < 0.001, "config saves subtitle delay", result);
    Expect(loaded.Current().subtitleFont == L"Test Font", "config saves subtitle font", result);
    Expect(loaded.Current().subtitleBackgroundEnabled, "config saves subtitle background toggle", result);
    Expect(loaded.Current().subtitleBackgroundColor == L"202020B3", "config saves subtitle background color", result);
    Expect(loaded.Current().subtitleBorderSize == 9, "config saves subtitle border size", result);
    Expect(loaded.Current().subtitlePositionPreset == L"middle", "config saves subtitle position preset", result);
    Expect(loaded.Current().subtitleOffsetUp == 0 && loaded.Current().subtitleOffsetDown == 6, "config saves subtitle vertical offsets", result);
    Expect(loaded.Current().subtitleHorizontalOffset == 0, "config clears subtitle horizontal offset", result);
    Expect(loaded.Current().subtitleOffsetLeft == 0 && loaded.Current().subtitleOffsetRight == 0,
           "config clears legacy subtitle horizontal offsets", result);
    Expect(loaded.Current().subtitleEncoding == L"utf8", "config saves subtitle encoding", result);
    Expect(loaded.Current().preferredAspectRatio == L"16:9", "config saves aspect ratio", result);
    Expect(loaded.Current().videoRotateDegrees == 90, "config saves rotation", result);
    Expect(loaded.Current().mirrorVideo, "config saves mirror toggle", result);
    Expect(loaded.Current().deinterlaceEnabled, "config saves deinterlace toggle", result);
    Expect(loaded.Current().recentItems.empty() && loaded.Current().recentFiles.empty(), "config clears recent items", result);
    Expect(loaded.Current().historyEntries.size() == 1, "config saves history entries", result);
    Expect(loaded.Current().bookmarkEntries.size() == 1, "config saves bookmarks", result);
    Expect(loaded.Current().resumeEntries.size() == 1 && loaded.Current().resumeEntries[0].positionSeconds == 91.0,
           "config saves resume entry", result);
    Expect(!loaded.Current().keyBindings.empty() && loaded.Current().keyBindings.front().virtualKey == VK_RETURN,
           "config saves custom key bindings", result);
}

void TestConfigImportExportAndFallback(ScenarioResult& result) {
    const auto root = std::filesystem::temp_directory_path() / "nativplayer-tests" / "config-import";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);

    velo::config::AppConfig exported = velo::config::DefaultAppConfig();
    exported.seekStepMode = velo::config::SeekStepMode::Percent;
    exported.seekStepSeconds = 15;
    exported.seekStepPercent = 18;
    exported.showSeekPreview = false;
    exported.repeatMode = velo::config::RepeatMode::One;
    exported.subtitleFont = L"PortableFont";
    exported.languageCode = L"zh-CN";
    exported.keyBindings = {{L"play_next", VK_F11}};
    const auto exportPath = root / "portable.ini";
    Expect(velo::config::ExportConfigSnapshot(exported, exportPath), "config export snapshot succeeds", result);

    velo::config::AppConfig imported;
    Expect(velo::config::ImportConfigSnapshot(exportPath, imported), "config import snapshot succeeds", result);
    Expect(imported.seekStepMode == velo::config::SeekStepMode::Percent, "config import restores seek step mode", result);
    Expect(imported.seekStepSeconds == 15, "config import restores seek step", result);
    Expect(imported.seekStepPercent == 18, "config import restores seek step percent", result);
    Expect(!imported.showSeekPreview, "config import restores seek preview toggle", result);
    Expect(imported.repeatMode == velo::config::RepeatMode::One, "config import restores repeat mode", result);
    Expect(imported.subtitleFont == L"PortableFont", "config import restores subtitle font", result);
    Expect(imported.languageCode == L"zh-CN", "config import restores language code", result);

    const auto appRoot = root / "service";
    std::filesystem::create_directories(appRoot, error);
    velo::config::ConfigService service(appRoot);
    exported.volumeStep = 11;
    service.ScheduleSave(exported);
    service.Flush();

    std::ofstream(appRoot / "settings.ini", std::ios::trunc) << "broken_line_without_equals";
    velo::config::ConfigService recovered(appRoot);
    recovered.Load();
    Expect(recovered.Current().volumeStep == 11, "config service falls back to backup copy", result);

    std::ofstream(appRoot / "settings.ini", std::ios::trunc)
        << "config_version=2\nrecent_item_path_0=movie.mkv\nrecent_item_opened_0=-1\n";
    velo::config::ConfigService recoveredNegativeTimestamp(appRoot);
    recoveredNegativeTimestamp.Load();
    Expect(recoveredNegativeTimestamp.Current().volumeStep == 11,
           "config service rejects negative timestamps and falls back", result);

    const auto malformedBindingPath = root / "bad-binding.ini";
    std::ofstream(malformedBindingPath, std::ios::trunc) << "config_version=2\nbinding_toggle_pause=-1\n";
    velo::config::AppConfig malformedBinding;
    Expect(!velo::config::ImportConfigSnapshot(malformedBindingPath, malformedBinding),
           "config import rejects negative key bindings", result);

    const auto malformedColorPath = root / "bad-color.ini";
    std::ofstream(malformedColorPath, std::ios::trunc)
        << "config_version=2\nsubtitle_text_color=#GGGGGG\nsubtitle_border_color=12\nsubtitle_shadow_color=\nsubtitle_background_color=123456\n";
    velo::config::AppConfig malformedColorConfig;
    Expect(velo::config::ImportConfigSnapshot(malformedColorPath, malformedColorConfig),
           "config import tolerates malformed subtitle colors", result);
    Expect(malformedColorConfig.subtitleTextColor == L"FFFFFFFF", "config import normalizes malformed text color", result);
    Expect(malformedColorConfig.subtitleBorderColor == L"101010FF", "config import normalizes malformed border color", result);
    Expect(malformedColorConfig.subtitleShadowColor == L"101010AA", "config import normalizes malformed shadow color", result);
    Expect(malformedColorConfig.subtitleBackgroundColor == L"123456FF", "config import expands 6-digit background color", result);

    const auto legacyHorizontalPath = root / "legacy-horizontal.ini";
    std::ofstream(legacyHorizontalPath, std::ios::trunc)
        << "config_version=2\nsubtitle_offset_left=9\nsubtitle_offset_right=2\n";
    velo::config::AppConfig legacyHorizontalConfig;
    Expect(velo::config::ImportConfigSnapshot(legacyHorizontalPath, legacyHorizontalConfig),
           "config import accepts legacy subtitle horizontal offsets", result);
    Expect(legacyHorizontalConfig.subtitleHorizontalOffset == 0,
           "config import clears legacy subtitle horizontal offsets", result);
    Expect(legacyHorizontalConfig.subtitleOffsetLeft == 0 && legacyHorizontalConfig.subtitleOffsetRight == 0,
           "config import clears legacy subtitle offset fields", result);
}

void TestHexColorHelpers(ScenarioResult& result) {
    Expect(velo::common::NormalizeRgbaHexColor(L"abcdef12") == L"ABCDEF12", "hex color helper uppercases rgba colors", result);
    Expect(velo::common::NormalizeRgbaHexColor(L"123456") == L"123456FF", "hex color helper appends alpha for rgb colors", result);
    Expect(velo::common::NormalizeRgbaHexColor(L"oops", L"101010FF") == L"101010FF", "hex color helper falls back for invalid colors", result);
    const auto rgba = velo::common::TryParseRgbaColor(L"804020AA");
    Expect(rgba.has_value() && rgba->red == 0x80 && rgba->green == 0x40 && rgba->blue == 0x20 && rgba->alpha == 0xAA,
           "hex color helper parses rgba channels", result);
    Expect(velo::common::FormatRgbaHexColor(velo::common::RgbaColor{0x12, 0x34, 0x56, 0x78}) == L"12345678",
           "hex color helper formats rgba colors", result);
    const auto parsed = velo::common::TryParseRgbColor(L"804020AA");
    Expect(parsed.has_value() && GetRValue(*parsed) == 0x80 && GetGValue(*parsed) == 0x40 && GetBValue(*parsed) == 0x20,
           "hex color helper parses rgb channels", result);
}

void TestLocalizationHelpers(ScenarioResult& result) {
    Expect(velo::localization::ResolveLanguage(L"zh-TW") == velo::localization::AppLanguage::ZhTw, "localization resolves zh-TW", result);
    Expect(velo::localization::ResolveLanguage(L"zh-CN") == velo::localization::AppLanguage::ZhCn, "localization resolves zh-CN", result);
    Expect(velo::localization::ResolveLanguage(L"en-US") == velo::localization::AppLanguage::EnUs, "localization resolves en-US", result);
    Expect(velo::localization::LanguageCode(velo::localization::AppLanguage::ZhCn) == L"zh-CN", "localization returns canonical code", result);
    Expect(!velo::localization::Text(velo::localization::AppLanguage::EnUs, velo::localization::TextId::SettingsTitle).empty(),
           "localization returns translated text", result);
    Expect(velo::localization::Text(velo::localization::AppLanguage::ZhTw, velo::localization::TextId::OpeningPrefix) == L"正在開啟: ",
           "localization returns zh-TW opening prefix", result);
    Expect(velo::localization::Text(velo::localization::AppLanguage::ZhCn, velo::localization::TextId::OpeningPrefix) == L"正在打开: ",
           "localization returns zh-CN opening prefix", result);
    Expect(velo::localization::Text(velo::localization::AppLanguage::EnUs, velo::localization::TextId::OpeningPrefix) == L"Opening: ",
           "localization returns en-US opening prefix", result);
    Expect(!velo::localization::ActionLabel(velo::localization::AppLanguage::ZhTw, L"toggle_pause").empty(),
           "localization returns shortcut action labels", result);
}

void TestSubtitleColorFormatting(ScenarioResult& result) {
    Expect(velo::playback::SubtitleColorToMpv(L"FFFFFFFF") == "#FFFFFFFF", "subtitle color helper preserves opaque white", result);
    Expect(velo::playback::SubtitleColorToMpv(L"FF0000FF") == "#FFFF0000", "subtitle color helper converts red to mpv argb format", result);
    Expect(velo::playback::SubtitleColorToMpv(L"102030B3") == "#B3102030", "subtitle color helper converts rgba payload to mpv argb format", result);
}

void TestSubtitleVerticalPositionNormalization(ScenarioResult& result) {
    velo::config::AppConfig preservedMargin = velo::config::DefaultAppConfig();
    preservedMargin.subtitlePositionPreset = L"bottom";
    preservedMargin.subtitleVerticalMargin = 92;
    preservedMargin.subtitleOffsetUp = 0;
    preservedMargin.subtitleOffsetDown = 0;
    velo::config::NormalizeConfig(preservedMargin);
    Expect(preservedMargin.subtitleVerticalMargin == 92, "config normalization preserves explicit subtitle margin when no legacy offsets are set", result);
    Expect(preservedMargin.subtitleOffsetUp == 4 && preservedMargin.subtitleOffsetDown == 0,
           "config normalization backfills subtitle direction from the preserved margin", result);

    velo::config::AppConfig legacyOffsets = velo::config::DefaultAppConfig();
    legacyOffsets.subtitlePositionPreset = L"bottom";
    legacyOffsets.subtitleVerticalMargin = 96;
    legacyOffsets.subtitleOffsetUp = 5;
    legacyOffsets.subtitleOffsetDown = 0;
    velo::config::NormalizeConfig(legacyOffsets);
    Expect(legacyOffsets.subtitleVerticalMargin == 91, "config normalization converts positive subtitle direction into upward movement", result);

    Expect(velo::config::SubtitleVerticalDirectionFromMargin(L"bottom", 96) == 0,
           "subtitle direction helper treats the default bottom margin as neutral", result);
    Expect(velo::config::SubtitleVerticalMarginFromDirection(L"bottom", -2) == 98,
           "subtitle direction helper moves bottom subtitles downward for negative values", result);
}

void RunConfigScenarios(ScenarioResult& result) {
    TestCommandLine(result);
    TestConfigRoundTrip(result);
    TestConfigImportExportAndFallback(result);
    TestHexColorHelpers(result);
    TestLocalizationHelpers(result);
    TestSubtitleColorFormatting(result);
    TestSubtitleVerticalPositionNormalization(result);
}

}  // namespace velo::tests
