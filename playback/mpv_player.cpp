#include "playback/mpv_player.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>

#include "common/text_encoding.h"
#include "playback/end_file_reason.h"
#include "playback/low_latency_seek.h"
#include "playback/subtitle_layout.h"
#include "playback/video_filter_profile.h"

namespace velo::playback {
namespace {

std::string ToUtf8(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<char>(ch - 'A' + 'a');
        }
        return static_cast<char>(ch);
    });
    return value;
}

bool LooksLikeHardwareOutputFailure(const std::string& text) {
    const std::string normalized = ToLowerAscii(text);
    return normalized.find("gpu") != std::string::npos || normalized.find("hwdec") != std::string::npos ||
           normalized.find("vulkan") != std::string::npos || normalized.find("d3d") != std::string::npos ||
           normalized.find("opengl") != std::string::npos || normalized.find("vo") != std::string::npos;
}

bool HasPrefixInsensitive(const std::wstring& value, const std::wstring& prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }

    const std::wstring head = value.substr(0, prefix.size());
    return ToLowerAscii(ToUtf8(head)) == ToLowerAscii(ToUtf8(prefix));
}

bool IsNetworkLikePath(const std::wstring& path) {
    return HasPrefixInsensitive(path, L"http://") || HasPrefixInsensitive(path, L"https://") ||
           HasPrefixInsensitive(path, L"rtsp://") || HasPrefixInsensitive(path, L"rtmp://") ||
           HasPrefixInsensitive(path, L"mms://") || HasPrefixInsensitive(path, L"udp://") ||
           HasPrefixInsensitive(path, L"tcp://") || HasPrefixInsensitive(path, L"smb://") ||
           HasPrefixInsensitive(path, L"ftp://") || HasPrefixInsensitive(path, L"magnet:");
}

bool IsDiscLikePath(const std::wstring& path) {
    return HasPrefixInsensitive(path, L"dvd://") || HasPrefixInsensitive(path, L"bd://") ||
           HasPrefixInsensitive(path, L"bluray://") || HasPrefixInsensitive(path, L"cdda://");
}

const struct mpv_node* FindMapNodeValue(const struct mpv_node& node, const char* key) {
    if (key == nullptr || node.format != MPV_FORMAT_NODE_MAP || node.u.list == nullptr) {
        return nullptr;
    }
    for (int index = 0; index < node.u.list->num; ++index) {
        const char* entryKey = node.u.list->keys != nullptr ? node.u.list->keys[index] : nullptr;
        if (entryKey != nullptr && std::string_view(entryKey) == key) {
            return &node.u.list->values[index];
        }
    }
    return nullptr;
}

std::string NodeString(const struct mpv_node* node) {
    if (node == nullptr) {
        return {};
    }
    switch (node->format) {
        case MPV_FORMAT_STRING:
            return node->u.string != nullptr ? node->u.string : std::string{};
        case MPV_FORMAT_INT64:
            return std::to_string(node->u.int64);
        case MPV_FORMAT_DOUBLE: {
            std::ostringstream stream;
            stream << node->u.double_;
            return stream.str();
        }
        case MPV_FORMAT_FLAG:
            return node->u.flag != 0 ? "yes" : "no";
        default:
            return {};
    }
}

long long NodeInt64(const struct mpv_node* node, const long long fallback = -1) {
    if (node == nullptr) {
        return fallback;
    }
    if (node->format == MPV_FORMAT_INT64) {
        return node->u.int64;
    }
    if (node->format == MPV_FORMAT_DOUBLE) {
        return static_cast<long long>(node->u.double_);
    }
    return fallback;
}

bool NodeFlag(const struct mpv_node* node, const bool fallback = false) {
    if (node == nullptr) {
        return fallback;
    }
    if (node->format == MPV_FORMAT_FLAG) {
        return node->u.flag != 0;
    }
    if (node->format == MPV_FORMAT_INT64) {
        return node->u.int64 != 0;
    }
    return fallback;
}

std::string BuildTrackLabel(const std::string& type, const long long id, const std::string& title, const std::string& language,
                            const std::string& codec, const bool external) {
    if (!title.empty()) {
        return title;
    }

    std::ostringstream stream;
    stream << (type == "audio" ? "Audio" : "Subtitle") << ' ' << id;
    if (!language.empty()) {
        stream << " [" << language << ']';
    }
    if (!codec.empty()) {
        stream << " - " << codec;
    }
    if (external) {
        stream << " (external)";
    }
    return stream.str();
}

void SyncTrackSelections(velo::ui::PlayerState& state) {
    for (auto& option : state.audioTracks) {
        option.selected = option.id == state.audioTrackId;
    }
    for (auto& option : state.subtitleTracks) {
        option.selected = option.id == state.subtitleTrackId;
    }
    for (auto& option : state.audioOutputs) {
        option.selected = option.id == state.audioOutputDeviceId;
    }
}

