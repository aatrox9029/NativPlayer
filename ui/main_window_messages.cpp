#include "ui/main_window_internal.h"

#include <cmath>
#include <memory>

namespace velo::ui {

LRESULT MainWindow::HandleCreateMessage() {
    uiFont_ = tokens::CreateAppFont(tokens::FontRole::Body);
    captionFont_ = tokens::CreateAppFont(tokens::FontRole::Caption);
    iconFont_ = tokens::CreateAppFont(tokens::FontRole::Icon);
    largeTextGlyphFont_ = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft JhengHei UI");
    largeIconFont_ = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    titleFont_ = tokens::CreateAppFont(tokens::FontRole::Display);
    osdFont_ = tokens::CreateAppFont(tokens::FontRole::H2);
    CreateChildControls();
    LayoutChildren();
    DragAcceptFiles(hwnd_, TRUE);
    UpdateEmptyState();
    return 0;
}

LRESULT MainWindow::HandleSizeMessage() {
    RememberWindowedPlacement();
    LayoutChildren();
    UpdateOsdLayout();
    if (!fullscreen_) {
        SetFullscreenCaptionButtonsVisible(false);
    }
    RefreshLocalizedText();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return 0;
}

LRESULT MainWindow::HandleExitSizeMoveMessage() {
    uiSuppressed_ = false;
    RememberWindowedPlacement();
    LayoutChildren();
    UpdateOsdLayout();
    for (HWND control : {seekSlider_.WindowHandle(), currentTimeLabel_, durationLabel_, volumeLabel_, volumeSlider_.WindowHandle(), volumeEdit_,
                         speedButton_, speedSlider_.WindowHandle(), speedEdit_}) {
        if (control != nullptr) {
            RedrawWindow(control, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(hwnd_, &cursor);
    if (state_.isLoaded) {
        HandlePointerActivity(cursor);
    } else {
        RedrawUiSurface(true);
    }
    RedrawUiSurface(true);
    return 0;
}

LRESULT MainWindow::HandleGetMinMaxInfo(const LPARAM lParam) const {
    auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
    minMaxInfo->ptMinTrackSize.x = kMinimumWindowWidth;
    minMaxInfo->ptMinTrackSize.y = kMinimumWindowHeight;

    MONITORINFO monitorInfo{sizeof(MONITORINFO)};
    if (hwnd_ != nullptr && GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo) != FALSE) {
        const RECT bounds = fullscreen_ ? monitorInfo.rcMonitor : monitorInfo.rcWork;
        minMaxInfo->ptMaxPosition.x = bounds.left - monitorInfo.rcMonitor.left;
        minMaxInfo->ptMaxPosition.y = bounds.top - monitorInfo.rcMonitor.top;
        minMaxInfo->ptMaxSize.x = bounds.right - bounds.left;
        minMaxInfo->ptMaxSize.y = bounds.bottom - bounds.top;
    }

    return 0;
}

LRESULT MainWindow::HandleMouseWheelMessage(WPARAM wParam, LPARAM lParam) {
    POINT cursor{};
    GetCursorPos(&cursor);
    if (quickBrowseVisible_ && quickBrowsePanel_.IsScreenPointInside(cursor)) {
        SendMessageW(quickBrowsePanel_.WindowHandle(), WM_MOUSEWHEEL, wParam, lParam);
        return 0;
    }
    ScreenToClient(hwnd_, &cursor);
    HandlePointerActivity(cursor);
    const int delta = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? config_.wheelVolumeStep : -config_.wheelVolumeStep;
    const int current = volumeSlider_.Value();
    const int next = std::clamp(current + delta, 0, 100);
    DispatchVolumeChange(next);
    volumeSlider_.SetValue(next);
    suppressVolumeTextEvents_ = true;
    SetWindowTextW(volumeEdit_, FormatVolumeInput(next).c_str());
    suppressVolumeTextEvents_ = false;
    return 0;
}

LRESULT MainWindow::HandleAppCommandMessage(LPARAM lParam) {
    switch (GET_APPCOMMAND_LPARAM(lParam)) {
        case APPCOMMAND_MEDIA_PLAY_PAUSE:
            state_.isPaused = !state_.isPaused;
            RefreshPlayPauseButton(true);
            callbacks_.togglePause();
            return 1;
        case APPCOMMAND_MEDIA_NEXTTRACK:
            callbacks_.playNext();
            return 1;
        case APPCOMMAND_MEDIA_PREVIOUSTRACK:
            callbacks_.playPrevious();
            return 1;
        default:
            return 0;
    }
}

double MainWindow::ConfiguredSeekDeltaSeconds(const bool forward) const {
    double delta = static_cast<double>(std::clamp(config_.seekStepSeconds, 1, 60));
    if (config_.seekStepMode == velo::config::SeekStepMode::Percent && state_.durationSeconds > 0.0) {
        delta = state_.durationSeconds * static_cast<double>(std::clamp(config_.seekStepPercent, 1, 100)) / 100.0;
    }
    return forward ? delta : -delta;
}

double MainWindow::ConfiguredSeekTargetSeconds(const bool forward) const {
    const double target = state_.positionSeconds + ConfiguredSeekDeltaSeconds(forward);
    if (state_.durationSeconds > 0.0) {
        return std::clamp(target, 0.0, state_.durationSeconds);
    }
    return std::max(0.0, target);
}

LRESULT MainWindow::HandleTimerMessage(WPARAM wParam) {
    if (wParam == kTimerHideControls) {
        if (!fullscreen_) {
            KillTimer(hwnd_, kTimerHideControls);
            SetControlsVisible(true, false);
            return 0;
        }
        if (mouseInsideWindow_ && (IsCursorInControlsRetentionZone() || IsCursorInFullscreenCaptionActivationZone())) {
            KillTimer(hwnd_, kTimerHideControls);
            return 0;
        }
        if (!startupControlsPinned_ && !quickBrowseVisible_ && !IsControlInteractionActive()) {
            SetControlsVisible(false, false);
        }
        return 0;
    }
    if (wParam == kTimerHideOsd) {
        HideOsdNow();
        return 0;
    }
    if (wParam == kTimerPendingSingleClick) {
        KillTimer(hwnd_, kTimerPendingSingleClick);
        if (pendingSingleClick_) {
            pendingSingleClick_ = false;
            SetControlsVisible(!controlsVisible_, false);
        }
        return 0;
    }
    if (wParam == kTimerAutoAdvance) {
        KillTimer(hwnd_, kTimerAutoAdvance);
        if (endPromptVisible_ && endPromptHasNext_) {
            HideEndPrompt();
            callbacks_.playNext();
        }
        return 0;
    }
    return 1;
}

LRESULT MainWindow::HandleCommandMessage(WPARAM wParam, LPARAM lParam) {
    const auto text = [&](const velo::localization::TextId id) {
        return velo::localization::Text(config_.languageCode, id);
    };
    const UINT commandId = LOWORD(wParam);

    if (commandId >= kDynamicAudioTrackCommandBase &&
        commandId < kDynamicAudioTrackCommandBase + static_cast<UINT>(state_.audioTracks.size() + 1)) {
        if (callbacks_.selectAudioTrack != nullptr) {
            if (commandId == kDynamicAudioTrackCommandBase) {
                callbacks_.selectAudioTrack(-1);
                ShowOsdNow(text(velo::localization::TextId::CycleAudioTrack) + L": off", 1000);
            } else {
                const auto& option = state_.audioTracks[static_cast<size_t>(commandId - kDynamicAudioTrackCommandBase - 1)];
                callbacks_.selectAudioTrack(option.id);
                ShowOsdNow(text(velo::localization::TextId::CycleAudioTrack) + L": " + Utf8ToWide(option.label), 1000);
            }
        }
        return 0;
    }

    if (commandId >= kRecentFileCommandBase &&
        commandId < kRecentFileCommandBase + static_cast<UINT>(recentFiles_.size())) {
        const size_t index = static_cast<size_t>(commandId - kRecentFileCommandBase);
        if (index < recentFiles_.size()) {
            return 0;
        }
        return 0;
    }

    if (commandId >= kDynamicSubtitleTrackCommandBase &&
        commandId < kDynamicSubtitleTrackCommandBase + static_cast<UINT>(state_.subtitleTracks.size() + 1)) {
        if (callbacks_.selectSubtitleTrack != nullptr) {
            if (commandId == kDynamicSubtitleTrackCommandBase) {
                callbacks_.selectSubtitleTrack(-1);
                ShowOsdNow(text(velo::localization::TextId::CycleSubtitleTrack) + L": off", 1000);
            } else {
                const auto& option = state_.subtitleTracks[static_cast<size_t>(commandId - kDynamicSubtitleTrackCommandBase - 1)];
                callbacks_.selectSubtitleTrack(option.id);
                ShowOsdNow(text(velo::localization::TextId::CycleSubtitleTrack) + L": " + Utf8ToWide(option.label), 1000);
            }
        }
        return 0;
    }

    if (commandId >= kDynamicAudioOutputCommandBase &&
        commandId < kDynamicAudioOutputCommandBase + static_cast<UINT>(state_.audioOutputs.size() + 1)) {
        if (callbacks_.setAudioOutputDevice != nullptr) {
            if (commandId == kDynamicAudioOutputCommandBase) {
                callbacks_.setAudioOutputDevice(L"auto");
                ShowOsdNow(text(velo::localization::TextId::SettingsAudioOutput) + L": " +
                               text(velo::localization::TextId::SettingsAudioDefault),
                           1000);
            } else {
                const auto& option = state_.audioOutputs[static_cast<size_t>(commandId - kDynamicAudioOutputCommandBase - 1)];
                callbacks_.setAudioOutputDevice(Utf8ToWide(option.id));
                ShowOsdNow(text(velo::localization::TextId::SettingsAudioOutput) + L": " + Utf8ToWide(option.label), 1000);
            }
        }
        return 0;
    }

    if (reinterpret_cast<HWND>(lParam) == seekSlider_.WindowHandle() && HIWORD(wParam) == ThemedSlider::kNotificationValueChanged &&
        !suppressSeekEvents_ && state_.durationSeconds > 0.0) {
        const int position = seekSlider_.Value();
        const double target = state_.durationSeconds * static_cast<double>(position) / 1000.0;
        if (seekSlider_.IsDragging() && callbacks_.seekAbsolutePreview != nullptr) {
            callbacks_.seekAbsolutePreview(target);
        } else {
            callbacks_.seekAbsolute(target);
        }
        UpdateSeekPreview();
        return 0;
    }
    if (reinterpret_cast<HWND>(lParam) == seekSlider_.WindowHandle() && HIWORD(wParam) == ThemedSlider::kNotificationHoverChanged) {
        UpdateSeekPreview();
        return 0;
    }
    if (reinterpret_cast<HWND>(lParam) == seekSlider_.WindowHandle() && HIWORD(wParam) == ThemedSlider::kNotificationHoverEnded) {
        HideSeekPreview();
        return 0;
    }
    if (reinterpret_cast<HWND>(lParam) == volumeSlider_.WindowHandle() && HIWORD(wParam) == ThemedSlider::kNotificationValueChanged &&
        !suppressVolumeEvents_) {
        const int position = volumeSlider_.Value();
        DispatchVolumeChange(position);
        suppressVolumeTextEvents_ = true;
        SetWindowTextW(volumeEdit_, FormatVolumeInput(position).c_str());
        suppressVolumeTextEvents_ = false;
        return 0;
    }
    if (reinterpret_cast<HWND>(lParam) == speedSlider_.WindowHandle() && HIWORD(wParam) == ThemedSlider::kNotificationValueChanged &&
        !suppressSpeedEvents_) {
        const double speed = static_cast<double>(speedSlider_.Value()) / 100.0;
        callbacks_.setPlaybackSpeed(speed);
        SetControlTextIfChanged(speedEdit_, FormatSpeedInput(speed));
        return 0;
    }
    if (reinterpret_cast<HWND>(lParam) == volumeEdit_) {
        if (HIWORD(wParam) == EN_CHANGE && !suppressVolumeTextEvents_) {
            SyncVolumeEditPreview();
        }
        if (HIWORD(wParam) == EN_KILLFOCUS) {
            CommitVolumeText();
        }
        return 0;
    }
    if (reinterpret_cast<HWND>(lParam) == speedEdit_) {
        if (HIWORD(wParam) == EN_CHANGE && !suppressSpeedTextEvents_) {
            SyncSpeedEditPreview();
        }
        if (HIWORD(wParam) == EN_KILLFOCUS) {
            CommitSpeedText();
        }
        return 0;
    }

    switch (commandId) {
        case kControlOpen:
        case static_cast<UINT>(ContextCommand::OpenFile):
            OpenFilePicker();
            return 0;
        case kControlRecent:
            return 0;
        case kControlQuickBrowse:
            ToggleQuickBrowse();
            return 0;
        case kControlPreviousTrack:
        case static_cast<UINT>(ContextCommand::PlayPrevious):
            callbacks_.playPrevious();
            return 0;
        case static_cast<UINT>(ContextCommand::OpenFolder):
            OpenFolderPicker();
            return 0;
        case static_cast<UINT>(ContextCommand::LoadSubtitle):
            OpenSubtitlePicker();
            return 0;
        case static_cast<UINT>(ContextCommand::TakeScreenshot):
            callbacks_.takeScreenshot();
            return 0;
        case kControlPlayPause:
        case static_cast<UINT>(ContextCommand::TogglePause):
            state_.isPaused = !state_.isPaused;
            RefreshPlayPauseButton(true);
            callbacks_.togglePause();
            ShowOsdNow(text(state_.isPaused ? velo::localization::TextId::Pause : velo::localization::TextId::Play), 850);
            return 0;
        case kControlNextTrack:
        case static_cast<UINT>(ContextCommand::PlayNext):
            callbacks_.playNext();
            return 0;
        case kControlMute:
        case kControlVolumeToggle:
        case static_cast<UINT>(ContextCommand::ToggleMute): {
            const bool nextMuted = !state_.isMuted;
            state_.isMuted = nextMuted;
            RefreshMuteButton(true);
            callbacks_.setMute(nextMuted);
            ShowOsdNow(text(nextMuted ? velo::localization::TextId::Mute : velo::localization::TextId::Unmute), 850);
            return 0;
        }
        case kControlSpeed:
        case static_cast<UINT>(ContextCommand::CycleSpeed):
            callbacks_.resetPlaybackSpeed();
            return 0;
        case static_cast<UINT>(ContextCommand::SlowerSpeed):
            callbacks_.adjustPlaybackSpeed(-0.10);
            return 0;
        case static_cast<UINT>(ContextCommand::FasterSpeed):
            callbacks_.adjustPlaybackSpeed(0.10);
            return 0;
        case static_cast<UINT>(ContextCommand::ResetSpeed):
            callbacks_.resetPlaybackSpeed();
            return 0;
        case static_cast<UINT>(ContextCommand::CycleAudio):
            callbacks_.cycleAudioTrack();
            ShowOsdNow(text(velo::localization::TextId::CycleAudioTrack), 1000);
            return 0;
        case kControlSubtitle:
        case static_cast<UINT>(ContextCommand::CycleSubtitle):
            callbacks_.cycleSubtitleTrack();
            ShowOsdNow(text(velo::localization::TextId::CycleSubtitleTrack), 1000);
            return 0;
        case kControlSettings:
        case static_cast<UINT>(ContextCommand::OpenSettings):
            OpenSettingsDialog();
            return 0;
        case kControlMore:
            ShowOverflowMenu();
            return 0;
        case kControlFullscreen:
        case static_cast<UINT>(ContextCommand::ToggleFullscreen):
            ToggleFullscreen();
            return 0;
        case kControlFullscreenDownload:
            if (callbacks_.openUpdateDownload != nullptr) {
                callbacks_.openUpdateDownload(updateDownloadUrl_);
            }
            return 0;
        case kControlFullscreenMinimize:
            ShowWindow(hwnd_, SW_MINIMIZE);
            return 0;
        case kControlFullscreenWindowed:
            if (fullscreen_) {
                ExitFullscreenToWindowed();
            } else {
                ToggleWindowMaximizeState();
                RefreshLocalizedText();
            }
            return 0;
        case kControlFullscreenClose:
            PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            return 0;
        case static_cast<UINT>(ContextCommand::ShowShortcuts):
            ShowShortcutHelp();
            return 0;
        case static_cast<UINT>(ContextCommand::ToggleMediaInfo):
            ToggleMediaInfo();
            return 0;
        case static_cast<UINT>(ContextCommand::ExportDiagnostics): {
            const std::wstring exportPath = callbacks_.exportDiagnostics != nullptr ? callbacks_.exportDiagnostics() : L"";
            ShowOsdNow(text(exportPath.empty() ? velo::localization::TextId::DiagnosticsExportFailed : velo::localization::TextId::DiagnosticsExported), 1000);
            return 0;
        }
        case static_cast<UINT>(ContextCommand::ShowAbout):
            MessageBoxW(hwnd_, callbacks_.buildAboutText != nullptr ? callbacks_.buildAboutText().c_str() : L"", app::AppDisplayName().c_str(),
                        MB_OK | MB_ICONINFORMATION);
            return 0;
        case kControlReplay:
        case static_cast<UINT>(ContextCommand::ReplayCurrent):
            HideEndPrompt();
            callbacks_.replayCurrent();
            ShowOsdNow(text(velo::localization::TextId::RestartedCurrentItem), 850);
            return 0;
        case static_cast<UINT>(ContextCommand::EndActionReplay):
            config_.endOfPlaybackAction = velo::config::EndOfPlaybackAction::Replay;
            callbacks_.applyConfig(config_);
            ShowOsdNow(text(velo::localization::TextId::EndActionReplay), 1000);
            return 0;
        case static_cast<UINT>(ContextCommand::EndActionPlayNext):
            config_.endOfPlaybackAction = velo::config::EndOfPlaybackAction::PlayNext;
            callbacks_.applyConfig(config_);
            ShowOsdNow(text(velo::localization::TextId::EndActionPlayNext), 1000);
            return 0;
        case static_cast<UINT>(ContextCommand::EndActionStop):
            config_.endOfPlaybackAction = velo::config::EndOfPlaybackAction::Stop;
            callbacks_.applyConfig(config_);
            ShowOsdNow(text(velo::localization::TextId::EndActionStop), 1000);
            return 0;
        case static_cast<UINT>(ContextCommand::EndActionCloseWindow):
            config_.endOfPlaybackAction = velo::config::EndOfPlaybackAction::CloseWindow;
            callbacks_.applyConfig(config_);
            ShowOsdNow(text(velo::localization::TextId::EndActionCloseWindow), 1000);
            return 0;
        case kControlEndNext:
        case static_cast<UINT>(ContextCommand::EndPromptNext):
            if (endPromptHasNext_) {
                HideEndPrompt();
                callbacks_.playNext();
            }
            return 0;
        default:
            return 1;
    }
}

LRESULT MainWindow::HandleKeyDownMessage(WPARAM wParam) {
    const auto text = [&](const velo::localization::TextId id) {
        return velo::localization::Text(config_.languageCode, id);
    };

    const platform::win32::InputCommandCallbacks inputCallbacks{
        .togglePause = callbacks_.togglePause,
        .seekRelative = [this, &text](const double seconds) {
            callbacks_.seekRelative(seconds);
            ShowOsdNow((seconds >= 0 ? text(velo::localization::TextId::ActionSeekForward) : text(velo::localization::TextId::ActionSeekBackward)) +
                           L" " + FormatTimeLabel(std::abs(seconds)),
                       850);
        },
        .seekConfigured = [this, &text](const bool forward) {
            const double seconds = ConfiguredSeekDeltaSeconds(forward);
            callbacks_.seekAbsolute(ConfiguredSeekTargetSeconds(forward));
            ShowOsdNow((seconds >= 0 ? text(velo::localization::TextId::ActionSeekForward) : text(velo::localization::TextId::ActionSeekBackward)) +
                           L" " + FormatTimeLabel(std::abs(seconds)),
                       850);
        },
        .adjustVolume = [this](const int delta) {
            const int current = volumeSlider_.Value();
            const int next = std::clamp(current + delta, 0, 100);
            DispatchVolumeChange(next);
            volumeSlider_.SetValue(next);
            suppressVolumeTextEvents_ = true;
            SetWindowTextW(volumeEdit_, FormatVolumeInput(next).c_str());
            suppressVolumeTextEvents_ = false;
        },
        .toggleMute = [this, &text]() {
            const bool nextMuted = !state_.isMuted;
            state_.isMuted = nextMuted;
            RefreshMuteButton(true);
            callbacks_.setMute(nextMuted);
            ShowOsdNow(text(nextMuted ? velo::localization::TextId::Mute : velo::localization::TextId::Unmute));
        },
        .openFile = [this]() { OpenFilePicker(); },
        .showRecentFiles = [this]() { ShowRecentFilesMenu(); },
        .cycleAudioTrack = [this, &text]() {
            callbacks_.cycleAudioTrack();
            ShowOsdNow(text(velo::localization::TextId::CycleAudioTrack));
        },
        .cycleSubtitleTrack = [this, &text]() {
            callbacks_.cycleSubtitleTrack();
            ShowOsdNow(text(velo::localization::TextId::CycleSubtitleTrack));
        },
        .takeScreenshot = callbacks_.takeScreenshot,
        .adjustPlaybackSpeed = [this](const double delta) {
            const double nextSpeed = std::clamp(state_.playbackSpeed + delta, 0.25, 3.0);
            callbacks_.adjustPlaybackSpeed(delta);
            speedSlider_.SetValue(static_cast<int>(std::lround(nextSpeed * 100.0)));
            SetControlTextIfChanged(speedEdit_, FormatSpeedInput(nextSpeed));
        },
        .resetPlaybackSpeed = [this]() {
            callbacks_.resetPlaybackSpeed();
            speedSlider_.SetValue(100);
            SetControlTextIfChanged(speedEdit_, L"1.00x");
        },
        .toggleFullscreen = [this]() { ToggleFullscreen(); },
        .exitFullscreen = [this]() { ExitFullscreen(); },
        .playPrevious = [this]() { callbacks_.playPrevious(); },
        .playNext = [this]() { callbacks_.playNext(); },
        .showMediaInfo = [this]() { ToggleMediaInfo(); },
    };
    if (platform::win32::HandleKeyDown(wParam, config_, inputCallbacks)) {
        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(hwnd_, &cursor);
        HandlePointerActivity(cursor);
        return 0;
    }
    return 1;
}

LRESULT MainWindow::HandlePaintMessage() {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(hwnd_, &paint);
    RECT client{};
    GetClientRect(hwnd_, &client);
    PaintWindowBackdrop(dc, client);
    EndPaint(hwnd_, &paint);
    return 0;
}

LRESULT MainWindow::HandlePlayerStateMessage(LPARAM lParam) {
    static_cast<void>(lParam);
    std::unique_ptr<PlayerState> updated;
    {
        std::scoped_lock lock(pendingPlayerStateMutex_);
        updated = std::move(pendingPlayerState_);
        playerStateMessageQueued_ = false;
    }
    if (updated == nullptr) {
        return 0;
    }

    const bool pausedChanged = state_.isPaused != updated->isPaused || state_.isLoaded != updated->isLoaded;
    const bool muteChanged = state_.isMuted != updated->isMuted;
    const bool audioChanged = state_.audioTrackId != updated->audioTrackId || state_.isLoaded != updated->isLoaded;
    const bool subtitleChanged = state_.subtitleTrackId != updated->subtitleTrackId || state_.isLoaded != updated->isLoaded;
    state_ = *updated;
    ApplyStateToControls();
    if (pausedChanged) {
        RefreshPlayPauseButton();
    }
    if (muteChanged) {
        RefreshMuteButton();
    }
    if (audioChanged) {
        RedrawUiSurface(true);
    }
    if (subtitleChanged) {
        RefreshSubtitleButton();
    }
    if (endPromptVisible_ && !ShouldKeepEndPromptVisible()) {
        HideEndPrompt();
    }

    bool shouldPost = false;
    {
        std::scoped_lock lock(pendingPlayerStateMutex_);
        if (pendingPlayerState_ != nullptr && !playerStateMessageQueued_) {
            playerStateMessageQueued_ = true;
            shouldPost = true;
        }
    }
    if (shouldPost && PostMessageW(hwnd_, kMessagePlayerState, 0, 0) == FALSE) {
        std::scoped_lock lock(pendingPlayerStateMutex_);
        playerStateMessageQueued_ = false;
    }
    return 0;
}

LRESULT MainWindow::HandleOsdMessage(LPARAM lParam) {
    std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lParam));
    ShowOsdNow(*text);
    return 0;
}

LRESULT MainWindow::HandleOpenFileMessage(LPARAM lParam) {
    std::unique_ptr<std::wstring> path(reinterpret_cast<std::wstring*>(lParam));
    BringToFront();
    callbacks_.openFile(*path);
    return 0;
}

LRESULT MainWindow::HandleEndPromptMessage(LPARAM lParam) {
    std::unique_ptr<EndPromptState> prompt(reinterpret_cast<EndPromptState*>(lParam));
    if (!ShouldKeepEndPromptVisible()) {
        return 0;
    }
    ShowEndPrompt(*prompt);
    return 0;
}

LRESULT MainWindow::HandleUpdateAvailabilityMessage(LPARAM lParam) {
    std::unique_ptr<UpdateAvailabilityState> update(reinterpret_cast<UpdateAvailabilityState*>(lParam));
    if (update == nullptr || update->versionTag.empty()) {
        return 0;
    }

    updateAvailable_ = true;
    availableUpdateTag_ = update->versionTag;
    updateDownloadUrl_ = update->downloadUrl;
    LayoutChildren();
    RedrawUiSurface(true);
    return 0;
}

LRESULT MainWindow::HandleStaticColorMessage(WPARAM wParam, LPARAM lParam) {
    const auto& palette = tokens::DarkPalette();
    const HWND control = reinterpret_cast<HWND>(lParam);
    HDC dc = reinterpret_cast<HDC>(wParam);
    if (control == osdLabel_) {
        SetBkMode(dc, OPAQUE);
        SetTextColor(dc, palette.textPrimary);
        SetBkColor(dc, palette.bgCanvas);
        return reinterpret_cast<LRESULT>(BackgroundBrush());
    }
    if (control == currentTimeLabel_ || control == durationLabel_) {
        SetBkMode(dc, OPAQUE);
        SetTextColor(dc, palette.textPrimary);
        SetBkColor(dc, RGB(0, 0, 0));
        return reinterpret_cast<LRESULT>(GetStockObject(BLACK_BRUSH));
    }
    if (control == titleBarBackground_ || control == panelBackground_ || control == titleLabel_) {
        SetBkMode(dc, OPAQUE);
        SetTextColor(dc, palette.textPrimary);
        SetBkColor(dc, RGB(0, 0, 0));
        return reinterpret_cast<LRESULT>(GetStockObject(BLACK_BRUSH));
    }
    if (control == mediaInfoLabel_) {
        SetBkMode(dc, OPAQUE);
        SetTextColor(dc, palette.textSecondary);
        SetBkColor(dc, tokens::MixColor(palette.bgOverlay, palette.bgSurface1, 0.18));
        return reinterpret_cast<LRESULT>(ControlsPanelBrush());
    }
    SetBkMode(dc, TRANSPARENT);
    const bool secondaryText = control == volumeLabel_ || control == speedLabel_ || control == endPromptHintLabel_;
    SetTextColor(dc, secondaryText ? palette.textSecondary : palette.textPrimary);
    SetBkColor(dc, palette.bgCanvas);
    return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
}

LRESULT MainWindow::HandleEditColorMessage(WPARAM wParam) {
    const auto& palette = tokens::DarkPalette();
    HDC dc = reinterpret_cast<HDC>(wParam);
    SetBkMode(dc, OPAQUE);
    SetTextColor(dc, palette.textPrimary);
    SetBkColor(dc, RGB(0, 0, 0));
    return reinterpret_cast<LRESULT>(InputBrush());
}

LRESULT MainWindow::HandleNcHitTest(const LPARAM lParam) const {
    if (hwnd_ == nullptr) {
        return HTCLIENT;
    }
    if (fullscreen_) {
        return HTCLIENT;
    }

    POINT screenPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    RECT windowRect{};
    GetWindowRect(hwnd_, &windowRect);
    const bool onLeft = screenPoint.x >= windowRect.left && screenPoint.x < windowRect.left + kWindowResizeBorder;
    const bool onRight = screenPoint.x < windowRect.right && screenPoint.x >= windowRect.right - kWindowResizeBorder;
    const bool onTop = screenPoint.y >= windowRect.top && screenPoint.y < windowRect.top + kWindowResizeBorder;
    const bool onBottom = screenPoint.y < windowRect.bottom && screenPoint.y >= windowRect.bottom - kWindowResizeBorder;

    if (onTop && onLeft) {
        return HTTOPLEFT;
    }
    if (onTop && onRight) {
        return HTTOPRIGHT;
    }
    if (onBottom && onLeft) {
        return HTBOTTOMLEFT;
    }
    if (onBottom && onRight) {
        return HTBOTTOMRIGHT;
    }
    if (onTop) {
        return HTTOP;
    }
    if (onBottom) {
        return HTBOTTOM;
    }
    if (onLeft) {
        return HTLEFT;
    }
    if (onRight) {
        return HTRIGHT;
    }

    POINT clientPoint = screenPoint;
    ScreenToClient(hwnd_, &clientPoint);
    RECT client{};
    GetClientRect(hwnd_, &client);
    const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_,
                                                            !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_));
    if (layout.topBarHeight > 0 && clientPoint.y >= 0 && clientPoint.y < layout.topBarHeight && !IsPointInCaptionButtons(clientPoint)) {
        return HTCAPTION;
    }
    return HTCLIENT;
}

