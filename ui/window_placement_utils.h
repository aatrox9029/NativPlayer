#pragma once

#include <Windows.h>

namespace velo::ui {

bool HasUsableWindowedRect(const RECT& rect);
bool ShouldTrackWindowedPlacement(const WINDOWPLACEMENT& placement);
bool ShouldTrackWindowedPlacement(const WINDOWPLACEMENT& placement, bool isZoomed, bool isIconic);
WINDOWPLACEMENT ResolveWindowedRestorePlacement(const WINDOWPLACEMENT& fallbackPlacement, const WINDOWPLACEMENT& lastWindowedPlacement);

}  // namespace velo::ui
