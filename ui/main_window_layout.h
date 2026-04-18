#pragma once

#include <Windows.h>

namespace velo::ui {

struct MainWindowLayout {
    int width = 0;
    int height = 0;
    int quickBrowseWidth = 0;
    int contentLeft = 0;
    int contentWidth = 0;
    int captionBarHeight = 0;
    int topBarHeight = 0;
    int panelHeight = 0;
    int bottomInset = 0;
    int panelTop = 0;
    int panelPaddingTop = 0;
    int panelPaddingBottom = 0;
    int panelRowGap = 0;
    int seekTop = 0;
    int seekHeight = 0;
    int seekRowHeight = 0;
    int timeRowTop = 0;
    int timeLabelHeight = 18;
    int seekLeft = 0;
    int seekWidth = 0;
    int currentTimeLeft = 0;
    int currentTimeWidth = 64;
    int durationLeft = 0;
    int currentTimeCenterX = 0;
    int durationWidth = 64;
    int seekGap = 10;
    int gap = 8;
    int buttonBaseWidth = 72;
    int minButtonWidth = 44;
    int controlHeight = 0;
    int sliderHeight = 0;
    int labelWidth = 34;
    int editWidth = 54;
    int speedGroupLeft = 0;
    int volumeGroupLeft = 0;
    int speedSliderWidth = 0;
    int volumeSliderWidth = 0;
    int buttonAreaLeft = 0;
    int buttonAreaWidth = 0;
    int buttonAreaCenterX = 0;
    int buttonsY = 0;
    int speedRowTop = 0;
    int volumeRowTop = 0;
    RECT videoRect{};
    RECT titleBarRect{};
    RECT titleTextRect{};
    RECT panelRect{};
    RECT speedPanelRect{};
    RECT volumePanelRect{};
};

MainWindowLayout ComputeMainWindowLayout(const RECT& clientRect, bool quickBrowseVisible, bool fullscreen, bool controlsVisible);
int ComputeCaptionBarHeight(int windowHeight);
int ComputeFullscreenCaptionOverlayHeight(int windowHeight);
int ComputeFullscreenControlsShowActivationTop(const MainWindowLayout& layout);
int ComputeFullscreenControlsHideActivationTop(const MainWindowLayout& layout);
int ComputeCaptionButtonExtent(int windowWidth, int captionBarHeight);

}  // namespace velo::ui
