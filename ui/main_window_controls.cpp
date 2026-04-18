#include "ui/main_window_internal.h"

#include "app/media_file_types.h"

namespace velo::ui {

void MainWindow::CreateChildControls() {
    videoHost_.Create(instance_, hwnd_);
    quickBrowsePanel_.Create(instance_, hwnd_, {
        .openFile = [this](const std::wstring& path) {
            if (callbacks_.openFile != nullptr) {
                callbacks_.openFile(path);
            }
        },
        .navigateFolder = [this](const std::wstring& folderPath) { NavigateQuickBrowseFolder(folderPath); },
        .closePanel = [this]() { HideQuickBrowse(); },
    });
    quickBrowsePanel_.SetLanguageCode(config_.languageCode);
    seekPreviewPopup_.Create(instance_, hwnd_);
    seekPreviewPopup_.SetEnabled(config_.showSeekPreview);

    titleBarBackground_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    panelBackground_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    titleLabel_ = CreateWindowExW(0, L"STATIC", app::AppDisplayName().c_str(),
                                  WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS,
                                  0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);

    openButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(kControlOpen), instance_, nullptr);
    recentButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                    reinterpret_cast<HMENU>(kControlRecent), instance_, nullptr);
    quickBrowseButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                         reinterpret_cast<HMENU>(kControlQuickBrowse), instance_, nullptr);
    previousButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                      reinterpret_cast<HMENU>(kControlPreviousTrack), instance_, nullptr);
    playPauseButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                       reinterpret_cast<HMENU>(kControlPlayPause), instance_, nullptr);
    nextTrackButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                       reinterpret_cast<HMENU>(kControlNextTrack), instance_, nullptr);
    muteButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(kControlMute), instance_, nullptr);
    speedButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                   reinterpret_cast<HMENU>(kControlSpeed), instance_, nullptr);
    subtitleButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                      reinterpret_cast<HMENU>(kControlSubtitle), instance_, nullptr);
    settingsButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                      reinterpret_cast<HMENU>(kControlSettings), instance_, nullptr);
    moreButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(kControlMore), instance_, nullptr);
    fullscreenButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                        reinterpret_cast<HMENU>(kControlFullscreen), instance_, nullptr);
    volumeLabel_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                   reinterpret_cast<HMENU>(kControlVolumeToggle), instance_, nullptr);
    speedLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    currentTimeLabel_ = CreateWindowExW(0, L"STATIC", L"0:00", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    durationLabel_ = CreateWindowExW(0, L"STATIC", L"0:00", WS_CHILD | WS_VISIBLE | SS_RIGHT, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);

    seekSlider_.Create(instance_, hwnd_, kControlSeek);
    seekSlider_.SetRange(0, 1000);
    volumeSlider_.Create(instance_, hwnd_, kControlVolume);
    volumeSlider_.SetRange(0, 100);
    volumeSlider_.SetValue(100);
    speedSlider_.Create(instance_, hwnd_, kControlSpeedSlider);
    speedSlider_.SetRange(kSpeedMinimum, kSpeedMaximum);
    speedSlider_.SetValue(100);

    volumeEdit_ = CreateWindowExW(0, L"EDIT", L"100", WS_CHILD | ES_CENTER | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(kControlVolumeEdit), instance_, nullptr);
    speedEdit_ = CreateWindowExW(0, L"EDIT", L"1.00x", WS_CHILD | ES_CENTER | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd_,
                                 reinterpret_cast<HMENU>(kControlSpeedEdit), instance_, nullptr);
    SendMessageW(volumeEdit_, EM_SETLIMITTEXT, 3, 0);
    SendMessageW(speedEdit_, EM_SETLIMITTEXT, 5, 0);
    SetWindowSubclass(volumeEdit_, EditInputSubclassProc, 1, 0);
    SetWindowSubclass(speedEdit_, EditInputSubclassProc, 2, 0);

    statusLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS, 0, 0, 0, 0,
                                   hwnd_, nullptr, instance_, nullptr);
    mediaInfoLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    emptyTitleLabel_ = CreateWindowExW(0, L"STATIC", app::AppDisplayName().c_str(), WS_CHILD | SS_CENTER, 0, 0, 0, 0,
                                       hwnd_, nullptr, instance_, nullptr);
    emptyHintLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    emptyRecentLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_CENTER, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    osdLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_CENTER | SS_CENTERIMAGE, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    endPromptTitleLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_CENTER, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    endPromptHintLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_CENTER, 0, 0, 0, 0, hwnd_, nullptr, instance_, nullptr);
    replayButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                    reinterpret_cast<HMENU>(kControlReplay), instance_, nullptr);
    nextButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                  reinterpret_cast<HMENU>(kControlEndNext), instance_, nullptr);
    fullscreenDownloadButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                reinterpret_cast<HMENU>(kControlFullscreenDownload), instance_, nullptr);
    fullscreenMinimizeButton_ = CreateWindowExW(0, L"BUTTON", L"_", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                reinterpret_cast<HMENU>(kControlFullscreenMinimize), instance_, nullptr);
    fullscreenWindowedButton_ = CreateWindowExW(0, L"BUTTON", L"[]", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                                reinterpret_cast<HMENU>(kControlFullscreenWindowed), instance_, nullptr);
    fullscreenCloseButton_ = CreateWindowExW(0, L"BUTTON", L"X", WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd_,
                                             reinterpret_cast<HMENU>(kControlFullscreenClose), instance_, nullptr);

    for (HWND control : {titleBarBackground_, panelBackground_, titleLabel_, openButton_, recentButton_, quickBrowseButton_, previousButton_, playPauseButton_, nextTrackButton_, muteButton_, speedButton_,
                         subtitleButton_, settingsButton_, moreButton_, fullscreenButton_, volumeLabel_, speedLabel_, currentTimeLabel_, durationLabel_,
                         volumeEdit_, speedEdit_, statusLabel_, mediaInfoLabel_, emptyTitleLabel_, emptyHintLabel_, emptyRecentLabel_, osdLabel_,
                         endPromptTitleLabel_, endPromptHintLabel_, replayButton_, nextButton_, fullscreenDownloadButton_, fullscreenMinimizeButton_, fullscreenWindowedButton_,
                         fullscreenCloseButton_}) {
        SetControlFont(control, uiFont_);
    }

    SetControlFont(titleLabel_, captionFont_);
    SetControlFont(statusLabel_, captionFont_);
    SetControlFont(emptyHintLabel_, osdFont_);
    SetControlFont(emptyRecentLabel_, captionFont_);
    SetControlFont(endPromptHintLabel_, captionFont_);
    SetControlFont(emptyTitleLabel_, titleFont_);
    SetControlFont(osdLabel_, osdFont_);
    SetControlFont(endPromptTitleLabel_, titleFont_);

    ShowWindow(osdLabel_, SW_HIDE);
    ShowWindow(endPromptTitleLabel_, SW_HIDE);
    ShowWindow(endPromptHintLabel_, SW_HIDE);
    ShowWindow(replayButton_, SW_HIDE);
    ShowWindow(nextButton_, SW_HIDE);
    ShowWindow(fullscreenDownloadButton_, SW_HIDE);
    ShowWindow(fullscreenMinimizeButton_, SW_HIDE);
    ShowWindow(fullscreenWindowedButton_, SW_HIDE);
    ShowWindow(fullscreenCloseButton_, SW_HIDE);
    ShowWindow(mediaInfoLabel_, SW_HIDE);
    ShowWindow(moreButton_, SW_HIDE);
    ShowWindow(muteButton_, SW_HIDE);
    ShowWindow(speedLabel_, SW_HIDE);
    ShowWindow(statusLabel_, SW_HIDE);
    quickBrowsePanel_.SetVisible(false);
    seekPreviewPopup_.Hide();
    ShowWindow(emptyHintLabel_, SW_HIDE);
    RefreshLocalizedText();
}

