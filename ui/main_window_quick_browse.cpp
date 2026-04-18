#include "ui/main_window_internal.h"

namespace velo::ui {

void MainWindow::ToggleQuickBrowse() {
    if (quickBrowseVisible_) {
        HideQuickBrowse();
        return;
    }
    ShowQuickBrowse();
}

void MainWindow::ShowQuickBrowse() {
    HideSeekPreview();
    const std::wstring mediaPath = Utf8ToWide(state_.currentPath);
    if (!velo::app::CanBuildQuickBrowseCatalog(mediaPath)) {
        ShowOsdNow(velo::localization::Text(config_.languageCode, velo::localization::TextId::QuickBrowseUnavailable), 1000);
        return;
    }

    quickBrowseVisible_ = true;
    if (quickBrowseRootFolder_.empty()) {
        quickBrowseRootFolder_ = std::filesystem::path(mediaPath).parent_path().wstring();
    }
    if (quickBrowseFolder_.empty()) {
        quickBrowseFolder_ = quickBrowseRootFolder_;
    }
    RefreshQuickBrowse();

    SetControlsVisible(true, false);
    LayoutChildren();
    RedrawUiSurface();
}

void MainWindow::HideQuickBrowse() {
    if (!quickBrowseVisible_) {
        return;
    }

    HideSeekPreview();
    quickBrowseVisible_ = false;
    quickBrowsePanel_.SetVisible(false);
    LayoutChildren();
    if (fullscreen_ && !mouseInsideWindow_ && !IsControlInteractionActive()) {
        SetControlsVisible(false, false);
    } else {
        SetControlsVisible(true, true);
    }
    RedrawUiSurface();
}

void MainWindow::RefreshQuickBrowse() {
    const std::wstring mediaPath = Utf8ToWide(state_.currentPath);
    if (!quickBrowseVisible_ || !velo::app::CanBuildQuickBrowseCatalog(mediaPath)) {
        return;
    }

    const auto catalog = velo::app::BuildQuickBrowseCatalog(mediaPath, quickBrowseFolder_, quickBrowseRootFolder_);
    if (catalog.currentFolder.empty()) {
        HideQuickBrowse();
        return;

    }

    quickBrowseFolder_ = catalog.currentFolder;
    quickBrowseRootFolder_ = catalog.rootFolder;
    quickBrowsePanel_.SetCatalog(catalog);
}

void MainWindow::NavigateQuickBrowseFolder(const std::wstring& folderPath) {
    quickBrowseFolder_ = folderPath;
    RefreshQuickBrowse();
}

bool MainWindow::HandleQuickBrowseOutsideClick(const POINT clientPoint) {
    if (!quickBrowseVisible_ || quickBrowsePanel_.WindowHandle() == nullptr) {
        return false;
    }

    RECT bounds{};
    GetWindowRect(quickBrowsePanel_.WindowHandle(), &bounds);
    POINT screenPoint = clientPoint;
    ClientToScreen(hwnd_, &screenPoint);
    if (PtInRect(&bounds, screenPoint) != FALSE) {
        return false;
    }

    HideQuickBrowse();
    return true;
}


}  // namespace velo::ui

