#include "ui/main_window_internal.h"

namespace velo::ui {

void MainWindow::LayoutChildren() {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const bool effectiveControlsVisible = !uiSuppressed_ && (quickBrowseVisible_ || controlsVisible_);
    const bool captionBarVisible = !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_);
    const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_, effectiveControlsVisible || captionBarVisible);
    const int captionRegionHeight = std::max(0, static_cast<int>(layout.titleBarRect.bottom - layout.titleBarRect.top));
    const int captionButtonExtent = ComputeCaptionButtonExtent(layout.width, std::max(layout.captionBarHeight, captionRegionHeight));
    const bool showTitleOverlay = captionBarVisible && captionRegionHeight > 0;

    std::vector<HWND> primaryButtons{openButton_, quickBrowseButton_, previousButton_, playPauseButton_, nextTrackButton_, fullscreenButton_};
    std::vector<HWND> optionalButtons{subtitleButton_, settingsButton_};
    std::vector<HWND> visibleButtons = primaryButtons;
    int buttonWidth = layout.buttonBaseWidth;
    bool showMoreButton = false;
    const auto widthForCount = [&](const int count, const int widthPerButton) {
        if (count <= 0) {
            return 0;
        }
        return count * widthPerButton + (count - 1) * layout.gap;
    };

    const int allButtonCount = static_cast<int>(primaryButtons.size() + optionalButtons.size());
    if (widthForCount(allButtonCount, layout.buttonBaseWidth) <= layout.buttonAreaWidth) {
        visibleButtons.insert(visibleButtons.end(), optionalButtons.begin(), optionalButtons.end());
    } else {
        showMoreButton = true;
        const int requiredCount = static_cast<int>(primaryButtons.size()) + 1;
        if (widthForCount(requiredCount, layout.buttonBaseWidth) > layout.buttonAreaWidth) {
            buttonWidth = std::max(layout.minButtonWidth,
                                   (layout.buttonAreaWidth - (requiredCount - 1) * layout.gap) / std::max(1, requiredCount));
        }
        if (widthForCount(requiredCount, buttonWidth) > layout.buttonAreaWidth) {
            showMoreButton = false;
            buttonWidth = std::max(40, (layout.buttonAreaWidth - (static_cast<int>(primaryButtons.size()) - 1) * layout.gap) /
                                         std::max(1, static_cast<int>(primaryButtons.size())));
        }
        const int requiredCountWithOverflow = static_cast<int>(primaryButtons.size()) + (showMoreButton ? 1 : 0);
        const int maxVisibleCount = std::max(requiredCountWithOverflow,
                                             (layout.buttonAreaWidth + layout.gap) / std::max(1, buttonWidth + layout.gap));
        const int optionalSlots = std::max(0, maxVisibleCount - requiredCountWithOverflow);
        visibleButtons.insert(visibleButtons.end(), optionalButtons.begin(), optionalButtons.begin() + std::min<int>(optionalSlots, optionalButtons.size()));
        if (showMoreButton) {
            visibleButtons.push_back(moreButton_);
        }
    }

    useOverflowControls_ = showMoreButton;
    videoHost_.Resize(layout.videoRect);
    const int quickBrowseLeft = fullscreen_ ? 0 : kWindowResizeBorder;
    quickBrowsePanel_.SetBounds({quickBrowseLeft, 0, quickBrowseLeft + layout.quickBrowseWidth, layout.height});
    quickBrowsePanel_.SetVisible(quickBrowseVisible_ && !uiSuppressed_);
    MoveWindow(titleBarBackground_, layout.titleBarRect.left, layout.titleBarRect.top,
               std::max(0, static_cast<int>(layout.titleBarRect.right - layout.titleBarRect.left)),
               std::max(0, static_cast<int>(layout.titleBarRect.bottom - layout.titleBarRect.top)), TRUE);
    MoveWindow(panelBackground_, layout.panelRect.left, layout.panelRect.top,
               std::max(0, static_cast<int>(layout.panelRect.right - layout.panelRect.left)),
               std::max(0, static_cast<int>(layout.panelRect.bottom - layout.panelRect.top)), TRUE);
    MoveWindow(titleLabel_, layout.titleTextRect.left, layout.titleTextRect.top,
               std::max(0, static_cast<int>(layout.titleTextRect.right - layout.titleTextRect.left)),
               std::max(0, static_cast<int>(layout.titleTextRect.bottom - layout.titleTextRect.top)), TRUE);
    SetControlVisibility(titleBarBackground_, showTitleOverlay);
    SetControlVisibility(panelBackground_, effectiveControlsVisible && fullscreen_);
    SetControlVisibility(titleLabel_, showTitleOverlay);
    showRecentButton_ = std::find(visibleButtons.begin(), visibleButtons.end(), recentButton_) != visibleButtons.end();
    showSubtitleButton_ = std::find(visibleButtons.begin(), visibleButtons.end(), subtitleButton_) != visibleButtons.end();
    showSettingsButton_ = std::find(visibleButtons.begin(), visibleButtons.end(), settingsButton_) != visibleButtons.end();
    showMoreButton_ = std::find(visibleButtons.begin(), visibleButtons.end(), moreButton_) != visibleButtons.end();

    MoveWindow(mediaInfoLabel_, layout.contentLeft + 32, 70, std::min(std::max(0, layout.width - layout.contentLeft - 64), kInfoPanelWidth), 126, TRUE);
    MoveWindow(seekSlider_.WindowHandle(), layout.seekLeft, layout.seekTop, layout.seekWidth, layout.seekHeight, TRUE);
    MoveWindow(currentTimeLabel_, layout.currentTimeLeft, layout.timeRowTop, layout.currentTimeWidth, layout.timeLabelHeight, TRUE);
    MoveWindow(durationLabel_, layout.durationLeft, layout.timeRowTop, layout.durationWidth, layout.timeLabelHeight, TRUE);

    const auto isVisibleButton = [&](HWND control) {
        return std::find(visibleButtons.begin(), visibleButtons.end(), control) != visibleButtons.end();
    };
    const auto widthForControl = [&](HWND control) {
        if (control == openButton_ || control == settingsButton_) {
            return std::max(24, static_cast<int>(std::lround(buttonWidth * 0.9)));
        }
        return buttonWidth;
    };
    const int playPauseWidth = widthForControl(playPauseButton_);
    const int playPauseX = layout.buttonAreaCenterX - playPauseWidth / 2;
    const int playPauseRight = playPauseX + playPauseWidth;
    std::vector<HWND> leftButtons;
    std::vector<HWND> rightButtons;
    bool onRightSide = false;
    for (HWND control : {openButton_, recentButton_, quickBrowseButton_, previousButton_, playPauseButton_, nextTrackButton_, subtitleButton_, settingsButton_, fullscreenButton_, moreButton_}) {
        const bool visible = isVisibleButton(control);
        SetControlVisibility(control, visible && effectiveControlsVisible);
        if (!visible) {
            continue;
        }
        if (control == playPauseButton_) {
            MoveWindow(control, playPauseX, layout.buttonsY, playPauseWidth, layout.controlHeight, TRUE);
            onRightSide = true;
            continue;
        }
        if (onRightSide) {
            rightButtons.push_back(control);
        } else {
            leftButtons.push_back(control);
        }
    }

    int rightX = playPauseRight + layout.gap;
    for (HWND control : rightButtons) {
        const int controlWidth = widthForControl(control);
        MoveWindow(control, rightX, layout.buttonsY, controlWidth, layout.controlHeight, TRUE);
        rightX += controlWidth + layout.gap;
    }

    int leftX = playPauseX - layout.gap;
    for (auto it = leftButtons.rbegin(); it != leftButtons.rend(); ++it) {
        const int controlWidth = widthForControl(*it);
        leftX -= controlWidth;
        MoveWindow(*it, leftX, layout.buttonsY, controlWidth, layout.controlHeight, TRUE);
        leftX -= layout.gap;
    }

    MoveWindow(speedButton_, layout.speedGroupLeft, layout.buttonsY, layout.labelWidth, layout.controlHeight, TRUE);
    MoveWindow(speedSlider_.WindowHandle(), layout.speedGroupLeft + layout.labelWidth + layout.gap, layout.speedRowTop, layout.speedSliderWidth, layout.sliderHeight, TRUE);
    MoveWindow(speedEdit_, layout.speedGroupLeft + layout.labelWidth + layout.gap + layout.speedSliderWidth + layout.gap, layout.buttonsY, layout.editWidth, layout.controlHeight, TRUE);
    MoveWindow(speedLabel_, 0, 0, 0, 0, TRUE);
    MoveWindow(volumeLabel_, layout.volumeGroupLeft, layout.buttonsY, layout.labelWidth, layout.controlHeight, TRUE);
    MoveWindow(volumeSlider_.WindowHandle(), layout.volumeGroupLeft + layout.labelWidth + layout.gap, layout.volumeRowTop, layout.volumeSliderWidth, layout.sliderHeight, TRUE);
    MoveWindow(volumeEdit_, 0, 0, 0, 0, TRUE);
    MoveWindow(emptyTitleLabel_, 0, 0, 0, 0, TRUE);
    MoveWindow(emptyHintLabel_, 0, 0, 0, 0, TRUE);
    MoveWindow(emptyRecentLabel_, 0, 0, 0, 0, TRUE);

    const int promptWidth = std::min(layout.contentWidth - 60, fullscreen_ ? 360 : 460);
    const int promptCenterX = fullscreen_ ? layout.currentTimeCenterX : layout.contentLeft + layout.contentWidth / 2;
    const int promptTop = fullscreen_ ? std::max(52, layout.panelTop - 92) : std::max(72, layout.height / 2 - 98);
    MoveWindow(endPromptTitleLabel_, promptCenterX - promptWidth / 2, promptTop, promptWidth, 42, TRUE);
    MoveWindow(endPromptHintLabel_, promptCenterX - promptWidth / 2, promptTop + 40, promptWidth, 26, TRUE);
    MoveWindow(replayButton_, promptCenterX - 132, promptTop + 56, 120, kControlHeight, TRUE);
    MoveWindow(nextButton_, promptCenterX + 12, promptTop + 56, 120, kControlHeight, TRUE);

    SetControlVisibility(mediaInfoLabel_, mediaInfoVisible_ && !uiSuppressed_);
    SetControlVisibility(endPromptTitleLabel_, endPromptVisible_ && !uiSuppressed_);
    SetControlVisibility(endPromptHintLabel_, endPromptVisible_ && !uiSuppressed_);
    SetControlVisibility(replayButton_, endPromptVisible_ && !uiSuppressed_);
    SetControlVisibility(nextButton_, endPromptVisible_ && !uiSuppressed_);

    const int captionButtonCount = updateAvailable_ ? 4 : 3;
    const int captionButtonsWidth = captionButtonExtent * captionButtonCount + kFullscreenCaptionButtonGap * (captionButtonCount - 1);
    const int captionButtonsX = std::max(static_cast<int>(layout.titleBarRect.left) + 6,
                                         static_cast<int>(layout.titleBarRect.right) - captionButtonsWidth - 6);
    const int captionButtonsY = std::max(static_cast<int>(layout.titleBarRect.top),
                                         static_cast<int>(layout.titleBarRect.top) +
                                             (std::max(0, captionRegionHeight - captionButtonExtent) / 2));
    int captionButtonX = captionButtonsX;
    if (updateAvailable_) {
        MoveWindow(fullscreenDownloadButton_, captionButtonX, captionButtonsY, captionButtonExtent, captionButtonExtent, TRUE);
        captionButtonX += captionButtonExtent + kFullscreenCaptionButtonGap;
    }
    MoveWindow(fullscreenMinimizeButton_, captionButtonX, captionButtonsY, captionButtonExtent, captionButtonExtent, TRUE);
    MoveWindow(fullscreenWindowedButton_, captionButtonX + captionButtonExtent + kFullscreenCaptionButtonGap, captionButtonsY,
               captionButtonExtent, captionButtonExtent, TRUE);
    MoveWindow(fullscreenCloseButton_, captionButtonX + (captionButtonExtent + kFullscreenCaptionButtonGap) * 2, captionButtonsY,
               captionButtonExtent, captionButtonExtent, TRUE);
    const bool showCaptionButtons = showTitleOverlay;
    SetControlVisibility(fullscreenDownloadButton_, showCaptionButtons && updateAvailable_);
    SetControlVisibility(fullscreenMinimizeButton_, showCaptionButtons);
    SetControlVisibility(fullscreenWindowedButton_, showCaptionButtons);
    SetControlVisibility(fullscreenCloseButton_, showCaptionButtons);

    SetWindowPos(videoHost_.WindowHandle(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    for (HWND overlay : {panelBackground_, titleBarBackground_, titleLabel_, quickBrowsePanel_.WindowHandle(), mediaInfoLabel_, seekSlider_.WindowHandle(), currentTimeLabel_, durationLabel_, openButton_, quickBrowseButton_, previousButton_, playPauseButton_, nextTrackButton_, subtitleButton_, settingsButton_, fullscreenButton_, moreButton_, volumeLabel_, volumeSlider_.WindowHandle(), speedLabel_, speedSlider_.WindowHandle(), speedButton_, speedEdit_, seekPreviewPopup_.WindowHandle(), emptyTitleLabel_, emptyHintLabel_, emptyRecentLabel_, osdLabel_, endPromptTitleLabel_, endPromptHintLabel_, replayButton_, nextButton_}) {
        if (overlay != nullptr) {
            SetWindowPos(overlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }
    for (HWND overlay : {fullscreenDownloadButton_, fullscreenMinimizeButton_, fullscreenWindowedButton_, fullscreenCloseButton_}) {
        if (overlay != nullptr) {
            SetWindowPos(overlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }

    UpdateOsdLayout();
}

void MainWindow::HandlePointerActivity(const POINT clientPoint) {
    mouseInsideWindow_ = true;
    TRACKMOUSEEVENT trackMouse{sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd_, 0};
    TrackMouseEvent(&trackMouse);
    if (uiSuppressed_ || !state_.isLoaded) {
        return;
    }
    if (!fullscreen_) {
        SetControlsVisible(true, false);
        return;
    }
    if (quickBrowseVisible_) {
        SetControlsVisible(true, false);
        return;
    }
    if (IsControlInteractionActive()) {
        SetControlsVisible(true, false);
        return;
    }
    if (!controlsVisible_) {
        if (IsPointInControlsActivationZone(clientPoint)) {
            SetControlsVisible(true, false);
        }
        return;
    }
    if (!IsPointInControlsRetentionZone(clientPoint) && !endPromptVisible_) {
        SetControlsVisible(false, false);
    }
}

void MainWindow::HandlePointerLeave() {
    POINT cursor{};
    GetCursorPos(&cursor);
    if (IsPointInsideWindowRect(hwnd_, cursor)) {
        return;
    }
    mouseInsideWindow_ = false;
    HideSeekPreview();
    if (fullscreen_ && !startupControlsPinned_ && state_.isLoaded) {
        SetControlsVisible(false, false);
    } else if (!fullscreen_) {
        SetControlsVisible(true, false);
    }
}

void MainWindow::SetFullscreenCaptionButtonsVisible(const bool visible) {
    const bool nextVisible = fullscreen_ && visible && !uiSuppressed_;
    if (fullscreenCaptionButtonsVisible_ == nextVisible) {
        return;
    }
    fullscreenCaptionButtonsVisible_ = nextVisible;
    LayoutChildren();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::SetControlsVisible(const bool visible, const bool resetTimer) {
    const bool previousVisible = controlsVisible_;
    const bool visibilityChanged = previousVisible != (fullscreen_ ? visible : true);
    controlsVisible_ = fullscreen_ ? visible : true;
    const bool effectiveControlsVisible = !uiSuppressed_ && (quickBrowseVisible_ || controlsVisible_);
    for (HWND control : {openButton_, quickBrowseButton_, previousButton_, playPauseButton_, nextTrackButton_, fullscreenButton_, volumeLabel_, currentTimeLabel_, durationLabel_, speedButton_, speedEdit_}) {
        SetControlVisibility(control, effectiveControlsVisible);
    }
    SetControlVisibility(speedLabel_, false);
    SetControlVisibility(recentButton_, effectiveControlsVisible && showRecentButton_);
    SetControlVisibility(subtitleButton_, effectiveControlsVisible && showSubtitleButton_);
    SetControlVisibility(settingsButton_, effectiveControlsVisible && showSettingsButton_);
    SetControlVisibility(moreButton_, effectiveControlsVisible && showMoreButton_);
    SetControlVisibility(muteButton_, false);
    SetControlVisibility(speedLabel_, false);
    SetControlVisibility(seekSlider_.WindowHandle(), effectiveControlsVisible);
    SetControlVisibility(volumeSlider_.WindowHandle(), effectiveControlsVisible);
    SetControlVisibility(volumeEdit_, false);
    SetControlVisibility(speedSlider_.WindowHandle(), effectiveControlsVisible);

    if (!fullscreen_) {
        KillTimer(hwnd_, kTimerHideControls);
    } else if (!effectiveControlsVisible || !state_.isLoaded || quickBrowseVisible_ || uiSuppressed_) {
        KillTimer(hwnd_, kTimerHideControls);
    } else if (resetTimer && !mouseInsideWindow_) {
        SetTimer(hwnd_, kTimerHideControls, fullscreen_ ? 2000 : std::max(900, autoHideDelayMs_), nullptr);
    } else if (resetTimer && !IsCursorInControlsActivationZone()) {
        SetTimer(hwnd_, kTimerHideControls, fullscreen_ ? 2000 : std::max(900, autoHideDelayMs_), nullptr);
    } else if (resetTimer) {
        KillTimer(hwnd_, kTimerHideControls);
    }

    if (previousVisible != controlsVisible_) {
        LayoutChildren();
    }
    if (visibilityChanged) {
        if (videoHost_.WindowHandle() != nullptr) {
            RedrawWindow(videoHost_.WindowHandle(), nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
        RedrawWindow(hwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        for (HWND control : {titleBarBackground_, panelBackground_, titleLabel_, openButton_, recentButton_, quickBrowseButton_, previousButton_, playPauseButton_, nextTrackButton_, subtitleButton_, settingsButton_, moreButton_, fullscreenButton_, volumeLabel_, seekSlider_.WindowHandle(), currentTimeLabel_, durationLabel_, volumeSlider_.WindowHandle(), speedButton_, speedSlider_.WindowHandle(), speedEdit_, fullscreenMinimizeButton_, fullscreenWindowedButton_, fullscreenCloseButton_}) {
            if (control != nullptr && IsWindowVisible(control)) {
                RedrawWindow(control, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
        }
    }
    if (!effectiveControlsVisible) {
        HideSeekPreview();
    }
}

void MainWindow::RedrawUiSurface(const bool immediate) const {
    if (hwnd_ == nullptr) {
        return;
    }
    UINT flags = RDW_INVALIDATE | RDW_ALLCHILDREN;
    if (immediate) {
        flags |= RDW_UPDATENOW;
    }
    RedrawWindow(hwnd_, nullptr, nullptr, flags);
}

void MainWindow::UpdateEmptyState() {
    const bool empty = !state_.isLoaded;
    ShowWindow(emptyTitleLabel_, SW_HIDE);
    ShowWindow(emptyHintLabel_, SW_HIDE);
    ShowWindow(emptyRecentLabel_, SW_HIDE);
    if (empty) {
        HideQuickBrowse();
        HideSeekPreview();
        HideEndPrompt();
        HideOsdNow();
        SetControlsVisible(!fullscreen_, false);
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::UpdateOsdLayout() {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_, !uiSuppressed_ && (quickBrowseVisible_ || controlsVisible_));
    wchar_t text[256] = {};
    GetWindowTextW(osdLabel_, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    HDC dc = GetDC(hwnd_);
    RECT textRect{0, 0, 0, 0};
    if (dc != nullptr) {
        HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(dc, osdFont_));
        DrawTextW(dc, text, -1, &textRect, DT_CALCRECT | DT_SINGLELINE);
        SelectObject(dc, previousFont);
        ReleaseDC(hwnd_, dc);
    }

    const int textWidth = static_cast<int>(textRect.right - textRect.left);
    const int textHeight = static_cast<int>(textRect.bottom - textRect.top);
    const int width = std::clamp(textWidth + 32, 72, fullscreen_ ? 320 : static_cast<int>(client.right - client.left) - 40);
    const int height = std::max(36, textHeight + 16);
    const int y = (!uiSuppressed_ && (quickBrowseVisible_ || controlsVisible_)) ? std::max(40, layout.panelTop - height - (fullscreen_ ? 10 : 22))
                                                                                 : std::max(40, static_cast<int>(client.bottom) - 104);
    const int x = fullscreen_ ? std::clamp(layout.currentTimeCenterX - width / 2, layout.contentLeft + 20, layout.width - width - 20)
                              : (client.right - width) / 2;
    MoveWindow(osdLabel_, x, y, width, height, TRUE);
}

void MainWindow::UpdateSeekPreview() {
    if (!config_.showSeekPreview || !state_.isLoaded || state_.durationSeconds <= 0.0 || !(quickBrowseVisible_ || controlsVisible_) || uiSuppressed_) {
        HideSeekPreview();
        return;
    }
    RECT seekRect{};
    GetWindowRect(seekSlider_.WindowHandle(), &seekRect);
    MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<POINT*>(&seekRect), 2);
    const int hoverPosition = seekSlider_.HoverValue();
    const double sliderRatio = static_cast<double>(hoverPosition) / 1000.0;
    const double target = state_.durationSeconds * sliderRatio;
    seekPreviewPopup_.SetEnabled(config_.showSeekPreview);
    seekPreviewPopup_.SetMedia(Utf8ToWide(state_.currentPath), config_.hwdecPolicy);
    seekPreviewPopup_.ShowAt(seekRect, sliderRatio, target, state_.durationSeconds);
}

void MainWindow::HideSeekPreview() {
    seekPreviewPopup_.Hide();
}

bool MainWindow::IsControlInteractionActive() const {
    const HWND capture = GetCapture();
    if (capture == seekSlider_.WindowHandle() || capture == volumeSlider_.WindowHandle() || capture == speedSlider_.WindowHandle()) {
        return true;
    }
    const HWND focus = GetFocus();
    return focus == volumeEdit_ || focus == speedEdit_;
}

RECT MainWindow::VideoClientRect() const {
    RECT videoRect{};
    GetWindowRect(videoHost_.WindowHandle(), &videoRect);
    POINT topLeft{videoRect.left, videoRect.top};
    POINT bottomRight{videoRect.right, videoRect.bottom};
    ScreenToClient(hwnd_, &topLeft);
    ScreenToClient(hwnd_, &bottomRight);
    return RECT{topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
}

RECT MainWindow::HeaderTitleRect() const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_,
                                                            !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_));
    return layout.titleTextRect;
}

RECT MainWindow::EmptyDropZoneRect() const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const int contentLeft = quickBrowseVisible_ ? std::clamp(static_cast<int>(client.right - client.left) / 5, 220, 360) : 0;
    const int width = std::min(kEmptyDropZoneWidth, std::max(280, static_cast<int>(client.right - contentLeft) - 80));
    const int left = contentLeft + std::max(40, (static_cast<int>(client.right - contentLeft) - width) / 2);
    const int top = std::max(96, static_cast<int>(client.bottom) / 2 - kEmptyDropZoneHeight / 2);
    return RECT{left, top, left + width, top + kEmptyDropZoneHeight};
}

bool MainWindow::IsPointInControlsActivationZone(const POINT clientPoint) const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_,
                                                            !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_));
    if (fullscreen_) {
        const int topActivationBottom = ComputeFullscreenCaptionOverlayHeight(layout.height);
        if (clientPoint.y >= 0 && clientPoint.y < topActivationBottom) {
            return true;
        }
    }
    const int activationTop = fullscreen_ ? ComputeFullscreenControlsShowActivationTop(layout) : layout.panelTop;
    return clientPoint.y >= activationTop;
}

bool MainWindow::IsCursorInControlsActivationZone() const {
    if (hwnd_ == nullptr) {
        return false;
    }
    POINT cursor{};
    if (!GetCursorPos(&cursor) || !ScreenToClient(hwnd_, &cursor)) {
        return false;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    if (cursor.x < client.left || cursor.x >= client.right || cursor.y < client.top || cursor.y >= client.bottom) {
        return false;
    }
    return IsPointInControlsActivationZone(cursor);
}

bool MainWindow::IsPointInControlsRetentionZone(const POINT clientPoint) const {
    RECT client{};
    GetClientRect(hwnd_, &client);
    const MainWindowLayout layout = ComputeMainWindowLayout(client, quickBrowseVisible_, fullscreen_,
                                                            !uiSuppressed_ && (!fullscreen_ || quickBrowseVisible_ || controlsVisible_));
    if (fullscreen_) {
        const int topActivationBottom = ComputeFullscreenCaptionOverlayHeight(layout.height);
        if (clientPoint.y >= 0 && clientPoint.y < topActivationBottom) {
            return true;
        }
        return clientPoint.y >= ComputeFullscreenControlsHideActivationTop(layout);
    }
    return clientPoint.y >= layout.panelTop;
}

bool MainWindow::IsCursorInControlsRetentionZone() const {
    if (hwnd_ == nullptr) {
        return false;
    }
    POINT cursor{};
    if (!GetCursorPos(&cursor) || !ScreenToClient(hwnd_, &cursor)) {
        return false;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    if (cursor.x < client.left || cursor.x >= client.right || cursor.y < client.top || cursor.y >= client.bottom) {
        return false;
    }
    return IsPointInControlsRetentionZone(cursor);
}

bool MainWindow::IsPointInFullscreenCaptionActivationZone(const POINT clientPoint) const {
    if (!fullscreen_ || hwnd_ == nullptr) {
        return false;
    }
    RECT client{};
    GetClientRect(hwnd_, &client);
    const int captionRegionHeight = ComputeFullscreenCaptionOverlayHeight(static_cast<int>(client.bottom - client.top));
    return clientPoint.x >= client.left && clientPoint.x < client.right && clientPoint.y >= 0 && clientPoint.y < captionRegionHeight;
}

bool MainWindow::IsPointInCaptionButtons(const POINT clientPoint) const {
    for (HWND control : {fullscreenDownloadButton_, fullscreenMinimizeButton_, fullscreenWindowedButton_, fullscreenCloseButton_}) {
        if (control == nullptr || !IsWindowVisible(control)) {
            continue;
        }
        RECT rect{};
        GetWindowRect(control, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd_, reinterpret_cast<POINT*>(&rect), 2);
        if (PtInRect(&rect, clientPoint) != FALSE) {
            return true;
        }
    }
    return false;
}

bool MainWindow::IsCursorInFullscreenCaptionActivationZone() const {
    if (hwnd_ == nullptr) {
        return false;
    }
    POINT cursor{};
    if (!GetCursorPos(&cursor) || !ScreenToClient(hwnd_, &cursor)) {
        return false;
    }
    return IsPointInFullscreenCaptionActivationZone(cursor);
}

bool MainWindow::IsPointInsideVideo(POINT clientPoint) const {
    const RECT videoRect = VideoClientRect();
    return PtInRect(&videoRect, clientPoint) != FALSE;
}

}  // namespace velo::ui
