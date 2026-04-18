#include "app/app_metadata.h"

namespace velo::app {

std::wstring AppDisplayName() {
    return L"NativPlayer";
}

std::wstring AppVersion() {
    return L"v1.0.0";
}

std::wstring AppVersionLabel() {
    return AppDisplayName() + L" " + AppVersion();
}

}  // namespace velo::app