void MainWindow::RefreshLocalizedText() {
    quickBrowsePanel_.SetLanguageCode(config_.languageCode);
    SetControlTextIfChanged(openButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Open));
    SetControlTextIfChanged(quickBrowseButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::QuickBrowse));
    SetControlTextIfChanged(previousButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::PreviousItem));
    SetControlTextIfChanged(playPauseButton_, state_.isPaused ? velo::localization::Text(config_.languageCode, velo::localization::TextId::Play)
                                                             : velo::localization::Text(config_.languageCode, velo::localization::TextId::Pause));
    SetControlTextIfChanged(nextTrackButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::NextItem));
    SetControlTextIfChanged(muteButton_, state_.isMuted ? velo::localization::Text(config_.languageCode, velo::localization::TextId::Unmute)
                                                        : velo::localization::Text(config_.languageCode, velo::localization::TextId::Mute));
    SetControlTextIfChanged(speedButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Speed));
    SetControlTextIfChanged(subtitleButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Subtitle));
    SetControlTextIfChanged(settingsButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Settings));
    SetControlTextIfChanged(moreButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::More));
    SetControlTextIfChanged(fullscreenButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Fullscreen));
    SetControlTextIfChanged(volumeLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Volume));
    SetControlTextIfChanged(speedLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::Speed));
    SetControlTextIfChanged(statusLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainStatusReady));
    SetControlTextIfChanged(emptyHintLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEmptyHint));
    SetControlTextIfChanged(endPromptTitleLabel_, velo::localization::Text(config_.languageCode, velo::localization::TextId::MainEndPromptTitle));
    SetControlTextIfChanged(replayButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::ReplayCurrentFile));
    SetControlTextIfChanged(nextButton_, velo::localization::Text(config_.languageCode, velo::localization::TextId::NextItem));
    SetControlTextIfChanged(fullscreenDownloadButton_, L"\u2B07");
    SetControlTextIfChanged(fullscreenMinimizeButton_, L"-");
    SetControlTextIfChanged(fullscreenWindowedButton_, L"\u25A1");
    SetControlTextIfChanged(fullscreenCloseButton_, L"x");
    SetControlTextIfChanged(titleLabel_, currentHeaderTitle_.empty() ? velo::app::AppDisplayName() : currentHeaderTitle_);
}

