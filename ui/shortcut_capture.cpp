#include "ui/shortcut_capture.h"

#include <Windows.h>

namespace velo::ui {

bool IsAllowedShortcutCaptureKey(const unsigned int virtualKey) {
    return virtualKey != VK_LEFT && virtualKey != VK_RIGHT;
}

bool IsAllowedShortcutCapturePointer(const unsigned int virtualKey) {
    return virtualKey == VK_MBUTTON || virtualKey == VK_XBUTTON1 || virtualKey == VK_XBUTTON2;
}

unsigned int ResolveShortcutCaptureKey(const unsigned int virtualKey, const LPARAM lParam) {
    if (virtualKey != VK_PROCESSKEY) {
        return virtualKey;
    }

    const unsigned int scanCode = (static_cast<unsigned int>(lParam) >> 16) & 0xFF;
    if (scanCode == 0) {
        return virtualKey;
    }

    const bool extended = ((static_cast<unsigned int>(lParam) >> 24) & 0x1U) != 0;
    const unsigned int resolved = MapVirtualKeyW(scanCode | (extended ? 0xE000 : 0), MAPVK_VSC_TO_VK_EX);
    return resolved == 0 ? virtualKey : resolved;
}

}  // namespace velo::ui
