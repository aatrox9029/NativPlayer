#include "common/http_download.h"

#include <Windows.h>
#include <winhttp.h>

#include <array>
#include <fstream>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace velo::common {
namespace {

struct ParsedUrl {
    INTERNET_SCHEME scheme = INTERNET_SCHEME_HTTPS;
    std::wstring host;
    std::wstring pathAndQuery;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
};

struct WinHttpHandleCloser {
    void operator()(HINTERNET handle) const noexcept {
        if (handle != nullptr) {
            WinHttpCloseHandle(handle);
        }
    }
};

using WinHttpHandle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, WinHttpHandleCloser>;

struct PreparedRequest {
    WinHttpHandle session;
    WinHttpHandle connection;
    WinHttpHandle request;
};

std::wstring FormatWin32Message(const DWORD errorCode) {
    LPWSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageW(flags, nullptr, errorCode, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (length == 0 || buffer == nullptr) {
        return L"Win32 error " + std::to_wstring(errorCode);
    }

    std::wstring message(buffer, length);
    LocalFree(buffer);
    while (!message.empty() &&
           (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' ' || message.back() == L'.')) {
        message.pop_back();
    }
    return message;
}

bool EqualsInsensitive(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()), right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

bool IsHostAllowed(const std::wstring& host, const HttpRequestPolicy& policy) {
    if (policy.allowedHosts.empty()) {
        return true;
    }
    for (const auto& allowedHost : policy.allowedHosts) {
        if (EqualsInsensitive(host, allowedHost)) {
            return true;
        }
    }
    return false;
}

std::optional<ParsedUrl> TryParseUrl(const std::wstring& url, std::wstring& errorMessage) {
    URL_COMPONENTSW components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    std::wstring mutableUrl = url;
    if (!WinHttpCrackUrl(mutableUrl.data(), static_cast<DWORD>(mutableUrl.size()), 0, &components)) {
        errorMessage = L"Failed to parse URL: " + url + L" (" + FormatWin32Message(GetLastError()) + L")";
        return std::nullopt;
    }

    ParsedUrl parsed;
    parsed.scheme = components.nScheme;
    parsed.host.assign(components.lpszHostName, components.dwHostNameLength);
    parsed.pathAndQuery.assign(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0 && components.lpszExtraInfo != nullptr) {
        parsed.pathAndQuery.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }
    parsed.port = components.nPort;
    if (parsed.host.empty() || parsed.pathAndQuery.empty()) {
        errorMessage = L"URL is missing host or path: " + url;
        return std::nullopt;
    }
    return parsed;
}

bool ValidateUrlAgainstPolicy(const ParsedUrl& parsed, const HttpRequestPolicy& policy, const std::wstring& url, std::wstring& errorMessage) {
    if (policy.requireHttps && parsed.scheme != INTERNET_SCHEME_HTTPS) {
        errorMessage = L"Refused non-HTTPS URL: " + url;
        return false;
    }
    if (!IsHostAllowed(parsed.host, policy)) {
        errorMessage = L"Refused URL with unapproved host '" + parsed.host + L"': " + url;
        return false;
    }
    return true;
}

bool ConfigureTimeouts(const HINTERNET handle, const int timeoutMs, std::wstring& errorMessage) {
    const int effectiveTimeoutMs = timeoutMs > 0 ? timeoutMs : 15000;
    const std::array<DWORD, 4> values = {
        static_cast<DWORD>(effectiveTimeoutMs),
        static_cast<DWORD>(effectiveTimeoutMs),
        static_cast<DWORD>(effectiveTimeoutMs),
        static_cast<DWORD>(effectiveTimeoutMs),
    };
    const DWORD options[] = {
        WINHTTP_OPTION_RESOLVE_TIMEOUT,
        WINHTTP_OPTION_CONNECT_TIMEOUT,
        WINHTTP_OPTION_SEND_TIMEOUT,
        WINHTTP_OPTION_RECEIVE_TIMEOUT,
    };
    for (size_t index = 0; index < values.size(); ++index) {
        DWORD value = values[index];
        if (!WinHttpSetOption(handle, options[index], &value, sizeof(value))) {
            errorMessage = L"Failed to configure WinHTTP timeouts (" + FormatWin32Message(GetLastError()) + L")";
            return false;
        }
    }

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    if (!WinHttpSetOption(handle, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy))) {
        errorMessage = L"Failed to enable HTTP redirects (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }
    return true;
}

std::wstring QueryFinalUrl(const HINTERNET request, std::wstring& errorMessage) {
    DWORD sizeBytes = 0;
    if (!WinHttpQueryOption(request, WINHTTP_OPTION_URL, nullptr, &sizeBytes) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        errorMessage = L"Failed to query final response URL (" + FormatWin32Message(GetLastError()) + L")";
        return {};
    }

    std::wstring finalUrl(sizeBytes / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryOption(request, WINHTTP_OPTION_URL, finalUrl.data(), &sizeBytes)) {
        errorMessage = L"Failed to query final response URL (" + FormatWin32Message(GetLastError()) + L")";
        return {};
    }

    if (!finalUrl.empty() && finalUrl.back() == L'\0') {
        finalUrl.pop_back();
    }
    return finalUrl;
}

bool ReceiveResponse(const HINTERNET request,
                     const HttpRequestPolicy& policy,
                     int& statusCode,
                     std::wstring& errorMessage) {
    if (!WinHttpReceiveResponse(request, nullptr)) {
        errorMessage = L"Failed to receive HTTP response (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }

    const std::wstring finalUrl = QueryFinalUrl(request, errorMessage);
    if (!errorMessage.empty()) {
        return false;
    }
    const auto parsedFinalUrl = TryParseUrl(finalUrl, errorMessage);
    if (!parsedFinalUrl.has_value()) {
        return false;
    }
    if (!ValidateUrlAgainstPolicy(*parsedFinalUrl, policy, finalUrl, errorMessage)) {
        return false;
    }

    DWORD rawStatusCode = 0;
    DWORD rawStatusSize = sizeof(rawStatusCode);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                             &rawStatusCode, &rawStatusSize, WINHTTP_NO_HEADER_INDEX)) {
        errorMessage = L"Failed to query HTTP status code (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }

    statusCode = static_cast<int>(rawStatusCode);
    return true;
}

bool ReceiveResponseBytes(const HINTERNET request, const HttpRequestPolicy& policy, std::vector<char>& buffer, std::wstring& errorMessage) {
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            errorMessage = L"Failed while reading HTTP response (" + FormatWin32Message(GetLastError()) + L")";
            return false;
        }
        if (available == 0) {
            return true;
        }

        if (policy.maxBytes > 0 && buffer.size() + available > policy.maxBytes) {
            errorMessage = L"HTTP response exceeded the allowed size limit.";
            return false;
        }

        const size_t offset = buffer.size();
        buffer.resize(offset + available);
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data() + offset, available, &read)) {
            errorMessage = L"Failed while downloading HTTP payload (" + FormatWin32Message(GetLastError()) + L")";
            return false;
        }
        buffer.resize(offset + read);
    }
}

