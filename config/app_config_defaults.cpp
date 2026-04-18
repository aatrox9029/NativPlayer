#include "config/config_service_internal.h"
#include "config/default_language_hint.h"

namespace velo::config {

AppConfig DefaultAppConfig() {
    AppConfig config;
    if (const auto hintedLanguage = TryResolvePackagedDefaultLanguage(); hintedLanguage.has_value()) {
        config.languageCode = *hintedLanguage;
    }
    return config;
}

std::filesystem::path DefaultConfigRoot() {
    PWSTR path = nullptr;
    std::filesystem::path root = std::filesystem::temp_directory_path() / kCompanyFolder;
    std::filesystem::path legacyRoot = std::filesystem::temp_directory_path() / kLegacyCompanyFolder;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)) && path != nullptr) {
        root = std::filesystem::path(path) / kCompanyFolder;
        legacyRoot = std::filesystem::path(path) / kLegacyCompanyFolder;
    }
    if (path != nullptr) {
        CoTaskMemFree(path);
    }
    if (!std::filesystem::exists(root) && std::filesystem::exists(legacyRoot)) {
        return legacyRoot;
    }
    return root;
}

}  // namespace velo::config
