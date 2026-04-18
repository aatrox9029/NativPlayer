#include "ui/main_window_internal.h"

namespace velo::ui {

bool MainWindow::Create(HINSTANCE instance, const velo::config::AppConfig& initialConfig, Callbacks callbacks) {
    instance_ = instance;
    callbacks_ = std::move(callbacks);
    config_ = initialConfig;
    autoHideDelayMs_ = initialConfig.controlsHideDelayMs;
    recentFiles_.clear();
    appIcons_ = LoadAppIconSet(instance_);

    INITCOMMONCONTROLSEX commonControls{};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&commonControls);

    RegisterClass(instance, appIcons_.largeIcon);
    const int x = initialConfig.hasSavedWindowPlacement ? initialConfig.windowPosX : CW_USEDEFAULT;
    const int y = initialConfig.hasSavedWindowPlacement ? initialConfig.windowPosY : CW_USEDEFAULT;
    hwnd_ = CreateWindowExW(0, kMainWindowClassName, app::AppDisplayName().c_str(), MainWindowStyle(), x, y,
                            std::max(kMinimumWindowWidth, initialConfig.windowWidth),
                            std::max(kMinimumWindowHeight, initialConfig.windowHeight), nullptr, nullptr, instance, this);
    if (hwnd_ == nullptr) {
        DestroyAppIconSet(appIcons_);
        return false;
    }
    GetWindowPlacement(hwnd_, &lastWindowedPlacement_);

    if (appIcons_.largeIcon != nullptr) {
        SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(appIcons_.largeIcon));
    }
    if (appIcons_.smallIcon != nullptr) {
        SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(appIcons_.smallIcon));
    }
    return true;
}

int MainWindow::RunMessageLoop() {
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

void MainWindow::Show(const int showCommand) const {
    ShowWindow(hwnd_, showCommand);
    UpdateWindow(hwnd_);
}

void MainWindow::PostPlayerState(const PlayerState& state) const {
    bool shouldPost = false;
    {
        std::scoped_lock lock(pendingPlayerStateMutex_);
        if (pendingPlayerState_ == nullptr) {
            pendingPlayerState_ = std::make_unique<PlayerState>(state);
        } else {
            *pendingPlayerState_ = state;
        }
        if (!playerStateMessageQueued_) {
            playerStateMessageQueued_ = true;
            shouldPost = true;
        }
    }

    if (shouldPost && PostMessageW(hwnd_, kMessagePlayerState, 0, 0) == FALSE) {
        std::scoped_lock lock(pendingPlayerStateMutex_);
        playerStateMessageQueued_ = false;
        pendingPlayerState_.reset();
    }
}

void MainWindow::PostOsd(std::wstring text) const {
    MSG pending{};
    while (PeekMessageW(&pending, hwnd_, kMessageOsd, kMessageOsd, PM_REMOVE) != FALSE) {
        delete reinterpret_cast<std::wstring*>(pending.lParam);
    }
    auto* copy = new std::wstring(std::move(text));
    if (PostMessageW(hwnd_, kMessageOsd, 0, reinterpret_cast<LPARAM>(copy)) == FALSE) {
        delete copy;
    }
}

void MainWindow::PostOpenFile(std::wstring path) const {
    auto* copy = new std::wstring(std::move(path));
    if (PostMessageW(hwnd_, kMessageOpenFile, 0, reinterpret_cast<LPARAM>(copy)) == FALSE) {
        delete copy;
    }
}

void MainWindow::PostUpdateAvailability(std::wstring versionTag, std::wstring downloadUrl) const {
    MSG pending{};
    while (PeekMessageW(&pending, hwnd_, kMessageUpdateAvailable, kMessageUpdateAvailable, PM_REMOVE) != FALSE) {
        delete reinterpret_cast<UpdateAvailabilityState*>(pending.lParam);
    }

    auto* payload = new UpdateAvailabilityState{
        .versionTag = std::move(versionTag),
        .downloadUrl = std::move(downloadUrl),
    };
    if (PostMessageW(hwnd_, kMessageUpdateAvailable, 0, reinterpret_cast<LPARAM>(payload)) == FALSE) {
        delete payload;
    }
}

void MainWindow::BringToFront() const {
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd_);
}

