#pragma once

#include "app/application.h"
#include "app/end_of_playback_policy.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <shellapi.h>

#include "common/text_encoding.h"
#include "diagnostics/crash_dump.h"
#include "localization/localization.h"

namespace velo::app {
namespace {

inline bool SamePathInsensitive(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()), right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

inline std::string Narrow(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

inline std::wstring Utf8ToWide(const std::string& value) {
    return velo::common::Utf8ToWide(value);
}

inline bool PlayerStateMatchesCurrentPath(const velo::ui::PlayerState& state, const std::wstring& currentOpenPath) {
    if (currentOpenPath.empty() || state.currentPath.empty()) {
        return false;
    }
    return SamePathInsensitive(Utf8ToWide(state.currentPath), currentOpenPath);
}

}  // namespace
}  // namespace velo::app