void ApplyTrackListNode(velo::ui::PlayerState& state, const struct mpv_node& node) {
    state.audioTracks.clear();
    state.subtitleTracks.clear();
    if (node.format != MPV_FORMAT_NODE_ARRAY || node.u.list == nullptr) {
        return;
    }

    for (int index = 0; index < node.u.list->num; ++index) {
        const struct mpv_node& entry = node.u.list->values[index];
        const std::string type = NodeString(FindMapNodeValue(entry, "type"));
        if (type != "audio" && type != "sub") {
            continue;
        }

        velo::ui::PlayerTrackOption option;
        option.id = NodeInt64(FindMapNodeValue(entry, "id"));
        option.language = NodeString(FindMapNodeValue(entry, "lang"));
        option.codec = NodeString(FindMapNodeValue(entry, "codec"));
        option.external = NodeFlag(FindMapNodeValue(entry, "external"));
        option.selected = NodeFlag(FindMapNodeValue(entry, "selected"));
        option.label = BuildTrackLabel(type == "audio" ? "audio" : "subtitle", option.id,
                                       NodeString(FindMapNodeValue(entry, "title")), option.language, option.codec,
                                       option.external);
        if (type == "audio") {
            state.audioTracks.push_back(std::move(option));
        } else {
            state.subtitleTracks.push_back(std::move(option));
        }
    }
    SyncTrackSelections(state);
}

void ApplyAudioDeviceListNode(velo::ui::PlayerState& state, const struct mpv_node& node) {
    state.audioOutputs.clear();
    if (node.format != MPV_FORMAT_NODE_ARRAY || node.u.list == nullptr) {
        return;
    }

    for (int index = 0; index < node.u.list->num; ++index) {
        const struct mpv_node& entry = node.u.list->values[index];
        velo::ui::AudioOutputOption option;
        option.id = NodeString(FindMapNodeValue(entry, "name"));
        option.label = NodeString(FindMapNodeValue(entry, "description"));
        if (option.label.empty()) {
            option.label = option.id;
        }
        option.selected = NodeFlag(FindMapNodeValue(entry, "current"));
        if (!option.id.empty()) {
            state.audioOutputs.push_back(std::move(option));
        }
    }
    SyncTrackSelections(state);
}

std::wstring SanitizeSubtitleFontName(std::wstring fontName) {
    if (!fontName.empty() && fontName.front() == L'@') {
        fontName.erase(fontName.begin());
    }
    return fontName;
}

}  // namespace

void MpvPlayer::SetStateCallback(StateCallback callback) {
    stateCallback_ = std::move(callback);
}

bool MpvPlayer::Initialize(HWND videoWindow, const std::wstring& hwdecPolicy, velo::diagnostics::Logger* logger,
                           velo::diagnostics::StartupMetrics* metrics, const bool lightweightMode) {
    logger_ = logger;
    metrics_ = metrics;
    lightweightMode_ = lightweightMode;
    activeHwdecPolicy_ = hwdecPolicy;

    if (!loader_.Load()) {
        const std::string diagnostic = loader_.LastLoadDiagnostic();
        normalizer_.ApplyLoadFailed(state_, diagnostic.empty() ? "Failed to load libmpv" : diagnostic);
        EmitState();
        return false;
    }

    handle_ = loader_.create();
    if (handle_ == nullptr) {
        normalizer_.ApplyLoadFailed(state_, "mpv_create failed");
        EmitState();
        return false;
    }

    std::ostringstream wid;
    wid << reinterpret_cast<uintptr_t>(videoWindow);
    SetInitialOption("wid", wid.str());
    SetInitialOption("terminal", "no");
    SetInitialOption("config", "no");
    SetInitialOption("osc", "no");
    SetInitialOption("keep-open", "yes");
    SetInitialOption("input-default-bindings", "no");
    SetInitialOption("input-vo-keyboard", "no");
    SetInitialOption("cache", "yes");
    SetInitialOption("demuxer-readahead-secs", "5");
    SetInitialOption("demuxer-max-bytes", lightweightMode_ ? "4MiB" : "12MiB");
    SetInitialOption("demuxer-max-back-bytes", lightweightMode_ ? "512KiB" : "1MiB");
    SetInitialOption("audio-buffer", lightweightMode_ ? "0.05" : "0.10");
    SetInitialOption("vd-lavc-skiploopfilter", "all");
    SetInitialOption("hr-seek", "no");
    SetInitialOption("video-latency-hacks", lightweightMode_ ? "no" : "yes");
    SetInitialOption("hwdec", ResolveMpvHwdecPolicy(hwdecPolicy));

    const int initializeResult = loader_.initialize(handle_);
    if (initializeResult < 0) {
        normalizer_.ApplyLoadFailed(state_, "libmpv initialize failed: " + ErrorText(initializeResult));
        EmitState();
        return false;
    }

    loader_.observeProperty(handle_, 0, "pause", MPV_FORMAT_FLAG);
    loader_.observeProperty(handle_, 0, "time-pos", MPV_FORMAT_DOUBLE);
    loader_.observeProperty(handle_, 0, "duration", MPV_FORMAT_DOUBLE);
    if (!lightweightMode_) {
        loader_.observeProperty(handle_, 0, "volume", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "speed", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "audio-delay", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "sub-delay", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "avsync", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "demuxer-cache-duration", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "cache-speed", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "estimated-vf-fps", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "container-fps", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "video-params/sig-peak", MPV_FORMAT_DOUBLE);
        loader_.observeProperty(handle_, 0, "mute", MPV_FORMAT_FLAG);
        loader_.observeProperty(handle_, 0, "aid", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "sid", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "eof-reached", MPV_FORMAT_FLAG);
        loader_.observeProperty(handle_, 0, "drop-frame-count", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "decoder-frame-drop-count", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "audio-params/channel-count", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "audio-params/samplerate", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "video-params/w", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "video-params/h", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "video-params/bit-depth", MPV_FORMAT_INT64);
        loader_.observeProperty(handle_, 0, "media-title", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "path", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "file-format", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "hwdec-current", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "video-codec", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "audio-codec-name", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "current-vo", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "current-ao", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "audio-device", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "video-params/pixelformat", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "video-params/primaries", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "video-params/gamma", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "video-params/colormatrix", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "video-params/hdr-type", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "sub-codepage", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "stream-open-filename", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "current-tracks/audio/lang", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "current-tracks/sub/lang", MPV_FORMAT_STRING);
        loader_.observeProperty(handle_, 0, "track-list", MPV_FORMAT_NODE);
        loader_.observeProperty(handle_, 0, "audio-device-list", MPV_FORMAT_NODE);
    }

    if (logger_ != nullptr) {
        logger_->Info("Loaded libmpv from " + loader_.LibraryPath() + " (process=" + loader_.ProcessArchitecture() +
                      ", library=" + loader_.LibraryArchitecture() + ")");
    }
    return true;
}

