#include "playback/video_filter_profile.h"

#include <Windows.h>

#include <sstream>
#include <vector>
#include <iomanip>

#include "common/hex_color.h"
#include "common/text_encoding.h"

namespace velo::playback {
namespace {

std::string Narrow(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

}  // namespace

std::string BuildVideoFilterGraph(const velo::config::AppConfig& config) {
    std::vector<std::string> filters;
    if (config.mirrorVideo) {
        filters.emplace_back("hflip");
    }
    if (config.deinterlaceEnabled) {
        filters.emplace_back("yadif");
    }
    if (config.sharpenStrength > 0) {
        filters.emplace_back("unsharp=5:5:" + std::to_string(config.sharpenStrength / 10.0));
    }
    if (config.denoiseStrength > 0) {
        filters.emplace_back("hqdn3d=" + std::to_string(config.denoiseStrength / 12.0));
    }

    std::ostringstream stream;
    for (size_t index = 0; index < filters.size(); ++index) {
        if (index > 0) {
            stream << ',';
        }
        stream << filters[index];
    }
    return stream.str();
}

std::string BuildAudioFilterGraph(const velo::config::AppConfig& config) {
    if (config.equalizerProfile == L"voice") {
        return "lavfi=[firequalizer=gain_entry='entry(120,2);entry(2200,4);entry(6000,1)']";
    }
    if (config.equalizerProfile == L"cinema") {
        return "lavfi=[firequalizer=gain_entry='entry(50,3);entry(120,-1);entry(8000,2)']";
    }
    if (config.equalizerProfile == L"music") {
        return "lavfi=[firequalizer=gain_entry='entry(60,4);entry(170,2);entry(1000,-1);entry(12000,2)']";
    }
    return {};
}

std::string SubtitleColorToMpv(const std::wstring& color) {
    const auto parsed = velo::common::TryParseRgbaColor(color);
    if (!parsed.has_value()) {
        return "#FFFFFFFF";
    }

    std::ostringstream stream;
    stream << '#'
           << std::uppercase << std::hex << std::setfill('0')
           << std::setw(2) << static_cast<int>(parsed->alpha)
           << std::setw(2) << static_cast<int>(parsed->red)
           << std::setw(2) << static_cast<int>(parsed->green)
           << std::setw(2) << static_cast<int>(parsed->blue);
    return stream.str();
}

}  // namespace velo::playback
