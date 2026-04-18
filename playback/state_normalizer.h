#pragma once

#include <cstdint>
#include <string>

#include "ui/player_state.h"

namespace velo::playback {

class StateNormalizer {
public:
    void ApplyFlag(velo::ui::PlayerState& state, const std::string& name, bool value) const;
    void ApplyDouble(velo::ui::PlayerState& state, const std::string& name, double value) const;
    void ApplyInt64(velo::ui::PlayerState& state, const std::string& name, int64_t value) const;
    void ApplyString(velo::ui::PlayerState& state, const std::string& name, std::string value) const;
    void ApplyPlaybackRestart(velo::ui::PlayerState& state) const;
    void ApplyFileLoaded(velo::ui::PlayerState& state) const;
    void ApplyLoadFailed(velo::ui::PlayerState& state, const std::string& error) const;
};

}  // namespace velo::playback