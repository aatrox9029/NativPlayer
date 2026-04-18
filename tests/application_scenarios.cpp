#include "tests/scenario_runner_internal.h"

namespace velo::tests {

void TestLowLatencySeekHelpers(ScenarioResult& result) {
    Expect(velo::playback::ResolveMpvHwdecPolicy(L"auto") == "auto", "low-latency seek maps auto hwdec directly", result);
    Expect(velo::playback::ResolveMpvHwdecPolicy(L"auto-safe") == "auto", "low-latency seek maps legacy auto-safe hwdec to auto", result);
    Expect(velo::playback::ResolveMpvHwdecPolicy(L"auto-copy") == "auto-copy", "low-latency seek preserves auto-copy hwdec", result);
    Expect(velo::playback::ResolveMpvHwdecPolicy(L"no") == "no", "low-latency seek preserves software decode mode", result);

    const auto defaultAbsolute = velo::playback::BuildLowLatencySeekCommand(velo::playback::SeekCommandType::Absolute, false);
    const auto exactAbsolute = velo::playback::BuildLowLatencySeekCommand(velo::playback::SeekCommandType::Absolute, true);
    const auto defaultRelative = velo::playback::BuildLowLatencySeekCommand(velo::playback::SeekCommandType::Relative, false);
    Expect(defaultAbsolute.mode == "absolute+keyframes", "low-latency seek defaults absolute seeks to keyframes", result);
    Expect(exactAbsolute.mode == "absolute+exact", "low-latency seek keeps explicit exact absolute seeks available", result);
    Expect(defaultRelative.mode == "relative+keyframes", "low-latency seek keeps relative seeks on keyframes", result);
    Expect(defaultAbsolute.resumeAfterSeek && defaultRelative.resumeAfterSeek, "low-latency seek commands request immediate resume", result);
}

void TestMpvRuntimeReleaseAssetSelection(ScenarioResult& result) {
    const std::string releaseJson = R"json(
{
  "tag_name": "2026-04-17-1fea31f",
  "assets": [
    {
      "name": "mpv-dev-x86_64-20260417-git-1fea31f.7z",
      "digest": "sha256:0000",
      "browser_download_url": "https://github.com/zhongfly/mpv-winbuild/releases/download/2026-04-17-1fea31f/mpv-dev-x86_64-20260417-git-1fea31f.7z"
    },
    {
      "name": "mpv-dev-lgpl-x86_64-20260417-git-1fea31f.7z",
      "digest": "sha256:1111",
      "browser_download_url": "https://github.com/zhongfly/mpv-winbuild/releases/download/2026-04-17-1fea31f/mpv-dev-lgpl-x86_64-20260417-git-1fea31f.7z"
    },
    {
      "name": "mpv-dev-lgpl-x86_64-v3-20260417-git-1fea31f.7z",
      "digest": "sha256:2222",
      "browser_download_url": "https://github.com/zhongfly/mpv-winbuild/releases/download/2026-04-17-1fea31f/mpv-dev-lgpl-x86_64-v3-20260417-git-1fea31f.7z"
    }
  ]
}
)json";

    velo::app::MpvRuntimeReleaseAsset asset;
    Expect(velo::app::TrySelectApprovedMpvRuntimeReleaseAsset(releaseJson, asset),
           "libmpv release asset selector accepts official LGPL x64 assets", result);
    Expect(asset.releaseTag == L"2026-04-17-1fea31f",
           "libmpv release asset selector preserves the upstream release tag", result);
    Expect(asset.name == L"mpv-dev-lgpl-x86_64-20260417-git-1fea31f.7z",
           "libmpv release asset selector prefers the baseline approved x64 asset", result);
}

