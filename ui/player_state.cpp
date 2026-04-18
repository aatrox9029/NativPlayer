#include "ui/player_state.h"

#include <Windows.h>

#include <cmath>
#include <iomanip>
#include <sstream>

#include "common/text_encoding.h"
#include "ui/error_messages.h"

namespace velo::ui {
namespace {

std::wstring ToWide(const std::string& value) {
    return velo::common::Utf8ToWide(value);
}

std::wstring SelectedTrackLabel(const std::vector<PlayerTrackOption>& options, const long long trackId) {
    if (trackId < 0) {
        return L"off";
    }
    for (const auto& option : options) {
        if (option.id == trackId) {
            return option.label.empty() ? std::to_wstring(trackId) : ToWide(option.label);
        }
    }
    return std::to_wstring(trackId);
}

}  // namespace

std::wstring FormatTimeLabel(const double seconds) {
    const int totalSeconds = seconds < 0.0 ? 0 : static_cast<int>(seconds);
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int secs = totalSeconds % 60;

    std::wostringstream stream;
    if (hours > 0) {
        stream << hours << L':' << std::setw(2) << std::setfill(L'0') << minutes << L':' << std::setw(2)
               << std::setfill(L'0') << secs;
        return stream.str();
    }

    stream << minutes << L':' << std::setw(2) << std::setfill(L'0') << secs;
    return stream.str();
}

std::wstring BuildStatusText(const PlayerState& state) {
    if (!state.errorState.empty()) {
        return L"Error: " + FriendlyPlaybackError(state.errorState);
    }
    if (!state.isLoaded) {
        return L"Press O to open media";
    }

    std::wstring text = state.isBuffering ? L"Buffering" : (state.isPaused ? L"Paused" : L"Playing");
    text += L"  ";
    text += FormatTimeLabel(state.positionSeconds);
    text += L" / ";
    text += FormatTimeLabel(state.durationSeconds);
    text += L"  Vol ";
    text += std::to_wstring(static_cast<int>(state.volume));
    if (std::abs(state.playbackSpeed - 1.0) > 0.01) {
        std::wostringstream speed;
        speed << std::fixed << std::setprecision(2) << state.playbackSpeed;
        text += L"  Speed ";
        text += speed.str();
        text += L"x";
    }
    text += L"  A ";
    text += SelectedTrackLabel(state.audioTracks, state.audioTrackId);
    text += L"  S ";
    text += SelectedTrackLabel(state.subtitleTracks, state.subtitleTrackId);

    if (std::abs(state.audioDelaySeconds) > 0.01) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(2) << state.audioDelaySeconds;
        text += L"  A.Delay ";
        text += stream.str();
        text += L"s";
    }
    if (std::abs(state.subtitleDelaySeconds) > 0.01) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(2) << state.subtitleDelaySeconds;
        text += L"  S.Delay ";
        text += stream.str();
        text += L"s";
    }
    if (state.packetQueueDepth > 0 || state.frameQueueDepth > 0) {
        text += L"  Q ";
        text += std::to_wstring(state.packetQueueDepth);
        text += L"/";
        text += std::to_wstring(state.frameQueueDepth);
    }
    if (state.nativeMemoryUsedMb > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.nativeMemoryUsedMb;
        text += L"  Mem ";
        text += stream.str();
        text += L"MB";
    }
    if (state.gpuMemoryUsedMb > 0.0 || state.gpuMemoryBudgetMb > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.gpuMemoryUsedMb;
        text += L"  GPU ";
        text += stream.str();
        if (state.gpuMemoryBudgetMb > 0.0) {
            stream.str(L"");
            stream.clear();
            stream << std::fixed << std::setprecision(1) << state.gpuMemoryBudgetMb;
            text += L"/";
            text += stream.str();
        }
        text += L"MB";
    }
    if (state.shaderPassCount > 0) {
        text += L"  FX ";
        text += std::to_wstring(state.shaderPassCount);
    }
    if (state.shaderDrawCount > 0) {
        text += L"  Draw ";
        text += std::to_wstring(state.shaderDrawCount);
    }
    if (state.subtitleAtlasGlyphCount > 0) {
        text += L"  SubAtlas ";
        text += std::to_wstring(state.subtitleAtlasGlyphCount);
    }
    if (state.audioBufferSeconds > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(2) << state.audioBufferSeconds;
        text += L"  ABuf ";
        text += stream.str();
        text += L"s";
    }
    if (state.audioDeviceLatencyMs > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.audioDeviceLatencyMs;
        text += L"  ALat ";
        text += stream.str();
        text += L"ms";
    }
    if (state.decodeLatencyMs > 0.0 || state.renderLatencyMs > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.decodeLatencyMs;
        text += L"  Dec ";
        text += stream.str();
        text += L"ms";
        stream.str(L"");
        stream.clear();
        stream << std::fixed << std::setprecision(1) << state.renderLatencyMs;
        text += L"  Ren ";
        text += stream.str();
        text += L"ms";
    }
    if (state.presentJitterMs > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.presentJitterMs;
        text += L"  Jit ";
        text += stream.str();
        text += L"ms";
    }
    if (state.seekLatencyMs > 0.0) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.seekLatencyMs;
        text += L"  Seek ";
        text += stream.str();
        text += L"ms";
    }
    if (std::abs(state.audioClockDriftMs) > 0.1) {
        std::wostringstream stream;
        stream << std::fixed << std::setprecision(1) << state.audioClockDriftMs;
        text += L"  Drift ";
        text += stream.str();
        text += L"ms";
    }
    if (!state.fileFormat.empty()) {
        text += L"  ";
        text += ToWide(state.fileFormat);
    }
    if (!state.hwdecCurrent.empty()) {
        text += L"  hwdec=";
        text += ToWide(state.hwdecCurrent);
    }
    if (!state.decodePath.empty()) {
        text += L"  path=";
        text += ToWide(state.decodePath);
    }
    if (!state.seekOptimizationProfile.empty()) {
        text += L"  profile=";
        text += ToWide(state.seekOptimizationProfile);
    }
    if (!state.seekMode.empty()) {
        text += L"  seek=";
        text += ToWide(state.seekMode);
    }
    if (state.zeroCopyVideoPathActive) {
        text += L"  zero-copy";
    } else if (state.decoderFallbackActive) {
        text += L"  fallback";
    }
    if (!state.videoCodec.empty()) {
        text += L"  v=";
        text += ToWide(state.videoCodec);
    }
    if (!state.audioCodec.empty()) {
        text += L"  a=";
        text += ToWide(state.audioCodec);
    }
    if (!state.hdrType.empty()) {
        text += L"  HDR=";
        text += ToWide(state.hdrType);
    }
    if (state.isNetworkSource) {
        text += L"  stream";
    }
    if (!state.mediaTitle.empty()) {
        text += L"  ";
        text += ToWide(state.mediaTitle);
    }
    return text;
}

}  // namespace velo::ui
