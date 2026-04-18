#include "app/mpv_runtime_bootstrap.h"
#include "app/mpv_runtime_license_prompt.h"
#include "app/mpv_runtime_license_prompt_text.h"
#include "app/mpv_runtime_release_asset.h"

#include <Windows.h>

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "common/http_download.h"
#include "common/portable_executable.h"
#include "common/sha256.h"
#include "common/text_encoding.h"
#include "diagnostics/logger.h"
#include "playback/mpv_runtime_probe.h"
#include "playback/mpv_runtime_provenance.h"
#include "playback/mpv_runtime_paths.h"

namespace velo::app {
namespace {

std::string Narrow(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

std::wstring QuoteArg(const std::wstring& value) {
    std::wstring escaped = L"\"";
    for (const wchar_t ch : value) {
        if (ch == L'"') {
            escaped += L"\\\"";
        } else {
            escaped.push_back(ch);
        }
    }
    escaped += L"\"";
    return escaped;
}

bool IsCompatibleRuntime(const std::filesystem::path& path) {
    const auto probe = velo::playback::ProbeMpvRuntimeLibrary(path);
    return probe.found && !probe.architectureMismatch && probe.libraryImage.valid;
}

bool CopyRuntimeFile(const std::filesystem::path& sourcePath, const std::filesystem::path& destinationPath) {
    std::error_code error;
    std::filesystem::create_directories(destinationPath.parent_path(), error);
    return CopyFileW(sourcePath.c_str(), destinationPath.c_str(), FALSE) != FALSE;
}

bool RunTarExtract(const std::filesystem::path& archivePath, const std::filesystem::path& destinationDirectory) {
    const std::wstring commandLine =
        L"tar.exe -xf " + QuoteArg(archivePath.wstring()) + L" -C " + QuoteArg(destinationDirectory.wstring()) + L" libmpv-2.dll";
    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo,
                        &processInfo)) {
        return false;
    }

    CloseHandle(processInfo.hThread);
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hProcess);
    return exitCode == 0;
}

MpvRuntimeBootstrapResult BuildFailure(const std::wstring& detail) {
    MpvRuntimeBootstrapResult result;
    result.ready = false;
    result.detail = detail;
    return result;
}

bool PersistRuntimeProvenance(const std::filesystem::path& dllPath, const MpvRuntimeReleaseAsset& asset,
                              velo::diagnostics::Logger& logger) {
    std::wstring dllSha256Hex;
    std::wstring dllHashError;
    if (!velo::common::TryComputeFileSha256Hex(dllPath, dllSha256Hex, dllHashError)) {
        logger.Warn("Failed to hash libmpv DLL for provenance manifest: " + Narrow(dllHashError));
        return false;
    }

    const velo::playback::MpvRuntimeProvenance provenance{
        .sourceRepo = L"zhongfly/mpv-winbuild",
        .releaseTag = asset.releaseTag,
        .assetName = asset.name,
        .assetSha256Hex = asset.sha256Hex,
        .dllSha256Hex = dllSha256Hex,
        .downloadUrl = asset.downloadUrl,
        .licenseId = L"LGPL-2.1-or-later",
        .commercialUseNote = L"Commercial use and redistribution are allowed when the LGPL obligations are satisfied.",
    };

    std::wstring provenanceError;
    if (!velo::playback::WriteMpvRuntimeProvenance(dllPath, provenance, provenanceError)) {
        logger.Warn("Failed to write libmpv provenance manifest: " + Narrow(provenanceError));
        return false;
    }

    return true;
}

}  // namespace

