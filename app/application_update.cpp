#include "app/application_internal.h"
#include "app/release_update.h"

namespace velo::app {

void Application::StartReleaseUpdateCheck() {
    const int timeoutMs = std::clamp(currentConfig_.networkTimeoutMs, 2000, 5000);
    updateCheckThread_ = std::jthread([this, timeoutMs](std::stop_token stopToken) {
        std::wstring errorMessage;
        const auto availableUpdate = QueryAvailableReleaseUpdate(timeoutMs, errorMessage);
        if (stopToken.stop_requested()) {
            return;
        }
        if (!availableUpdate.has_value()) {
            if (!errorMessage.empty()) {
                logger_.Info("Release update check skipped: " + Narrow(errorMessage));
            }
            return;
        }

        logger_.Info("Found newer NativPlayer release: " + Narrow(availableUpdate->tagName));
        mainWindow_.PostUpdateAvailability(availableUpdate->tagName, availableUpdate->downloadUrl);
    });
}

void Application::OpenUpdateDownload() const {
    ShellExecuteW(nullptr, L"open", L"https://github.com/aatrox9029/NativPlayer/releases/latest", nullptr, nullptr, SW_SHOWNORMAL);
}

void Application::OpenUpdateDownload(const std::wstring& url) const {
    if (url.empty()) {
        OpenUpdateDownload();
        return;
    }
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}  // namespace velo::app