void MainWindow::ApplyStateToControls() {
    const std::wstring titleText = BuildMediaTitleText(state_);
    if (currentHeaderTitle_ != titleText) {
        currentHeaderTitle_ = titleText;
        SetWindowTextW(hwnd_, currentHeaderTitle_.c_str());
        SetControlTextIfChanged(titleLabel_, currentHeaderTitle_);
        const RECT titleBounds = HeaderTitleRect();
        InvalidateRect(hwnd_, &titleBounds, FALSE);
    }

    if (!seekSlider_.IsDragging()) {
        suppressSeekEvents_ = true;
        const int seekPos = state_.durationSeconds > 0.0
                                ? static_cast<int>((state_.positionSeconds / state_.durationSeconds) * 1000.0)
                                : 0;
        seekSlider_.SetValue(std::clamp(seekPos, 0, 1000));
        suppressSeekEvents_ = false;
    }

    suppressVolumeEvents_ = true;
    const int volumeValue = std::clamp(static_cast<int>(state_.volume), 0, 100);
    volumeSlider_.SetValue(volumeValue);
    suppressVolumeEvents_ = false;

    suppressSpeedEvents_ = true;
    const int speedValue = std::clamp(static_cast<int>(std::lround(state_.playbackSpeed * 100.0)), kSpeedMinimum, kSpeedMaximum);
    speedSlider_.SetValue(speedValue);
    suppressSpeedEvents_ = false;

    if (GetFocus() != speedEdit_) {
        suppressSpeedTextEvents_ = true;
        SetControlTextIfChanged(speedEdit_, FormatSpeedInput(state_.playbackSpeed));
        suppressSpeedTextEvents_ = false;
    }

    SetControlTextIfChanged(currentTimeLabel_, FormatTimeLabel(state_.positionSeconds));
    SetControlTextIfChanged(durationLabel_, FormatTimeLabel(state_.durationSeconds));
    if (!state_.errorState.empty()) {
        const std::wstring errorText = FriendlyPlaybackError(state_.errorState, config_.languageCode);
        if (errorText != lastErrorText_) {
            lastErrorText_ = errorText;
            ShowOsdNow(errorText, 1400);
        }
    } else {
        lastErrorText_.clear();
    }

    if (endPromptVisible_ && !ShouldKeepEndPromptVisible()) {
        HideEndPrompt();
    }

    UpdateEmptyState();
    UpdateMediaInfoPanel();
    if (state_.isLoaded && activeMediaPath_ != state_.currentPath) {
        activeMediaPath_ = state_.currentPath;
        if (quickBrowseVisible_) {
            RefreshQuickBrowse();
        }
        SetControlsVisible(!fullscreen_, false);
    }
    if (!state_.isLoaded) {
        activeMediaPath_.clear();
        HideQuickBrowse();
        HideSeekPreview();
        startupControlsPinned_ = false;
        mouseInsideWindow_ = false;
        quickBrowseRootFolder_.clear();
    } else if (fullscreen_ && !mouseInsideWindow_ && !IsControlInteractionActive()) {
        SetControlsVisible(false, false);
    } else if (!fullscreen_) {
        SetControlsVisible(true, false);
    }
}

