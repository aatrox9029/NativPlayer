#pragma once

#include <Windows.h>

namespace velo::ui {

struct AppIconSet {
    HICON largeIcon = nullptr;
    HICON smallIcon = nullptr;
};

AppIconSet LoadAppIconSet(HINSTANCE instance);
void DestroyAppIconSet(AppIconSet& icons) noexcept;

}  // namespace velo::ui
