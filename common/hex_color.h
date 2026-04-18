#pragma once

#include <Windows.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace velo::common {

struct RgbaColor {
	std::uint8_t red = 0;
	std::uint8_t green = 0;
	std::uint8_t blue = 0;
	std::uint8_t alpha = 0xFF;
};

std::wstring NormalizeRgbaHexColor(std::wstring_view value, std::wstring_view fallback = L"FFFFFFFF");
std::optional<RgbaColor> TryParseRgbaColor(std::wstring_view value);
std::wstring FormatRgbaHexColor(const RgbaColor& value);
std::optional<COLORREF> TryParseRgbColor(std::wstring_view value);

}  // namespace velo::common