LRESULT MainWindow::HandleNcCalcSize(const WPARAM wParam, const LPARAM lParam) const {
    static_cast<void>(lParam);
    if (wParam == TRUE || wParam == FALSE) {
        return 0;
    }
    return 0;
}

LRESULT MainWindow::HandleNcActivate(const WPARAM wParam, const LPARAM lParam) {
    static_cast<void>(wParam);
    static_cast<void>(lParam);
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    return TRUE;
}

LRESULT MainWindow::HandleNcPaint(const WPARAM wParam, const LPARAM lParam) const {
    static_cast<void>(wParam);
    static_cast<void>(lParam);
    return 0;
}

LRESULT MainWindow::HandleNcDestroyMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (uiFont_ != nullptr) {
        DeleteObject(uiFont_);
    }
    if (captionFont_ != nullptr) {
        DeleteObject(captionFont_);
    }
    if (iconFont_ != nullptr) {
        DeleteObject(iconFont_);
    }
    if (largeTextGlyphFont_ != nullptr) {
        DeleteObject(largeTextGlyphFont_);
    }
    if (largeIconFont_ != nullptr) {
        DeleteObject(largeIconFont_);
    }
    if (titleFont_ != nullptr) {
        DeleteObject(titleFont_);
    }
    if (osdFont_ != nullptr) {
        DeleteObject(osdFont_);
    }
    DestroyAppIconSet(appIcons_);
    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    const auto text = [&](const velo::localization::TextId id) {
        return velo::localization::Text(config_.languageCode, id);
    };

    switch (message) {
        case WM_CREATE:
            return HandleCreateMessage();
        case WM_SIZE:
            return HandleSizeMessage();
        case WM_GETMINMAXINFO:
            return HandleGetMinMaxInfo(lParam);
        case WM_NCCALCSIZE:
            return HandleNcCalcSize(wParam, lParam);
        case WM_NCACTIVATE:
            return HandleNcActivate(wParam, lParam);
        case WM_NCPAINT:
            return HandleNcPaint(wParam, lParam);
        case WM_NCHITTEST:
            return HandleNcHitTest(lParam);
        case WM_ENTERSIZEMOVE:
            uiSuppressed_ = true;
            HideOsdNow();
            SetControlsVisible(false, false);
            LayoutChildren();
            RedrawUiSurface(true);
            return 0;
        case WM_EXITSIZEMOVE:
            return HandleExitSizeMoveMessage();
        case WM_LBUTTONDOWN:
            if (HandleQuickBrowseOutsideClick({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)})) {
                return 0;
            }
            if (!fullscreen_) {
                const POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                const HWND hit = ChildWindowFromPointEx(hwnd_, clientPoint, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
                const bool dragUiSurface = hit == hwnd_ || hit == titleBarBackground_ || hit == panelBackground_ || hit == titleLabel_;
                if (dragUiSurface && IsPointInControlsActivationZone(clientPoint) && !IsPointInCaptionButtons(clientPoint)) {
                    ReleaseCapture();
                    SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(clientPoint.x, clientPoint.y));
                    return 0;
                }
            }
            break;
        case WM_LBUTTONDBLCLK: {
            if (!fullscreen_) {
                POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                RECT client{};
                GetClientRect(hwnd_, &client);
                const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_,
                                                                        !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_));
                if (layout.topBarHeight > 0 && clientPoint.y < layout.topBarHeight && !IsPointInCaptionButtons(clientPoint)) {
                    ToggleWindowMaximizeState();
                    RefreshLocalizedText();
                    return 0;
                }
            }
            break;
        }
        case WM_LBUTTONUP:
            if (!state_.isLoaded) {
                const RECT dropRect = EmptyDropZoneRect();
                const POINT clickPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (PtInRect(&dropRect, clickPoint) != FALSE) {
                    OpenFolderPicker();
                    return 0;
                }
            }
            break;
        case WM_MOUSEMOVE:
        case velo::platform::win32::kVideoHostMouseMoveMessage:
            if (message == velo::platform::win32::kVideoHostMouseMoveMessage && videoDragPending_ && (wParam & MK_LBUTTON) != 0 && !fullscreen_) {
                POINT cursor{};
                GetCursorPos(&cursor);
                const int dragX = GetSystemMetrics(SM_CXDRAG);
                const int dragY = GetSystemMetrics(SM_CYDRAG);
                if (std::abs(cursor.x - videoDragStartScreen_.x) >= dragX || std::abs(cursor.y - videoDragStartScreen_.y) >= dragY) {
                    videoDragPending_ = false;
                    suppressNextVideoSingleClick_ = true;
                    pendingSingleClick_ = false;
                    KillTimer(hwnd_, kTimerPendingSingleClick);
                    ReleaseCapture();
                    SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(cursor.x, cursor.y));
                    return 0;
                }
            }
            HandlePointerActivity({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            return 0;
        case WM_MOUSELEAVE:
        case velo::platform::win32::kVideoHostMouseLeaveMessage:
            HandlePointerLeave();
            return 0;
        case WM_MOUSEWHEEL:
            return HandleMouseWheelMessage(wParam, lParam);
        case WM_MBUTTONUP: {
            const platform::win32::InputCommandCallbacks inputCallbacks{
                .togglePause = callbacks_.togglePause,
                .seekRelative = [this](double seconds) { callbacks_.seekRelative(seconds); },
                .seekConfigured = [this](const bool forward) { callbacks_.seekAbsolute(ConfiguredSeekTargetSeconds(forward)); },
                .adjustVolume = [this](int delta) {
                    const int next = std::clamp(volumeSlider_.Value() + delta, 0, 100);
                    DispatchVolumeChange(next);
                    volumeSlider_.SetValue(next);
                },
                .toggleMute = [this]() { callbacks_.setMute(!state_.isMuted); },
                .openFile = [this]() { OpenFilePicker(); },
                .showRecentFiles = [this]() { ShowRecentFilesMenu(); },
                .cycleAudioTrack = callbacks_.cycleAudioTrack,
                .cycleSubtitleTrack = callbacks_.cycleSubtitleTrack,
                .takeScreenshot = callbacks_.takeScreenshot,
                .adjustPlaybackSpeed = callbacks_.adjustPlaybackSpeed,
                .resetPlaybackSpeed = callbacks_.resetPlaybackSpeed,
                .toggleFullscreen = [this]() { ToggleFullscreen(); },
                .exitFullscreen = [this]() { ExitFullscreen(); },
                .playPrevious = callbacks_.playPrevious,
                .playNext = callbacks_.playNext,
                .showMediaInfo = [this]() { ToggleMediaInfo(); },
            };
            if (platform::win32::HandlePointerBinding(VK_MBUTTON, config_, inputCallbacks)) {
                return 0;
            }
            ExecuteMouseAction(config_.middleClickAction);
            return 0;
        }
        case velo::platform::win32::kVideoHostMouseWheelMessage:
            return HandleMouseWheelMessage(wParam, lParam);
        case velo::platform::win32::kVideoHostMButtonUpMessage:
            return HandleMessage(WM_MBUTTONUP, wParam, lParam);
        case velo::platform::win32::kVideoHostKeyDownMessage:
            return HandleKeyDownMessage(wParam) == 1 ? DefWindowProcW(hwnd_, WM_KEYDOWN, wParam, lParam) : 0;
        case WM_XBUTTONUP:
        case velo::platform::win32::kVideoHostXButtonUpMessage: {
            const unsigned int virtualKey = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
            const platform::win32::InputCommandCallbacks inputCallbacks{
                .togglePause = callbacks_.togglePause,
                .seekRelative = [this](double seconds) { callbacks_.seekRelative(seconds); },
                .seekConfigured = [this](const bool forward) { callbacks_.seekAbsolute(ConfiguredSeekTargetSeconds(forward)); },
                .adjustVolume = [this](int delta) {
                    const int next = std::clamp(volumeSlider_.Value() + delta, 0, 100);
                    DispatchVolumeChange(next);
                    volumeSlider_.SetValue(next);
                },
                .toggleMute = [this]() { callbacks_.setMute(!state_.isMuted); },
                .openFile = [this]() { OpenFilePicker(); },
                .showRecentFiles = [this]() { ShowRecentFilesMenu(); },
                .cycleAudioTrack = callbacks_.cycleAudioTrack,
                .cycleSubtitleTrack = callbacks_.cycleSubtitleTrack,
                .takeScreenshot = callbacks_.takeScreenshot,
                .adjustPlaybackSpeed = callbacks_.adjustPlaybackSpeed,
                .resetPlaybackSpeed = callbacks_.resetPlaybackSpeed,
                .toggleFullscreen = [this]() { ToggleFullscreen(); },
                .exitFullscreen = [this]() { ExitFullscreen(); },
                .playPrevious = callbacks_.playPrevious,
                .playNext = callbacks_.playNext,
                .showMediaInfo = [this]() { ToggleMediaInfo(); },
            };
            if (platform::win32::HandlePointerBinding(virtualKey, config_, inputCallbacks)) {
                return TRUE;
            }
            return 0;
        }
        case WM_DROPFILES:
            HandleDroppedFile(reinterpret_cast<HDROP>(wParam));
            return 0;
        case WM_CONTEXTMENU:
            ShowContextMenu(ContextMenuPointFromLParam(lParam), lParam == -1);
            return 0;
        case velo::platform::win32::kVideoHostContextMenuMessage: {
            POINT screen{};
            GetCursorPos(&screen);
            ShowContextMenu(screen, false);
            return 0;
        }
        case velo::platform::win32::kVideoHostSingleClickMessage:
            if (suppressNextVideoSingleClick_) {
                suppressNextVideoSingleClick_ = false;
                return 0;
            }
            if (HandleQuickBrowseOutsideClick({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)})) {
                return 0;
            }
            pendingSingleClick_ = true;
            SetTimer(hwnd_, kTimerPendingSingleClick, GetDoubleClickTime(), nullptr);
            return 0;
        case velo::platform::win32::kVideoHostLButtonDownMessage:
            if (!fullscreen_ && state_.isLoaded && !quickBrowseVisible_) {
                videoDragPending_ = true;
                GetCursorPos(&videoDragStartScreen_);
            }
            return 0;
        case velo::platform::win32::kVideoHostLButtonUpMessage:
            videoDragPending_ = false;
            return 0;
        case velo::platform::win32::kVideoHostDoubleClickMessage:
            pendingSingleClick_ = false;
            KillTimer(hwnd_, kTimerPendingSingleClick);
            ExecuteMouseAction(config_.doubleClickAction);
            return 0;
        case WM_DEVICECHANGE:
            callbacks_.recoverAudioOutput();
            ShowOsdNow(text(velo::localization::TextId::AudioDeviceRecovered));
            return TRUE;
        case WM_DISPLAYCHANGE:
            ShowOsdNow(text(velo::localization::TextId::DisplayConfigApplied));
            return 0;
        case WM_DPICHANGED:
            ShowOsdNow(text(velo::localization::TextId::DpiUpdated));
            return 0;
        case WM_POWERBROADCAST:
            if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) {
                callbacks_.recoverAudioOutput();
                ShowOsdNow(text(velo::localization::TextId::WakeRecoveredAudio));
            }
            return TRUE;
        case WM_APPCOMMAND:
            return HandleAppCommandMessage(lParam) == 0 ? DefWindowProcW(hwnd_, message, wParam, lParam) : 1;
        case WM_TIMER:
            return HandleTimerMessage(wParam);
        case WM_COMMAND:
            return HandleCommandMessage(wParam, lParam) == 1 ? DefWindowProcW(hwnd_, message, wParam, lParam) : 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            return HandleKeyDownMessage(wParam) == 1 ? DefWindowProcW(hwnd_, message, wParam, lParam) : 0;
        case WM_DRAWITEM:
            if (reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->CtlType == ODT_MENU) {
                DrawMenuItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            } else {
                DrawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            }
            return TRUE;
        case WM_MEASUREITEM:
            if (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlType == ODT_MENU) {
                MeasureMenuItem(*reinterpret_cast<MEASUREITEMSTRUCT*>(lParam));
                return TRUE;
            }
            break;
        case WM_CTLCOLORSTATIC:
            return HandleStaticColorMessage(wParam, lParam);
        case WM_CTLCOLOREDIT:
            return HandleEditColorMessage(wParam);
        case WM_PAINT:
            return HandlePaintMessage();
        case kMessagePlayerState:
            return HandlePlayerStateMessage(lParam);
        case kMessageOsd:
            return HandleOsdMessage(lParam);
        case kMessageOpenFile:
            return HandleOpenFileMessage(lParam);
        case kMessageEndPrompt:
            return HandleEndPromptMessage(lParam);
        case kMessageUpdateAvailable:
            return HandleUpdateAvailabilityMessage(lParam);
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            KillTimer(hwnd_, kTimerHideControls);
            KillTimer(hwnd_, kTimerHideOsd);
            KillTimer(hwnd_, kTimerPendingSingleClick);
            KillTimer(hwnd_, kTimerAutoAdvance);
            PostQuitMessage(0);
            return 0;
        case WM_NCDESTROY:
            return HandleNcDestroyMessage(message, wParam, lParam);
        default:
            break;
    }
    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

}  // namespace velo::ui
