#pragma once

#include <string>

#include "config/config_service.h"

namespace velo::playback {

struct SubtitleLayout {
    int verticalPosition = 96;
    int horizontalOffset = 0;
    int marginX = 24;
    int marginLeft = 24;
    int marginRight = 24;
    std::string alignX = "center";
    std::string alignY = "bottom";
    std::string justify = "center";
    std::string styleOverrides;
};

SubtitleLayout BuildSubtitleLayout(const velo::config::AppConfig& config);
std::string BuildSubtitleDiagnosticsSnapshot(const velo::config::AppConfig& config, const SubtitleLayout& layout);

}  // namespace velo::playback