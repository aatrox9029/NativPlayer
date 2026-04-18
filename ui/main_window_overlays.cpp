#include "ui/main_window_internal.h"

namespace velo::ui {

bool MainWindow::ShouldKeepEndPromptVisible() const {
    if (state_.eofReached) {
        return true;
    }

    if (!state_.isLoaded || state_.durationSeconds <= 0.0) {
        return false;
    }

    constexpr double kEndPromptToleranceSeconds = 0.75;
    return state_.positionSeconds >= std::max(0.0, state_.durationSeconds - kEndPromptToleranceSeconds);
}

void MainWindow::ShowShortcutHelp() {
    const auto shortcutText = BuildShortcutHelpText(config_);
    MessageBoxW(hwnd_, shortcutText.c_str(), velo::localization::Text(config_.languageCode, velo::localization::TextId::ShortcutHelpTitle).c_str(),
                MB_OK | MB_ICONINFORMATION);
}

void MainWindow::ShowOsdNow(const std::wstring& text, int durationMs) {
    if (text.empty()) {
        HideOsdNow();
        return;
    }

    RECT previousRect{};
    const bool hadVisibleOsd = IsWindowVisible(osdLabel_) != FALSE;
    if (hadVisibleOsd) {
        GetWindowRect(osdLabel_, &previousRect);
        MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<POINT*>(&previousRect), 2);
        ShowWindow(osdLabel_, SW_HIDE);
        SetWindowTextW(osdLabel_, L"");
        RedrawWindow(hwnd_, &previousRect, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    }

    SetWindowTextW(osdLabel_, text.c_str());
    UpdateOsdLayout();
    SetWindowPos(osdLabel_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    RedrawWindow(osdLabel_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    KillTimer(hwnd_, kTimerHideOsd);
    SetTimer(hwnd_, kTimerHideOsd, std::max(1, durationMs), nullptr);
}

void MainWindow::HideOsdNow() {
    if (hwnd_ == nullptr || osdLabel_ == nullptr) {
        return;
    }

    KillTimer(hwnd_, kTimerHideOsd);
    if (IsWindowVisible(osdLabel_) == FALSE) {
        SetWindowTextW(osdLabel_, L"");
        return;
    }

    RECT previousRect{};
    GetWindowRect(osdLabel_, &previousRect);
    MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<POINT*>(&previousRect), 2);
    ShowWindow(osdLabel_, SW_HIDE);
    SetWindowTextW(osdLabel_, L"");
    RedrawWindow(hwnd_, &previousRect, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void MainWindow::OpenSettingsDialog() {
    const auto updated = settingsDialog_.ShowModal(instance_, hwnd_, CaptureConfig(), [this](const velo::config::AppConfig& previewConfig) {
        config_ = previewConfig;
        callbacks_.applyConfig(config_);
        RefreshLocalizedText();
        UpdateMediaInfoPanel();
    });
    if (!updated.has_value()) {
        return;
    }

    config_ = *updated;
    autoHideDelayMs_ = config_.controlsHideDelayMs;
    seekPreviewPopup_.SetEnabled(config_.showSeekPreview);
    callbacks_.applyConfig(config_);
    RefreshLocalizedText();
    UpdateMediaInfoPanel();
    ShowOsdNow(velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsUpdated), 1000);

    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(hwnd_, &cursor);
    HandlePointerActivity(cursor);
}

void MainWindow::ToggleMediaInfo() {
    mediaInfoVisible_ = !mediaInfoVisible_;
    UpdateMediaInfoPanel();
    ShowOsdNow(velo::localization::Text(config_.languageCode,
                                        mediaInfoVisible_ ? velo::localization::TextId::MediaInfoShown
                                                          : velo::localization::TextId::MediaInfoHidden),
               1000);
}

void MainWindow::UpdateMediaInfoPanel() {
    std::wstring content;
    if (state_.isLoaded) {
        content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoTitle);
        content += L": ";
        content += state_.mediaTitle.empty() ? velo::app::ShortDisplayName(Utf8ToWide(state_.currentPath)) : Utf8ToWide(state_.mediaTitle);
        content += L"\r\n";
        content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoTime);
        content += L": ";
        content += FormatTimeLabel(state_.positionSeconds);
        content += L" / ";
        content += FormatTimeLabel(state_.durationSeconds);
        content += L"\r\n";
        content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoFormat);
        content += L": ";
        content += state_.fileFormat.empty() ? velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoUnknown)
                                             : Utf8ToWide(state_.fileFormat);
        content += L"\r\n";
        content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoHardwareDecode);
        content += L": ";
        content += state_.hwdecCurrent.empty() ? velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoSoftwareAuto)
                                               : Utf8ToWide(state_.hwdecCurrent);
        if (playlistTotalCount_ > 0) {
            content += L"\r\n";
            content += velo::localization::Text(config_.languageCode, velo::localization::TextId::PlaylistLabel);
            content += L": ";
            content += std::to_wstring(std::max(0, playlistCurrentIndex_ + 1));
            content += L"/";
            content += std::to_wstring(playlistTotalCount_);
        }
        if (config_.showDebugInfo) {
            content += L"\r\n\r\n[Debug] ";
            content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoDebugPath);
            content += L": ";
            content += state_.currentPath.empty() ? L"-" : Utf8ToWide(state_.currentPath);
            content += L"\r\n[Debug] ";
            content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoDebugAudio);
            content += L": ";
            content += state_.audioTrackId >= 0 ? std::to_wstring(state_.audioTrackId)
                                                : velo::localization::Text(config_.languageCode, velo::localization::TextId::StateOff);
            content += L"\r\n[Debug] ";
            content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoDebugSubtitle);
            content += L": ";
            content += state_.subtitleTrackId >= 0 ? std::to_wstring(state_.subtitleTrackId)
                                                   : velo::localization::Text(config_.languageCode, velo::localization::TextId::StateOff);
        }
    } else {
        content = velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoNotLoaded);
        content += L"\r\n";
        content += velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaInfoNotLoadedHint);
    }

    SetWindowTextW(mediaInfoLabel_, content.c_str());
    InvalidateRect(mediaInfoLabel_, nullptr, TRUE);
    ShowWindow(mediaInfoLabel_, mediaInfoVisible_ ? SW_SHOW : SW_HIDE);
}

