#pragma once

#include <string>
#include <vector>

namespace velo::ui {

struct PlayerTrackOption {
    long long id = -1;
    std::string label;
    std::string language;
    std::string codec;
    bool selected = false;
    bool external = false;
};

struct AudioOutputOption {
    std::string id;
    std::string label;
    bool selected = false;
};

struct PlayerState {
    bool isLoaded = false;
    bool isPaused = false;
    bool isMuted = false;
    bool isBuffering = false;
    bool eofReached = false;
    bool isNetworkSource = false;
    bool isDiscSource = false;
    bool audioDeviceDriven = false;
    bool decoderFallbackActive = false;
    bool zeroCopyVideoPathActive = false;
    bool exactSeek = false;
    bool highResolutionSeekOptimizationActive = false;
    double positionSeconds = 0.0;
    double durationSeconds = 0.0;
    double volume = 100.0;
    double playbackSpeed = 1.0;
    double audioDelaySeconds = 0.0;
    double subtitleDelaySeconds = 0.0;
    double avSyncSeconds = 0.0;
    double cacheBufferSeconds = 0.0;
    double cacheSpeedKbps = 0.0;
    double estimatedFps = 0.0;
    double containerFps = 0.0;
    double hdrPeak = 0.0;
    double decodeLatencyMs = 0.0;
    double renderLatencyMs = 0.0;
    double presentJitterMs = 0.0;
    double startupLatencyMs = 0.0;
    double seekLatencyMs = 0.0;
    double nativeMemoryUsedMb = 0.0;
    double nativeMemoryPeakMb = 0.0;
    double gpuMemoryUsedMb = 0.0;
    double gpuMemoryPeakMb = 0.0;
    double gpuMemoryBudgetMb = 0.0;
    double audioBufferSeconds = 0.0;
    double audioClockDriftMs = 0.0;
    double audioClockPeakDriftMs = 0.0;
    double audioDeviceLatencyMs = 0.0;
    long long audioTrackId = -1;
    long long subtitleTrackId = -1;
    long long droppedFrameCount = 0;
    long long decoderDropCount = 0;
    long long audioChannelCount = 0;
    long long audioSampleRate = 0;
    long long videoWidth = 0;
    long long videoHeight = 0;
    long long videoBitDepth = 0;
    long long packetQueueDepth = 0;
    long long frameQueueDepth = 0;
    long long rebufferCount = 0;
    long long resourcePoolHits = 0;
    long long resourcePoolMisses = 0;
    long long decoderSurfacePoolSharedInUse = 0;
    long long decoderSurfacePoolTransferInUse = 0;
    long long decoderSurfacePoolUploadInUse = 0;
    long long texturePoolHits = 0;
    long long texturePoolMisses = 0;
    long long shaderPassCount = 0;
    long long shaderCacheHits = 0;
    long long shaderCacheMisses = 0;
    long long shaderDrawCount = 0;
    long long subtitleAtlasPageCount = 0;
    long long subtitleAtlasGlyphCount = 0;
    long long subtitleAtlasHits = 0;
    long long subtitleAtlasMisses = 0;
    long long subtitleCompositeCount = 0;
    long long swapchainPresentCount = 0;
    long long audioUnderrunCount = 0;
    long long audioClockCorrectionCount = 0;
    std::string mediaTitle;
    std::string currentPath;
    std::string fileFormat;
    std::string hwdecCurrent;
    std::string videoCodec;
    std::string audioCodec;
    std::string audioLanguage;
    std::string subtitleLanguage;
    std::string renderer;
    std::string decodePath;
    std::string decoderSurfacePool;
    std::string audioOutput;
    std::string pixelFormat;
    std::string colorPrimaries;
    std::string colorTransfer;
    std::string colorMatrix;
    std::string hdrType;
    std::string subtitleEncoding;
    std::string networkProtocol;
    std::string audioOutputDeviceId;
    std::string seekMode;
    std::string seekOptimizationProfile;
    std::string errorState;
    std::vector<PlayerTrackOption> audioTracks;
    std::vector<PlayerTrackOption> subtitleTracks;
    std::vector<AudioOutputOption> audioOutputs;
};

std::wstring FormatTimeLabel(double seconds);
std::wstring BuildStatusText(const PlayerState& state);

}  // namespace velo::ui