void MpvPlayer::Shutdown() {
    if (handle_ != nullptr) {
        loader_.terminateDestroy(handle_);
        handle_ = nullptr;
    }
}

void MpvPlayer::LoadFile(const std::wstring& path) {
    if (handle_ == nullptr) {
        normalizer_.ApplyLoadFailed(state_, "Playback core is not initialized");
        EmitState();
        return;
    }

    lastRequestedPath_ = path;
    hardwareFallbackAttempted_ = false;
    state_.videoWidth = 0;
    state_.videoHeight = 0;
    state_.highResolutionSeekOptimizationActive = false;
    state_.seekOptimizationProfile.clear();
    state_.seekMode = BuildLowLatencySeekCommand(SeekCommandType::Absolute, exactSeekEnabled_).mode;
    state_.seekLatencyMs = 0.0;
    seekMeasurementPending_ = false;
    pendingSeekContext_.clear();
    ReleasePlaybackMemory(true);
    ApplyRuntimeCacheProfile(path);
    RefreshSeekOptimizationProfile();
    ApplyStringProperty("hwdec", ResolveMpvHwdecPolicy(activeHwdecPolicy_));
    const std::string utf8Path = ToUtf8(path);
    const char* command[] = {"loadfile", utf8Path.c_str(), "replace", nullptr};
    const int result = loader_.commandAsync(handle_, 0, command);
    if (result < 0) {
        normalizer_.ApplyLoadFailed(state_, "loadfile failed: " + ErrorText(result));
        EmitState();
    }
}

void MpvPlayer::LoadSubtitle(const std::wstring& path) {
    if (handle_ == nullptr) {
        return;
    }

    const std::string utf8Path = ToUtf8(path);
    const char* command[] = {"sub-add", utf8Path.c_str(), "select", nullptr};
    const int result = loader_.commandAsync(handle_, 0, command);
    if (result < 0) {
        normalizer_.ApplyLoadFailed(state_, "subtitle load failed: " + ErrorText(result));
        EmitState();
    }
}

void MpvPlayer::StopPlayback() {
    if (handle_ == nullptr) {
        return;
    }

    ReleasePlaybackMemory(true);
    const double volume = state_.volume;
    const bool muted = state_.isMuted;
    const double playbackSpeed = state_.playbackSpeed <= 0.0 ? 1.0 : state_.playbackSpeed;
    state_ = velo::ui::PlayerState{};
    state_.volume = volume;
    state_.isMuted = muted;
    state_.playbackSpeed = playbackSpeed;
    lastRequestedPath_.clear();
    hardwareFallbackAttempted_ = false;
    firstFrameMarked_ = false;
    seekMeasurementPending_ = false;
    pendingSeekContext_.clear();
    state_.seekLatencyMs = 0.0;
    EmitState();
}

