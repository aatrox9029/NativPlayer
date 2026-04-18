#include "playback/state_normalizer.h"

#include <algorithm>
#include <cmath>

namespace velo::playback {
namespace {

bool HasPrefix(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

}  // namespace

void StateNormalizer::ApplyFlag(velo::ui::PlayerState& state, const std::string& name, const bool value) const {
    if (name == "pause") {
        state.isPaused = value;
    } else if (name == "mute") {
        state.isMuted = value;
    } else if (name == "eof-reached") {
        state.eofReached = value;
    }
}

void StateNormalizer::ApplyDouble(velo::ui::PlayerState& state, const std::string& name, const double value) const {
    if (name == "time-pos") {
        state.positionSeconds = std::max(0.0, value);
    } else if (name == "duration") {
        state.durationSeconds = std::max(0.0, value);
    } else if (name == "volume") {
        state.volume = std::clamp(value, 0.0, 100.0);
    } else if (name == "speed") {
        state.playbackSpeed = std::clamp(value, 0.25, 3.0);
    } else if (name == "audio-delay") {
        state.audioDelaySeconds = value;
    } else if (name == "sub-delay") {
        state.subtitleDelaySeconds = value;
    } else if (name == "avsync") {
        state.avSyncSeconds = value;
    } else if (name == "demuxer-cache-duration") {
        state.cacheBufferSeconds = std::max(0.0, value);
    } else if (name == "cache-speed") {
        state.cacheSpeedKbps = std::max(0.0, value);
    } else if (name == "estimated-vf-fps") {
        state.estimatedFps = std::max(0.0, value);
    } else if (name == "container-fps") {
        state.containerFps = std::max(0.0, value);
    } else if (name == "video-params/sig-peak") {
        state.hdrPeak = std::max(0.0, value);
    }
}

void StateNormalizer::ApplyInt64(velo::ui::PlayerState& state, const std::string& name, const int64_t value) const {
    if (name == "aid") {
        state.audioTrackId = value;
    } else if (name == "sid") {
        state.subtitleTrackId = value;
    } else if (name == "drop-frame-count") {
        state.droppedFrameCount = std::max<int64_t>(0, value);
    } else if (name == "decoder-frame-drop-count") {
        state.decoderDropCount = std::max<int64_t>(0, value);
    } else if (name == "audio-params/channel-count") {
        state.audioChannelCount = std::max<int64_t>(0, value);
    } else if (name == "audio-params/samplerate") {
        state.audioSampleRate = std::max<int64_t>(0, value);
    } else if (name == "video-params/w") {
        state.videoWidth = std::max<int64_t>(0, value);
    } else if (name == "video-params/h") {
        state.videoHeight = std::max<int64_t>(0, value);
    } else if (name == "video-params/bit-depth") {
        state.videoBitDepth = std::max<int64_t>(0, value);
    }
}

void StateNormalizer::ApplyString(velo::ui::PlayerState& state, const std::string& name, std::string value) const {
    if (name == "media-title") {
        state.mediaTitle = std::move(value);
    } else if (name == "path") {
        state.currentPath = std::move(value);
        state.isNetworkSource = HasPrefix(state.currentPath, "http://") || HasPrefix(state.currentPath, "https://") ||
                                HasPrefix(state.currentPath, "rtsp://") || HasPrefix(state.currentPath, "rtmp://");
        state.isDiscSource = HasPrefix(state.currentPath, "dvd://") || HasPrefix(state.currentPath, "bd://") ||
                             HasPrefix(state.currentPath, "bluray://");
    } else if (name == "file-format") {
        state.fileFormat = std::move(value);
    } else if (name == "hwdec-current") {
        state.hwdecCurrent = std::move(value);
    } else if (name == "error") {
        state.errorState = std::move(value);
    } else if (name == "video-codec") {
        state.videoCodec = std::move(value);
    } else if (name == "audio-codec-name") {
        state.audioCodec = std::move(value);
    } else if (name == "current-tracks/audio/lang") {
        state.audioLanguage = std::move(value);
    } else if (name == "current-tracks/sub/lang") {
        state.subtitleLanguage = std::move(value);
    } else if (name == "current-vo") {
        state.renderer = std::move(value);
    } else if (name == "current-ao") {
        state.audioOutput = std::move(value);
    } else if (name == "audio-device") {
        state.audioOutputDeviceId = std::move(value);
    } else if (name == "video-params/pixelformat") {
        state.pixelFormat = std::move(value);
    } else if (name == "video-params/primaries") {
        state.colorPrimaries = std::move(value);
    } else if (name == "video-params/gamma") {
        state.colorTransfer = std::move(value);
    } else if (name == "video-params/colormatrix") {
        state.colorMatrix = std::move(value);
    } else if (name == "video-params/hdr-type") {
        state.hdrType = std::move(value);
    } else if (name == "sub-codepage") {
        state.subtitleEncoding = std::move(value);
    } else if (name == "stream-open-filename") {
        state.networkProtocol = std::move(value);
    }
}

void StateNormalizer::ApplyPlaybackRestart(velo::ui::PlayerState& state) const {
    state.isBuffering = false;
    state.eofReached = false;
}

void StateNormalizer::ApplyFileLoaded(velo::ui::PlayerState& state) const {
    state.isLoaded = true;
    state.isBuffering = false;
    state.errorState.clear();
    state.eofReached = false;
}

void StateNormalizer::ApplyLoadFailed(velo::ui::PlayerState& state, const std::string& error) const {
    state.isLoaded = false;
    state.isBuffering = false;
    state.errorState = error;
}

}  // namespace velo::playback
