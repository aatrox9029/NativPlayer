#include "app/application_internal.h"

namespace velo::app {

std::wstring Application::ExportDiagnosticsBundle() const {
    const std::filesystem::path exportRoot = velo::diagnostics::DefaultLogRoot() / "exports";
    const auto bundlePath = velo::diagnostics::ExportDiagnosticsBundle(exportRoot, configService_.ConfigPath(), configService_.BackupPath(),
                                                                       logger_.SessionLogPath(), Narrow(BuildAboutText()), startupMetrics_,
                                                                       playbackFailureCounts_);
    if (bundlePath.empty()) {
        return {};
    }
    ShellExecuteW(nullptr, L"open", bundlePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return bundlePath.wstring();
}


std::wstring Application::BuildAboutText() const {
    const auto language = velo::localization::ResolveLanguage(currentConfig_.languageCode);
    std::wstring text = velo::app::AppVersionLabel();
    text += L"\n\n" + velo::localization::Text(language, velo::localization::TextId::AboutConfig) + L": ";
    text += configService_.ConfigPath().wstring();
    text += L"\n" + velo::localization::Text(language, velo::localization::TextId::AboutLog) + L": ";
    text += logger_.SessionLogPath().wstring();
    text += L"\n" + velo::localization::Text(language, velo::localization::TextId::AboutRecoveryMarker) + L": ";
    text += sessionRecovery_.MarkerPath().wstring();
    text += L"\n\n" + velo::localization::Text(language, velo::localization::TextId::AboutDiagnosticsHint);
    return text;
}


}  // namespace velo::app

