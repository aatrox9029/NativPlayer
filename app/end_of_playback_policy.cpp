#include "app/end_of_playback_policy.h"

namespace velo::app {

EndOfPlaybackDecision DecideEndOfPlaybackAction(const velo::config::EndOfPlaybackAction action, const int currentPlaylistIndex,
                                                const std::optional<int> nextPlaylistIndex) {
    EndOfPlaybackDecision decision;
    switch (action) {
        case velo::config::EndOfPlaybackAction::Replay:
            if (currentPlaylistIndex >= 0) {
                decision.playbackIndex = currentPlaylistIndex;
                decision.replayCurrent = true;
            }
            break;
        case velo::config::EndOfPlaybackAction::PlayNext:
            decision.playbackIndex = nextPlaylistIndex;
            break;
        case velo::config::EndOfPlaybackAction::CloseWindow:
            decision.closeWindow = true;
            break;
        case velo::config::EndOfPlaybackAction::Stop:
            break;
    }
    return decision;
}

bool ShouldClearPendingFileLoad(const velo::ui::PlayerState& state) {
    return state.isLoaded && !state.eofReached;
}

bool ShouldPauseAfterOpeningItem(const bool preservePauseOnOpen, const bool isPaused, const bool eofReached,
                                 const bool forcePauseAfterLoad) {
    if (forcePauseAfterLoad) {
        return true;
    }
    return preservePauseOnOpen && isPaused && !eofReached;
}

std::optional<double> ResolvePendingResumeSeconds(const bool rememberPlaybackPosition, const bool suppressResumeRestore,
                                                  const double storedResumeSeconds) {
    if (!rememberPlaybackPosition || suppressResumeRestore) {
        return std::nullopt;
    }
    return storedResumeSeconds;
}

}  // namespace velo::app
