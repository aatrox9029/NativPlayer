#pragma once

#include <string>

#include "config/config_service.h"

namespace velo::playback {

std::string BuildVideoFilterGraph(const velo::config::AppConfig& config);
std::string BuildAudioFilterGraph(const velo::config::AppConfig& config);
std::string SubtitleColorToMpv(const std::wstring& color);

}  // namespace velo::playback
