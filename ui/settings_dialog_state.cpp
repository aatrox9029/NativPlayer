#include "ui/settings_dialog_state.h"

#include <algorithm>

#include "platform/win32/input_profile.h"

namespace velo::ui {

SettingsDialogState BuildSettingsDialogState(const velo::config::AppConfig& config) {
    SettingsDialogState state;
    state.languageCode = config.languageCode;
    state.autoplayNextFile = config.autoplayNextFile;
    state.rememberPlaybackPosition = config.rememberPlaybackPosition;
    state.preservePauseOnOpen = config.preservePauseOnOpen;
    state.showSeekPreview = config.showSeekPreview;
    state.controlsHideDelayMs = config.controlsHideDelayMs;
    state.seekStepMode = config.seekStepMode;
    state.seekStepSeconds = config.seekStepSeconds;
    state.seekStepPercent = config.seekStepPercent;
    state.repeatMode = config.repeatMode;
    state.endOfPlaybackAction = config.endOfPlaybackAction;

    state.autoLoadSubtitle = config.autoLoadSubtitle;
    state.subtitleDelaySeconds = config.subtitleDelaySeconds;
    state.subtitleFont = config.subtitleFont;
    state.subtitleTextColor = config.subtitleTextColor;
    state.subtitleBorderColor = config.subtitleBorderColor;
    state.subtitleShadowColor = config.subtitleShadowColor;
    state.subtitleBackgroundEnabled = config.subtitleBackgroundEnabled;
    state.subtitleBackgroundColor = config.subtitleBackgroundColor;
    state.subtitleFontSize = config.subtitleFontSize;
    state.subtitleBold = config.subtitleBold;
    state.subtitleItalic = config.subtitleItalic;
    state.subtitleUnderline = config.subtitleUnderline;
    state.subtitleStrikeOut = config.subtitleStrikeOut;
    state.subtitleBorderSize = config.subtitleBorderSize;
    state.subtitleShadowDepth = config.subtitleShadowDepth;
    state.subtitleVerticalMargin = config.subtitleVerticalMargin;
    state.subtitlePositionPreset = config.subtitlePositionPreset;
    state.subtitleOffsetUp = config.subtitleOffsetUp;
    state.subtitleOffsetDown = config.subtitleOffsetDown;
    state.subtitleHorizontalOffset = 0;
    state.subtitleOffsetLeft = 0;
    state.subtitleOffsetRight = 0;
    state.subtitleEncoding = config.subtitleEncoding;

    state.rememberVolume = config.rememberVolume;
    state.startupVolume = config.startupVolume;
    state.volumeStep = config.volumeStep;
    state.wheelVolumeStep = config.wheelVolumeStep;
    state.audioDelaySeconds = config.audioDelaySeconds;
    state.audioOutputDevice = config.audioOutputDevice;

    state.doubleClickAction = config.doubleClickAction;
    state.middleClickAction = config.middleClickAction;
    state.shortcutBindingValues.reserve(velo::platform::win32::InputBindingDefinitions().size());
    for (const auto& definition : velo::platform::win32::InputBindingDefinitions()) {
        state.shortcutBindingValues.push_back(velo::platform::win32::ResolveVirtualKey(config, definition.actionId));
    }

    state.hwdecPolicy = config.hwdecPolicy;
    state.showDebugInfo = config.showDebugInfo;
    state.preferredAspectRatio = config.preferredAspectRatio;
    state.videoRotateDegrees = config.videoRotateDegrees;
    state.mirrorVideo = config.mirrorVideo;
    state.deinterlaceEnabled = config.deinterlaceEnabled;
    state.sharpenStrength = config.sharpenStrength;
    state.denoiseStrength = config.denoiseStrength;
    state.equalizerProfile = config.equalizerProfile;
    state.screenshotFormat = config.screenshotFormat;
    state.screenshotQuality = config.screenshotQuality;
    state.networkTimeoutMs = config.networkTimeoutMs;
    state.streamReconnectCount = config.streamReconnectCount;
    return state;
}

velo::config::AppConfig ApplySettingsDialogState(const velo::config::AppConfig& baseConfig, const SettingsDialogState& state) {
    velo::config::AppConfig updated = baseConfig;
    updated.languageCode = state.languageCode;
    updated.autoplayNextFile = state.autoplayNextFile;
    updated.rememberPlaybackPosition = state.rememberPlaybackPosition;
    updated.preservePauseOnOpen = state.preservePauseOnOpen;
    updated.showSeekPreview = state.showSeekPreview;
    updated.controlsHideDelayMs = state.controlsHideDelayMs;
    updated.seekStepMode = state.seekStepMode;
    updated.seekStepSeconds = state.seekStepSeconds;
    updated.seekStepPercent = state.seekStepPercent;
    updated.repeatMode = state.repeatMode;
    updated.endOfPlaybackAction = state.endOfPlaybackAction;

    updated.autoLoadSubtitle = state.autoLoadSubtitle;
    updated.subtitleDelaySeconds = state.subtitleDelaySeconds;
    updated.subtitleFont = state.subtitleFont;
    updated.subtitleTextColor = state.subtitleTextColor;
    updated.subtitleBorderColor = state.subtitleBorderColor;
    updated.subtitleShadowColor = state.subtitleShadowColor;
    updated.subtitleBackgroundEnabled = state.subtitleBackgroundEnabled;
    updated.subtitleBackgroundColor = state.subtitleBackgroundColor;
    updated.subtitleFontSize = state.subtitleFontSize;
    updated.subtitleBold = state.subtitleBold;
    updated.subtitleItalic = state.subtitleItalic;
    updated.subtitleUnderline = state.subtitleUnderline;
    updated.subtitleStrikeOut = state.subtitleStrikeOut;
    updated.subtitleBorderSize = state.subtitleBorderSize;
    updated.subtitleShadowDepth = state.subtitleShadowDepth;
    updated.subtitleVerticalMargin = state.subtitleVerticalMargin;
    updated.subtitlePositionPreset = state.subtitlePositionPreset;
    updated.subtitleOffsetUp = state.subtitleOffsetUp;
    updated.subtitleOffsetDown = state.subtitleOffsetDown;
    updated.subtitleHorizontalOffset = 0;
    updated.subtitleOffsetLeft = 0;
    updated.subtitleOffsetRight = 0;
    updated.subtitleEncoding = state.subtitleEncoding;

    updated.rememberVolume = state.rememberVolume;
    updated.startupVolume = state.startupVolume;
    updated.volumeStep = state.volumeStep;
    updated.wheelVolumeStep = state.wheelVolumeStep;
    updated.audioDelaySeconds = state.audioDelaySeconds;
    updated.audioOutputDevice = state.audioOutputDevice;

    updated.doubleClickAction = state.doubleClickAction;
    updated.middleClickAction = state.middleClickAction;
    updated.keyBindings.clear();
    for (size_t index = 0; index < velo::platform::win32::InputBindingDefinitions().size() &&
                           index < state.shortcutBindingValues.size();
         ++index) {
        velo::platform::win32::SetVirtualKey(updated, velo::platform::win32::InputBindingDefinitions()[index].actionId,
                                             state.shortcutBindingValues[index]);
    }

    updated.hwdecPolicy = state.hwdecPolicy;
    updated.showDebugInfo = state.showDebugInfo;
    updated.preferredAspectRatio = state.preferredAspectRatio;
    updated.videoRotateDegrees = state.videoRotateDegrees;
    updated.mirrorVideo = state.mirrorVideo;
    updated.deinterlaceEnabled = state.deinterlaceEnabled;
    updated.sharpenStrength = state.sharpenStrength;
    updated.denoiseStrength = state.denoiseStrength;
    updated.equalizerProfile = state.equalizerProfile;
    updated.screenshotFormat = state.screenshotFormat;
    updated.screenshotQuality = state.screenshotQuality;
    updated.networkTimeoutMs = state.networkTimeoutMs;
    updated.streamReconnectCount = state.streamReconnectCount;
    return updated;
}

}  // namespace velo::ui
