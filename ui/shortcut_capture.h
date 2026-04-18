#pragma once

#include <Windows.h>

namespace velo::ui {

bool IsAllowedShortcutCaptureKey(unsigned int virtualKey);
bool IsAllowedShortcutCapturePointer(unsigned int virtualKey);
unsigned int ResolveShortcutCaptureKey(unsigned int virtualKey, LPARAM lParam);

}  // namespace velo::ui
