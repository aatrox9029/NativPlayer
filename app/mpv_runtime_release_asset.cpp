#include "app/mpv_runtime_release_asset.h"

#include <regex>
#include <vector>

#include "common/text_encoding.h"

namespace velo::app {
namespace {

std::wstring Wide(const std::string& value) {
    return velo::common::Utf8ToWide(value);
}

std::wstring ExtractReleaseTag(const std::string& json) {
    const std::regex tagPattern("\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (!std::regex_search(json, match, tagPattern) || match.size() < 2) {
        return {};
    }
    return Wide(match[1].str());
}

std::vector<MpvRuntimeReleaseAsset> ExtractMatchingAssets(const std::string& json) {
    std::vector<MpvRuntimeReleaseAsset> assets;
    const std::wstring releaseTag = ExtractReleaseTag(json);
    const std::regex assetPattern(
        "\"name\"\\s*:\\s*\"(mpv-dev-lgpl-x86_64-[^\"]+\\.7z)\"[\\s\\S]*?\"digest\"\\s*:\\s*\"sha256:([0-9a-fA-F]+)\"[\\s\\S]*?\"browser_download_url\"\\s*:\\s*\"([^\"]+)\"");
    for (auto it = std::sregex_iterator(json.begin(), json.end(), assetPattern); it != std::sregex_iterator(); ++it) {
        assets.push_back({
            .releaseTag = releaseTag,
            .name = Wide((*it)[1].str()),
            .downloadUrl = Wide((*it)[3].str()),
            .sha256Hex = Wide((*it)[2].str()),
        });
    }
    return assets;
}

}  // namespace

bool TrySelectApprovedMpvRuntimeReleaseAsset(const std::string& releaseJson, MpvRuntimeReleaseAsset& selectedAsset) {
    auto assets = ExtractMatchingAssets(releaseJson);
    if (assets.empty()) {
        return false;
    }
    selectedAsset = std::move(assets.front());
    return true;
}

}  // namespace velo::app
