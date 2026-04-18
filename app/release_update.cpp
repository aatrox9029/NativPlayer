#include "app/release_update.h"

#include <regex>
#include <tuple>

#include "app/app_metadata.h"
#include "common/http_download.h"
#include "common/text_encoding.h"

namespace velo::app {
namespace {

constexpr wchar_t kLatestReleaseApiUrl[] = L"https://api.github.com/repos/aatrox9029/NativPlayer/releases/latest";
constexpr wchar_t kApprovedReleaseHtmlPrefix[] = L"https://github.com/aatrox9029/NativPlayer/releases/";
constexpr wchar_t kApprovedInstallerPrefix[] = L"https://github.com/aatrox9029/NativPlayer/releases/download/";
constexpr wchar_t kPreferredInstallerName[] = L"NativPlayer-Setup.exe";

std::wstring Wide(const std::string& value) {
    return velo::common::Utf8ToWide(value);
}

bool StartsWith(const std::wstring& value, std::wstring_view prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix.data(), prefix.size()) == 0;
}

std::optional<std::tuple<int, int, int>> ParseReleaseTag(const std::wstring_view tag) {
    std::wstring normalized(tag);
    if (!normalized.empty() && (normalized.front() == L'v' || normalized.front() == L'V')) {
        normalized.erase(normalized.begin());
    }

    const std::wregex pattern(LR"(^(\d+)\.(\d+)\.(\d+)(?:[-+].*)?$)");
    std::wsmatch match;
    if (!std::regex_match(normalized, match, pattern) || match.size() < 4) {
        return std::nullopt;
    }

    return std::tuple<int, int, int>{
        std::stoi(match[1].str()),
        std::stoi(match[2].str()),
        std::stoi(match[3].str()),
    };
}

std::wstring ExtractSingleField(const std::string& json, const char* patternText) {
    const std::regex pattern(patternText);
    std::smatch match;
    if (!std::regex_search(json, match, pattern) || match.size() < 2) {
        return {};
    }
    return Wide(match[1].str());
}

}  // namespace

bool TryParseLatestReleaseUpdate(const std::string& releaseJson, ReleaseUpdateInfo& updateInfo) {
    updateInfo = {};
    updateInfo.tagName = ExtractSingleField(releaseJson, "\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
    updateInfo.htmlUrl = ExtractSingleField(releaseJson, "\"html_url\"\\s*:\\s*\"([^\"]+)\"");

    const std::regex assetPattern(
        "\"name\"\\s*:\\s*\"([^\"]+)\"[\\s\\S]*?\"browser_download_url\"\\s*:\\s*\"([^\"]+)\"");
    for (auto it = std::sregex_iterator(releaseJson.begin(), releaseJson.end(), assetPattern); it != std::sregex_iterator(); ++it) {
        const std::wstring assetName = Wide((*it)[1].str());
        const std::wstring assetUrl = Wide((*it)[2].str());
        if (_wcsicmp(assetName.c_str(), kPreferredInstallerName) == 0) {
            updateInfo.installerDownloadUrl = assetUrl;
            break;
        }
    }

    if (updateInfo.tagName.empty() || updateInfo.htmlUrl.empty()) {
        return false;
    }
    if (!StartsWith(updateInfo.htmlUrl, kApprovedReleaseHtmlPrefix)) {
        return false;
    }
    if (!updateInfo.installerDownloadUrl.empty() && !StartsWith(updateInfo.installerDownloadUrl, kApprovedInstallerPrefix)) {
        return false;
    }
    return true;
}

bool IsNewerReleaseTag(const std::wstring_view latestTag, const std::wstring_view currentTag) {
    const auto latest = ParseReleaseTag(latestTag);
    const auto current = ParseReleaseTag(currentTag);
    if (!latest.has_value() || !current.has_value()) {
        return latestTag != currentTag;
    }
    return *latest > *current;
}

std::optional<AvailableReleaseUpdate> QueryAvailableReleaseUpdate(const int timeoutMs, std::wstring& errorMessage) {
    errorMessage.clear();

    const velo::common::HttpRequestPolicy policy{
        .requireHttps = true,
        .maxBytes = 256 * 1024,
        .allowedHosts = {L"api.github.com"},
    };
    const auto response = velo::common::HttpGetText(kLatestReleaseApiUrl, timeoutMs, policy);
    if (!response.errorMessage.empty()) {
        errorMessage = response.errorMessage;
        return std::nullopt;
    }
    if (response.statusCode < 200 || response.statusCode >= 300) {
        errorMessage = L"GitHub release API returned HTTP status " + std::to_wstring(response.statusCode) + L".";
        return std::nullopt;
    }

    ReleaseUpdateInfo updateInfo;
    if (!TryParseLatestReleaseUpdate(response.body, updateInfo)) {
        errorMessage = L"GitHub release metadata is missing a usable tag or download URL.";
        return std::nullopt;
    }
    if (!IsNewerReleaseTag(updateInfo.tagName, AppVersion())) {
        return std::nullopt;
    }

    return AvailableReleaseUpdate{
        .tagName = updateInfo.tagName,
        .downloadUrl = updateInfo.installerDownloadUrl.empty() ? updateInfo.htmlUrl : updateInfo.installerDownloadUrl,
    };
}

}  // namespace velo::app
