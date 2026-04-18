#pragma once

#include <string>
#include <string_view>

namespace velo::app {

struct MpvRuntimeLicensePromptText {
    std::wstring windowTitle;
    std::wstring headline;
    std::wstring body;
    std::wstring pathLabel;
    std::wstring footer;
    std::wstring autoButton;
    std::wstring manualButton;
    std::wstring restartButton;
    std::wstring cancelButton;
    std::wstring runtimeMissingStatus;
    std::wstring fallbackText;
};

MpvRuntimeLicensePromptText BuildMpvRuntimeLicensePromptText(std::wstring_view languageCode);
std::wstring BuildManualDownloadFailureText(std::wstring_view languageCode);
std::wstring BuildCancelledDownloadFailureText(std::wstring_view languageCode);

}  // namespace velo::app
