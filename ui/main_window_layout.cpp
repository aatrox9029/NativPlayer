#include "ui/main_window_layout.h"

#include <algorithm>

namespace velo::ui {

namespace {
constexpr int kCaptionButtonGap = 2;
constexpr int kCaptionRegionPadding = 1;
constexpr int kBottomControlsLift = 4;
constexpr int kWindowResizeGutter = 8;
}

int ComputeCaptionBarHeight(const int windowHeight) {
    return std::clamp(windowHeight / 18, 32, 44);
}

int ComputeFullscreenCaptionOverlayHeight(const int windowHeight) {
    return std::max(1, (windowHeight + 49) / 50);
}

int ComputeFullscreenControlsShowActivationTop(const MainWindowLayout& layout) {
    return std::max(0, layout.panelTop - 8);
}

int ComputeFullscreenControlsHideActivationTop(const MainWindowLayout& layout) {
    return std::max(0, layout.panelTop - 28);
}

int ComputeCaptionButtonExtent(const int windowWidth, const int captionBarHeight) {
    const int maxHeightExtent = std::max(1, captionBarHeight - kCaptionRegionPadding * 2);
    const int maxWidthExtent = std::max(1, (windowWidth - 12 - kCaptionButtonGap * 2) / 3);
    return std::max(1, std::min(maxHeightExtent, maxWidthExtent));
}

MainWindowLayout ComputeMainWindowLayout(const RECT& clientRect, const bool quickBrowseVisible, const bool fullscreen,
                                         const bool controlsVisible) {
    MainWindowLayout layout;
    layout.width = clientRect.right - clientRect.left;
    layout.height = clientRect.bottom - clientRect.top;
    const int horizontalInset = fullscreen ? 0 : kWindowResizeGutter;
    const int usableWidth = std::max(320, layout.width - horizontalInset * 2);
    layout.quickBrowseWidth = quickBrowseVisible ? std::clamp(usableWidth / 5, 220, 360) : 0;
    layout.contentLeft = horizontalInset + layout.quickBrowseWidth;
    layout.contentWidth = std::max(320, usableWidth - layout.quickBrowseWidth);
    layout.captionBarHeight = fullscreen ? ComputeFullscreenCaptionOverlayHeight(layout.height) : ComputeCaptionBarHeight(layout.height);
    const int captionButtonExtent = ComputeCaptionButtonExtent(layout.width, layout.captionBarHeight);
    const int visibleCaptionBarHeight = controlsVisible ? layout.captionBarHeight : 0;
    layout.topBarHeight = 0;
    layout.bottomInset = 0;
    layout.gap = std::clamp(layout.contentWidth / 180, 6, 12);
    layout.timeLabelHeight = 14;
    layout.panelPaddingTop = fullscreen ? 4 : 6;
    layout.panelPaddingBottom = fullscreen ? 0 : 6;
    layout.panelRowGap = 4;
    layout.seekHeight = 10;
    layout.seekRowHeight = std::max(layout.seekHeight, layout.timeLabelHeight);
    layout.controlHeight = 28;
    layout.sliderHeight = 14;
    const int minimumPanelHeight = layout.panelPaddingTop + layout.seekRowHeight + layout.panelRowGap + layout.controlHeight + layout.panelPaddingBottom;
    layout.panelHeight = fullscreen ? (minimumPanelHeight + 6) : std::max(minimumPanelHeight, layout.height / 10);
    layout.panelTop = std::max(0, layout.height - layout.panelHeight);
    layout.currentTimeWidth = 60;
    layout.durationWidth = 60;
    layout.seekGap = 10;
    layout.labelWidth = std::clamp(layout.panelHeight / 2, 24, 34);
    layout.editWidth = 46;
    layout.currentTimeLeft = layout.contentLeft + 20;
    layout.seekLeft = layout.currentTimeLeft + layout.currentTimeWidth + layout.seekGap;
    layout.seekWidth =
        std::max(180, layout.width - layout.seekLeft - 20 - layout.seekGap - layout.durationWidth);
    layout.durationLeft = layout.seekLeft + layout.seekWidth + layout.seekGap;
    layout.currentTimeCenterX = layout.seekLeft + layout.seekWidth / 2;
    layout.buttonBaseWidth = std::clamp(layout.panelHeight / 2, 28, 42);
    layout.minButtonWidth = 24;
    layout.volumeSliderWidth = std::clamp(layout.contentWidth / 10, 56, 120);
    layout.speedSliderWidth = std::clamp(layout.contentWidth / 12, 46, 96);
    const int volumeGroupWidth = layout.labelWidth + layout.gap + layout.volumeSliderWidth;
    const int speedGroupWidth = layout.labelWidth + layout.gap + layout.speedSliderWidth + layout.gap + layout.editWidth;
    const int footerRight = layout.contentLeft + layout.contentWidth - 18;
    layout.speedGroupLeft = layout.contentLeft + 18;
    layout.volumeGroupLeft = footerRight - volumeGroupWidth;
    layout.buttonAreaLeft = layout.speedGroupLeft + speedGroupWidth + 18;
    layout.buttonAreaWidth = std::max(120, layout.volumeGroupLeft - layout.buttonAreaLeft - 18);
    layout.buttonAreaCenterX = layout.contentLeft + layout.contentWidth / 2;
    const int rowBlockHeight = layout.seekRowHeight + layout.panelRowGap + layout.controlHeight;
    int seekRowTop = 0;
    if (fullscreen) {
        const int buttonBottom = layout.panelTop + layout.panelHeight - layout.panelPaddingBottom;
        layout.buttonsY = std::max(layout.panelTop, buttonBottom - layout.controlHeight - kBottomControlsLift);
        seekRowTop = layout.buttonsY - layout.panelRowGap - layout.seekRowHeight;
    } else {
        const int extraVerticalSpace = std::max(0, layout.panelHeight - layout.panelPaddingTop - layout.panelPaddingBottom - rowBlockHeight);
        seekRowTop = layout.panelTop + layout.panelPaddingTop + extraVerticalSpace / 2;
        layout.buttonsY = std::max(layout.panelTop, seekRowTop + layout.seekRowHeight + layout.panelRowGap - kBottomControlsLift);
    }
    layout.timeRowTop = seekRowTop + (layout.seekRowHeight - layout.timeLabelHeight) / 2;
    layout.seekTop = seekRowTop + (layout.seekRowHeight - layout.seekHeight) / 2;
    layout.speedRowTop = layout.buttonsY + (layout.controlHeight - layout.sliderHeight) / 2;
    layout.volumeRowTop = layout.speedRowTop;

    if (visibleCaptionBarHeight > 0) {
        const int captionButtonsWidth = captionButtonExtent * 3 + kCaptionButtonGap * 2;
        const int titleLeft = layout.contentLeft + 10;
        const int titleRight = std::max(titleLeft, layout.contentLeft + layout.contentWidth - captionButtonsWidth - 14);
        layout.titleBarRect = RECT{layout.contentLeft, 0, layout.contentLeft + layout.contentWidth, visibleCaptionBarHeight};
        layout.titleTextRect = RECT{titleLeft, 0, titleRight, visibleCaptionBarHeight};
        layout.topBarHeight = visibleCaptionBarHeight;
    } else {
        layout.titleBarRect = RECT{layout.contentLeft, 0, layout.contentLeft + layout.contentWidth, 0};
        layout.titleTextRect = RECT{layout.contentLeft + 18, 0, layout.contentLeft + 18, 0};
    }
    const int videoTop = fullscreen ? 0 : layout.topBarHeight;
    const int videoBottom = fullscreen ? layout.height : (controlsVisible ? layout.panelTop : layout.height);
    layout.videoRect = RECT{layout.contentLeft, videoTop, layout.contentLeft + layout.contentWidth, std::max(videoTop, videoBottom)};
    layout.panelRect = RECT{layout.contentLeft, layout.panelTop, layout.contentLeft + layout.contentWidth, clientRect.bottom};
    layout.speedPanelRect = RECT{layout.speedGroupLeft - 4, layout.speedRowTop - 3,
                                 layout.speedGroupLeft + speedGroupWidth + 4, layout.speedRowTop + layout.sliderHeight + 3};
    layout.volumePanelRect = RECT{layout.volumeGroupLeft - 4, layout.volumeRowTop - 3,
                                  layout.volumeGroupLeft + volumeGroupWidth + 4, layout.volumeRowTop + layout.sliderHeight + 3};
    return layout;
}

}  // namespace velo::ui
