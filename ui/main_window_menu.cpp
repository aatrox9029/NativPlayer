#include "ui/main_window_internal.h"

namespace velo::ui {

namespace {

std::wstring MenuSelectionLabel(const std::wstring& label, const bool selected) {
    return selected ? (L"> " + label) : label;
}

}  // namespace

void MainWindow::ShowRecentFilesMenu() {
    return;
}

void MainWindow::ShowContextMenu(POINT screenPoint, const bool fromKeyboard) {
    HMENU endActionMenu = CreatePopupMenu();
    HMENU menu = CreatePopupMenu();
    if (endActionMenu == nullptr || menu == nullptr) {
        if (endActionMenu != nullptr) {
            DestroyMenu(endActionMenu);
        }
        if (menu != nullptr) {
            DestroyMenu(menu);
        }
        return;
    }

    if (fullscreen_) {
        SetControlsVisible(true, false);
        SetFullscreenCaptionButtonsVisible(true);
    }

    menuVisualItems_.clear();
    ApplyMenuTheme(menu);
    ApplyMenuTheme(endActionMenu);
    const auto language = velo::localization::ResolveLanguage(config_.languageCode);
    UINT separatorId = kMenuSeparatorBase;
    const auto appendSeparator = [&](HMENU target) {
        menuVisualItems_[separatorId] = {.label = L"", .shortcut = L"", .enabled = false, .separator = true};
        AppendMenuW(target, MF_OWNERDRAW | MF_DISABLED, separatorId, reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(separatorId)));
        ++separatorId;
    };
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::OpenFolder), velo::localization::Text(config_.languageCode, velo::localization::TextId::OpenFolder));
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::LoadSubtitle), velo::localization::Text(config_.languageCode, velo::localization::TextId::LoadSubtitle));
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::TakeScreenshot), velo::localization::ActionLabel(language, L"take_screenshot"),
                    ShortcutForAction(config_, L"take_screenshot"), state_.isLoaded);
    appendSeparator(menu);
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::ReplayCurrent), velo::localization::Text(config_.languageCode, velo::localization::TextId::ReplayCurrentFile));
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::ResetSpeed), velo::localization::Text(config_.languageCode, velo::localization::TextId::ResetSpeed), ShortcutForAction(config_, L"reset_speed"));
    appendSeparator(menu);
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::OpenSettings), velo::localization::Text(config_.languageCode, velo::localization::TextId::Settings));
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::ShowShortcuts), velo::localization::Text(config_.languageCode, velo::localization::TextId::ShortcutHelp));

    AppendMenuEntry(endActionMenu, static_cast<UINT>(ContextCommand::EndActionReplay), velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsEndActionReplay));
    AppendMenuEntry(endActionMenu, static_cast<UINT>(ContextCommand::EndActionPlayNext), velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsEndActionPlayNext));
    AppendMenuEntry(endActionMenu, static_cast<UINT>(ContextCommand::EndActionStop), velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsEndActionStop));
    AppendMenuEntry(endActionMenu, static_cast<UINT>(ContextCommand::EndActionCloseWindow), velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsEndActionCloseWindow));
    AppendPopupMenuEntry(menu, kPopupEndActionMenuCommand, endActionMenu, velo::localization::Text(config_.languageCode, velo::localization::TextId::PlaybackEnded));

    if (fromKeyboard) {
        RECT client{};
        GetClientRect(hwnd_, &client);
        POINT center{client.left + 60, client.top + 60};
        ClientToScreen(hwnd_, &center);
        screenPoint = center;
    }

    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    if (fullscreen_) {
        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(hwnd_, &cursor);
        HandlePointerActivity(cursor);
    }
    DestroyMenu(menu);
}

