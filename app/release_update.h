#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace velo::app {

struct ReleaseUpdateInfo {
    std::wstring tagName;
    std::wstring htmlUrl;
    std::wstring installerDownloadUrl;
};

struct AvailableReleaseUpdate {
    std::wstring tagName;
    std::wstring downloadUrl;
};

bool TryParseLatestReleaseUpdate(const std::string& releaseJson, ReleaseUpdateInfo& updateInfo);
bool IsNewerReleaseTag(std::wstring_view latestTag, std::wstring_view currentTag);
std::optional<AvailableReleaseUpdate> QueryAvailableReleaseUpdate(int timeoutMs, std::wstring& errorMessage);

}  // namespace velo::app
