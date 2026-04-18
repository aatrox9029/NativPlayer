#include "playback/end_file_reason.h"

namespace velo::playback {
namespace {

constexpr int kMpvEndFileReasonEof = 0;

}  // namespace

bool IsPlaybackEofEvent(const mpv_event_end_file* endFile) noexcept {
    return endFile != nullptr && endFile->reason == kMpvEndFileReasonEof;
}

}  // namespace velo::playback