void MainWindow::ShowOverflowMenu() {
    HMENU audioTrackMenu = CreatePopupMenu();
    HMENU subtitleTrackMenu = CreatePopupMenu();
    HMENU audioOutputMenu = CreatePopupMenu();
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr || audioTrackMenu == nullptr || subtitleTrackMenu == nullptr || audioOutputMenu == nullptr) {
        if (audioTrackMenu != nullptr) {
            DestroyMenu(audioTrackMenu);
        }
        if (subtitleTrackMenu != nullptr) {
            DestroyMenu(subtitleTrackMenu);
        }
        if (audioOutputMenu != nullptr) {
            DestroyMenu(audioOutputMenu);
        }
        if (menu != nullptr) {
            DestroyMenu(menu);
        }
        return;
    }

    menuVisualItems_.clear();
    ApplyMenuTheme(menu);
    UINT separatorId = kMenuSeparatorBase;
    const auto appendSeparator = [&](HMENU target) {
        menuVisualItems_[separatorId] = {.label = L"", .shortcut = L"", .enabled = false, .separator = true};
        AppendMenuW(target, MF_OWNERDRAW | MF_DISABLED, separatorId, reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(separatorId)));
        ++separatorId;
    };
    const auto appendTrackSelectionEntries = [&](HMENU targetMenu, const std::vector<PlayerTrackOption>& options, const UINT commandBase,
                                                 const std::wstring& offLabel, const long long selectedTrackId) {
        AppendMenuEntry(targetMenu, commandBase, MenuSelectionLabel(offLabel, selectedTrackId < 0));
        for (size_t index = 0; index < options.size(); ++index) {
            const auto& option = options[index];
            AppendMenuEntry(targetMenu, commandBase + static_cast<UINT>(index + 1),
                            MenuSelectionLabel(Utf8ToWide(option.label), option.selected));
        }
    };
    const auto appendAudioOutputEntries = [&](HMENU targetMenu, const std::vector<AudioOutputOption>& options, const UINT commandBase,
                                              const std::wstring& autoLabel, const std::string& selectedOutputId) {
        const bool autoSelected = selectedOutputId.empty() || selectedOutputId == "auto";
        AppendMenuEntry(targetMenu, commandBase, MenuSelectionLabel(autoLabel, autoSelected));
        for (size_t index = 0; index < options.size(); ++index) {
            const auto& option = options[index];
            AppendMenuEntry(targetMenu, commandBase + static_cast<UINT>(index + 1),
                            MenuSelectionLabel(Utf8ToWide(option.label), option.selected));
        }
    };
    if (!IsControlCurrentlyVisible(subtitleButton_)) {
        if (!state_.subtitleTracks.empty() || state_.subtitleTrackId >= 0) {
            appendTrackSelectionEntries(subtitleTrackMenu, state_.subtitleTracks, kDynamicSubtitleTrackCommandBase,
                                        velo::localization::Text(config_.languageCode, velo::localization::TextId::Mute), state_.subtitleTrackId);
            AppendPopupMenuEntry(menu, kPopupSubtitleTrackMenuCommand, subtitleTrackMenu,
                                 velo::localization::Text(config_.languageCode, velo::localization::TextId::Subtitle));
        } else {
            AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::CycleSubtitle), velo::localization::Text(config_.languageCode, velo::localization::TextId::CycleSubtitleTrack), ShortcutForAction(config_, L"cycle_subtitle"));
        }
    }
    if (!state_.audioTracks.empty() || state_.audioTrackId >= 0) {
        appendTrackSelectionEntries(audioTrackMenu, state_.audioTracks, kDynamicAudioTrackCommandBase,
                                    velo::localization::Text(config_.languageCode, velo::localization::TextId::Mute), state_.audioTrackId);
        AppendPopupMenuEntry(menu, kPopupAudioTrackMenuCommand, audioTrackMenu,
                             velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsPageAudio));
    }
    if (!state_.audioOutputs.empty() || !state_.audioOutputDeviceId.empty()) {
        appendAudioOutputEntries(audioOutputMenu, state_.audioOutputs, kDynamicAudioOutputCommandBase,
                                 velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsAudioDefault),
                                 state_.audioOutputDeviceId);
        AppendPopupMenuEntry(menu, kPopupAudioOutputMenuCommand, audioOutputMenu,
                             velo::localization::Text(config_.languageCode, velo::localization::TextId::SettingsAudioOutput));
    }
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::CycleSpeed), velo::localization::Text(config_.languageCode, velo::localization::TextId::ToggleSpeed), L"]");
    appendSeparator(menu);
    if (!IsControlCurrentlyVisible(settingsButton_)) {
        AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::OpenSettings), velo::localization::Text(config_.languageCode, velo::localization::TextId::Settings));
    }
    AppendMenuEntry(menu, static_cast<UINT>(ContextCommand::ShowShortcuts), velo::localization::Text(config_.languageCode, velo::localization::TextId::ShortcutHelp));

    RECT bounds{};
    GetWindowRect(moreButton_, &bounds);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, bounds.left, bounds.top - 4, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

void MainWindow::ApplyMenuTheme(HMENU menu) const {
    if (menu == nullptr) {
        return;
    }
    MENUINFO menuInfo{};
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_BACKGROUND;
    menuInfo.hbrBack = MenuBrush();
    SetMenuInfo(menu, &menuInfo);
}

void MainWindow::AppendMenuEntry(HMENU menu, const UINT commandId, std::wstring label, std::wstring shortcut, const bool enabled) {
    menuVisualItems_[commandId] = {.label = std::move(label), .shortcut = std::move(shortcut), .enabled = enabled};
    AppendMenuW(menu, (enabled ? MF_OWNERDRAW : (MF_OWNERDRAW | MF_GRAYED)), commandId, reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(commandId)));
}