void MainWindow::ExecuteMouseAction(const std::wstring& actionId) {
    if (actionId == L"toggle_pause") {
        callbacks_.togglePause();
        ShowOsdNow(velo::localization::Text(config_.languageCode,
                                            state_.isPaused ? velo::localization::TextId::Pause : velo::localization::TextId::Play),
                   850);
    } else if (actionId == L"play_next") {
        callbacks_.playNext();
    } else if (actionId == L"none" || actionId == L"show_info") {
        return;
    } else {
        ToggleFullscreen();
    }
}

void MainWindow::ShowEndPrompt(const EndPromptState& state) {
    if (!ShouldKeepEndPromptVisible()) {
        return;
    }

    const bool alreadyVisible = endPromptVisible_;
    const bool stateChanged = !alreadyVisible || endPromptHasNext_ != state.hasNext || endPromptWillAutoplayNext_ != state.willAutoplayNext;
    if (!stateChanged) {
        return;
    }

    HideOsdNow();
    endPromptVisible_ = true;
    endPromptHasNext_ = state.hasNext;
    endPromptWillAutoplayNext_ = state.willAutoplayNext;
    SetControlTextIfChanged(endPromptTitleLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEndPromptTitle));
    if (state.hasNext && state.willAutoplayNext) {
        SetControlTextIfChanged(endPromptHintLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEndPromptAutoplayNext));
        SetTimer(hwnd_, kTimerAutoAdvance, 1800, nullptr);
    } else if (state.hasNext) {
        SetControlTextIfChanged(endPromptHintLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEndPromptReplayNext));
        KillTimer(hwnd_, kTimerAutoAdvance);
    } else {
        SetControlTextIfChanged(endPromptHintLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEndPromptReplayOnly));
        KillTimer(hwnd_, kTimerAutoAdvance);
    }
    EnableWindow(nextButton_, state.hasNext ? TRUE : FALSE);
    ShowWindow(endPromptTitleLabel_, SW_SHOW);
    ShowWindow(endPromptHintLabel_, SW_SHOW);
    ShowWindow(replayButton_, SW_SHOW);
    ShowWindow(nextButton_, SW_SHOW);
    LayoutChildren();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::HideEndPrompt() {
    if (!endPromptVisible_) {
        KillTimer(hwnd_, kTimerAutoAdvance);
        endPromptWillAutoplayNext_ = false;
        return;
    }

    endPromptVisible_ = false;
    endPromptHasNext_ = false;
    endPromptWillAutoplayNext_ = false;
    KillTimer(hwnd_, kTimerAutoAdvance);
    ShowWindow(endPromptTitleLabel_, SW_HIDE);
    ShowWindow(endPromptHintLabel_, SW_HIDE);
    ShowWindow(replayButton_, SW_HIDE);
    ShowWindow(nextButton_, SW_HIDE);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

}  // namespace velo::ui
