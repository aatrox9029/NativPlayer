#pragma once

#include <optional>

#include "config/config_service.h"
#include "ui/player_state.h"

namespace velo::app {

struct EndOfPlaybackDecision {
    std::optional<int> playbackIndex;
    bool replayCurrent = false;
    bool closeWindow = false;
};

EndOfPlaybackDecision DecideEndOfPlaybackAction(velo::config::EndOfPlaybackAction action, int currentPlaylistIndex,
                                                std::optional<int> nextPlaylistIndex);
bool ShouldClearPendingFileLoad(const velo::ui::PlayerState& state);
bool ShouldPauseAfterOpeningItem(bool preservePauseOnOpen, bool isPaused, bool eofReached, bool forcePauseAfterLoad);
std::optional<double> ResolvePendingResumeSeconds(bool rememberPlaybackPosition, bool suppressResumeRestore, double storedResumeSeconds);

}  // namespace velo::app