void MpvPlayer::ApplyConfig(const velo::config::AppConfig& config) {
    if (handle_ == nullptr) {
        return;
    }

    const SubtitleLayout subtitleLayout = BuildSubtitleLayout(config);
    const std::string subtitleSnapshot = BuildSubtitleDiagnosticsSnapshot(config, subtitleLayout);
    if (logger_ != nullptr && config.showDebugInfo) {
        logger_->Info("subtitle apply snapshot: " + subtitleSnapshot);
    }

    exactSeekEnabled_ = config.exactSeek;
    state_.exactSeek = config.exactSeek;
    RefreshSeekOptimizationProfile();
    SetAudioDelay(config.audioDelaySeconds);
    SetSubtitleDelay(config.subtitleDelaySeconds);

    bool subtitleApplySucceeded = true;
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-font", ToUtf8(SanitizeSubtitleFontName(config.subtitleFont)));
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-color", SubtitleColorToMpv(config.subtitleTextColor));
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-border-color", SubtitleColorToMpv(config.subtitleBorderColor));
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-shadow-color", SubtitleColorToMpv(config.subtitleShadowColor));
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-codepage", ToUtf8(config.subtitleEncoding));
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-align-x", subtitleLayout.alignX);
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-align-y", subtitleLayout.alignY);
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-justify", subtitleLayout.justify);
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-ass-override", "force");
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-ass-style-overrides", subtitleLayout.styleOverrides);
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-use-margins", "no");
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-border-style",
                                                         config.subtitleBackgroundEnabled ? "background-box" : "outline-and-shadow");
    subtitleApplySucceeded &= ApplyCommandStringProperty("sub-back-color",
                                                         SubtitleColorToMpv(config.subtitleBackgroundEnabled ? config.subtitleBackgroundColor
                                                                                                            : config.subtitleShadowColor));
    subtitleApplySucceeded &= ApplyInt64Property("sub-margin-x", subtitleLayout.marginX);
    ApplyStringProperty("screenshot-format", ToUtf8(config.screenshotFormat));
    SetAudioOutputDevice(config.audioOutputDevice);

    subtitleApplySucceeded &= ApplyDoubleProperty("sub-font-size", static_cast<double>(config.subtitleFontSize));
    subtitleApplySucceeded &= ApplyDoubleProperty("sub-border-size", static_cast<double>(config.subtitleBorderSize));
    subtitleApplySucceeded &= ApplyDoubleProperty("sub-shadow-offset", static_cast<double>(config.subtitleShadowDepth));
    subtitleApplySucceeded &= ApplyInt64Property("sub-pos", subtitleLayout.verticalPosition);
    int64_t jpegQuality = std::clamp<int64_t>(config.screenshotQuality, 10, 100);
    loader_.setProperty(handle_, "screenshot-jpeg-quality", MPV_FORMAT_INT64, &jpegQuality);

    if (!subtitleApplySucceeded && logger_ != nullptr) {
        logger_->Warn("subtitle apply incomplete: " + subtitleSnapshot);
    }

    if (config.preferredAspectRatio == L"default") {
        ApplyStringProperty("video-aspect-override", "-1");
    } else {
        ApplyStringProperty("video-aspect-override", ToUtf8(config.preferredAspectRatio));
    }
    int64_t rotate = config.videoRotateDegrees;
    loader_.setProperty(handle_, "video-rotate", MPV_FORMAT_INT64, &rotate);
    ApplyStringProperty("vf", BuildVideoFilterGraph(config));
    ApplyStringProperty("af", BuildAudioFilterGraph(config));
}

void MpvPlayer::TogglePause() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"cycle", "pause", nullptr};
    loader_.commandAsync(handle_, 0, command);
}

void MpvPlayer::SetPause(const bool value) {
    if (handle_ == nullptr) {
        return;
    }
    int pause = value ? 1 : 0;
    loader_.setProperty(handle_, "pause", MPV_FORMAT_FLAG, &pause);
}

void MpvPlayer::SeekRelative(const double seconds) {
    if (handle_ == nullptr) {
        return;
    }
    const std::string amount = std::to_string(seconds);
    const auto seekCommand = BuildLowLatencySeekCommand(SeekCommandType::Relative, !seekOptimizationProfile_.preferKeyframeSeek);
    const char* command[] = {"seek", amount.c_str(), seekCommand.mode.c_str(), nullptr};
    if (ExecuteCommand(command, "seek-relative")) {
        BeginSeekMeasurement("seek-relative");
        if (seekCommand.resumeAfterSeek) {
            ResumePlaybackAfterSeek("seek-relative");
        }
    }
}

void MpvPlayer::SeekAbsolute(const double seconds, const bool exact) {
    if (handle_ == nullptr) {
        return;
    }
    const std::string amount = std::to_string(seconds);
    const bool exactRequested = exact && !seekOptimizationProfile_.preferKeyframeSeek;
    const auto seekCommand = BuildLowLatencySeekCommand(SeekCommandType::Absolute, exactRequested);
    const char* context = exact ? "seek-absolute" : "seek-absolute-preview";
    const char* command[] = {"seek", amount.c_str(), seekCommand.mode.c_str(), nullptr};
    if (ExecuteCommand(command, context)) {
        BeginSeekMeasurement(context);
        if (seekCommand.resumeAfterSeek) {
            ResumePlaybackAfterSeek(context);
        }
    }
}

void MpvPlayer::SetVolume(const double volume) {
    if (handle_ == nullptr) {
        return;
    }
    double value = volume;
    loader_.setProperty(handle_, "volume", MPV_FORMAT_DOUBLE, &value);
}

