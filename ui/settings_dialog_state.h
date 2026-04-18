#pragma once

#include <vector>

#include "config/config_service.h"

namespace velo::ui {

struct SettingsDialogState {
    std::wstring languageCode = L"zh-TW";
    bool autoplayNextFile = true;
    bool rememberPlaybackPosition = true;
    bool preservePauseOnOpen = true;
    bool showSeekPreview = true;
    int controlsHideDelayMs = 2000;
    velo::config::SeekStepMode seekStepMode = velo::config::SeekStepMode::Seconds;
    int seekStepSeconds = 10;
    int seekStepPercent = 5;
    velo::config::RepeatMode repeatMode = velo::config::RepeatMode::Off;
    velo::config::EndOfPlaybackAction endOfPlaybackAction = velo::config::EndOfPlaybackAction::PlayNext;

    bool autoLoadSubtitle = true;
    double subtitleDelaySeconds = 0.0;
    std::wstring subtitleFont;
    std::wstring subtitleTextColor;
    std::wstring subtitleBorderColor;
    std::wstring subtitleShadowColor;
    bool subtitleBackgroundEnabled = false;
    std::wstring subtitleBackgroundColor;
    int subtitleFontSize = 42;
    bool subtitleBold = false;
    bool subtitleItalic = false;
    bool subtitleUnderline = false;
    bool subtitleStrikeOut = false;
    int subtitleBorderSize = 2;
    int subtitleShadowDepth = 1;
    int subtitleVerticalMargin = 96;
    std::wstring subtitlePositionPreset = L"bottom";
    int subtitleOffsetUp = 0;
    int subtitleOffsetDown = 0;
    int subtitleHorizontalOffset = 0;
    int subtitleOffsetLeft = 0;
    int subtitleOffsetRight = 0;
    std::wstring subtitleEncoding;

    bool rememberVolume = true;
    double startupVolume = 20.0;
    int volumeStep = 5;
    int wheelVolumeStep = 5;
    double audioDelaySeconds = 0.0;
    std::wstring audioOutputDevice;

    std::wstring doubleClickAction;
    std::wstring middleClickAction;
    std::vector<unsigned int> shortcutBindingValues;

    std::wstring hwdecPolicy;
    bool showDebugInfo = false;
    std::wstring preferredAspectRatio;
    int videoRotateDegrees = 0;
    bool mirrorVideo = false;
    bool deinterlaceEnabled = false;
    int sharpenStrength = 0;
    int denoiseStrength = 0;
    std::wstring equalizerProfile;
    std::wstring screenshotFormat;
    int screenshotQuality = 92;
    int networkTimeoutMs = 8000;
    int streamReconnectCount = 2;
};

SettingsDialogState BuildSettingsDialogState(const velo::config::AppConfig& config);
velo::config::AppConfig ApplySettingsDialogState(const velo::config::AppConfig& baseConfig, const SettingsDialogState& state);

}  // namespace velo::ui