bool ReceiveResponseToFile(const HINTERNET request,
                           const HttpRequestPolicy& policy,
                           const std::filesystem::path& destinationPath,
                           std::wstring& errorMessage) {
    std::error_code createError;
    std::filesystem::create_directories(destinationPath.parent_path(), createError);

    std::ofstream output(destinationPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        errorMessage = L"Failed to open download destination: " + destinationPath.wstring();
        return false;
    }

    size_t totalBytes = 0;
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            errorMessage = L"Failed while reading HTTP response (" + FormatWin32Message(GetLastError()) + L")";
            return false;
        }
        if (available == 0) {
            return output.good();
        }

        totalBytes += available;
        if (policy.maxBytes > 0 && totalBytes > policy.maxBytes) {
            errorMessage = L"HTTP download exceeded the allowed size limit.";
            return false;
        }

        std::vector<char> chunk(available);
        DWORD read = 0;
        if (!WinHttpReadData(request, chunk.data(), available, &read)) {
            errorMessage = L"Failed while downloading HTTP payload (" + FormatWin32Message(GetLastError()) + L")";
            return false;
        }
        if (read == 0) {
            continue;
        }
        output.write(chunk.data(), static_cast<std::streamsize>(read));
        if (!output.good()) {
            errorMessage = L"Failed to write download destination: " + destinationPath.wstring();
            return false;
        }
    }
}