void MpvPlayer::SetMute(const bool value) {
    if (handle_ == nullptr) {
        return;
    }
    int mute = value ? 1 : 0;
    loader_.setProperty(handle_, "mute", MPV_FORMAT_FLAG, &mute);
}

void MpvPlayer::SetAudioOutputDevice(const std::wstring& device) {
    if (handle_ == nullptr) {
        return;
    }
    const std::wstring resolved = device.empty() ? L"auto" : device;
    const std::string utf8Value = ToUtf8(resolved);
    const char* command[] = {"set", "audio-device", utf8Value.c_str(), nullptr};
    loader_.commandAsync(handle_, 0, command);
}

void MpvPlayer::SetPlaybackSpeed(const double speed) {
    if (handle_ == nullptr) {
        return;
    }
    double value = std::clamp(speed, 0.25, 3.0);
    loader_.setProperty(handle_, "speed", MPV_FORMAT_DOUBLE, &value);
}

void MpvPlayer::SetAudioDelay(const double seconds) {
    if (handle_ == nullptr) {
        return;
    }
    double value = std::clamp(seconds, -10.0, 10.0);
    loader_.setProperty(handle_, "audio-delay", MPV_FORMAT_DOUBLE, &value);
}

void MpvPlayer::SetSubtitleDelay(const double seconds) {
    if (handle_ == nullptr) {
        return;
    }
    double value = std::clamp(seconds, -10.0, 10.0);
    loader_.setProperty(handle_, "sub-delay", MPV_FORMAT_DOUBLE, &value);
}

void MpvPlayer::SetSubtitlePosition(const int positionPercent) {
    if (handle_ == nullptr) {
        return;
    }
    ApplyInt64Property("sub-pos", std::clamp<int64_t>(positionPercent, 0, 100));
}

void MpvPlayer::ResetSyncDelays() {
    SetAudioDelay(0.0);
    SetSubtitleDelay(0.0);
}

void MpvPlayer::StepFrame() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"frame-step", nullptr};
    loader_.commandAsync(handle_, 0, command);
}

void MpvPlayer::StepBackwardFrame() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"frame-back-step", nullptr};
    loader_.commandAsync(handle_, 0, command);
}

void MpvPlayer::SetLoopPointA() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"ab-loop-a", nullptr};
    loader_.commandAsync(handle_, 0, command);
}

void MpvPlayer::SetLoopPointB() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"ab-loop-b", nullptr};
    loader_.commandAsync(handle_, 0, command);
}

void MpvPlayer::ClearLoopPoints() {
    if (handle_ == nullptr) {
        return;
    }
    const char* clearA[] = {"set", "ab-loop-a", "no", nullptr};
    const char* clearB[] = {"set", "ab-loop-b", "no", nullptr};
    ExecuteCommand(clearA, "clear-loop-a");
    ExecuteCommand(clearB, "clear-loop-b");
}

void MpvPlayer::TakeScreenshot(const std::wstring& outputPath) {
    if (handle_ == nullptr || outputPath.empty()) {
        return;
    }

    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path(), error);
    const std::string utf8Path = ToUtf8(outputPath);
    const char* command[] = {"screenshot-to-file", utf8Path.c_str(), "video", nullptr};
    const int result = loader_.commandAsync(handle_, 0, command);
    if (result < 0) {
        normalizer_.ApplyLoadFailed(state_, "screenshot failed: " + ErrorText(result));
        EmitState();
    }
}

void MpvPlayer::CycleAudioTrack() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"cycle", "aid", nullptr};
    ExecuteCommand(command, "cycle-audio-track");
}

void MpvPlayer::CycleSubtitleTrack() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"cycle", "sid", nullptr};
    ExecuteCommand(command, "cycle-subtitle-track");
}

void MpvPlayer::SelectAudioTrack(const long long trackId) {
    if (handle_ == nullptr) {
        return;
    }
    if (trackId < 0) {
        ApplyCommandStringProperty("aid", "no");
        return;
    }
    ApplyInt64Property("aid", trackId);
}

void MpvPlayer::SelectSubtitleTrack(const long long trackId) {
    if (handle_ == nullptr) {
        return;
    }
    if (trackId < 0) {
        ApplyCommandStringProperty("sid", "no");
        return;
    }
    ApplyInt64Property("sid", trackId);
}

void MpvPlayer::RecoverAudioOutput() {
    if (handle_ == nullptr) {
        return;
    }
    const char* command[] = {"set", "audio-device", "auto", nullptr};
    ExecuteCommand(command, "recover-audio-output");
    SetVolume(state_.volume);
    SetMute(state_.isMuted);
}

void MpvPlayer::PumpEvents(double timeoutSeconds) {
    if (handle_ == nullptr) {
        return;
    }

    while (true) {
        const mpv_event* event = loader_.waitEvent(handle_, timeoutSeconds);
        timeoutSeconds = 0.0;
        if (event == nullptr || event->event_id == MPV_EVENT_NONE) {
            return;
        }
        HandleEvent(*event);
    }
}

void MpvPlayer::EmitState() {
    if (stateCallback_ != nullptr) {
        stateCallback_(state_);
    }
}