void MainWindow::OpenFilePicker() {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;

    std::wstring filter = velo::localization::Text(config_.languageCode, velo::localization::TextId::MediaFileFilter);
    filter.push_back(L'\0');
    filter += velo::app::BuildMediaOpenDialogPattern();
    filter.push_back(L'\0');
    filter += velo::localization::Text(config_.languageCode, velo::localization::TextId::AllFilesFilter);
    filter.push_back(L'\0');
    filter += L"*.*";
    filter.push_back(L'\0');
    filter.push_back(L'\0');
    dialog.lpstrFilter = filter.c_str();
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&dialog)) {
        callbacks_.openFile(filePath);
    }
}

void MainWindow::OpenFolderPicker() {
    HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog)))) {
        if (SUCCEEDED(initResult)) {
            CoUninitialize();
        }
        return;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);
    if (SUCCEEDED(dialog->Show(hwnd_))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR folderPath = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &folderPath)) && folderPath != nullptr) {
                callbacks_.openFolder(folderPath);
                CoTaskMemFree(folderPath);
            }
            item->Release();
        }
    }

    dialog->Release();
    if (SUCCEEDED(initResult)) {
        CoUninitialize();
    }
}

void MainWindow::OpenSubtitlePicker() {
    wchar_t filePath[MAX_PATH] = {};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;

    std::wstring filter = velo::localization::Text(config_.languageCode, velo::localization::TextId::LoadSubtitle);
    filter.push_back(L'\0');
    filter += velo::app::BuildSubtitleOpenDialogPattern();
    filter.push_back(L'\0');
    filter += velo::localization::Text(config_.languageCode, velo::localization::TextId::AllFilesFilter);
    filter.push_back(L'\0');
    filter += L"*.*";
    filter.push_back(L'\0');
    filter.push_back(L'\0');
    dialog.lpstrFilter = filter.c_str();
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&dialog)) {
        callbacks_.loadSubtitle(filePath);
    }
}

void MainWindow::HandleDroppedFile(HDROP dropHandle) {
    const UINT count = DragQueryFileW(dropHandle, 0xFFFFFFFF, nullptr, 0);
    std::vector<std::wstring> videoFiles;
    std::wstring subtitleFile;
    for (UINT index = 0; index < count; ++index) {
        wchar_t path[MAX_PATH] = {};
        if (DragQueryFileW(dropHandle, index, path, MAX_PATH) == 0) {
            continue;
        }

        const std::filesystem::path droppedPath(path);
        if (std::filesystem::is_directory(droppedPath)) {
            callbacks_.openFolder(path);
            DragFinish(dropHandle);
            return;
        }
        if (velo::app::IsSubtitleFile(droppedPath)) {
            if (subtitleFile.empty()) {
                subtitleFile = path;
            }
        } else {
            videoFiles.push_back(path);
        }
    }

    if (videoFiles.size() > 1 && callbacks_.openFiles != nullptr) {
        callbacks_.openFiles(videoFiles);
    } else if (!videoFiles.empty()) {
        callbacks_.openFile(videoFiles.front());
    }
    if (!subtitleFile.empty()) {
        callbacks_.loadSubtitle(subtitleFile);
        ShowOsdNow(velo::localization::Text(config_.languageCode, velo::localization::TextId::SubtitleLoadedPrefix), 1000);
    }
    DragFinish(dropHandle);
}

