#include "app/application_internal.h"

#include <ShlObj.h>

#include <filesystem>

namespace velo::app {
namespace {

std::wstring SanitizeScreenshotStem(std::wstring value) {
    for (wchar_t& ch : value) {
        switch (ch) {
            case L'\\':
            case L'/':
            case L':':
            case L'*':
            case L'?':
            case L'"':
            case L'<':
            case L'>':
            case L'|':
                ch = L'_';
                break;
            default:
                break;
        }
    }
    while (!value.empty() && (value.back() == L' ' || value.back() == L'.')) {
        value.pop_back();
    }
    return value.empty() ? L"nativplayer" : value;
}

std::wstring BuildScreenshotTimestamp() {
    SYSTEMTIME localTime{};
    GetLocalTime(&localTime);
    wchar_t buffer[32] = {};
    swprintf_s(buffer, L"%04u%02u%02u-%02u%02u%02u",
               localTime.wYear,
               localTime.wMonth,
               localTime.wDay,
               localTime.wHour,
               localTime.wMinute,
               localTime.wSecond);
    return buffer;
}

std::filesystem::path ResolveScreenshotDirectory() {
    PWSTR picturesPath = nullptr;
    std::filesystem::path directory;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Pictures, KF_FLAG_DEFAULT, nullptr, &picturesPath)) && picturesPath != nullptr) {
        directory = std::filesystem::path(picturesPath) / L"NativPlayer Screenshots";
        CoTaskMemFree(picturesPath);
    }
    if (directory.empty()) {
        directory = velo::config::DefaultConfigRoot() / L"screenshots";
    }
    return directory;
}

std::wstring BuildScreenshotBaseName(const velo::ui::PlayerState& state, const std::wstring& currentOpenPath) {
    if (!state.mediaTitle.empty()) {
        return SanitizeScreenshotStem(Utf8ToWide(state.mediaTitle));
    }
    if (!currentOpenPath.empty()) {
        const std::filesystem::path inputPath(currentOpenPath);
        const std::wstring stem = inputPath.stem().wstring();
        if (!stem.empty()) {
            return SanitizeScreenshotStem(stem);
        }
        return SanitizeScreenshotStem(ShortDisplayName(currentOpenPath));
    }
    return L"nativplayer";
}

}  // namespace

std::wstring Application::BuildScreenshotOutputPath() const {
    std::scoped_lock lock(stateMutex_);
    const std::wstring baseName = BuildScreenshotBaseName(latestState_, currentOpenPath_);
    const std::wstring extension = currentConfig_.screenshotFormat == L"jpg" ? L"jpg" : L"png";
    return (ResolveScreenshotDirectory() / (baseName + L"-" + BuildScreenshotTimestamp() + L"." + extension)).wstring();
}

void Application::TakeScreenshot() {
    std::wstring languageCode;
    {
        std::scoped_lock lock(stateMutex_);
        if (!latestState_.isLoaded && currentOpenPath_.empty()) {
            return;
        }
        languageCode = currentConfig_.languageCode;
    }

    const std::wstring outputPath = BuildScreenshotOutputPath();
    if (outputPath.empty()) {
        return;
    }

    playerThread_.TakeScreenshot(outputPath);
    mainWindow_.PostOsd(velo::localization::Text(languageCode, velo::localization::TextId::ScreenshotSavedPrefix) +
                        std::filesystem::path(outputPath).filename().wstring());
}

}  // namespace velo::app
