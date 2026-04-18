#pragma once

#include "include/mpv/client.h"

namespace velo::playback {

[[nodiscard]] bool IsPlaybackEofEvent(const mpv_event_end_file* endFile) noexcept;

}  // namespace velo::playback
