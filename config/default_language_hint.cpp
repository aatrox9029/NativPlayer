#include "config/default_language_hint.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "localization/localization.h"

namespace velo::config {
namespace {

constexpr wchar_t kDefaultLanguageHintFileName[] = L"nativplayer.default-language.txt";

std::wstring ExecutableDirectory() {
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            return std::filesystem::path(buffer.data()).parent_path().wstring();
        }
        buffer.resize(buffer.size() * 2);
    }
}

std::wstring Trim(std::wstring value) {
    if (!value.empty() && value.front() == 0xFEFF) {
        value.erase(value.begin());
    }
    const auto begin = value.find_first_not_of(L" \t\r\n");
    if (begin == std::wstring::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(L" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

}  // namespace

std::optional<std::wstring> TryResolvePackagedDefaultLanguage() {
    const auto executableDirectory = ExecutableDirectory();
    if (executableDirectory.empty()) {
        return std::nullopt;
    }

    const auto hintPath = std::filesystem::path(executableDirectory) / kDefaultLanguageHintFileName;
    std::wifstream input(hintPath);
    if (!input.is_open()) {
        return std::nullopt;
    }

    std::wstring languageCode;
    std::getline(input, languageCode);
    languageCode = Trim(languageCode);
    if (languageCode.empty()) {
        return std::nullopt;
    }

    const auto resolvedLanguage = velo::localization::ResolveLanguage(languageCode);
    return velo::localization::LanguageCode(resolvedLanguage);
}

}  // namespace velo::config
