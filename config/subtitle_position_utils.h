#pragma once

#include <string_view>

namespace velo::config {

constexpr int kSubtitleVerticalDirectionMin = -999;
constexpr int kSubtitleVerticalDirectionMax = 999;
constexpr int kSubtitleVerticalPositionMin = 0;
constexpr int kSubtitleVerticalPositionMax = 100;

int ClampSubtitleVerticalDirection(int direction);
int ClampSubtitleVerticalPosition(int position);
int SubtitlePresetAnchorVerticalPosition(std::wstring_view preset);
int SubtitleVerticalDirectionFromMargin(std::wstring_view preset, int verticalMargin);
int SubtitleVerticalMarginFromDirection(std::wstring_view preset, int direction);

}  // namespace velo::config
