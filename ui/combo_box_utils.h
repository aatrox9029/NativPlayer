#pragma once

#include <Windows.h>

#include <string>

namespace velo::ui {

std::wstring GetComboBoxItemText(HWND combo, int index);

}  // namespace velo::ui
