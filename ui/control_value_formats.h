#pragma once

#include <optional>
#include <string>

namespace velo::ui {

std::wstring FormatVolumeInput(int volume);
std::wstring FormatSpeedInput(double speed);
std::optional<int> ParseVolumeInput(const std::wstring& text);
std::optional<double> ParseSpeedInput(const std::wstring& text);

}  // namespace velo::ui
