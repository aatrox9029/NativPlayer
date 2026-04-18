#pragma once

#include <Windows.h>

#include <string>
#include <string_view>

#include "ui/player_state.h"

namespace velo::ui {

enum class ButtonKind {
    OpenFile,
    RecentFiles,
    QuickBrowse,
    PreviousItem,
    PlayPause,
    NextPlaybackItem,
    Mute,
    Speed,
    AudioTrack,
    SubtitleTrack,
    Settings,
    More,
    Fullscreen,
    Download,
    Replay,
    NextItem,
};

struct ButtonVisualSpec {
    std::wstring glyph;
    std::wstring imageAsset;
    std::wstring label;
    std::wstring badge;
    bool accent = false;
    bool warning = false;
    bool textGlyph = false;
    bool largeGlyph = false;
};

ButtonVisualSpec ResolveButtonVisual(ButtonKind kind, const PlayerState& state, bool fullscreen, std::wstring_view languageCode);

}  // namespace velo::ui
