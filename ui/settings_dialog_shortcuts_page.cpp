#include "ui/settings_dialog_internal.h"

namespace velo::ui {

void SettingsDialog::RefreshShortcutBindingButton(const size_t index) {
    if (index >= shortcutComboControls_.size()) {
        return;
    }
    SetWindowTextW(shortcutComboControls_[index], velo::platform::win32::VirtualKeyDisplayName(shortcutBindingValues_[index]).c_str());
    InvalidateRect(shortcutComboControls_[index], nullptr, FALSE);
}

void SettingsDialog::BeginShortcutCapture(const size_t index) {
    if (index >= shortcutComboControls_.size()) {
        return;
    }
    capturingShortcutIndex_ = static_cast<int>(index);
    SetWindowTextW(shortcutComboControls_[index],
                   velo::localization::Text(initialConfig_.languageCode, velo::localization::TextId::SettingsKeyPrompt).c_str());
    UpdateShortcutWarning();
    SetFocus(hwnd_);
}

void SettingsDialog::CancelShortcutCapture() {
    const int captured = capturingShortcutIndex_;
    capturingShortcutIndex_ = -1;
    if (captured >= 0) {
        RefreshShortcutBindingButton(static_cast<size_t>(captured));
    }
    UpdateShortcutWarning();
}

bool SettingsDialog::CaptureShortcutInput(const unsigned int virtualKey) {
    if (capturingShortcutIndex_ < 0 || capturingShortcutIndex_ >= static_cast<int>(shortcutBindingValues_.size())) {
        return false;
    }
    shortcutBindingValues_[capturingShortcutIndex_] = virtualKey;
    const int captured = capturingShortcutIndex_;
    capturingShortcutIndex_ = -1;
    RefreshShortcutBindingButton(static_cast<size_t>(captured));
    UpdateShortcutWarning();
    return true;
}

}  // namespace velo::ui
