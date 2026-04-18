#include "ui/settings_dialog_internal.h"

namespace velo::ui {

void SettingsDialog::ToggleCheckbox(HWND checkbox) const {
    if (checkbox == nullptr) {
        return;
    }
    SetCheckboxState(checkbox, !GetCheckboxState(checkbox));

    InvalidateRect(checkbox, nullptr, FALSE);
}


}  // namespace velo::ui

