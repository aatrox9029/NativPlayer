#include "playback/subtitle_layout.h"

#include <algorithm>
#include <sstream>

#include "common/text_encoding.h"

namespace velo::playback {
namespace {

constexpr int kBaseSubtitleMarginX = 24;

int ClampInt(const int value, const int minimum, const int maximum) {
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

int EffectiveSubtitleVerticalPosition(const velo::config::AppConfig& config) {
    return ClampInt(config.subtitleVerticalMargin, 0, 100);
}

std::string EffectiveSubtitleAlignY(const velo::config::AppConfig& config) {
    if (config.subtitlePositionPreset == L"top") {
        return "top";
    }
    if (config.subtitlePositionPreset == L"middle") {
        return "center";
    }
    return "bottom";
}

int EffectiveAlignmentCode(const velo::config::AppConfig& config) {
    if (config.subtitlePositionPreset == L"top") {
        return 8;
    }
    if (config.subtitlePositionPreset == L"middle") {
        return 5;
    }
    return 2;
}

std::string ToUtf8(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

}  // namespace

SubtitleLayout BuildSubtitleLayout(const velo::config::AppConfig& config) {
    SubtitleLayout layout;
    layout.verticalPosition = EffectiveSubtitleVerticalPosition(config);
    layout.horizontalOffset = 0;
    layout.marginX = kBaseSubtitleMarginX;
    layout.marginLeft = kBaseSubtitleMarginX;
    layout.marginRight = kBaseSubtitleMarginX;
    layout.alignY = EffectiveSubtitleAlignY(config);

    std::ostringstream stream;
    stream << "Default.Alignment=" << EffectiveAlignmentCode(config)
           << ",Default.Bold=" << (config.subtitleBold ? 1 : 0)
           << ",Default.Italic=" << (config.subtitleItalic ? 1 : 0)
           << ",Default.Underline=" << (config.subtitleUnderline ? 1 : 0)
           << ",Default.StrikeOut=" << (config.subtitleStrikeOut ? 1 : 0)
           << ",Default.MarginL=" << layout.marginLeft
           << ",Default.MarginR=" << layout.marginRight;
    layout.styleOverrides = stream.str();
    return layout;
}

std::string BuildSubtitleDiagnosticsSnapshot(const velo::config::AppConfig& config, const SubtitleLayout& layout) {
    std::ostringstream stream;
    stream << "font=" << ToUtf8(config.subtitleFont)
           << " text_color=" << ToUtf8(config.subtitleTextColor)
           << " border_color=" << ToUtf8(config.subtitleBorderColor)
           << " shadow_color=" << ToUtf8(config.subtitleShadowColor)
           << " background_enabled=" << (config.subtitleBackgroundEnabled ? "yes" : "no")
           << " background_color=" << ToUtf8(config.subtitleBackgroundColor)
           << " position=" << ToUtf8(config.subtitlePositionPreset)
           << " offset_up=" << config.subtitleOffsetUp
           << " offset_down=" << config.subtitleOffsetDown
           << " vertical_margin=" << config.subtitleVerticalMargin
           << " horizontal_offset=" << layout.horizontalOffset
           << " legacy_left=" << config.subtitleOffsetLeft
           << " legacy_right=" << config.subtitleOffsetRight
           << " align_x=" << layout.alignX
           << " align_y=" << layout.alignY
           << " justify=" << layout.justify
           << " margin_x=" << layout.marginX
           << " margin_left=" << layout.marginLeft
           << " margin_right=" << layout.marginRight
           << " sub_pos=" << layout.verticalPosition
           << " style_overrides=" << layout.styleOverrides;
    return stream.str();
}

}  // namespace velo::playback
