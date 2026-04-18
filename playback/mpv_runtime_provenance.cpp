#include "playback/mpv_runtime_provenance.h"

#include <cwctype>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "common/sha256.h"
#include "common/text_encoding.h"

namespace velo::playback {
namespace {

constexpr wchar_t kApprovedSourceRepo[] = L"zhongfly/mpv-winbuild";
constexpr wchar_t kApprovedAssetPrefix[] = L"mpv-dev-lgpl-x86_64-";
constexpr wchar_t kApprovedDownloadPrefix[] = L"https://github.com/zhongfly/mpv-winbuild/releases/download/";
constexpr wchar_t kManifestFileName[] = L"libmpv-2.provenance.txt";

std::wstring ToLower(std::wstring value) {
    for (auto& ch : value) {
        ch = static_cast<wchar_t>(towlower(ch));
    }
    return value;
}

std::string TrimAscii(std::string value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, (end - start) + 1);
}

bool StartsWith(const std::wstring& value, const std::wstring_view prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix.data(), prefix.size()) == 0;
}

bool ContainsInsensitive(const std::wstring& value, const std::wstring_view token) {
    const auto loweredValue = ToLower(value);
    std::wstring loweredToken(token);
    loweredToken = ToLower(std::move(loweredToken));
    return loweredValue.find(loweredToken) != std::wstring::npos;
}

std::wstring WideValue(const std::unordered_map<std::string, std::string>& values, const char* key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return {};
    }
    return velo::common::Utf8ToWide(it->second);
}

bool ValidateProvenance(const std::filesystem::path& dllPath, const MpvRuntimeProvenance& provenance, std::wstring& errorMessage) {
    if (provenance.sourceRepo != kApprovedSourceRepo) {
        errorMessage = L"Bundled libmpv source repo is not approved.";
        return false;
    }
    if (!StartsWith(provenance.assetName, kApprovedAssetPrefix)) {
        errorMessage = L"Bundled libmpv asset is not an approved LGPL x64 package.";
        return false;
    }
    if (provenance.releaseTag.empty()) {
        errorMessage = L"Bundled libmpv provenance is missing the release tag.";
        return false;
    }
    if (provenance.assetSha256Hex.empty()) {
        errorMessage = L"Bundled libmpv provenance is missing the archive SHA-256.";
        return false;
    }
    if (provenance.dllSha256Hex.empty()) {
        errorMessage = L"Bundled libmpv provenance is missing the DLL SHA-256.";
        return false;
    }
    if (!StartsWith(provenance.downloadUrl, kApprovedDownloadPrefix)) {
        errorMessage = L"Bundled libmpv provenance does not point at the approved GitHub release source.";
        return false;
    }
    if (!ContainsInsensitive(provenance.licenseId, L"lgpl")) {
        errorMessage = L"Bundled libmpv provenance does not describe an LGPL runtime.";
        return false;
    }

    std::wstring actualDllSha256Hex;
    std::wstring hashError;
    if (!velo::common::TryComputeFileSha256Hex(dllPath, actualDllSha256Hex, hashError)) {
        errorMessage = L"Failed to hash bundled libmpv DLL. " + hashError;
        return false;
    }
    if (_wcsicmp(actualDllSha256Hex.c_str(), provenance.dllSha256Hex.c_str()) != 0) {
        errorMessage = L"Bundled libmpv DLL hash does not match its approved provenance manifest.";
        return false;
    }

    errorMessage.clear();
    return true;
}

}  // namespace

std::filesystem::path MpvRuntimeProvenancePath(const std::filesystem::path& dllPath) {
    if (dllPath.empty()) {
        return {};
    }
    return dllPath.parent_path() / kManifestFileName;
}

bool TryReadMpvRuntimeProvenance(const std::filesystem::path& dllPath, MpvRuntimeProvenance& provenance, std::wstring& errorMessage) {
    provenance = {};
    errorMessage.clear();

    const auto manifestPath = MpvRuntimeProvenancePath(dllPath);
    std::ifstream input(manifestPath, std::ios::binary);
    if (!input.is_open()) {
        errorMessage = L"Bundled libmpv provenance manifest is missing: " + manifestPath.wstring();
        return false;
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const auto key = TrimAscii(line.substr(0, separator));
        const auto value = TrimAscii(line.substr(separator + 1));
        if (!key.empty()) {
            values[key] = value;
        }
    }

    provenance.sourceRepo = WideValue(values, "source_repo");
    provenance.releaseTag = WideValue(values, "release_tag");
    provenance.assetName = WideValue(values, "asset_name");
    provenance.assetSha256Hex = WideValue(values, "asset_sha256");
    provenance.dllSha256Hex = WideValue(values, "dll_sha256");
    provenance.downloadUrl = WideValue(values, "download_url");
    provenance.licenseId = WideValue(values, "license_id");
    provenance.commercialUseNote = WideValue(values, "commercial_use_note");
    return true;
}

bool WriteMpvRuntimeProvenance(const std::filesystem::path& dllPath, const MpvRuntimeProvenance& provenance, std::wstring& errorMessage) {
    errorMessage.clear();

    const auto manifestPath = MpvRuntimeProvenancePath(dllPath);
    std::error_code directoryError;
    std::filesystem::create_directories(manifestPath.parent_path(), directoryError);

    std::ofstream output(manifestPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        errorMessage = L"Failed to write libmpv provenance manifest: " + manifestPath.wstring();
        return false;
    }

    const auto writeValue = [&](const char* key, const std::wstring& value) {
        output << key << '=' << velo::common::WideToUtf8(value) << '\n';
    };

    writeValue("source_repo", provenance.sourceRepo);
    writeValue("release_tag", provenance.releaseTag);
    writeValue("asset_name", provenance.assetName);
    writeValue("asset_sha256", provenance.assetSha256Hex);
    writeValue("dll_sha256", provenance.dllSha256Hex);
    writeValue("download_url", provenance.downloadUrl);
    writeValue("license_id", provenance.licenseId);
    writeValue("commercial_use_note", provenance.commercialUseNote);
    output.flush();
    if (!output.good()) {
        errorMessage = L"Failed while flushing libmpv provenance manifest: " + manifestPath.wstring();
        return false;
    }

    return true;
}

bool IsApprovedBundledMpvRuntime(const std::filesystem::path& dllPath, std::wstring& errorMessage) {
    MpvRuntimeProvenance provenance;
    if (!TryReadMpvRuntimeProvenance(dllPath, provenance, errorMessage)) {
        return false;
    }
    return ValidateProvenance(dllPath, provenance, errorMessage);
}

}  // namespace velo::playback
