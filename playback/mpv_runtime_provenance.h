#pragma once

#include <filesystem>
#include <string>

namespace velo::playback {

struct MpvRuntimeProvenance {
    std::wstring sourceRepo;
    std::wstring releaseTag;
    std::wstring assetName;
    std::wstring assetSha256Hex;
    std::wstring dllSha256Hex;
    std::wstring downloadUrl;
    std::wstring licenseId;
    std::wstring commercialUseNote;
};

std::filesystem::path MpvRuntimeProvenancePath(const std::filesystem::path& dllPath);
bool TryReadMpvRuntimeProvenance(const std::filesystem::path& dllPath, MpvRuntimeProvenance& provenance, std::wstring& errorMessage);
bool WriteMpvRuntimeProvenance(const std::filesystem::path& dllPath, const MpvRuntimeProvenance& provenance, std::wstring& errorMessage);
bool IsApprovedBundledMpvRuntime(const std::filesystem::path& dllPath, std::wstring& errorMessage);

}  // namespace velo::playback
