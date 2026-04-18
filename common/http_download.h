#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace velo::common {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::wstring errorMessage;
};

struct HttpRequestPolicy {
    bool requireHttps = true;
    size_t maxBytes = 0;
    std::vector<std::wstring> allowedHosts;
};

HttpResponse HttpGetText(const std::wstring& url, int timeoutMs, const HttpRequestPolicy& policy = {});
bool HttpDownloadToFile(const std::wstring& url,
                        const std::filesystem::path& destinationPath,
                        int timeoutMs,
                        std::wstring& errorMessage,
                        const HttpRequestPolicy& policy = {});

}  // namespace velo::common
