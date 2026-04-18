#pragma once

#include <string>

namespace velo::app {

struct MpvRuntimeReleaseAsset {
    std::wstring releaseTag;
    std::wstring name;
    std::wstring downloadUrl;
    std::wstring sha256Hex;
};

bool TrySelectApprovedMpvRuntimeReleaseAsset(const std::string& releaseJson, MpvRuntimeReleaseAsset& selectedAsset);

}  // namespace velo::app