void TestBundledMpvRuntimeProvenance(ScenarioResult& result) {
    std::error_code error;
    const auto root = std::filesystem::temp_directory_path(error) / "nativplayer-tests" / "mpv-runtime-provenance";
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);

    const auto dllPath = root / "libmpv-2.dll";
    {
        std::ofstream dllOutput(dllPath, std::ios::binary | std::ios::trunc);
        dllOutput << "approved-lgpl-libmpv";
    }

    std::wstring dllSha256Hex;
    std::wstring hashError;
    const bool hashed = velo::common::TryComputeFileSha256Hex(dllPath, dllSha256Hex, hashError);
    Expect(hashed, "libmpv provenance test can hash the bundled runtime fixture", result);
    if (!hashed) {
        std::filesystem::remove_all(root, error);
        return;
    }

    const velo::playback::MpvRuntimeProvenance approvedProvenance{
        .sourceRepo = L"zhongfly/mpv-winbuild",
        .releaseTag = L"2026-04-17-1fea31f",
        .assetName = L"mpv-dev-lgpl-x86_64-20260417-git-1fea31f.7z",
        .assetSha256Hex = L"84784053e13f51a30b1fd663355de38f986b24f08867070d5d14ec73b3c70af5",
        .dllSha256Hex = dllSha256Hex,
        .downloadUrl = L"https://github.com/zhongfly/mpv-winbuild/releases/download/2026-04-17-1fea31f/mpv-dev-lgpl-x86_64-20260417-git-1fea31f.7z",
        .licenseId = L"LGPL-2.1-or-later",
        .commercialUseNote = L"Commercial use is allowed when LGPL obligations are satisfied.",
    };

    std::wstring writeError;
    Expect(velo::playback::WriteMpvRuntimeProvenance(dllPath, approvedProvenance, writeError),
           "libmpv provenance writer persists approval metadata", result);

    std::wstring approvalError;
    Expect(velo::playback::IsApprovedBundledMpvRuntime(dllPath, approvalError),
           "libmpv provenance validator accepts approved LGPL runtime metadata", result);

    auto rejectedProvenance = approvedProvenance;
    rejectedProvenance.assetName = L"mpv-dev-x86_64-20260417-git-1fea31f.7z";
    Expect(velo::playback::WriteMpvRuntimeProvenance(dllPath, rejectedProvenance, writeError),
           "libmpv provenance writer can overwrite metadata for negative coverage", result);
    Expect(!velo::playback::IsApprovedBundledMpvRuntime(dllPath, approvalError),
           "libmpv provenance validator rejects non-LGPL runtime metadata", result);

    std::filesystem::remove_all(root, error);
}

void TestStateNormalizer(ScenarioResult& result) {
    velo::ui::PlayerState state;
    velo::playback::StateNormalizer normalizer;
    normalizer.ApplyFileLoaded(state);
    normalizer.ApplyFlag(state, "pause", true);
    normalizer.ApplyDouble(state, "time-pos", 15.5);
    normalizer.ApplyDouble(state, "duration", 120.0);
    normalizer.ApplyDouble(state, "speed", 1.25);
    normalizer.ApplyDouble(state, "audio-delay", 0.2);
    normalizer.ApplyDouble(state, "sub-delay", -0.5);
    normalizer.ApplyInt64(state, "aid", 2);
    normalizer.ApplyInt64(state, "sid", -1);
    normalizer.ApplyInt64(state, "audio-params/channel-count", 6);
    normalizer.ApplyString(state, "media-title", "Clip");
    normalizer.ApplyString(state, "path", "https://example.com/live.m3u8");
    normalizer.ApplyString(state, "file-format", "matroska");
    normalizer.ApplyString(state, "video-codec", "hevc");
    normalizer.ApplyString(state, "audio-device", "auto");
    Expect(state.isLoaded, "state normalizer marks file loaded", result);
    Expect(state.isPaused, "state normalizer marks pause", result);
    Expect(state.positionSeconds == 15.5, "state normalizer applies position", result);
    Expect(state.playbackSpeed == 1.25, "state normalizer applies playback speed", result);
    Expect(state.audioDelaySeconds == 0.2, "state normalizer applies audio delay", result);
    Expect(state.subtitleDelaySeconds == -0.5, "state normalizer applies subtitle delay", result);
    Expect(state.audioTrackId == 2, "state normalizer applies audio track", result);
    Expect(state.subtitleTrackId == -1, "state normalizer applies subtitle track", result);
    Expect(state.audioChannelCount == 6, "state normalizer applies channel count", result);
    Expect(state.fileFormat == "matroska", "state normalizer applies file format", result);
    Expect(state.mediaTitle == "Clip", "state normalizer applies title", result);
    Expect(state.videoCodec == "hevc", "state normalizer applies video codec", result);
    Expect(state.audioOutputDeviceId == "auto", "state normalizer applies selected audio output device", result);
    Expect(state.isNetworkSource, "state normalizer detects network source", result);

    state.seekLatencyMs = 48.6;
    state.seekOptimizationProfile = "low-latency";
    state.seekMode = "absolute+keyframes";
    const auto status = velo::ui::BuildStatusText(state);
    Expect(status.find(L"Seek 48.6ms") != std::wstring::npos, "player status text shows measured seek latency", result);
    Expect(status.find(L"profile=low-latency") != std::wstring::npos, "player status text shows seek optimization profile", result);
}

