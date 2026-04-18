#include "ui/combo_box_utils.h"

#include <commctrl.h>

namespace velo::ui {

std::wstring GetComboBoxItemText(HWND combo, const int index) {
    if (combo == nullptr || index < 0) {
        return {};
    }

    const LRESULT length = SendMessageW(combo, CB_GETLBTEXTLEN, static_cast<WPARAM>(index), 0);
    if (length == CB_ERR) {
        return {};
    }

    std::wstring text(static_cast<size_t>(length), L'\0');
    if (length == 0) {
        return text;
    }

    SendMessageW(combo, CB_GETLBTEXT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(text.data()));
    return text;
}

}  // namespace velo::ui
