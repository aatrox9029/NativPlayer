#pragma once

#include <Windows.h>

namespace velo::ui {

struct SettingsPageSurfaceTheme {
    HWND owner = nullptr;
    HBRUSH pageBrush = nullptr;
    HBRUSH controlBrush = nullptr;
};

void AttachSettingsPageSurface(HWND page, const SettingsPageSurfaceTheme* theme);

}  // namespace velo::ui