void TestReleaseUpdateHelpers(ScenarioResult& result) {
    const std::string releaseJson = R"json(
{
  "tag_name": "v1.0.1",
  "html_url": "https://github.com/aatrox9029/NativPlayer/releases/tag/v1.0.1",
  "assets": [
    {
      "name": "NativPlayer-Setup.exe",
      "browser_download_url": "https://github.com/aatrox9029/NativPlayer/releases/download/v1.0.1/NativPlayer-Setup.exe"
    }
  ]
}
)json";

    velo::app::ReleaseUpdateInfo updateInfo;
    Expect(velo::app::TryParseLatestReleaseUpdate(releaseJson, updateInfo),
           "release update parser accepts latest release metadata", result);
    Expect(updateInfo.tagName == L"v1.0.1",
           "release update parser preserves tag name", result);
    Expect(updateInfo.installerDownloadUrl ==
               L"https://github.com/aatrox9029/NativPlayer/releases/download/v1.0.1/NativPlayer-Setup.exe",
           "release update parser prefers installer asset download url", result);
    Expect(velo::app::IsNewerReleaseTag(L"v1.0.1", L"v1.0.0"),
           "release update helper detects newer semantic version tags", result);
    Expect(!velo::app::IsNewerReleaseTag(L"v1.0.0", L"v1.0.0"),
           "release update helper ignores identical version tags", result);
}

void TestCommandDispatcher(ScenarioResult& result) {
    velo::playback::CommandDispatcher dispatcher;
    velo::playback::PlayerCommand command{velo::playback::CommandType::SelectAudioTrack};
    command.intValue = 3;
    dispatcher.Push(command);
    const auto item = dispatcher.WaitAndPop(10);
    Expect(item.has_value(), "command dispatcher returns queued command", result);
    Expect(item.has_value() && item->intValue == 3, "command dispatcher preserves int payload", result);
}

void TestStartupMetrics(ScenarioResult& result) {
    velo::diagnostics::StartupMetrics metrics;
    metrics.Mark("phase_window");
    metrics.MarkFirstFrame();
    const std::string report = metrics.BuildInlineSummary();
    Expect(report.find("phase_window=") != std::string::npos, "startup metrics records phase", result);
    Expect(report.find("first_frame=") != std::string::npos, "startup metrics records first frame", result);
}


void TestInputRouterCustomBindings(ScenarioResult& result) {
    velo::config::AppConfig config = velo::config::DefaultAppConfig();
    velo::platform::win32::SetVirtualKey(config, L"toggle_pause", VK_RETURN);
    velo::platform::win32::SetVirtualKey(config, L"play_next", VK_F11);
    velo::platform::win32::SetVirtualKey(config, L"take_screenshot", VK_F12);

    bool paused = false;
    bool next = false;
    bool screenshot = false;
    bool seekForward = false;
    const velo::platform::win32::InputCommandCallbacks callbacks{
        .togglePause = [&]() { paused = true; },
        .seekRelative = [&](double) {},
        .seekConfigured = [&](bool forward) { seekForward = forward; },
        .adjustVolume = [&](int) {},
        .toggleMute = [&]() {},
        .openFile = [&]() {},
        .showRecentFiles = [&]() {},
        .cycleAudioTrack = [&]() {},
        .cycleSubtitleTrack = [&]() {},
        .takeScreenshot = [&]() { screenshot = true; },
        .adjustPlaybackSpeed = [&](double) {},
        .resetPlaybackSpeed = [&]() {},
        .toggleFullscreen = [&]() {},
        .exitFullscreen = [&]() {},
        .playPrevious = [&]() {},
        .playNext = [&]() { next = true; },
        .showMediaInfo = [&]() {},
    };

    Expect(velo::platform::win32::HandleKeyDown(VK_RETURN, config, callbacks), "input router handles custom pause key", result);
    Expect(paused, "input router triggers custom pause callback", result);
    Expect(velo::platform::win32::HandleKeyDown(VK_F11, config, callbacks), "input router handles custom next key", result);
    Expect(next, "input router triggers custom next callback", result);
    Expect(velo::platform::win32::HandleKeyDown(VK_F12, config, callbacks), "input router handles screenshot key", result);
    Expect(screenshot, "input router triggers screenshot callback", result);
    Expect(!velo::platform::win32::HandleKeyDown(VK_SPACE, config, callbacks), "input router releases previous default key after remap", result);

    velo::platform::win32::SetVirtualKey(config, L"seek_forward", VK_F9);
    Expect(velo::platform::win32::HandleKeyDown(VK_F9, config, callbacks), "input router handles configured seek key", result);
    Expect(seekForward, "input router triggers configured seek callback", result);
}

