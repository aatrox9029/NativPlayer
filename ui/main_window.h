#pragma once

#include <Windows.h>
#include <shellapi.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/app_metadata.h"
#include "config/config_service.h"
#include "platform/win32/input_router.h"
#include "platform/win32/window_host.h"
#include "ui/app_icon.h"
#include "ui/player_state.h"
#include "ui/quick_browse_panel.h"
#include "ui/seek_preview_popup.h"
#include "ui/settings_dialog.h"
#include "ui/themed_slider.h"

namespace velo::ui {

class MainWindow {
public:
    struct Callbacks {
        std::function<void(const std::wstring&)> openFile;
        std::function<void(const std::vector<std::wstring>&)> openFiles;
        std::function<void(const std::wstring&)> openFolder;
        std::function<void(const std::wstring&)> loadSubtitle;
        std::function<void()> togglePause;
        std::function<void(double)> seekRelative;
        std::function<void(double)> seekAbsolutePreview;
        std::function<void(double)> seekAbsolute;
        std::function<void(double)> setVolume;
        std::function<void(bool)> setMute;
        std::function<void(double)> setPlaybackSpeed;
        std::function<void(double)> adjustPlaybackSpeed;
        std::function<void()> resetPlaybackSpeed;
        std::function<void()> cycleAudioTrack;
        std::function<void()> cycleSubtitleTrack;
        std::function<void(long long)> selectAudioTrack;
        std::function<void(long long)> selectSubtitleTrack;
        std::function<void(const std::wstring&)> setAudioOutputDevice;
        std::function<void()> takeScreenshot;
        std::function<void()> recoverAudioOutput;
        std::function<void()> playNext;
        std::function<void()> playPrevious;
        std::function<void()> replayCurrent;
        std::function<void(const velo::config::AppConfig&)> applyConfig;
        std::function<std::wstring()> exportDiagnostics;
        std::function<std::wstring()> buildAboutText;
        std::function<void(const std::wstring&)> openUpdateDownload;
    };

    bool Create(HINSTANCE instance, const velo::config::AppConfig& initialConfig, Callbacks callbacks);
    int RunMessageLoop();
    void Show(int showCommand) const;
    void PostPlayerState(const PlayerState& state) const;
    void PostOsd(std::wstring text) const;
    void PostOpenFile(std::wstring path) const;
    void PostUpdateAvailability(std::wstring versionTag, std::wstring downloadUrl) const;
    void BringToFront() const;
    void ToggleFullscreen();
    void ExitFullscreen();
    void ExitFullscreenToWindowed();
    void SetRecentFiles(std::vector<std::wstring> recentFiles);
    void SetPlaylistContext(int currentIndex, int totalCount, bool temporaryPlaylist);
    void PostEndOfPlayback(bool hasNext, bool willAutoplayNext) const;
    void PrepareForIncomingMedia();

    [[nodiscard]] HWND WindowHandle() const noexcept;
    [[nodiscard]] HWND VideoHostWindow() const noexcept;
    [[nodiscard]] velo::config::AppConfig CaptureConfig() const;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    struct HeaderControls {
        HWND titleBarBackground = nullptr;
        HWND panelBackground = nullptr;
        HWND titleLabel = nullptr;
        HWND openButton = nullptr;
        HWND recentButton = nullptr;
        HWND quickBrowseButton = nullptr;
        HWND previousButton = nullptr;
        HWND playPauseButton = nullptr;
        HWND nextTrackButton = nullptr;
        HWND muteButton = nullptr;
        HWND speedButton = nullptr;
        HWND subtitleButton = nullptr;
        HWND settingsButton = nullptr;
        HWND moreButton = nullptr;
        HWND fullscreenButton = nullptr;
        HWND currentTimeLabel = nullptr;
        HWND durationLabel = nullptr;
        HWND mediaInfoLabel = nullptr;
    };

    struct FooterControls {
        HWND volumeLabel = nullptr;
        HWND speedLabel = nullptr;
        HWND volumeEdit = nullptr;
        HWND speedEdit = nullptr;
        HWND statusLabel = nullptr;
    };

    struct EmptyStateControls {
        HWND emptyTitleLabel = nullptr;
        HWND emptyHintLabel = nullptr;
        HWND emptyRecentLabel = nullptr;
    };

    struct OverlayControls {
        HWND osdLabel = nullptr;
        HWND endPromptTitleLabel = nullptr;
        HWND endPromptHintLabel = nullptr;
        HWND replayButton = nullptr;
        HWND nextButton = nullptr;
    };

    struct FullscreenCaptionControls {
        HWND downloadButton = nullptr;
        HWND minimizeButton = nullptr;
        HWND windowedButton = nullptr;
        HWND closeButton = nullptr;
    };

    struct UpdateAvailabilityState {
        std::wstring versionTag;
        std::wstring downloadUrl;
    };

