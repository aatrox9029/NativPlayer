#include "config/config_service_internal.h"

namespace velo::config {

bool WriteConfigFileToPath(const AppConfig& config, const std::filesystem::path& destinationPath) {
    std::wofstream output(destinationPath, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << L"config_version=" << kCurrentConfigVersion << L'\n';
    output << L"window_width=" << config.windowWidth << L'\n';
    output << L"window_height=" << config.windowHeight << L'\n';
    if (config.hasSavedWindowPlacement) {
        output << L"window_pos_x=" << config.windowPosX << L'\n';
        output << L"window_pos_y=" << config.windowPosY << L'\n';
    }
    output << L"start_maximized=" << BoolToString(config.startMaximized) << L'\n';
    output << L"volume=" << config.volume << L'\n';
    output << L"startup_volume=" << config.startupVolume << L'\n';
    output << L"remember_volume=" << BoolToString(config.rememberVolume) << L'\n';
    output << L"muted=" << BoolToString(config.muted) << L'\n';
    output << ini::kLanguageCode << L"=" << config.languageCode << L'\n';
    output << L"hwdec_policy=" << config.hwdecPolicy << L'\n';
    output << ini::kAudioOutputDevice << L"=" << config.audioOutputDevice << L'\n';
    output << L"last_open_url=" << config.lastOpenUrl << L'\n';
    output << ini::kAutoplayNextFile << L"=" << BoolToString(config.autoplayNextFile) << L'\n';
    output << ini::kPreservePauseOnOpen << L"=" << BoolToString(config.preservePauseOnOpen) << L'\n';
    output << ini::kAutoLoadSubtitle << L"=" << BoolToString(config.autoLoadSubtitle) << L'\n';
    output << ini::kRememberPlaybackPosition << L"=" << BoolToString(config.rememberPlaybackPosition) << L'\n';
    output << L"exact_seek=" << BoolToString(config.exactSeek) << L'\n';
    output << L"show_playlist_sidebar=" << BoolToString(config.showPlaylistSidebar) << L'\n';
    output << L"show_debug_overlay=" << BoolToString(config.showDebugOverlay) << L'\n';
    output << ini::kControlsHideDelayMs << L"=" << config.controlsHideDelayMs << L'\n';
    output << ini::kSeekStepMode << L"=" << SeekStepModeToString(config.seekStepMode) << L'\n';
    output << ini::kSeekStepSeconds << L"=" << config.seekStepSeconds << L'\n';
    output << ini::kSeekStepPercent << L"=" << config.seekStepPercent << L'\n';
    output << ini::kShowSeekPreview << L"=" << BoolToString(config.showSeekPreview) << L'\n';
    output << ini::kVolumeStep << L"=" << config.volumeStep << L'\n';
    output << ini::kWheelVolumeStep << L"=" << config.wheelVolumeStep << L'\n';
    output << L"audio_delay_seconds=" << config.audioDelaySeconds << L'\n';
    output << L"subtitle_delay_seconds=" << config.subtitleDelaySeconds << L'\n';
    output << L"subtitle_font=" << config.subtitleFont << L'\n';
    output << L"subtitle_text_color=" << config.subtitleTextColor << L'\n';
    output << L"subtitle_border_color=" << config.subtitleBorderColor << L'\n';
    output << ini::kSubtitleShadowColor << L"=" << config.subtitleShadowColor << L'\n';
    output << ini::kSubtitleBackgroundEnabled << L"=" << BoolToString(config.subtitleBackgroundEnabled) << L'\n';
    output << ini::kSubtitleBackgroundColor << L"=" << config.subtitleBackgroundColor << L'\n';
    output << L"subtitle_font_size=" << config.subtitleFontSize << L'\n';
    output << L"subtitle_bold=" << BoolToString(config.subtitleBold) << L'\n';
    output << L"subtitle_italic=" << BoolToString(config.subtitleItalic) << L'\n';
    output << L"subtitle_underline=" << BoolToString(config.subtitleUnderline) << L'\n';
    output << L"subtitle_strike_out=" << BoolToString(config.subtitleStrikeOut) << L'\n';
    output << L"subtitle_border_size=" << config.subtitleBorderSize << L'\n';
    output << L"subtitle_shadow_depth=" << config.subtitleShadowDepth << L'\n';
    output << L"subtitle_vertical_margin=" << config.subtitleVerticalMargin << L'\n';
    output << ini::kSubtitlePositionPreset << L"=" << config.subtitlePositionPreset << L'\n';
    output << ini::kSubtitleOffsetUp << L"=" << config.subtitleOffsetUp << L'\n';
    output << ini::kSubtitleOffsetDown << L"=" << config.subtitleOffsetDown << L'\n';
    output << ini::kSubtitleHorizontalOffset << L"=" << config.subtitleHorizontalOffset << L'\n';
    output << ini::kSubtitleOffsetLeft << L"=" << config.subtitleOffsetLeft << L'\n';
    output << ini::kSubtitleOffsetRight << L"=" << config.subtitleOffsetRight << L'\n';
    output << L"subtitle_encoding=" << config.subtitleEncoding << L'\n';
    output << L"double_click_action=" << config.doubleClickAction << L'\n';
    output << L"middle_click_action=" << config.middleClickAction << L'\n';
    output << L"repeat_mode=" << RepeatModeToString(config.repeatMode) << L'\n';
    output << ini::kEndOfPlaybackAction << L"=" << EndOfPlaybackActionToString(config.endOfPlaybackAction) << L'\n';
    output << L"show_debug_info=" << BoolToString(config.showDebugInfo) << L'\n';
    output << L"preferred_aspect_ratio=" << config.preferredAspectRatio << L'\n';
    output << L"video_rotate_degrees=" << config.videoRotateDegrees << L'\n';
    output << L"mirror_video=" << BoolToString(config.mirrorVideo) << L'\n';
    output << L"deinterlace_enabled=" << BoolToString(config.deinterlaceEnabled) << L'\n';
    output << L"sharpen_strength=" << config.sharpenStrength << L'\n';
    output << L"denoise_strength=" << config.denoiseStrength << L'\n';
    output << L"equalizer_profile=" << config.equalizerProfile << L'\n';
    output << ini::kScreenshotFormat << L"=" << config.screenshotFormat << L'\n';
    output << ini::kScreenshotQuality << L"=" << config.screenshotQuality << L'\n';
    output << L"network_timeout_ms=" << config.networkTimeoutMs << L'\n';
    output << L"stream_reconnect_count=" << config.streamReconnectCount << L'\n';
    output << L"last_file=" << config.lastFile << L'\n';
    for (size_t index = 0; index < std::min(config.recentFiles.size(), kMaxRecentFiles); ++index) {
        output << L"recent_file_" << index << L"=" << config.recentFiles[index] << L'\n';
    }
    for (size_t index = 0; index < std::min(config.recentItems.size(), kMaxRecentItems); ++index) {
        output << L"recent_item_path_" << index << L"=" << config.recentItems[index].path << L'\n';
        output << L"recent_item_pinned_" << index << L"=" << BoolToString(config.recentItems[index].pinned) << L'\n';
        output << L"recent_item_opened_" << index << L"=" << config.recentItems[index].openedAtUnixSeconds << L'\n';
    }
    for (size_t index = 0; index < std::min(config.resumeEntries.size(), kMaxResumeEntries); ++index) {
        output << L"resume_path_" << index << L"=" << config.resumeEntries[index].path << L'\n';
        output << L"resume_pos_" << index << L"=" << config.resumeEntries[index].positionSeconds << L'\n';
    }
    for (size_t index = 0; index < std::min(config.historyEntries.size(), kMaxHistoryEntries); ++index) {
        output << L"history_path_" << index << L"=" << config.historyEntries[index].path << L'\n';
        output << L"history_title_" << index << L"=" << config.historyEntries[index].title << L'\n';
        output << L"history_pos_" << index << L"=" << config.historyEntries[index].positionSeconds << L'\n';
        output << L"history_is_url_" << index << L"=" << BoolToString(config.historyEntries[index].isUrl) << L'\n';
        output << L"history_opened_" << index << L"=" << config.historyEntries[index].openedAtUnixSeconds << L'\n';
    }
    for (size_t index = 0; index < std::min(config.bookmarkEntries.size(), kMaxBookmarkEntries); ++index) {
        output << L"bookmark_path_" << index << L"=" << config.bookmarkEntries[index].path << L'\n';
        output << L"bookmark_label_" << index << L"=" << config.bookmarkEntries[index].label << L'\n';
        output << L"bookmark_pos_" << index << L"=" << config.bookmarkEntries[index].positionSeconds << L'\n';
    }
    for (const auto& binding : config.keyBindings) {
        output << L"binding_" << binding.actionId << L"=" << binding.virtualKey << L'\n';
    }
    return true;
}

std::filesystem::path CorruptConfigCopyPath(const std::filesystem::path& sourcePath) {
    return sourcePath.parent_path() / L"settings.corrupt.ini";
}

}  // namespace velo::config