void MpvPlayer::SyncSelectionProperties() {
    if (handle_ == nullptr || loader_.getProperty == nullptr) {
        return;
    }

    int64_t trackId = -1;
    if (loader_.getProperty(handle_, "aid", MPV_FORMAT_INT64, &trackId) >= 0) {
        normalizer_.ApplyInt64(state_, "aid", trackId);
    }
    trackId = -1;
    if (loader_.getProperty(handle_, "sid", MPV_FORMAT_INT64, &trackId) >= 0) {
        normalizer_.ApplyInt64(state_, "sid", trackId);
    }

    char* stringValue = nullptr;
    if (loader_.getProperty(handle_, "audio-device", MPV_FORMAT_STRING, &stringValue) >= 0 && stringValue != nullptr) {
        normalizer_.ApplyString(state_, "audio-device", stringValue);
    }
    stringValue = nullptr;
    if (loader_.getProperty(handle_, "current-ao", MPV_FORMAT_STRING, &stringValue) >= 0 && stringValue != nullptr) {
        normalizer_.ApplyString(state_, "current-ao", stringValue);
    }

    mpv_node node{};
    if (loader_.getProperty(handle_, "track-list", MPV_FORMAT_NODE, &node) >= 0) {
        ApplyTrackListNode(state_, node);
        loader_.freeNodeContents(&node);
    }
    node = mpv_node{};
    if (loader_.getProperty(handle_, "audio-device-list", MPV_FORMAT_NODE, &node) >= 0) {
        ApplyAudioDeviceListNode(state_, node);
        loader_.freeNodeContents(&node);
    }
    SyncTrackSelections(state_);
}

void MpvPlayer::HandleEvent(const mpv_event& event) {
    switch (event.event_id) {
        case MPV_EVENT_PROPERTY_CHANGE:
            if (event.data != nullptr) {
                HandlePropertyChange(*static_cast<const mpv_event_property*>(event.data));
            }
            break;
        case MPV_EVENT_START_FILE:
            state_.isBuffering = true;
            EmitState();
            break;
        case MPV_EVENT_FILE_LOADED:
            normalizer_.ApplyFileLoaded(state_);
            SyncSelectionProperties();
            EmitState();
            break;
        case MPV_EVENT_PLAYBACK_RESTART:
            normalizer_.ApplyPlaybackRestart(state_);
            SyncSelectionProperties();
            CompleteSeekMeasurement("playback-restart");
            if (!firstFrameMarked_ && metrics_ != nullptr) {
                metrics_->MarkFirstFrame();
                firstFrameMarked_ = true;
            }
            EmitState();
            break;
        case MPV_EVENT_TRACKS_CHANGED:
        case MPV_EVENT_TRACK_SWITCHED:
            SyncSelectionProperties();
            EmitState();
            break;
        case MPV_EVENT_END_FILE:
            state_.eofReached = IsPlaybackEofEvent(static_cast<const mpv_event_end_file*>(event.data));
            EmitState();
            break;
        case MPV_EVENT_LOG_MESSAGE:
            if (logger_ != nullptr && event.data != nullptr) {
                const auto& message = *static_cast<const mpv_event_log_message*>(event.data);
                const std::string text = message.text != nullptr ? message.text : "";
                logger_->Info(std::string("mpv[") + (message.prefix != nullptr ? message.prefix : "core") + "] " + text);
                TryRecoverHardwareFallback(text);
            }
            break;
        default:
            break;
    }
}

void MpvPlayer::HandlePropertyChange(const mpv_event_property& property) {
    if (property.name == nullptr || property.data == nullptr) {
        return;
    }

    const std::string name = property.name;
    if (property.format == MPV_FORMAT_FLAG) {
        normalizer_.ApplyFlag(state_, name, *static_cast<int*>(property.data) != 0);
    } else if (property.format == MPV_FORMAT_DOUBLE) {
        normalizer_.ApplyDouble(state_, name, *static_cast<double*>(property.data));
    } else if (property.format == MPV_FORMAT_INT64) {
        normalizer_.ApplyInt64(state_, name, *static_cast<int64_t*>(property.data));
    } else if (property.format == MPV_FORMAT_STRING) {
        const auto* value = static_cast<char**>(property.data);
        normalizer_.ApplyString(state_, name, (*value != nullptr) ? *value : "");
    } else if (property.format == MPV_FORMAT_NODE) {
        const auto* value = static_cast<struct mpv_node*>(property.data);
        if (value != nullptr) {
            if (name == "track-list") {
                ApplyTrackListNode(state_, *value);
            } else if (name == "audio-device-list") {
                ApplyAudioDeviceListNode(state_, *value);
            }
        }
    }
    if (name == "path" || name == "video-params/w" || name == "video-params/h") {
        RefreshSeekOptimizationProfile();
    }
    SyncTrackSelections(state_);
    EmitState();
}

std::string MpvPlayer::ErrorText(const int errorCode) const {
    if (loader_.errorString == nullptr) {
        return std::to_string(errorCode);
    }
    const char* text = loader_.errorString(errorCode);
    return text != nullptr ? text : std::to_string(errorCode);
}

