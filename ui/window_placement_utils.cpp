#include "ui/window_placement_utils.h"

#include <algorithm>

namespace velo::ui {

namespace {

WINDOWPLACEMENT NormalizeToWindowedPlacement(const WINDOWPLACEMENT& placement) {
    WINDOWPLACEMENT normalized = placement;
    normalized.length = sizeof(WINDOWPLACEMENT);
    normalized.showCmd = SW_SHOWNORMAL;
    return normalized;
}

}  // namespace

bool HasUsableWindowedRect(const RECT& rect) {
    return rect.right > rect.left && rect.bottom > rect.top;
}

bool ShouldTrackWindowedPlacement(const WINDOWPLACEMENT& placement) {
    return ShouldTrackWindowedPlacement(placement, false, false);
}

bool ShouldTrackWindowedPlacement(const WINDOWPLACEMENT& placement, const bool isZoomed, const bool isIconic) {
    if (isZoomed || isIconic) {
        return false;
    }
    return placement.showCmd != SW_SHOWMAXIMIZED && placement.showCmd != SW_MAXIMIZE &&
           placement.showCmd != SW_SHOWMINIMIZED && placement.showCmd != SW_MINIMIZE &&
           HasUsableWindowedRect(placement.rcNormalPosition);
}

WINDOWPLACEMENT ResolveWindowedRestorePlacement(const WINDOWPLACEMENT& fallbackPlacement, const WINDOWPLACEMENT& lastWindowedPlacement) {
    if (ShouldTrackWindowedPlacement(lastWindowedPlacement)) {
        return NormalizeToWindowedPlacement(lastWindowedPlacement);
    }
    return NormalizeToWindowedPlacement(fallbackPlacement);
}

}  // namespace velo::ui
