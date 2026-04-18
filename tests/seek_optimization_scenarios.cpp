#include "tests/scenario_runner_internal.h"

#include "playback/seek_optimization_profile.h"

namespace velo::tests {
namespace {

void TestSeekOptimizationProfiles(ScenarioResult& result) {
    const auto standardProfile =
        velo::playback::BuildSeekOptimizationProfile(L"D:\\media\\clip-1080p.mp4", 1920, 1080, false, false, false);
    Expect(!standardProfile.active, "1080p local media keeps standard seek profile", result);
    Expect(standardProfile.preferKeyframeSeek, "1080p local media defaults to keyframe seek for low latency", result);
    Expect(standardProfile.preferFastAbsolutePreview, "1080p local media enables fast absolute preview seeks", result);
    Expect(standardProfile.collapseQueuedSeeks, "1080p local media collapses queued seeks", result);
    Expect(standardProfile.cacheMode == "yes", "1080p local media enables cache for low-latency seek", result);
    Expect(standardProfile.demuxerReadaheadSecs == "5", "1080p low-latency profile keeps 5-second readahead", result);
    Expect(standardProfile.skipLoopFilter == "all", "1080p low-latency profile skips loop filtering", result);
    Expect(standardProfile.statusLabel == "low-latency", "1080p local media reports the expected low-latency label", result);

    const auto profile2K =
        velo::playback::BuildSeekOptimizationProfile(L"D:\\media\\clip-1440p.mkv", 2560, 1440, false, false, false);
    Expect(profile2K.active, "2K local media enables fast seek optimization", result);
    Expect(profile2K.preferKeyframeSeek, "2K local media prefers keyframe seek", result);
    Expect(profile2K.preferFastAbsolutePreview, "2K local media uses fast absolute preview seeks during timeline drag", result);
    Expect(profile2K.collapseQueuedSeeks, "2K local media collapses queued seeks", result);
    Expect(profile2K.cacheMode == "yes", "2K local media enables demuxer cache", result);
    Expect(profile2K.demuxerReadaheadSecs == "5", "2K local media keeps the low-latency readahead budget", result);
    Expect(profile2K.skipLoopFilter == "all", "2K local media keeps the low-latency decode skip setting", result);
    Expect(profile2K.statusLabel == "2k-low-latency", "2K local media reports the expected seek profile label", result);

    const auto profile4K =
        velo::playback::BuildSeekOptimizationProfile(L"D:\\media\\clip-4k.mkv", 3840, 2160, false, false, false);
    Expect(profile4K.active, "4K local media enables fast seek optimization", result);
    Expect(profile4K.demuxerMaxBytes == "96MiB", "4K local media increases demuxer cache budget", result);
    Expect(profile4K.demuxerMaxBackBytes == "48MiB", "4K local media increases backward demuxer budget", result);
    Expect(profile4K.preferKeyframeSeek, "4K local media keeps relative seeks on keyframes", result);
    Expect(profile4K.preferFastAbsolutePreview, "4K local media uses keyframe preview seeks while dragging", result);
    Expect(profile4K.demuxerReadaheadSecs == "5", "4K local media keeps the low-latency readahead budget", result);
    Expect(profile4K.skipLoopFilter == "all", "4K local media keeps the low-latency decode skip setting", result);

    const auto networkProfile =
        velo::playback::BuildSeekOptimizationProfile(L"https://example.com/clip-4k.mkv", 3840, 2160, false, false, false);
    Expect(!networkProfile.active, "network 4K media avoids aggressive local seek optimization", result);
    Expect(networkProfile.preferKeyframeSeek, "network 4K media keeps low-latency keyframe seeks by default", result);
    Expect(networkProfile.preferFastAbsolutePreview, "network 4K media still uses fast absolute preview seeks", result);
    Expect(networkProfile.cacheMode == "yes", "network 4K media keeps cache enabled for low-latency seek", result);
}

void TestSeekCommandCollapsing(ScenarioResult& result) {
    velo::playback::CommandDispatcher dispatcher;
    dispatcher.Push({velo::playback::CommandType::SeekAbsolute, L"", 10.0, false});
    dispatcher.Push({velo::playback::CommandType::SeekAbsolute, L"", 25.0, false});
    dispatcher.Push({velo::playback::CommandType::SeekAbsolute, L"", 40.0, false});
    dispatcher.Push({velo::playback::CommandType::SetVolume, L"", 30.0, false});

    auto command = dispatcher.WaitAndPop(1);
    Expect(command.has_value(), "seek dispatcher returns queued command", result);
    const auto collapsed = dispatcher.CollapsePendingSeekCommands(std::move(*command));
    Expect(collapsed.type == velo::playback::CommandType::SeekAbsolute && std::abs(collapsed.numberValue - 40.0) < 0.001,
           "seek dispatcher keeps only the latest pending seek", result);

    auto remaining = dispatcher.TryPop();
    Expect(remaining.has_value() && remaining->type == velo::playback::CommandType::SetVolume,
           "seek collapsing preserves the first non-seek command", result);
}

}  // namespace

void RunSeekOptimizationScenarios(ScenarioResult& result) {
    AppendHeadlessTrace("seek_optimization:profiles:start");
    TestSeekOptimizationProfiles(result);
    AppendHeadlessTrace("seek_optimization:profiles:end");
    AppendHeadlessTrace("seek_optimization:dispatcher:start");
    TestSeekCommandCollapsing(result);
    AppendHeadlessTrace("seek_optimization:dispatcher:end");
}

}  // namespace velo::tests