bool MpvPlayer::ExecuteCommand(const char* const command[], const char* context) {
    if (handle_ == nullptr) {
        return false;
    }

    const int result = loader_.commandAsync(handle_, 0, command);
    if (result < 0 && logger_ != nullptr) {
        logger_->Warn(std::string("mpv command failed: ") + context + " error=" + ErrorText(result));
    }
    return result >= 0;
}

void MpvPlayer::ReleasePlaybackMemory(const bool clearPlaylist) {
    if (handle_ == nullptr) {
        return;
    }

    const char* stopCommand[] = {"stop", nullptr};
    ExecuteCommand(stopCommand, "stop-playback");
    if (clearPlaylist) {
        const char* clearPlaylistCommand[] = {"playlist-clear", nullptr};
        ExecuteCommand(clearPlaylistCommand, "playlist-clear");
    }
    DropBufferedPlaybackState(clearPlaylist ? "replace-file" : "stop-playback");
}

void MpvPlayer::DropBufferedPlaybackState(const char* context) {
    if (handle_ == nullptr) {
        return;
    }

    const char* command[] = {"drop-buffers", nullptr};
    ExecuteCommand(command, context);
}

bool MpvPlayer::SetInitialOption(const char* name, const std::string& value) {
    if (handle_ == nullptr) {
        return false;
    }
    const int result = loader_.setOptionString(handle_, name, value.c_str());
    if (result < 0 && logger_ != nullptr) {
        logger_->Warn(std::string("mpv option failed: ") + name + "=" + value + " error=" + ErrorText(result));
    }
    return result >= 0;
}

bool MpvPlayer::ApplyStringProperty(const char* name, const std::string& value) {
    if (handle_ == nullptr) {
        return false;
    }
    char* writable = const_cast<char*>(value.c_str());
    const int result = loader_.setProperty(handle_, name, MPV_FORMAT_STRING, &writable);
    if (result < 0 && logger_ != nullptr) {
        logger_->Warn(std::string("mpv property failed: ") + name + "=" + value + " error=" + ErrorText(result));
    }
    return result >= 0;
}

bool MpvPlayer::ApplyDoubleProperty(const char* name, const double value) {
    if (handle_ == nullptr) {
        return false;
    }
    double writable = value;
    const int result = loader_.setProperty(handle_, name, MPV_FORMAT_DOUBLE, &writable);
    if (result < 0 && logger_ != nullptr) {
        logger_->Warn(std::string("mpv property failed: ") + name + "=" + std::to_string(value) + " error=" + ErrorText(result));
    }
    return result >= 0;
}

bool MpvPlayer::ApplyInt64Property(const char* name, const int64_t value) {
    if (handle_ == nullptr) {
        return false;
    }
    int64_t writable = value;
    const int result = loader_.setProperty(handle_, name, MPV_FORMAT_INT64, &writable);
    if (result < 0 && logger_ != nullptr) {
        logger_->Warn(std::string("mpv property failed: ") + name + "=" + std::to_string(value) + " error=" + ErrorText(result));
    }
    return result >= 0;
}

bool MpvPlayer::ApplyCommandStringProperty(const char* name, const std::string& value) {
    if (handle_ == nullptr) {
        return false;
    }
    const char* command[] = {"set", name, value.c_str(), nullptr};
    return ExecuteCommand(command, name);
}

void MpvPlayer::ResumePlaybackAfterSeek(const char* context) {
    if (handle_ == nullptr) {
        return;
    }

    int pause = 0;
    const int result = loader_.setProperty(handle_, "pause", MPV_FORMAT_FLAG, &pause);
    if (result < 0 && logger_ != nullptr) {
        logger_->Warn(std::string("mpv pause resume failed after seek: ") + context + " error=" + ErrorText(result));
    }
}

void MpvPlayer::BeginSeekMeasurement(const char* context) {
    seekMeasurementPending_ = true;
    seekMeasurementStart_ = std::chrono::steady_clock::now();
    pendingSeekContext_ = context != nullptr ? context : "seek";
}

void MpvPlayer::CompleteSeekMeasurement(const char* context) {
    if (!seekMeasurementPending_) {
        return;
    }

    state_.seekLatencyMs =
        std::max(0.0, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - seekMeasurementStart_).count());
    seekMeasurementPending_ = false;
    if (logger_ != nullptr) {
        std::ostringstream stream;
        stream << "seek latency context=" << pendingSeekContext_ << " completed_by=" << (context != nullptr ? context : "event")
               << " latency_ms=" << state_.seekLatencyMs;
        if (state_.seekLatencyMs > kLowLatencySeekBudgetMs) {
            logger_->Warn(stream.str() + " budget_exceeded=true");
        } else {
            logger_->Info(stream.str());
        }
    }
    pendingSeekContext_.clear();
}

