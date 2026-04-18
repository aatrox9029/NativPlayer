#pragma once

#include <string>

namespace velo::app {

enum class MpvRuntimeLicenseChoice {
    AutoDownload,
    RuntimeReady,
    Cancel,
};

MpvRuntimeLicenseChoice PromptForMpvRuntimeDownloadConsent(std::wstring& languageCode);

}  // namespace velo::app
