#include "ui/app_icon.h"

namespace velo::ui {
namespace {

const wchar_t* const kAppIconResource = MAKEINTRESOURCEW(101);

HICON LoadResourceIcon(HINSTANCE instance, const int iconSize) {
    return static_cast<HICON>(LoadImageW(instance, kAppIconResource, IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
}

}  // namespace

AppIconSet LoadAppIconSet(HINSTANCE instance) {
    return AppIconSet{
        .largeIcon = LoadResourceIcon(instance, GetSystemMetrics(SM_CXICON)),
        .smallIcon = LoadResourceIcon(instance, GetSystemMetrics(SM_CXSMICON)),
    };
}

void DestroyAppIconSet(AppIconSet& icons) noexcept {
    if (icons.largeIcon != nullptr) {
        DestroyIcon(icons.largeIcon);
        icons.largeIcon = nullptr;
    }
    if (icons.smallIcon != nullptr) {
        DestroyIcon(icons.smallIcon);
        icons.smallIcon = nullptr;
    }
}

}  // namespace velo::ui
