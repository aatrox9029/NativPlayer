#include "config/subtitle_position_utils.h"

#include <algorithm>

namespace velo::config {

int ClampSubtitleVerticalDirection(const int direction) {
    return std::clamp(direction, kSubtitleVerticalDirectionMin, kSubtitleVerticalDirectionMax);
}

int ClampSubtitleVerticalPosition(const int position) {
    return std::clamp(position, kSubtitleVerticalPositionMin, kSubtitleVerticalPositionMax);
}

int SubtitlePresetAnchorVerticalPosition(const std::wstring_view preset) {
    if (preset == L"top") {
        return 4;
    }
    if (preset == L"middle") {
        return 50;
    }
    return 96;
}

int SubtitleVerticalDirectionFromMargin(const std::wstring_view preset, const int verticalMargin) {
    return ClampSubtitleVerticalDirection(SubtitlePresetAnchorVerticalPosition(preset) - ClampSubtitleVerticalPosition(verticalMargin));
}

int SubtitleVerticalMarginFromDirection(const std::wstring_view preset, const int direction) {
    return ClampSubtitleVerticalPosition(SubtitlePresetAnchorVerticalPosition(preset) - ClampSubtitleVerticalDirection(direction));
}

}  // namespace velo::config
