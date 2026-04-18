#include "playback/low_latency_seek.h"

namespace velo::playback {

std::string ResolveMpvHwdecPolicy(const std::wstring& configuredPolicy) {
    if (configuredPolicy == L"no") {
        return "no";
    }
    if (configuredPolicy == L"auto-copy") {
        return "auto-copy";
    }
    return "auto";
}

LowLatencySeekCommand BuildLowLatencySeekCommand(const SeekCommandType type, const bool exactRequested) {
    LowLatencySeekCommand command;
    if (type == SeekCommandType::Absolute) {
        command.mode = exactRequested ? "absolute+exact" : "absolute+keyframes";
        return command;
    }

    command.mode = exactRequested ? "relative+exact" : "relative+keyframes";
    return command;
}

}  // namespace velo::playback
