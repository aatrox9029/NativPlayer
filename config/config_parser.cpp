#include "config/config_service_internal.h"

namespace velo::config {

bool ParseConfigFile(const std::filesystem::path& filePath, AppConfig& loadedConfig) {
    std::wifstream input(filePath);
    if (!input.is_open()) {
        return false;
    }

    AppConfig loaded = DefaultAppConfig();
    loaded.recentFiles.clear();
    loaded.recentItems.clear();
    loaded.resumeEntries.clear();
    loaded.historyEntries.clear();
    loaded.bookmarkEntries.clear();
    loaded.keyBindings.clear();
    bool sawSubtitleHorizontalOffset = false;

    bool parseError = false;
    std::wstring line;
    while (std::getline(input, line)) {
        line = Trim(line);
        if (line.empty() || line.starts_with(L"#") || line.starts_with(L";")) {
            continue;
        }

        const auto equals = line.find(L'=');
        if (equals == std::wstring::npos) {
            parseError = true;
            continue;
        }

        const std::wstring key = Trim(line.substr(0, equals));
        const std::wstring value = Trim(line.substr(equals + 1));
        if (key == L"config_version") {
            parseError = !TryParseInt(value).has_value();
        } else if (key == L"window_width") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.windowWidth = *parsed;
            }
        } else if (key == L"window_height") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.windowHeight = *parsed;
            }
        } else if (key == L"window_pos_x") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.windowPosX = *parsed;
                loaded.hasSavedWindowPlacement = true;
            }
        } else if (key == L"window_pos_y") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.windowPosY = *parsed;
                loaded.hasSavedWindowPlacement = true;
            }
        } else if (key == L"start_maximized") {
            loaded.startMaximized = ParseBool(value);
        } else if (key == L"volume") {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.volume = *parsed;
            }
        } else if (key == L"startup_volume") {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.startupVolume = *parsed;
            }
        } else if (key == L"remember_volume") {
            loaded.rememberVolume = ParseBool(value);
        } else if (key == L"muted") {
            loaded.muted = ParseBool(value);
        } else if (key == ini::kLanguageCode) {
            loaded.languageCode = value;
        } else if (key == L"hwdec_policy") {
            loaded.hwdecPolicy = value;
        } else if (key == ini::kAudioOutputDevice) {
            loaded.audioOutputDevice = value;
        } else if (key == L"last_open_url") {
            loaded.lastOpenUrl = value;
        } else if (key == ini::kAutoplayNextFile) {
            loaded.autoplayNextFile = ParseBool(value);
        } else if (key == ini::kPreservePauseOnOpen) {
            loaded.preservePauseOnOpen = ParseBool(value);
        } else if (key == ini::kAutoLoadSubtitle) {
            loaded.autoLoadSubtitle = ParseBool(value);
        } else if (key == ini::kRememberPlaybackPosition) {
            loaded.rememberPlaybackPosition = ParseBool(value);
        } else if (key == L"exact_seek") {
            loaded.exactSeek = ParseBool(value);
        } else if (key == L"show_playlist_sidebar") {
            loaded.showPlaylistSidebar = ParseBool(value);
        } else if (key == L"show_debug_overlay") {
            loaded.showDebugOverlay = ParseBool(value);
        } else if (key == ini::kControlsHideDelayMs) {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.controlsHideDelayMs = *parsed;
            }
        } else if (key == ini::kSeekStepMode) {
            loaded.seekStepMode = ParseSeekStepMode(value);
        } else if (key == ini::kSeekStepSeconds) {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.seekStepSeconds = *parsed;
            }
        } else if (key == ini::kSeekStepPercent) {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.seekStepPercent = *parsed;
            }
        } else if (key == ini::kShowSeekPreview) {
            loaded.showSeekPreview = ParseBool(value);
        } else if (key == ini::kVolumeStep) {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.volumeStep = *parsed;
            }
        } else if (key == ini::kWheelVolumeStep) {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.wheelVolumeStep = *parsed;
            }
        } else if (key == L"audio_delay_seconds") {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.audioDelaySeconds = *parsed;
            }
        } else if (key == L"subtitle_delay_seconds") {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.subtitleDelaySeconds = *parsed;
            }
        } else if (key == L"subtitle_font") {
            loaded.subtitleFont = value;
        } else if (key == L"subtitle_text_color") {
            loaded.subtitleTextColor = value;
        } else if (key == L"subtitle_border_color") {
            loaded.subtitleBorderColor = value;
        } else if (key == ini::kSubtitleShadowColor) {
            loaded.subtitleShadowColor = value;
        } else if (key == ini::kSubtitleBackgroundEnabled) {
            loaded.subtitleBackgroundEnabled = ParseBool(value);
        } else if (key == ini::kSubtitleBackgroundColor) {
            loaded.subtitleBackgroundColor = value;
        } else if (key == ini::kSubtitlePositionPreset) {
            loaded.subtitlePositionPreset = value;
        } else if (key == ini::kSubtitleOffsetUp) {
            const auto parsed = TryParseInt(value);
            if (parsed.has_value()) {
                loaded.subtitleOffsetUp = *parsed;
            }
        } else if (key == ini::kSubtitleOffsetDown) {
            const auto parsed = TryParseInt(value);
            if (parsed.has_value()) {
                loaded.subtitleOffsetDown = *parsed;
            }
        } else if (key == ini::kSubtitleHorizontalOffset) {
            const auto parsed = TryParseInt(value);
            if (parsed.has_value()) {
                loaded.subtitleHorizontalOffset = *parsed;
                sawSubtitleHorizontalOffset = true;
            }
        } else if (key == ini::kSubtitleOffsetLeft) {
            const auto parsed = TryParseInt(value);
            if (parsed.has_value()) {
                loaded.subtitleOffsetLeft = *parsed;
            }
        } else if (key == ini::kSubtitleOffsetRight) {
            const auto parsed = TryParseInt(value);
            if (parsed.has_value()) {
                loaded.subtitleOffsetRight = *parsed;
            }
        } else if (key == L"subtitle_font_size") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.subtitleFontSize = *parsed;
            }
        } else if (key == L"subtitle_bold") {
            loaded.subtitleBold = ParseBool(value);
        } else if (key == L"subtitle_italic") {
            loaded.subtitleItalic = ParseBool(value);
        } else if (key == L"subtitle_underline") {
            loaded.subtitleUnderline = ParseBool(value);
        } else if (key == L"subtitle_strike_out") {
            loaded.subtitleStrikeOut = ParseBool(value);
        } else if (key == L"subtitle_border_size") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.subtitleBorderSize = *parsed;
            }
        } else if (key == L"subtitle_shadow_depth") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.subtitleShadowDepth = *parsed;
            }
        } else if (key == L"subtitle_vertical_margin") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.subtitleVerticalMargin = *parsed;
            }
        } else if (key == L"subtitle_encoding") {
            loaded.subtitleEncoding = value;
        } else if (key == L"double_click_action") {
            loaded.doubleClickAction = value;
        } else if (key == L"middle_click_action") {
            loaded.middleClickAction = value;
        } else if (key == L"repeat_mode") {
            loaded.repeatMode = ParseRepeatMode(value);
        } else if (key == ini::kEndOfPlaybackAction) {
            loaded.endOfPlaybackAction = ParseEndOfPlaybackAction(value);
        } else if (key == L"show_debug_info") {
            loaded.showDebugInfo = ParseBool(value);
        } else if (key == L"preferred_aspect_ratio") {
            loaded.preferredAspectRatio = value;
        } else if (key == L"video_rotate_degrees") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.videoRotateDegrees = *parsed;
            }
        } else if (key == L"mirror_video") {
            loaded.mirrorVideo = ParseBool(value);
        } else if (key == L"deinterlace_enabled") {
            loaded.deinterlaceEnabled = ParseBool(value);
        } else if (key == L"sharpen_strength") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.sharpenStrength = *parsed;
            }
        } else if (key == L"denoise_strength") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.denoiseStrength = *parsed;
            }
        } else if (key == L"equalizer_profile") {
            loaded.equalizerProfile = value;
        } else if (key == ini::kScreenshotFormat) {
            loaded.screenshotFormat = value;
        } else if (key == ini::kScreenshotQuality) {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.screenshotQuality = *parsed;
            }
        } else if (key == L"network_timeout_ms") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.networkTimeoutMs = *parsed;
            }
        } else if (key == L"stream_reconnect_count") {
            const auto parsed = TryParseInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.streamReconnectCount = *parsed;
            }
        } else if (key == L"last_file") {
            loaded.lastFile = value;
        } else if (key.starts_with(L"recent_file_") && !value.empty()) {
            loaded.recentFiles.push_back(value);
        } else if (key.starts_with(L"recent_item_path_") && !value.empty()) {
            loaded.recentItems.push_back({.path = value});
        } else if (key.starts_with(L"recent_item_pinned_") && !loaded.recentItems.empty()) {
            loaded.recentItems.back().pinned = ParseBool(value);
        } else if (key.starts_with(L"recent_item_opened_") && !loaded.recentItems.empty()) {
            const auto parsed = TryParseUint64(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.recentItems.back().openedAtUnixSeconds = *parsed;
            }
        } else if (key.starts_with(L"resume_path_") && !value.empty()) {
            loaded.resumeEntries.push_back({.path = value, .positionSeconds = 0.0});
        } else if (key.starts_with(L"resume_pos_") && !loaded.resumeEntries.empty()) {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.resumeEntries.back().positionSeconds = *parsed;
            }
        } else if (key.starts_with(L"history_path_") && !value.empty()) {
            loaded.historyEntries.push_back({.path = value});
        } else if (key.starts_with(L"history_title_") && !loaded.historyEntries.empty()) {
            loaded.historyEntries.back().title = value;
        } else if (key.starts_with(L"history_pos_") && !loaded.historyEntries.empty()) {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.historyEntries.back().positionSeconds = *parsed;
            }
        } else if (key.starts_with(L"history_is_url_") && !loaded.historyEntries.empty()) {
            loaded.historyEntries.back().isUrl = ParseBool(value);
        } else if (key.starts_with(L"history_opened_") && !loaded.historyEntries.empty()) {
            const auto parsed = TryParseUint64(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.historyEntries.back().openedAtUnixSeconds = *parsed;
            }
        } else if (key.starts_with(L"bookmark_path_") && !value.empty()) {
            loaded.bookmarkEntries.push_back({.path = value});
        } else if (key.starts_with(L"bookmark_label_") && !loaded.bookmarkEntries.empty()) {
            loaded.bookmarkEntries.back().label = value;
        } else if (key.starts_with(L"bookmark_pos_") && !loaded.bookmarkEntries.empty()) {
            const auto parsed = TryParseDouble(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                loaded.bookmarkEntries.back().positionSeconds = *parsed;
            }
        } else if (key.starts_with(L"binding_")) {
            const auto parsed = TryParseUnsignedInt(value);
            if (!parsed.has_value()) {
                parseError = true;
            } else {
                SetKeyBinding(loaded.keyBindings, key.substr(8), *parsed);
            }
        }

        if (!sawSubtitleHorizontalOffset) {
            loaded.subtitleHorizontalOffset = loaded.subtitleOffsetRight - loaded.subtitleOffsetLeft;
        }
    }

    NormalizeConfig(loaded);
    loadedConfig = std::move(loaded);
    return !parseError;
}


}  // namespace velo::config