void MainWindow::ToggleFullscreen() {
    fullscreen_ = !fullscreen_;
    if (fullscreen_) {
        suppressWindowedPlacementTracking_ = true;
        previousStyle_ = GetWindowLongW(hwnd_, GWL_STYLE);
        GetWindowPlacement(hwnd_, &previousPlacement_);
        if (previousPlacement_.showCmd == SW_SHOWMINIMIZED) {
            previousPlacement_.showCmd = SW_SHOWNORMAL;
        }

        MONITORINFO monitorInfo{sizeof(MONITORINFO)};
        GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);
        SetWindowLongW(hwnd_, GWL_STYLE, static_cast<LONG>(FullscreenWindowStyle()));
        SetWindowPos(hwnd_, HWND_TOPMOST, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                     monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                     monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        ShowWindow(hwnd_, SW_SHOW);
        SetActiveWindow(hwnd_);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
        ShowOsdNow(velo::localization::Text(config_.languageCode, velo::localization::TextId::EnterFullscreen), 850);
    } else {
        suppressWindowedPlacementTracking_ = true;
        SetWindowLongW(hwnd_, GWL_STYLE, previousStyle_);
        if (previousPlacement_.showCmd == SW_SHOWMAXIMIZED) {
            WINDOWPLACEMENT restorePlacement = previousPlacement_;
            restorePlacement.showCmd = SW_SHOWNORMAL;
            SetWindowPlacement(hwnd_, &restorePlacement);

            MONITORINFO monitorInfo{sizeof(MONITORINFO)};
            GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);
            const RECT& workArea = monitorInfo.rcWork;
            SetWindowPos(hwnd_, HWND_NOTOPMOST, workArea.left, workArea.top,
                         workArea.right - workArea.left, workArea.bottom - workArea.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            ShowWindow(hwnd_, SW_SHOWMAXIMIZED);
        } else {
            SetWindowPos(hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            SetWindowPlacement(hwnd_, &previousPlacement_);
            ShowWindow(hwnd_, SW_SHOWNORMAL);
        }
        SetActiveWindow(hwnd_);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
        fullscreenCaptionButtonsVisible_ = false;
        ShowOsdNow(velo::localization::Text(config_.languageCode, velo::localization::TextId::ExitFullscreen), 850);
    }
    suppressWindowedPlacementTracking_ = false;
    LayoutChildren();
    UpdateWindow(hwnd_);
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(hwnd_, &cursor);
    HandlePointerActivity(cursor);
}

void MainWindow::ExitFullscreen() {
    if (fullscreen_) {
        ToggleFullscreen();
    }
}

void MainWindow::ExitFullscreenToWindowed() {
    if (!fullscreen_) {
        return;
    }

    suppressWindowedPlacementTracking_ = true;
    fullscreen_ = false;
    SetWindowLongW(hwnd_, GWL_STYLE, previousStyle_);
    SetWindowPos(hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    const WINDOWPLACEMENT restorePlacement = ResolveWindowedRestorePlacement(previousPlacement_, lastWindowedPlacement_);
    SetWindowPlacement(hwnd_, &restorePlacement);
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    lastWindowedPlacement_ = restorePlacement;
    lastWindowedPlacement_.length = sizeof(WINDOWPLACEMENT);
    lastWindowedPlacement_.showCmd = SW_SHOWNORMAL;

    SetActiveWindow(hwnd_);
    SetForegroundWindow(hwnd_);
    SetFocus(hwnd_);
    fullscreenCaptionButtonsVisible_ = false;
    ShowOsdNow(velo::localization::Text(config_.languageCode, velo::localization::TextId::ExitFullscreen), 850);
    suppressWindowedPlacementTracking_ = false;
    LayoutChildren();
    UpdateWindow(hwnd_);
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(hwnd_, &cursor);
    HandlePointerActivity(cursor);
}

void MainWindow::SetRecentFiles(std::vector<std::wstring> recentFiles) {
    recentFiles_ = std::move(recentFiles);
    UpdateEmptyState();
}

void MainWindow::SetPlaylistContext(const int currentIndex, const int totalCount, const bool temporaryPlaylist) {
    playlistCurrentIndex_ = currentIndex;
    playlistTotalCount_ = totalCount;
    temporaryPlaylist_ = temporaryPlaylist;
    ApplyStateToControls();
    UpdateMediaInfoPanel();
}

void MainWindow::PostEndOfPlayback(const bool hasNext, const bool willAutoplayNext) const {
    MSG pending{};
    while (PeekMessageW(&pending, hwnd_, kMessageEndPrompt, kMessageEndPrompt, PM_REMOVE) != FALSE) {
        delete reinterpret_cast<EndPromptState*>(pending.lParam);
    }

    auto* payload = new EndPromptState{hasNext, willAutoplayNext};
    if (PostMessageW(hwnd_, kMessageEndPrompt, 0, reinterpret_cast<LPARAM>(payload)) == FALSE) {
        delete payload;
    }
}

void MainWindow::PrepareForIncomingMedia() {
    MSG pending{};
    while (PeekMessageW(&pending, hwnd_, kMessageEndPrompt, kMessageEndPrompt, PM_REMOVE) != FALSE) {
        delete reinterpret_cast<EndPromptState*>(pending.lParam);
    }
    HideEndPrompt();
    HideOsdNow();
    HideSeekPreview();
    KillTimer(hwnd_, kTimerPendingSingleClick);
    pendingSingleClick_ = false;
    if (!mouseInsideWindow_) {
        SetControlsVisible(false, false);
    }
}

HWND MainWindow::WindowHandle() const noexcept {
    return hwnd_;
}

HWND MainWindow::VideoHostWindow() const noexcept {
    return videoHost_.WindowHandle();
}

velo::config::AppConfig MainWindow::CaptureConfig() const {
    velo::config::AppConfig config = config_;
    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
    GetWindowPlacement(hwnd_, &placement);
    const WINDOWPLACEMENT savedPlacement =
        fullscreen_ ? ResolveWindowedRestorePlacement(previousPlacement_, lastWindowedPlacement_)
                    : ResolveWindowedRestorePlacement(placement, lastWindowedPlacement_);
    const RECT rect = savedPlacement.rcNormalPosition;

    config.windowWidth = std::max(static_cast<LONG>(kMinimumWindowWidth), rect.right - rect.left);
    config.windowHeight = std::max(static_cast<LONG>(kMinimumWindowHeight), rect.bottom - rect.top);
    config.windowPosX = rect.left;
    config.windowPosY = rect.top;
    config.hasSavedWindowPlacement = true;
    config.startMaximized = fullscreen_ ? previousPlacement_.showCmd == SW_SHOWMAXIMIZED : IsZoomed(hwnd_) != FALSE;
    config.volume = static_cast<double>(volumeSlider_.Value());
    config.startupVolume = config.volume;
    return config;
}

void MainWindow::RememberWindowedPlacement() {
    if (hwnd_ == nullptr || fullscreen_ || suppressWindowedPlacementTracking_) {
        return;
    }

    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
    if (GetWindowPlacement(hwnd_, &placement) == FALSE ||
        !ShouldTrackWindowedPlacement(placement, IsZoomed(hwnd_) != FALSE, IsIconic(hwnd_) != FALSE)) {
        return;
    }

    lastWindowedPlacement_ = placement;
    lastWindowedPlacement_.length = sizeof(WINDOWPLACEMENT);
    lastWindowedPlacement_.showCmd = SW_SHOWNORMAL;
}

void MainWindow::ToggleWindowMaximizeState() {
    if (hwnd_ == nullptr || fullscreen_) {
        return;
    }

    if (IsZoomed(hwnd_) == FALSE) {
        ShowWindow(hwnd_, SW_MAXIMIZE);
        return;
    }

    WINDOWPLACEMENT placement{sizeof(WINDOWPLACEMENT)};
    GetWindowPlacement(hwnd_, &placement);
    const WINDOWPLACEMENT restorePlacement = ResolveWindowedRestorePlacement(placement, lastWindowedPlacement_);
    suppressWindowedPlacementTracking_ = true;
    SetWindowPlacement(hwnd_, &restorePlacement);
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    lastWindowedPlacement_ = restorePlacement;
    lastWindowedPlacement_.length = sizeof(WINDOWPLACEMENT);
    lastWindowedPlacement_.showCmd = SW_SHOWNORMAL;
    suppressWindowedPlacementTracking_ = false;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<MainWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->HandleMessage(message, wParam, lParam);
}

}  // namespace velo::ui
