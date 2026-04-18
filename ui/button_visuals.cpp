#include "ui/button_visuals.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "localization/localization.h"

namespace velo::ui {
namespace {

std::wstring FormatSpeedBadge(const double speed) {
    std::wostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(2);
    stream << speed << L"x";
    return stream.str();
}

std::wstring FormatTrackBadge(const wchar_t* prefix, const long long trackId, const wchar_t* fallback) {
    if (trackId < 0) {
        return fallback;
    }
    return std::wstring(prefix) + std::to_wstring(trackId);
}

bool UsesLargeGlyph(const ButtonKind kind) {
    switch (kind) {
        case ButtonKind::PreviousItem:
        case ButtonKind::NextPlaybackItem:
        case ButtonKind::NextItem:
        case ButtonKind::SubtitleTrack:
            return false;
        default:
            return true;
    }
}

std::wstring ButtonImageAsset(const ButtonKind kind, const PlayerState& state, const bool fullscreen) {
    switch (kind) {
        case ButtonKind::OpenFile:
            return L"\u958b\u555f\u6a94\u6848";
        case ButtonKind::QuickBrowse:
            return L"\u5feb\u901f\u700f\u89bd";
        case ButtonKind::PreviousItem:
            return L"\u4e0a\u4e00\u90e8";
        case ButtonKind::PlayPause:
            return state.isPaused ? L"\u64a5\u653e" : L"\u66ab\u505c";
        case ButtonKind::NextPlaybackItem:
        case ButtonKind::NextItem:
            return L"\u4e0b\u4e00\u90e8";
        case ButtonKind::Replay:
            return L"\u64a5\u653e";
        case ButtonKind::Mute:
            return state.isMuted ? L"\u975c\u97f3" : L"\u97f3\u91cf";
        case ButtonKind::Speed:
            return L"\u500d\u901f";
        case ButtonKind::SubtitleTrack:
            return L"\u5b57\u5e55";
        case ButtonKind::Settings:
            return L"\u8a2d\u5b9a";
        case ButtonKind::Fullscreen:
            return fullscreen ? L"\u8996\u7a97\u5316" : L"\u5168\u87a2\u5e55";
        case ButtonKind::Download:
            return L"download";
        default:
            return {};
    }
}

}  // namespace

ButtonVisualSpec ResolveButtonVisual(const ButtonKind kind, const PlayerState& state, const bool fullscreen, const std::wstring_view languageCode) {
    const auto language = velo::localization::ResolveLanguage(languageCode);
    switch (kind) {
        case ButtonKind::OpenFile:
            return {.glyph = L"\xD83D\xDCC1",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::Open),
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::RecentFiles:
            return {.glyph = L"\u25F7", .label = velo::localization::Text(language, velo::localization::TextId::Recent), .textGlyph = true, .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::QuickBrowse:
            return {.glyph = L"\u2630",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::QuickBrowse),
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::PreviousItem:
            return {.glyph = L"<<",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::PreviousItem),
                    .accent = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::PlayPause:
            return {.glyph = state.isPaused ? L"\u25B6" : L"||",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = state.isPaused ? velo::localization::Text(language, velo::localization::TextId::Play)
                                            : velo::localization::Text(language, velo::localization::TextId::Pause),
                    .accent = true,
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::NextPlaybackItem:
            return {.glyph = L">>",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::NextItem),
                    .accent = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::Mute:
            return {.glyph = state.isMuted ? L"x)" : L"))",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::Volume),
                    .warning = state.isMuted,
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::Speed:
            return {.glyph = L"\u23F1\uFE0F",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::Speed),
                    .badge = FormatSpeedBadge(state.playbackSpeed),
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::AudioTrack:
            return {.glyph = L"AT", .label = velo::localization::Text(language, velo::localization::TextId::CycleAudioTrack), .badge = FormatTrackBadge(L"A", state.audioTrackId, L"\u81ea\u52d5"), .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::SubtitleTrack:
            return {.glyph = L"CC",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::Subtitle),
                    .accent = state.subtitleTrackId >= 0,
                    .warning = state.subtitleTrackId < 0,
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::Settings:
            return {.glyph = L"\xD83D\xDD27",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::Settings),
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::More:
            return {.glyph = L"...", .label = velo::localization::Text(language, velo::localization::TextId::More), .textGlyph = true, .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::Fullscreen:
            return {.glyph = L"\u25A1",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = fullscreen ? velo::localization::Text(language, velo::localization::TextId::ExitFullscreen)
                                        : velo::localization::Text(language, velo::localization::TextId::Fullscreen),
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::Download:
            return {.glyph = L"\u2B07",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = L"Download update",
                    .accent = true,
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::Replay:
            return {.glyph = L"R",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::ReplayCurrentFile),
                    .accent = true,
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
        case ButtonKind::NextItem:
            return {.glyph = L"N",
                    .imageAsset = ButtonImageAsset(kind, state, fullscreen),
                    .label = velo::localization::Text(language, velo::localization::TextId::NextItem),
                    .accent = true,
                    .textGlyph = true,
                    .largeGlyph = UsesLargeGlyph(kind)};
    }
    return {};
}

}  // namespace velo::ui
