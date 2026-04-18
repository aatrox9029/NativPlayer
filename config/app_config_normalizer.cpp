#include "config/config_service_internal.h"
#include "config/subtitle_position_utils.h"

namespace velo::config {

namespace {

int CanonicalSubtitleHorizontalOffset(const AppConfig& config) {
    static_cast<void>(config);
    return 0;
}

}

void NormalizeConfig(AppConfig& config) {
    config.windowWidth = std::max(960, config.windowWidth);
    config.windowHeight = std::max(600, config.windowHeight);
    config.volume = std::clamp(config.volume, 0.0, 100.0);
    config.startupVolume = std::clamp(config.startupVolume, 0.0, 100.0);
    config.languageCode = velo::localization::LanguageCode(velo::localization::ResolveLanguage(config.languageCode));
    config.audioDelaySeconds = std::clamp(config.audioDelaySeconds, -10.0, 10.0);
    config.subtitleDelaySeconds = std::clamp(config.subtitleDelaySeconds, -10.0, 10.0);
    config.controlsHideDelayMs = std::clamp(config.controlsHideDelayMs, 800, 8000);
    config.seekStepSeconds = std::clamp(config.seekStepSeconds, 1, 60);
    config.seekStepPercent = std::clamp(config.seekStepPercent, 1, 100);
    config.volumeStep = std::clamp(config.volumeStep, 1, 20);
    config.wheelVolumeStep = std::clamp(config.wheelVolumeStep, 1, 20);
    config.subtitleFontSize = std::clamp(config.subtitleFontSize, 16, 96);
    config.subtitleBorderSize = std::clamp(config.subtitleBorderSize, 0, 12);
    config.subtitleShadowDepth = std::clamp(config.subtitleShadowDepth, 0, 6);
    config.subtitleVerticalMargin = std::clamp(config.subtitleVerticalMargin, 0, 100);
    config.subtitleOffsetUp = std::clamp(config.subtitleOffsetUp, 0, 999);
    config.subtitleOffsetDown = std::clamp(config.subtitleOffsetDown, 0, 999);
    config.subtitleHorizontalOffset = CanonicalSubtitleHorizontalOffset(config);
    config.subtitleOffsetLeft = 0;
    config.subtitleOffsetRight = 0;
    if (config.subtitlePositionPreset != L"top" && config.subtitlePositionPreset != L"middle" && config.subtitlePositionPreset != L"bottom") {
        config.subtitlePositionPreset = L"bottom";
    }
    const bool hasExplicitVerticalOffsets = config.subtitleOffsetUp != 0 || config.subtitleOffsetDown != 0;
    if (hasExplicitVerticalOffsets) {
        config.subtitleVerticalMargin =
            SubtitleVerticalMarginFromDirection(config.subtitlePositionPreset, config.subtitleOffsetUp - config.subtitleOffsetDown);
    } else {
        config.subtitleVerticalMargin = ClampSubtitleVerticalPosition(config.subtitleVerticalMargin);
    }
    const int direction = SubtitleVerticalDirectionFromMargin(config.subtitlePositionPreset, config.subtitleVerticalMargin);
    config.subtitleOffsetUp = std::max(0, direction);
    config.subtitleOffsetDown = std::max(0, -direction);
    config.screenshotQuality = std::clamp(config.screenshotQuality, 10, 100);
    config.videoRotateDegrees = ((config.videoRotateDegrees % 360) + 360) % 360;
    config.sharpenStrength = std::clamp(config.sharpenStrength, 0, 20);
    config.denoiseStrength = std::clamp(config.denoiseStrength, 0, 20);
    config.networkTimeoutMs = std::clamp(config.networkTimeoutMs, 2000, 30000);
    config.streamReconnectCount = std::clamp(config.streamReconnectCount, 0, 10);
    if (config.doubleClickAction.empty()) {
        config.doubleClickAction = L"fullscreen";
    }
    if (config.middleClickAction.empty()) {
        config.middleClickAction = L"toggle_pause";
    }
    config.subtitleTextColor = velo::common::NormalizeRgbaHexColor(config.subtitleTextColor, L"FFFFFFFF");
    config.subtitleBorderColor = velo::common::NormalizeRgbaHexColor(config.subtitleBorderColor, L"101010FF");
    config.subtitleShadowColor = velo::common::NormalizeRgbaHexColor(config.subtitleShadowColor, L"101010AA");
    config.subtitleBackgroundColor = velo::common::NormalizeRgbaHexColor(config.subtitleBackgroundColor, L"101010B3");
    if (!config.subtitleFont.empty() && config.subtitleFont.front() == L'@') {
        config.subtitleFont.erase(config.subtitleFont.begin());
    }

    config.recentFiles.clear();
    config.recentItems.clear();

    std::vector<ResumeEntry> normalizedResume;
    for (const auto& entry : config.resumeEntries) {
        if (entry.path.empty()) {
            continue;
        }
        const bool alreadyAdded = std::any_of(normalizedResume.begin(), normalizedResume.end(), [&](const ResumeEntry& existing) {
            return SameTextInsensitive(existing.path, entry.path);
        });
        if (!alreadyAdded) {
            normalizedResume.push_back({entry.path, std::max(0.0, entry.positionSeconds)});
        }
        if (normalizedResume.size() >= kMaxResumeEntries) {
            break;
        }
    }
    config.resumeEntries = std::move(normalizedResume);

    std::vector<HistoryEntry> normalizedHistory;
    for (const auto& entry : config.historyEntries) {
        if (entry.path.empty()) {
            continue;
        }
        const bool alreadyAdded = std::any_of(normalizedHistory.begin(), normalizedHistory.end(), [&](const HistoryEntry& existing) {
            return SameTextInsensitive(existing.path, entry.path);
        });
        if (!alreadyAdded) {
            normalizedHistory.push_back(entry);
        }
        if (normalizedHistory.size() >= kMaxHistoryEntries) {
            break;
        }
    }
    config.historyEntries = std::move(normalizedHistory);

    std::vector<BookmarkEntry> normalizedBookmarks;
    for (const auto& entry : config.bookmarkEntries) {
        if (entry.path.empty()) {
            continue;
        }
        normalizedBookmarks.push_back(entry);
        if (normalizedBookmarks.size() >= kMaxBookmarkEntries) {
            break;
        }
    }
    config.bookmarkEntries = std::move(normalizedBookmarks);

    std::vector<KeyBindingEntry> normalizedBindings;
    for (const auto& entry : config.keyBindings) {
        if (entry.actionId.empty()) {
            continue;
        }
        SetKeyBinding(normalizedBindings, entry.actionId, entry.virtualKey);
    }
    config.keyBindings = std::move(normalizedBindings);
}


}  // namespace velo::config
