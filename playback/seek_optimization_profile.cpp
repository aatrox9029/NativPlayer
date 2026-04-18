#include "playback/seek_optimization_profile.h"

#include <algorithm>

namespace velo::playback {
namespace {

std::wstring ToLowerAscii(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const wchar_t ch) {
        if (ch >= L'A' && ch <= L'Z') {
            return static_cast<wchar_t>(ch - L'A' + L'a');
        }
        return ch;
    });
    return value;
}

bool HasPrefixInsensitive(const std::wstring& value, const std::wstring& prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    return ToLowerAscii(value.substr(0, prefix.size())) == ToLowerAscii(prefix);
}

bool IsResolutionAtLeast2K(const long long width, const long long height) {
    const long long clampedWidth = std::max<long long>(0, width);
    const long long clampedHeight = std::max<long long>(0, height);
    return clampedWidth >= 2048 || clampedHeight >= 1440 || (clampedWidth * clampedHeight) >= 3686400;
}

bool IsResolutionAtLeast4K(const long long width, const long long height) {
    const long long clampedWidth = std::max<long long>(0, width);
    const long long clampedHeight = std::max<long long>(0, height);
    return clampedWidth >= 3840 || clampedHeight >= 2160 || (clampedWidth * clampedHeight) >= 8294400;
}

bool IsLocalSeekOptimizationCandidate(const std::wstring& path, const bool isNetworkSource, const bool isDiscSource) {
    const bool looksLikeNetworkSource = HasPrefixInsensitive(path, L"http://") || HasPrefixInsensitive(path, L"https://") ||
                                        HasPrefixInsensitive(path, L"rtsp://") || HasPrefixInsensitive(path, L"rtmp://") ||
                                        HasPrefixInsensitive(path, L"mms://") || HasPrefixInsensitive(path, L"udp://") ||
                                        HasPrefixInsensitive(path, L"tcp://") || HasPrefixInsensitive(path, L"smb://") ||
                                        HasPrefixInsensitive(path, L"ftp://") || HasPrefixInsensitive(path, L"magnet:");
    const bool looksLikeDiscSource = HasPrefixInsensitive(path, L"dvd://") || HasPrefixInsensitive(path, L"bd://") ||
                                     HasPrefixInsensitive(path, L"bluray://") || HasPrefixInsensitive(path, L"cdda://");
    return !path.empty() && !isNetworkSource && !isDiscSource && !looksLikeNetworkSource && !looksLikeDiscSource;
}

}  // namespace

SeekOptimizationProfile BuildSeekOptimizationProfile(const std::wstring& path, const long long width, const long long height,
                                                     const bool isNetworkSource, const bool isDiscSource,
                                                     const bool exactSeekRequested) {
    SeekOptimizationProfile profile;
    profile.preferKeyframeSeek = !exactSeekRequested;
    profile.preferFastAbsolutePreview = true;
    profile.collapseQueuedSeeks = true;

    if (!IsLocalSeekOptimizationCandidate(path, isNetworkSource, isDiscSource)) {
        return profile;
    }

    if (IsResolutionAtLeast4K(width, height)) {
        profile.tier = SeekOptimizationTier::Resolution4K;
        profile.active = true;
        profile.preferKeyframeSeek = true;
        profile.preferFastAbsolutePreview = true;
        profile.collapseQueuedSeeks = true;
        profile.cacheMode = "yes";
        profile.demuxerMaxBytes = "96MiB";
        profile.demuxerMaxBackBytes = "48MiB";
        profile.audioBuffer = "0.03";
        profile.statusLabel = "4k-low-latency";
        return profile;
    }

    if (IsResolutionAtLeast2K(width, height)) {
        profile.tier = SeekOptimizationTier::Resolution2K;
        profile.active = true;
        profile.preferKeyframeSeek = true;
        profile.preferFastAbsolutePreview = true;
        profile.collapseQueuedSeeks = true;
        profile.cacheMode = "yes";
        profile.demuxerMaxBytes = "48MiB";
        profile.demuxerMaxBackBytes = "24MiB";
        profile.audioBuffer = "0.04";
        profile.statusLabel = "2k-low-latency";
        return profile;
    }

    return profile;
}

bool IsSeekOptimizationProfileEqual(const SeekOptimizationProfile& lhs, const SeekOptimizationProfile& rhs) {
    return lhs.tier == rhs.tier && lhs.active == rhs.active && lhs.preferKeyframeSeek == rhs.preferKeyframeSeek &&
           lhs.preferFastAbsolutePreview == rhs.preferFastAbsolutePreview &&
           lhs.collapseQueuedSeeks == rhs.collapseQueuedSeeks && lhs.cacheMode == rhs.cacheMode &&
           lhs.demuxerReadaheadSecs == rhs.demuxerReadaheadSecs &&
           lhs.demuxerMaxBytes == rhs.demuxerMaxBytes && lhs.demuxerMaxBackBytes == rhs.demuxerMaxBackBytes &&
           lhs.audioBuffer == rhs.audioBuffer && lhs.skipLoopFilter == rhs.skipLoopFilter &&
           lhs.statusLabel == rhs.statusLabel;
}

}  // namespace velo::playback