void TestEndOfPlaybackPolicy(ScenarioResult& result) {
    const auto replayDecision =
        velo::app::DecideEndOfPlaybackAction(velo::config::EndOfPlaybackAction::Replay, 3, std::optional<int>{4});
    Expect(replayDecision.playbackIndex == 3, "end-of-playback replay reuses current playlist index", result);
    Expect(replayDecision.replayCurrent, "end-of-playback replay marks current item replay", result);

    const auto playNextDecision =
        velo::app::DecideEndOfPlaybackAction(velo::config::EndOfPlaybackAction::PlayNext, 3, std::optional<int>{4});
    Expect(playNextDecision.playbackIndex == 4, "end-of-playback play-next advances without extra autoplay flag", result);
    Expect(!playNextDecision.replayCurrent, "end-of-playback play-next does not mark replay", result);

    const auto stopDecision =
        velo::app::DecideEndOfPlaybackAction(velo::config::EndOfPlaybackAction::Stop, 3, std::optional<int>{4});
    Expect(!stopDecision.playbackIndex.has_value() && !stopDecision.closeWindow, "end-of-playback stop keeps playback halted", result);

    const auto closeDecision =
        velo::app::DecideEndOfPlaybackAction(velo::config::EndOfPlaybackAction::CloseWindow, 3, std::optional<int>{4});
    Expect(closeDecision.closeWindow, "end-of-playback close-window requests shutdown", result);

    velo::ui::PlayerState pendingReplayState;
    pendingReplayState.isLoaded = true;
    pendingReplayState.eofReached = true;
    Expect(!velo::app::ShouldClearPendingFileLoad(pendingReplayState),
           "end-of-playback policy ignores stale eof states while a replay load is pending", result);

    velo::ui::PlayerState loadedPlaybackState;
    loadedPlaybackState.isLoaded = true;
    loadedPlaybackState.eofReached = false;
    Expect(velo::app::ShouldClearPendingFileLoad(loadedPlaybackState),
           "end-of-playback policy clears pending load after fresh playback starts", result);

    Expect(velo::app::ShouldPauseAfterOpeningItem(true, true, false, false),
           "end-of-playback policy preserves pause on normal manual advance when configured", result);
    Expect(velo::app::ShouldPauseAfterOpeningItem(true, false, true, true),
           "end-of-playback policy pauses manual next after eof", result);
    Expect(!velo::app::ShouldPauseAfterOpeningItem(false, false, false, false),
           "end-of-playback policy keeps normal navigation playing when pause carry is not requested", result);
    Expect(!velo::app::ShouldPauseAfterOpeningItem(true, true, true, false),
           "end-of-playback autoplay path does not inherit eof pause state", result);
    Expect(velo::app::ResolvePendingResumeSeconds(true, false, 91.0).value_or(0.0) == 91.0,
           "end-of-playback policy keeps stored resume time for normal open", result);
    Expect(!velo::app::ResolvePendingResumeSeconds(true, true, 91.0).has_value(),
           "end-of-playback policy suppresses stored resume time for replay current", result);
}

void TestReplaySelectionPolicy(ScenarioResult& result) {
    velo::app::RuntimePlaylist playlist;
    playlist.SetFolderBacked(L"D:\\media", {L"episode-01.mkv", L"episode-02.mkv"});

    Expect(playlist.FindIndexByPath(L"D:\\media\\episode-01.mkv") == 0,
           "replay selection matches folder-backed playlist entries by full path", result);
    Expect(velo::app::ResolveReplayPlaylistIndex(playlist, 1, L"D:\\media\\episode-01.mkv") == 0,
           "replay selection prefers the current open path over a drifted playlist index", result);
    Expect(velo::app::ResolveReplayPlaylistIndex(playlist, 1, L"") == 1,
           "replay selection falls back to the current playlist index when no path is known", result);
}

void TestEndFileReasonPolicy(ScenarioResult& result) {
    const mpv_event_end_file eofEvent{0, 0, 0, 0, 0};
    Expect(velo::playback::IsPlaybackEofEvent(&eofEvent),
           "mpv end-file EOF reason keeps end-of-playback actions enabled", result);

    const mpv_event_end_file stopEvent{2, 0, 0, 0, 0};
    Expect(!velo::playback::IsPlaybackEofEvent(&stopEvent),
           "mpv end-file stop reason does not trigger autoplay-next logic during replay", result);
}


void RunApplicationScenarios(ScenarioResult& result) {
    TestLowLatencySeekHelpers(result);
    TestMpvRuntimeReleaseAssetSelection(result);
    TestBundledMpvRuntimeProvenance(result);
    TestReleaseUpdateHelpers(result);
    TestStateNormalizer(result);
    TestCommandDispatcher(result);
    TestStartupMetrics(result);
    TestInputRouterCustomBindings(result);
    TestEndOfPlaybackPolicy(result);
    TestReplaySelectionPolicy(result);
    TestEndFileReasonPolicy(result);
}

}  // namespace velo::tests

