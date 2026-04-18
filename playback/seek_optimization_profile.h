#pragma once

#include <string>

namespace velo::playback {

enum class SeekOptimizationTier {
    None,
    Resolution2K,
    Resolution4K,
};

struct SeekOptimizationProfile {
    SeekOptimizationTier tier = SeekOptimizationTier::None;
    bool active = false;
    bool preferKeyframeSeek = false;
    bool preferFastAbsolutePreview = false;
    bool collapseQueuedSeeks = false;
    std::string cacheMode = "yes";
    std::string demuxerReadaheadSecs = "5";
    std::string demuxerMaxBytes = "12MiB";
    std::string demuxerMaxBackBytes = "1MiB";
    std::string audioBuffer = "0.05";
    std::string skipLoopFilter = "all";
    std::string statusLabel = "low-latency";
};

SeekOptimizationProfile BuildSeekOptimizationProfile(const std::wstring& path, long long width, long long height,
                                                     bool isNetworkSource, bool isDiscSource, bool exactSeekRequested);

bool IsSeekOptimizationProfileEqual(const SeekOptimizationProfile& lhs, const SeekOptimizationProfile& rhs);

}  // namespace velo::playback