MpvRuntimeBootstrapResult EnsureManagedMpvRuntimeAvailable(velo::diagnostics::Logger& logger, const int networkTimeoutMs,
                                                           const std::wstring_view languageCode) {
    const std::wstring initialLanguage(languageCode);
    const auto runtimePath = velo::playback::ManagedMpvRuntimePath(L"libmpv-2.dll");
    if (IsCompatibleRuntime(runtimePath)) {
        logger.Info("Managed libmpv runtime already available at " + Narrow(runtimePath.wstring()));
        return {
            .ready = true,
            .downloaded = false,
            .runtimePath = runtimePath.wstring(),
            .detail = L"Using managed libmpv runtime.",
            .effectiveLanguageCode = initialLanguage,
        };
    }

    const auto bundledRuntimePath = velo::playback::BundledMpvRuntimePath(L"libmpv-2.dll");
    if (!bundledRuntimePath.empty()) {
        std::wstring bundledApprovalError;
        if (!velo::playback::IsApprovedBundledMpvRuntime(bundledRuntimePath, bundledApprovalError)) {
            logger.Warn("Ignoring bundled libmpv runtime because its provenance check failed: " + Narrow(bundledApprovalError));
        } else if (IsCompatibleRuntime(bundledRuntimePath) && CopyRuntimeFile(bundledRuntimePath, runtimePath) && IsCompatibleRuntime(runtimePath)) {
            velo::playback::MpvRuntimeProvenance bundledProvenance;
            std::wstring provenanceError;
            if (velo::playback::TryReadMpvRuntimeProvenance(bundledRuntimePath, bundledProvenance, provenanceError)) {
                std::wstring writeError;
                if (!velo::playback::WriteMpvRuntimeProvenance(runtimePath, bundledProvenance, writeError)) {
                    logger.Warn("Failed to copy bundled libmpv provenance manifest into managed runtime folder: " + Narrow(writeError));
                }
            }

            logger.Info("Seeded managed libmpv runtime from approved bundled copy: " + Narrow(bundledRuntimePath.wstring()));
            return {
                .ready = true,
                .downloaded = false,
                .runtimePath = runtimePath.wstring(),
                .detail = L"Copied approved bundled libmpv runtime into the managed NativPlayer runtime folder.",
                .effectiveLanguageCode = initialLanguage,
            };
        }
    }

    std::wstring resolvedLanguage = initialLanguage;
    const auto licenseChoice = PromptForMpvRuntimeDownloadConsent(resolvedLanguage);
    if (licenseChoice == MpvRuntimeLicenseChoice::RuntimeReady) {
        logger.Info("User completed manual libmpv setup and restarted the runtime check successfully.");
        return {
            .ready = true,
            .downloaded = false,
            .runtimePath = runtimePath.wstring(),
            .detail = L"Using runtime after successful manual setup.",
            .effectiveLanguageCode = resolvedLanguage,
        };
    }
    if (licenseChoice == MpvRuntimeLicenseChoice::Cancel) {
        logger.Warn("User cancelled libmpv license prompt.");
        auto failure = BuildFailure(BuildCancelledDownloadFailureText(resolvedLanguage));
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }

    logger.Warn("Managed libmpv runtime missing. Starting automatic download after user consent.");
    const velo::common::HttpRequestPolicy releasePolicy{
        .requireHttps = true,
        .maxBytes = 256 * 1024,
        .allowedHosts = {L"api.github.com"},
    };
    const auto releaseResponse =
        velo::common::HttpGetText(L"https://api.github.com/repos/zhongfly/mpv-winbuild/releases/latest", networkTimeoutMs, releasePolicy);
    if (!releaseResponse.errorMessage.empty()) {
        logger.Error("Failed to fetch libmpv release metadata: " + Narrow(releaseResponse.errorMessage));
        auto failure = BuildFailure(L"Unable to fetch the latest libmpv release metadata. " + releaseResponse.errorMessage);
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }
    if (releaseResponse.statusCode < 200 || releaseResponse.statusCode >= 300) {
        auto failure = BuildFailure(L"GitHub release metadata request failed with HTTP status " + std::to_wstring(releaseResponse.statusCode) + L".");
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }

    MpvRuntimeReleaseAsset asset;
    if (!TrySelectApprovedMpvRuntimeReleaseAsset(releaseResponse.body, asset)) {
        auto failure = BuildFailure(L"Latest mpv release metadata did not include an `mpv-dev-lgpl-x86_64-*.7z` asset.");
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }

    const auto tempRoot = runtimePath.parent_path() / "download-temp";
    const auto archivePath = tempRoot / asset.name;
    const auto extractPath = tempRoot / "extract";
    std::error_code cleanupError;
    std::filesystem::remove_all(tempRoot, cleanupError);
    std::filesystem::create_directories(extractPath, cleanupError);

    std::wstring downloadError;
    const velo::common::HttpRequestPolicy downloadPolicy{
        .requireHttps = true,
        .maxBytes = 256 * 1024 * 1024,
        .allowedHosts = {L"github.com", L"objects.githubusercontent.com", L"release-assets.githubusercontent.com"},
    };
    if (!velo::common::HttpDownloadToFile(asset.downloadUrl, archivePath, networkTimeoutMs, downloadError, downloadPolicy)) {
        logger.Error("Failed to download libmpv archive: " + Narrow(downloadError));
        auto failure = BuildFailure(L"Unable to download libmpv runtime. " + downloadError);
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }

    std::wstring hashError;
    std::wstring actualDigest;
    if (!velo::common::TryComputeFileSha256Hex(archivePath, actualDigest, hashError)) {
        logger.Error("Failed to hash downloaded libmpv archive: " + Narrow(hashError));
        auto failure = BuildFailure(L"Downloaded libmpv archive could not be verified. " + hashError);
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }
    if (!asset.sha256Hex.empty() && _wcsicmp(actualDigest.c_str(), asset.sha256Hex.c_str()) != 0) {
        logger.Error("Downloaded libmpv archive SHA-256 mismatch.");
        auto failure = BuildFailure(L"Downloaded libmpv archive failed SHA-256 verification.");
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }

    if (!RunTarExtract(archivePath, extractPath)) {
        logger.Error("Failed to extract libmpv archive with tar.exe.");
        auto failure = BuildFailure(L"Downloaded libmpv archive could not be extracted.");
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }

    const auto extractedDllPath = extractPath / "libmpv-2.dll";
    if (!IsCompatibleRuntime(extractedDllPath)) {
        logger.Error("Extracted libmpv runtime is missing or has an incompatible architecture.");
        auto failure = BuildFailure(L"The downloaded libmpv package did not contain a compatible x64 `libmpv-2.dll`.");
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }
    if (!CopyRuntimeFile(extractedDllPath, runtimePath) || !IsCompatibleRuntime(runtimePath)) {
        logger.Error("Failed to place extracted libmpv runtime into the managed runtime folder.");
        auto failure = BuildFailure(L"NativPlayer downloaded libmpv successfully, but could not install it into the managed runtime folder.");
        failure.effectiveLanguageCode = resolvedLanguage;
        return failure;
    }
    PersistRuntimeProvenance(runtimePath, asset, logger);

    std::filesystem::remove_all(tempRoot, cleanupError);
    logger.Info("Downloaded managed libmpv runtime from GitHub release asset " + Narrow(asset.name));
    return {
        .ready = true,
        .downloaded = true,
        .runtimePath = runtimePath.wstring(),
        .detail = L"Downloaded `libmpv-2.dll` into the managed NativPlayer runtime folder.",
        .effectiveLanguageCode = resolvedLanguage,
    };
}

}  // namespace velo::app