void MainWindow::CommitVolumeText() {
    wchar_t text[16] = {};
    GetWindowTextW(volumeEdit_, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    const auto parsed = ParseVolumeInput(text);
    if (!parsed.has_value()) {
        suppressVolumeTextEvents_ = true;
        SetWindowTextW(volumeEdit_, FormatVolumeInput(volumeSlider_.Value()).c_str());
        suppressVolumeTextEvents_ = false;
        return;
    }

    const int value = *parsed;
    suppressVolumeTextEvents_ = true;
    SetWindowTextW(volumeEdit_, FormatVolumeInput(value).c_str());
    suppressVolumeTextEvents_ = false;
    suppressVolumeEvents_ = true;
    volumeSlider_.SetValue(value);
    suppressVolumeEvents_ = false;
    DispatchVolumeChange(value);
}

void MainWindow::SyncVolumeEditPreview() {
    wchar_t text[16] = {};
    GetWindowTextW(volumeEdit_, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    const auto parsed = ParseVolumeInput(text);
    if (!parsed.has_value()) {
        return;
    }

    suppressVolumeEvents_ = true;
    volumeSlider_.SetValue(*parsed);
    suppressVolumeEvents_ = false;
    DispatchVolumeChange(*parsed);
}

void MainWindow::DispatchVolumeChange(const int value) {
    const int clampedValue = std::clamp(value, 0, 100);
    config_.volume = static_cast<double>(clampedValue);
    config_.startupVolume = config_.volume;
    if (clampedValue > 0 && state_.isMuted) {
        state_.isMuted = false;
        RefreshMuteButton(true);
        callbacks_.setMute(false);
    }
    callbacks_.setVolume(static_cast<double>(clampedValue));
}

void MainWindow::CommitSpeedText() {
    wchar_t text[16] = {};
    GetWindowTextW(speedEdit_, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    const auto parsed = ParseSpeedInput(text);
    if (!parsed.has_value()) {
        suppressSpeedTextEvents_ = true;
        SetWindowTextW(speedEdit_, FormatSpeedInput(static_cast<double>(speedSlider_.Value()) / 100.0).c_str());
        suppressSpeedTextEvents_ = false;
        return;
    }

    const double value = *parsed;
    suppressSpeedTextEvents_ = true;
    SetWindowTextW(speedEdit_, FormatSpeedInput(value).c_str());
    suppressSpeedTextEvents_ = false;
    suppressSpeedEvents_ = true;
    speedSlider_.SetValue(static_cast<int>(std::lround(value * 100.0)));
    suppressSpeedEvents_ = false;
    callbacks_.setPlaybackSpeed(value);
}

void MainWindow::SyncSpeedEditPreview() {
    wchar_t text[16] = {};
    GetWindowTextW(speedEdit_, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    const auto parsed = ParseSpeedInput(text);
    if (!parsed.has_value()) {
        return;
    }

    suppressSpeedEvents_ = true;
    speedSlider_.SetValue(static_cast<int>(std::lround(*parsed * 100.0)));
    suppressSpeedEvents_ = false;
    callbacks_.setPlaybackSpeed(*parsed);
}

void MainWindow::RefreshPlayPauseButton(const bool immediate) const {
    if (playPauseButton_ == nullptr) {
        return;
    }
    RedrawWindow(playPauseButton_, nullptr, nullptr, RDW_INVALIDATE | (immediate ? RDW_UPDATENOW : 0));
}

void MainWindow::RefreshMuteButton(const bool immediate) const {
    if (volumeLabel_ == nullptr) {
        return;
    }
    RedrawWindow(volumeLabel_, nullptr, nullptr, RDW_INVALIDATE | (immediate ? RDW_UPDATENOW : 0));
}

void MainWindow::RefreshSubtitleButton(const bool immediate) const {
    if (subtitleButton_ == nullptr) {
        return;
    }
    RedrawWindow(subtitleButton_, nullptr, nullptr, RDW_INVALIDATE | (immediate ? RDW_UPDATENOW : 0));
}

}  // namespace velo::ui
