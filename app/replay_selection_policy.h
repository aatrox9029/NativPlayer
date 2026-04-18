#pragma once

#include <string>

#include "app/runtime_playlist.h"

namespace velo::app {

[[nodiscard]] int ResolveReplayPlaylistIndex(const RuntimePlaylist& playlist, int currentPlaylistIndex,
                                             const std::wstring& currentOpenPath);

}  // namespace velo::app