    enum class ContextCommand : UINT {
        OpenFile = 2001,
        OpenFolder,
        LoadSubtitle,
        TakeScreenshot,
        TogglePause,
        PlayPrevious,
        PlayNext,
        ToggleMute,
        CycleAudio,
        CycleSubtitle,
        CycleSpeed,
        SlowerSpeed,
        FasterSpeed,
        ResetSpeed,
        ToggleFullscreen,
        OpenSettings,
        ShowShortcuts,
        ToggleMediaInfo,
        ExportDiagnostics,
        ShowAbout,
        ReplayCurrent,
        EndPromptNext,
        EndActionReplay,
        EndActionPlayNext,
        EndActionStop,
        EndActionCloseWindow,
    };

    struct EndPromptState {
        bool hasNext = false;
        bool willAutoplayNext = false;
    };

    struct MenuVisualItem {
        std::wstring label;
        std::wstring shortcut;
        bool enabled = true;
        bool separator = false;
        bool popup = false;
    };

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleCreateMessage();
    LRESULT HandleSizeMessage();
    LRESULT HandleExitSizeMoveMessage();
    LRESULT HandleGetMinMaxInfo(LPARAM lParam) const;
    LRESULT HandleMouseWheelMessage(WPARAM wParam, LPARAM lParam);
    LRESULT HandleAppCommandMessage(LPARAM lParam);
    LRESULT HandleTimerMessage(WPARAM wParam);
    LRESULT HandleCommandMessage(WPARAM wParam, LPARAM lParam);
    LRESULT HandleKeyDownMessage(WPARAM wParam);
    LRESULT HandlePaintMessage();
    LRESULT HandlePlayerStateMessage(LPARAM lParam);
    LRESULT HandleOsdMessage(LPARAM lParam);
    LRESULT HandleOpenFileMessage(LPARAM lParam);
    LRESULT HandleEndPromptMessage(LPARAM lParam);
    LRESULT HandleUpdateAvailabilityMessage(LPARAM lParam);
    LRESULT HandleStaticColorMessage(WPARAM wParam, LPARAM lParam);
    LRESULT HandleEditColorMessage(WPARAM wParam);
    LRESULT HandleNcCalcSize(WPARAM wParam, LPARAM lParam) const;
    LRESULT HandleNcActivate(WPARAM wParam, LPARAM lParam);
    LRESULT HandleNcPaint(WPARAM wParam, LPARAM lParam) const;
    LRESULT HandleNcDestroyMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void CreateChildControls();
    void LayoutChildren();
    void ApplyStateToControls();
    void RefreshLocalizedText();
    void CommitVolumeText();
    void CommitSpeedText();
    void DispatchVolumeChange(int value);
    void ToggleQuickBrowse();
    void ShowQuickBrowse();
    void HideQuickBrowse();
    void RefreshQuickBrowse();
    void NavigateQuickBrowseFolder(const std::wstring& folderPath);
    void OpenFilePicker();
    void OpenFolderPicker();
    void OpenSubtitlePicker();
    void HandleDroppedFile(HDROP dropHandle);
    void HandlePointerActivity(POINT clientPoint);
    void HandlePointerLeave();
    void SetControlsVisible(bool visible, bool resetTimer);
    void RedrawUiSurface(bool immediate = false) const;
    void UpdateEmptyState();
    void UpdateOsdLayout();
    void UpdateSeekPreview();
    void HideSeekPreview();
    void SyncVolumeEditPreview();
    void SyncSpeedEditPreview();
    void RefreshPlayPauseButton(bool immediate = false) const;
    void RefreshMuteButton(bool immediate = false) const;
    void RefreshSubtitleButton(bool immediate = false) const;
    void ShowRecentFilesMenu();
    void ShowContextMenu(POINT screenPoint, bool fromKeyboard);
    void ShowOverflowMenu();
    void OpenSettingsDialog();
    void ShowShortcutHelp();
    void ShowOsdNow(const std::wstring& text, int durationMs = 1000);
    void HideOsdNow();
    void ToggleMediaInfo();
    void UpdateMediaInfoPanel();
    void ExecuteMouseAction(const std::wstring& actionId);
    bool IsControlInteractionActive() const;
    [[nodiscard]] RECT HeaderTitleRect() const;
    [[nodiscard]] RECT EmptyDropZoneRect() const;
    [[nodiscard]] RECT VideoClientRect() const;
    void ApplyMenuTheme(HMENU menu) const;
    void AppendMenuEntry(HMENU menu, UINT commandId, std::wstring label, std::wstring shortcut = {}, bool enabled = true);
    void AppendPopupMenuEntry(HMENU menu, UINT commandId, HMENU popup, std::wstring label);
    void ShowEndPrompt(const EndPromptState& state);
    void HideEndPrompt();
    [[nodiscard]] bool ShouldKeepEndPromptVisible() const;
    double NextPresetSpeed(double currentSpeed) const;
    void PaintWindowBackdrop(HDC dc, const RECT& clientRect) const;
    void DrawButton(const DRAWITEMSTRUCT& drawItem) const;
    void DrawMenuItem(const DRAWITEMSTRUCT& drawItem) const;
    void MeasureMenuItem(MEASUREITEMSTRUCT& measureItem) const;
    bool HandleQuickBrowseOutsideClick(POINT clientPoint);
    bool IsPointInsideVideo(POINT clientPoint) const;
    bool IsPointInControlsActivationZone(POINT clientPoint) const;
    bool IsPointInControlsRetentionZone(POINT clientPoint) const;
    [[nodiscard]] double ConfiguredSeekDeltaSeconds(bool forward) const;
    [[nodiscard]] double ConfiguredSeekTargetSeconds(bool forward) const;
    bool IsCursorInControlsActivationZone() const;
    bool IsCursorInControlsRetentionZone() const;
    void SetFullscreenCaptionButtonsVisible(bool visible);
    bool IsPointInFullscreenCaptionActivationZone(POINT clientPoint) const;
    bool IsCursorInFullscreenCaptionActivationZone() const;
    LRESULT HandleNcHitTest(LPARAM lParam) const;
    bool IsPointInCaptionButtons(POINT clientPoint) const;
    void RememberWindowedPlacement();
    void ToggleWindowMaximizeState();