void MpvPlayer::ApplyRuntimeCacheProfile(const std::wstring& path) {
    if (handle_ == nullptr) {
        return;
    }

    if (lightweightMode_) {
        ApplyCommandStringProperty("cache", "yes");
        ApplyCommandStringProperty("demuxer-readahead-secs", "5");
        ApplyCommandStringProperty("vd-lavc-skiploopfilter", "all");
        ApplyCommandStringProperty("demuxer-max-bytes", "2MiB");
        ApplyCommandStringProperty("demuxer-max-back-bytes", "256KiB");
        ApplyCommandStringProperty("audio-buffer", "0.03");
        return;
    }

    if (IsNetworkLikePath(path) || IsDiscLikePath(path)) {
        ApplyCommandStringProperty("cache", "yes");
        ApplyCommandStringProperty("demuxer-readahead-secs", "5");
        ApplyCommandStringProperty("vd-lavc-skiploopfilter", "all");
        ApplyCommandStringProperty("demuxer-max-bytes", "12MiB");
        ApplyCommandStringProperty("demuxer-max-back-bytes", "1MiB");
        ApplyCommandStringProperty("audio-buffer", "0.10");
        return;
    }

    ApplyCommandStringProperty("cache", "yes");
    ApplyCommandStringProperty("demuxer-readahead-secs", "5");
    ApplyCommandStringProperty("vd-lavc-skiploopfilter", "all");
    ApplyCommandStringProperty("demuxer-max-bytes", "12MiB");
    ApplyCommandStringProperty("demuxer-max-back-bytes", "1MiB");
    ApplyCommandStringProperty("audio-buffer", "0.05");
}

void MpvPlayer::RefreshSeekOptimizationProfile() {
    const std::wstring& activePath = !lastRequestedPath_.empty() ? lastRequestedPath_ : std::wstring{};
    const auto nextProfile =
        BuildSeekOptimizationProfile(activePath, state_.videoWidth, state_.videoHeight, state_.isNetworkSource, state_.isDiscSource,
                                     exactSeekEnabled_);
    if (IsSeekOptimizationProfileEqual(seekOptimizationProfile_, nextProfile)) {
        state_.highResolutionSeekOptimizationActive = nextProfile.active;
        state_.seekOptimizationProfile = nextProfile.statusLabel;
        state_.seekMode = BuildLowLatencySeekCommand(SeekCommandType::Absolute, !nextProfile.preferKeyframeSeek).mode;
        return;
    }

    seekOptimizationProfile_ = nextProfile;
    state_.highResolutionSeekOptimizationActive = seekOptimizationProfile_.active;
    state_.seekOptimizationProfile = seekOptimizationProfile_.statusLabel;
    state_.seekMode = BuildLowLatencySeekCommand(SeekCommandType::Absolute, !seekOptimizationProfile_.preferKeyframeSeek).mode;
    ApplySeekOptimizationProfile();
}

void MpvPlayer::ApplySeekOptimizationProfile() {
    if (handle_ == nullptr) {
        return;
    }

    ApplyCommandStringProperty("cache", seekOptimizationProfile_.cacheMode);
    ApplyCommandStringProperty("demuxer-readahead-secs", seekOptimizationProfile_.demuxerReadaheadSecs);
    ApplyCommandStringProperty("demuxer-max-bytes", seekOptimizationProfile_.demuxerMaxBytes);
    ApplyCommandStringProperty("demuxer-max-back-bytes", seekOptimizationProfile_.demuxerMaxBackBytes);
    ApplyCommandStringProperty("audio-buffer", seekOptimizationProfile_.audioBuffer);
    ApplyCommandStringProperty("vd-lavc-skiploopfilter", seekOptimizationProfile_.skipLoopFilter);
    ApplyCommandStringProperty("hr-seek", seekOptimizationProfile_.preferKeyframeSeek ? "no" : "default");

    if (logger_ != nullptr) {
        logger_->Info("seek optimization profile=" + seekOptimizationProfile_.statusLabel + " seek_mode=" + state_.seekMode +
                      " dimensions=" + std::to_string(state_.videoWidth) + "x" + std::to_string(state_.videoHeight));
    }
}

bool MpvPlayer::ShouldCollapseQueuedSeekCommands() const noexcept {
    return seekOptimizationProfile_.collapseQueuedSeeks;
}

bool MpvPlayer::TryRecoverHardwareFallback(const std::string& message) {
    if (handle_ == nullptr || hardwareFallbackAttempted_ || activeHwdecPolicy_ == L"no" || lastRequestedPath_.empty() ||
        !LooksLikeHardwareOutputFailure(message)) {
        return false;
    }

    hardwareFallbackAttempted_ = true;
    if (logger_ != nullptr) {
        logger_->Warn("Hardware decode/output issue detected; retrying with software decode");
    }
    ApplyStringProperty("hwdec", "no");
    ReleasePlaybackMemory(true);
    const std::string utf8Path = ToUtf8(lastRequestedPath_);
    const char* command[] = {"loadfile", utf8Path.c_str(), "replace", nullptr};
    ExecuteCommand(command, "hardware-fallback-reload");
    state_.errorState = "Hardware decode/output failed, retrying with software decode";
    EmitState();
    return true;
}

}  // namespace velo::playback
