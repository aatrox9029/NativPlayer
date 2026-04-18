#include "app/replay_selection_policy.h"

namespace velo::app {

int ResolveReplayPlaylistIndex(const RuntimePlaylist& playlist, const int currentPlaylistIndex, const std::wstring& currentOpenPath) {
    if (!currentOpenPath.empty()) {
        const int matchingIndex = playlist.FindIndexByPath(currentOpenPath);
        if (matchingIndex >= 0) {
            return matchingIndex;
        }
    }

    if (currentPlaylistIndex >= 0 && currentPlaylistIndex < static_cast<int>(playlist.Size())) {
        return currentPlaylistIndex;
    }

    return -1;
}

}  // namespace velo::app