    static constexpr UINT kMessagePlayerState = WM_APP + 1;
    static constexpr UINT kMessageOpenFile = WM_APP + 2;
    static constexpr UINT kMessageOsd = WM_APP + 3;
    static constexpr UINT kMessageEndPrompt = WM_APP + 4;
    static constexpr UINT kMessageUpdateAvailable = WM_APP + 5;
    static constexpr UINT kRecentFileCommandBase = 40000;
    static constexpr UINT_PTR kTimerHideControls = 1;
    static constexpr UINT_PTR kTimerHideOsd = 2;
    static constexpr UINT_PTR kTimerPendingSingleClick = 3;
    static constexpr UINT_PTR kTimerAutoAdvance = 4;

    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HeaderControls headerControls_{};
    FooterControls footerControls_{};
    EmptyStateControls emptyStateControls_{};
    OverlayControls overlayControls_{};
    FullscreenCaptionControls fullscreenCaptionControls_{};
    HFONT uiFont_ = nullptr;
    HFONT captionFont_ = nullptr;
    HFONT iconFont_ = nullptr;
    HFONT largeTextGlyphFont_ = nullptr;
    HFONT largeIconFont_ = nullptr;
    HFONT titleFont_ = nullptr;
    HFONT osdFont_ = nullptr;
    AppIconSet appIcons_;
    ThemedSlider seekSlider_;
    ThemedSlider volumeSlider_;
    ThemedSlider speedSlider_;
    QuickBrowsePanel quickBrowsePanel_;
    SeekPreviewPopup seekPreviewPopup_;
    platform::win32::WindowHost videoHost_;
    SettingsDialog settingsDialog_;
    Callbacks callbacks_;
    PlayerState state_;
    velo::config::AppConfig config_;
    std::vector<std::wstring> recentFiles_;
    std::wstring currentHeaderTitle_;
    std::wstring lastErrorText_;
    bool suppressSeekEvents_ = false;
    bool suppressVolumeEvents_ = false;
    bool suppressSpeedEvents_ = false;
    bool suppressVolumeTextEvents_ = false;
    bool suppressSpeedTextEvents_ = false;
    bool controlsVisible_ = true;
    bool quickBrowseVisible_ = false;
    bool fullscreen_ = false;
    bool pendingSingleClick_ = false;
    bool endPromptVisible_ = false;
    bool endPromptHasNext_ = false;
    bool endPromptWillAutoplayNext_ = false;
    bool mediaInfoVisible_ = false;
    bool mouseInsideWindow_ = false;
    bool startupControlsPinned_ = false;
    bool uiSuppressed_ = false;
    bool fullscreenCaptionButtonsVisible_ = false;
    bool useOverflowControls_ = false;
    bool showRecentButton_ = true;
    bool showSubtitleButton_ = true;
    bool showSettingsButton_ = true;
    bool showMoreButton_ = false;
    bool updateAvailable_ = false;
    bool videoDragPending_ = false;
    bool suppressNextVideoSingleClick_ = false;
    int autoHideDelayMs_ = 1800;
    int playlistCurrentIndex_ = -1;
    int playlistTotalCount_ = 0;
    bool temporaryPlaylist_ = false;
    std::string activeMediaPath_;
    std::wstring quickBrowseFolder_;
    std::wstring quickBrowseRootFolder_;
    std::wstring availableUpdateTag_;
    std::wstring updateDownloadUrl_;
    std::unordered_map<UINT, MenuVisualItem> menuVisualItems_;
    mutable std::mutex pendingPlayerStateMutex_;
    mutable std::unique_ptr<PlayerState> pendingPlayerState_;
    mutable bool playerStateMessageQueued_ = false;
    POINT videoDragStartScreen_{};
    WINDOWPLACEMENT previousPlacement_{sizeof(WINDOWPLACEMENT)};
    WINDOWPLACEMENT lastWindowedPlacement_{sizeof(WINDOWPLACEMENT)};
    DWORD previousStyle_ = 0;
    bool suppressWindowedPlacementTracking_ = false;
};

}  // namespace velo::ui