bool ExecuteRequest(const std::wstring& url,
                    const int timeoutMs,
                    const wchar_t* acceptTypes[],
                    const HttpRequestPolicy& policy,
                    PreparedRequest& preparedRequest,
                    int& statusCode,
                    std::wstring& errorMessage) {
    const auto parsed = TryParseUrl(url, errorMessage);
    if (!parsed.has_value()) {
        return false;
    }
    if (!ValidateUrlAgainstPolicy(*parsed, policy, url, errorMessage)) {
        return false;
    }

    preparedRequest.session.reset(
        WinHttpOpen(L"NativPlayer/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!preparedRequest.session) {
        errorMessage = L"Failed to initialize WinHTTP session (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }
    if (!ConfigureTimeouts(preparedRequest.session.get(), timeoutMs, errorMessage)) {
        return false;
    }

    preparedRequest.connection.reset(WinHttpConnect(preparedRequest.session.get(), parsed->host.c_str(), parsed->port, 0));
    if (!preparedRequest.connection) {
        errorMessage = L"Failed to connect to host '" + parsed->host + L"' (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }

    const DWORD requestFlags = parsed->scheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    preparedRequest.request.reset(WinHttpOpenRequest(preparedRequest.connection.get(), L"GET", parsed->pathAndQuery.c_str(), nullptr,
                                                     WINHTTP_NO_REFERER, acceptTypes, requestFlags));
    if (!preparedRequest.request) {
        errorMessage = L"Failed to create HTTP request (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }

    const std::wstring headers = L"Accept: */*\r\nUser-Agent: NativPlayer/1.0\r\n";
    if (!WinHttpSendRequest(preparedRequest.request.get(), headers.c_str(), static_cast<DWORD>(headers.size()), WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        errorMessage = L"Failed to send HTTP request (" + FormatWin32Message(GetLastError()) + L")";
        return false;
    }

    return ReceiveResponse(preparedRequest.request.get(), policy, statusCode, errorMessage);
}

}  // namespace

HttpResponse HttpGetText(const std::wstring& url, const int timeoutMs, const HttpRequestPolicy& policy) {
    HttpResponse response;
    PreparedRequest preparedRequest;
    const wchar_t* acceptTypes[] = {L"application/json", L"text/plain", L"*/*", nullptr};
    if (!ExecuteRequest(url, timeoutMs, acceptTypes, policy, preparedRequest, response.statusCode, response.errorMessage)) {
        return response;
    }

    std::vector<char> payload;
    if (!ReceiveResponseBytes(preparedRequest.request.get(), policy, payload, response.errorMessage)) {
        response.statusCode = 0;
        return response;
    }
    response.body.assign(payload.begin(), payload.end());
    return response;
}

bool HttpDownloadToFile(const std::wstring& url,
                        const std::filesystem::path& destinationPath,
                        const int timeoutMs,
                        std::wstring& errorMessage,
                        const HttpRequestPolicy& policy) {
    PreparedRequest preparedRequest;
    int statusCode = 0;
    const wchar_t* acceptTypes[] = {L"application/octet-stream", L"*/*", nullptr};
    if (!ExecuteRequest(url, timeoutMs, acceptTypes, policy, preparedRequest, statusCode, errorMessage)) {
        return false;
    }
    if (statusCode < 200 || statusCode >= 300) {
        errorMessage = L"Unexpected HTTP status " + std::to_wstring(statusCode) + L" while downloading " + url;
        return false;
    }

    if (!ReceiveResponseToFile(preparedRequest.request.get(), policy, destinationPath, errorMessage)) {
        std::error_code cleanupError;
        std::filesystem::remove(destinationPath, cleanupError);
        return false;
    }
    return true;
}

}  // namespace velo::common
