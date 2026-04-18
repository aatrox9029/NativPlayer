#pragma once

#include <string>

namespace velo::playback {

enum class SeekCommandType {
    Relative,
    Absolute,
};

struct LowLatencySeekCommand {
    std::string mode;
    bool resumeAfterSeek = true;
};

constexpr double kLowLatencySeekBudgetMs = 100.0;

std::string ResolveMpvHwdecPolicy(const std::wstring& configuredPolicy);
LowLatencySeekCommand BuildLowLatencySeekCommand(SeekCommandType type, bool exactRequested);

}  // namespace velo::playback
