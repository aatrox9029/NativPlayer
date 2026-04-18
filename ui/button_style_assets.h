#pragma once

#include <Windows.h>

#include <string_view>

namespace velo::ui {

bool DrawButtonStyleIcon(HINSTANCE instance, std::wstring_view assetStem, HDC deviceContext, const RECT& bounds, bool disabled);

}  // namespace velo::ui