void MainWindow::AppendPopupMenuEntry(HMENU menu, const UINT commandId, HMENU popup, std::wstring label) {
    if (menu == nullptr || popup == nullptr) {
        return;
    }
    menuVisualItems_[commandId] = {.label = std::move(label), .shortcut = L"", .enabled = true, .separator = false, .popup = true};
    MENUITEMINFOW menuItem{};
    menuItem.cbSize = sizeof(menuItem);
    menuItem.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID | MIIM_DATA | MIIM_STATE;
    menuItem.fType = MFT_OWNERDRAW;
    menuItem.wID = commandId;
    menuItem.dwItemData = static_cast<ULONG_PTR>(commandId);
    menuItem.hSubMenu = popup;
    menuItem.fState = MFS_ENABLED;
    InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &menuItem);
}

void MainWindow::DrawMenuItem(const DRAWITEMSTRUCT& drawItem) const {
    const auto& palette = tokens::DarkPalette();
    auto menuItem = menuVisualItems_.find(drawItem.itemID);
    if (menuItem == menuVisualItems_.end() && drawItem.itemData != 0) {
        menuItem = menuVisualItems_.find(static_cast<UINT>(drawItem.itemData));
    }
    if (menuItem == menuVisualItems_.end()) {
        return;
    }
    if (menuItem->second.separator) {
        HBRUSH fillBrush = CreateSolidBrush(palette.bgSurface1);
        FillRect(drawItem.hDC, &drawItem.rcItem, fillBrush);
        DeleteObject(fillBrush);
        RECT lineRect = drawItem.rcItem;
        lineRect.left += tokens::Space5();
        lineRect.right -= tokens::Space5();
        lineRect.top = (drawItem.rcItem.top + drawItem.rcItem.bottom) / 2;
        lineRect.bottom = lineRect.top + 1;
        HBRUSH lineBrush = CreateSolidBrush(palette.strokeSoft);
        FillRect(drawItem.hDC, &lineRect, lineBrush);
        DeleteObject(lineBrush);
        return;
    }

    const bool selected = (drawItem.itemState & ODS_SELECTED) != 0;
    const bool disabled = (drawItem.itemState & ODS_DISABLED) != 0 || !menuItem->second.enabled;
    const COLORREF fill = selected ? tokens::MixColor(palette.brandPrimary, palette.bgSurface2, 0.78) : palette.bgSurface1;
    HBRUSH fillBrush = CreateSolidBrush(fill);
    FillRect(drawItem.hDC, &drawItem.rcItem, fillBrush);
    DeleteObject(fillBrush);
    tokens::DrawRoundedOutline(drawItem.hDC, drawItem.rcItem, palette.strokeSoft, tokens::RadiusMd());

    RECT textRect = drawItem.rcItem;
    textRect.left += tokens::Space5();
    textRect.right -= tokens::Space5();
    SetBkMode(drawItem.hDC, TRANSPARENT);
    SetTextColor(drawItem.hDC, disabled ? palette.textTertiary : palette.textPrimary);
    HFONT previousFont = reinterpret_cast<HFONT>(SelectObject(drawItem.hDC, uiFont_));
    DrawTextW(drawItem.hDC, menuItem->second.label.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (!menuItem->second.shortcut.empty()) {
        SetTextColor(drawItem.hDC, disabled ? palette.textTertiary : palette.textSecondary);
        DrawTextW(drawItem.hDC, menuItem->second.shortcut.c_str(), -1, &textRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    } else if (menuItem->second.popup) {
        RECT arrowRect = drawItem.rcItem;
        arrowRect.left = arrowRect.right - 18;
        arrowRect.right -= tokens::Space5();
        SetTextColor(drawItem.hDC, disabled ? palette.textTertiary : palette.textSecondary);
        DrawTextW(drawItem.hDC, L"\u25B8", -1, &arrowRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
    SelectObject(drawItem.hDC, previousFont);
}

void MainWindow::MeasureMenuItem(MEASUREITEMSTRUCT& measureItem) const {
    auto menuItem = menuVisualItems_.find(measureItem.itemID);
    if (menuItem == menuVisualItems_.end() && measureItem.itemData != 0) {
        menuItem = menuVisualItems_.find(static_cast<UINT>(measureItem.itemData));
    }
    if (menuItem != menuVisualItems_.end() && menuItem->second.separator) {
        measureItem.itemHeight = 14;
        measureItem.itemWidth = 248;
        return;
    }
    measureItem.itemHeight = 32;
    measureItem.itemWidth = 248;
}

}  // namespace velo::ui
