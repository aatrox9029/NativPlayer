#pragma once

namespace velo::config::ini {

constexpr wchar_t kAutoplayNextFile[] = L"autoplay_next_file";
constexpr wchar_t kLanguageCode[] = L"language_code";
constexpr wchar_t kPreservePauseOnOpen[] = L"preserve_pause_on_open";
constexpr wchar_t kAutoLoadSubtitle[] = L"auto_load_subtitle";
constexpr wchar_t kRememberPlaybackPosition[] = L"remember_playback_position";
constexpr wchar_t kControlsHideDelayMs[] = L"controls_hide_delay_ms";
constexpr wchar_t kSeekStepMode[] = L"seek_step_mode";
constexpr wchar_t kSeekStepSeconds[] = L"seek_step_seconds";
constexpr wchar_t kSeekStepPercent[] = L"seek_step_percent";
constexpr wchar_t kShowSeekPreview[] = L"show_seek_preview";
constexpr wchar_t kVolumeStep[] = L"volume_step";
constexpr wchar_t kWheelVolumeStep[] = L"wheel_volume_step";
constexpr wchar_t kAudioOutputDevice[] = L"audio_output_device";
constexpr wchar_t kSubtitleShadowColor[] = L"subtitle_shadow_color";
constexpr wchar_t kSubtitleBackgroundEnabled[] = L"subtitle_background_enabled";
constexpr wchar_t kSubtitleBackgroundColor[] = L"subtitle_background_color";
constexpr wchar_t kSubtitlePositionPreset[] = L"subtitle_position_preset";
constexpr wchar_t kSubtitleOffsetUp[] = L"subtitle_offset_up";
constexpr wchar_t kSubtitleOffsetDown[] = L"subtitle_offset_down";
constexpr wchar_t kSubtitleHorizontalOffset[] = L"subtitle_horizontal_offset";
constexpr wchar_t kSubtitleOffsetLeft[] = L"subtitle_offset_left";
constexpr wchar_t kSubtitleOffsetRight[] = L"subtitle_offset_right";
constexpr wchar_t kEndOfPlaybackAction[] = L"end_of_playback_action";
constexpr wchar_t kScreenshotFormat[] = L"screenshot_format";
constexpr wchar_t kScreenshotQuality[] = L"screenshot_quality";

}  // namespace velo::config::ini
