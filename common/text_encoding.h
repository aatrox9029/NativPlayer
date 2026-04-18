#pragma once

#include <Windows.h>

#include <string>

namespace velo::common {

inline std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }

    std::string result(static_cast<size_t>(size), '\0');
    const int converted = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    if (converted <= 1) {
        return {};
    }

    result.resize(static_cast<size_t>(converted - 1));
    return result;
}

inline std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 1) {
        return {};
    }

    std::wstring result(static_cast<size_t>(size), L'\0');
    const int converted = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    if (converted <= 1) {
        return {};
    }

    result.resize(static_cast<size_t>(converted - 1));
    return result;
}

}  // namespace velo::common