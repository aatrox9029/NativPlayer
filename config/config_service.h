#pragma once

#include <Windows.h>

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace velo::config {

struct ResumeEntry {
    std::wstring path;
    double positionSeconds = 0.0;
};

struct RecentItem {
    std::wstring path;
    bool pinned = false;
    uint64_t openedAtUnixSeconds = 0;
};

struct HistoryEntry {
    std::wstring path;
    std::wstring title;
    double positionSeconds = 0.0;
    bool isUrl = false;
    uint64_t openedAtUnixSeconds = 0;
};

struct BookmarkEntry {
    std::wstring path;
    std::wstring label;
    double positionSeconds = 0.0;
};

struct KeyBindingEntry {
    std::wstring actionId;
    unsigned int virtualKey = 0;
};

enum class RepeatMode {
    Off,
    One,
    All,
};

enum class EndOfPlaybackAction {
    Replay,
    PlayNext,
    Stop,
    CloseWindow,
};

enum class SeekStepMode {
    Seconds,
    Percent,
};

struct AppConfig {
    int windowWidth = 960;
    int windowHeight = 600;
    int windowPosX = CW_USEDEFAULT;
    int windowPosY = CW_USEDEFAULT;
    bool hasSavedWindowPlacement = false;
    bool startMaximized = false;
    double volume = 20.0;
    double startupVolume = 20.0;
    bool rememberVolume = true;
    bool muted = false;
    std::wstring languageCode = L"en-US";
    std::wstring hwdecPolicy = L"auto";
    std::wstring audioOutputDevice = L"auto";
    std::wstring lastOpenUrl;
    bool autoplayNextFile = true;
    bool preservePauseOnOpen = true;
    bool autoLoadSubtitle = true;
    bool rememberPlaybackPosition = true;
    bool exactSeek = false;
    bool showPlaylistSidebar = true;
    bool showDebugOverlay = false;
    int controlsHideDelayMs = 2000;
    SeekStepMode seekStepMode = SeekStepMode::Seconds;
    int seekStepSeconds = 10;
    int seekStepPercent = 5;
    bool showSeekPreview = true;
    int volumeStep = 5;
    int wheelVolumeStep = 5;
    double audioDelaySeconds = 0.0;
    double subtitleDelaySeconds = 0.0;
    std::wstring subtitleFont = L"Noto Sans CJK TC";
    std::wstring subtitleTextColor = L"FFFFFFFF";
    std::wstring subtitleBorderColor = L"101010FF";
    std::wstring subtitleShadowColor = L"101010AA";
    bool subtitleBackgroundEnabled = false;
    std::wstring subtitleBackgroundColor = L"101010B3";
    int subtitleFontSize = 42;
    bool subtitleBold = false;
    bool subtitleItalic = false;
    bool subtitleUnderline = false;
    bool subtitleStrikeOut = false;
    int subtitleBorderSize = 2;
    int subtitleShadowDepth = 1;
    int subtitleVerticalMargin = 96;
    std::wstring subtitlePositionPreset = L"bottom";
    int subtitleOffsetUp = 0;
    int subtitleOffsetDown = 0;
    int subtitleHorizontalOffset = 0;
    int subtitleOffsetLeft = 0;
    int subtitleOffsetRight = 0;
    std::wstring subtitleEncoding = L"auto";
    std::wstring doubleClickAction = L"fullscreen";
    std::wstring middleClickAction = L"toggle_pause";
    RepeatMode repeatMode = RepeatMode::Off;
    EndOfPlaybackAction endOfPlaybackAction = EndOfPlaybackAction::PlayNext;
    bool showDebugInfo = false;
    std::wstring preferredAspectRatio = L"default";
    int videoRotateDegrees = 0;
    bool mirrorVideo = false;
    bool deinterlaceEnabled = false;
    int sharpenStrength = 0;
    int denoiseStrength = 0;
    std::wstring equalizerProfile = L"off";
    std::wstring screenshotFormat = L"png";
    int screenshotQuality = 92;
    int networkTimeoutMs = 8000;
    int streamReconnectCount = 2;
    std::wstring lastFile;
    std::vector<std::wstring> recentFiles;
    std::vector<RecentItem> recentItems;
    std::vector<ResumeEntry> resumeEntries;
    std::vector<HistoryEntry> historyEntries;
    std::vector<BookmarkEntry> bookmarkEntries;
    std::vector<KeyBindingEntry> keyBindings;
};

class ConfigService {
public:
    explicit ConfigService(std::filesystem::path rootDirectory);
    ~ConfigService();

    bool Load();
    void ScheduleSave(const AppConfig& config);
    void Flush();

    [[nodiscard]] const AppConfig& Current() const noexcept;
    [[nodiscard]] const std::filesystem::path& ConfigPath() const noexcept;
    [[nodiscard]] const std::filesystem::path& BackupPath() const noexcept;

private:
    void WorkerLoop();
    bool WriteConfigFile(const AppConfig& config);
    void EnsureRootDirectory();

    std::filesystem::path rootDirectory_;
    std::filesystem::path configPath_;
    std::filesystem::path backupPath_;
    mutable std::mutex mutex_;
    AppConfig current_;
    std::optional<AppConfig> pending_;
    bool stopWorker_ = false;
    std::thread worker_;
};

AppConfig DefaultAppConfig();
std::filesystem::path DefaultConfigRoot();
bool ExportConfigSnapshot(const AppConfig& config, const std::filesystem::path& destinationPath);
bool ImportConfigSnapshot(const std::filesystem::path& sourcePath, AppConfig& loadedConfig);

}  // namespace velo::config
