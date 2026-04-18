#pragma once

#include "config/config_service.h"

#include <Windows.h>
#include <ShlObj.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>

#include "common/hex_color.h"
#include "config/config_ini_keys.h"
#include "localization/localization.h"

namespace velo::config {
namespace {

constexpr wchar_t kCompanyFolder[] = L"NativPlayer";
constexpr wchar_t kLegacyCompanyFolder[] = L"VeloPlayer";
constexpr size_t kMaxRecentFiles = 10;
constexpr size_t kMaxRecentItems = 20;
constexpr size_t kMaxResumeEntries = 20;
constexpr size_t kMaxHistoryEntries = 120;
constexpr size_t kMaxBookmarkEntries = 80;
constexpr wchar_t kConfigFileName[] = L"settings.ini";
constexpr wchar_t kBackupFileName[] = L"settings.backup.ini";
constexpr int kCurrentConfigVersion = 3;

inline std::wstring Trim(const std::wstring& value) {
    const auto begin = value.find_first_not_of(L" \t\r\n");
    if (begin == std::wstring::npos) {
        return L"";
    }
    const auto end = value.find_last_not_of(L" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

inline std::wstring BoolToString(const bool value) {
    return value ? L"true" : L"false";
}

inline bool ParseBool(const std::wstring& value) {
    return value == L"1" || value == L"true" || value == L"yes" || value == L"on";
}

inline bool SameTextInsensitive(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()), right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

inline std::optional<int> TryParseInt(const std::wstring& value) {
    errno = 0;
    wchar_t* end = nullptr;
    const long parsed = wcstol(value.c_str(), &end, 10);
    if (end == value.c_str() || (end != nullptr && *end != L'\0') || errno == ERANGE ||
        parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }
    return static_cast<int>(parsed);
}

inline std::optional<uint64_t> TryParseUint64(const std::wstring& value) {
    if (value.empty() || value.starts_with(L"-")) {
        return std::nullopt;
    }
    errno = 0;
    wchar_t* end = nullptr;
    const unsigned long long parsed = wcstoull(value.c_str(), &end, 10);
    if (end == value.c_str() || (end != nullptr && *end != L'\0') || errno == ERANGE ||
        parsed > std::numeric_limits<uint64_t>::max()) {
        return std::nullopt;
    }
    return static_cast<uint64_t>(parsed);
}

inline std::optional<unsigned int> TryParseUnsignedInt(const std::wstring& value) {
    const auto parsed = TryParseUint64(value);
    if (!parsed.has_value() || *parsed > std::numeric_limits<unsigned int>::max()) {
        return std::nullopt;
    }
    return static_cast<unsigned int>(*parsed);
}

inline std::optional<double> TryParseDouble(const std::wstring& value) {
    wchar_t* end = nullptr;
    const double parsed = wcstod(value.c_str(), &end);
    if (end == value.c_str() || (end != nullptr && *end != L'\0')) {
        return std::nullopt;
    }
    return parsed;
}

inline std::wstring RepeatModeToString(const RepeatMode repeatMode) {
    switch (repeatMode) {
        case RepeatMode::One:
            return L"one";
        case RepeatMode::All:
            return L"all";
        default:
            return L"off";
    }
}

inline RepeatMode ParseRepeatMode(const std::wstring& value) {
    if (SameTextInsensitive(value, L"one")) {
        return RepeatMode::One;
    }
    if (SameTextInsensitive(value, L"all")) {
        return RepeatMode::All;
    }
    return RepeatMode::Off;
}

inline std::wstring EndOfPlaybackActionToString(const EndOfPlaybackAction action) {
    switch (action) {
        case EndOfPlaybackAction::Replay:
            return L"replay";
        case EndOfPlaybackAction::Stop:
            return L"stop";
        case EndOfPlaybackAction::CloseWindow:
            return L"close_window";
        case EndOfPlaybackAction::PlayNext:
        default:
            return L"play_next";
    }
}

inline EndOfPlaybackAction ParseEndOfPlaybackAction(const std::wstring& value) {
    if (SameTextInsensitive(value, L"replay")) {
        return EndOfPlaybackAction::Replay;
    }
    if (SameTextInsensitive(value, L"stop")) {
        return EndOfPlaybackAction::Stop;
    }
    if (SameTextInsensitive(value, L"close_window")) {
        return EndOfPlaybackAction::CloseWindow;
    }
    return EndOfPlaybackAction::PlayNext;
}

inline std::wstring SeekStepModeToString(const SeekStepMode mode) {
    switch (mode) {
        case SeekStepMode::Percent:
            return L"percent";
        case SeekStepMode::Seconds:
        default:
            return L"seconds";
    }
}

inline SeekStepMode ParseSeekStepMode(const std::wstring& value) {
    if (SameTextInsensitive(value, L"percent") || value == L"%") {
        return SeekStepMode::Percent;
    }
    return SeekStepMode::Seconds;
}

inline void SetKeyBinding(std::vector<KeyBindingEntry>& bindings, const std::wstring& actionId, const unsigned int virtualKey) {
    if (actionId.empty()) {
        return;
    }
    auto existing = std::find_if(bindings.begin(), bindings.end(), [&](const KeyBindingEntry& entry) {
        return SameTextInsensitive(entry.actionId, actionId);
    });
    if (existing == bindings.end()) {
        bindings.push_back({actionId, virtualKey});
        return;
    }
    existing->virtualKey = virtualKey;
}

}  // namespace

void NormalizeConfig(AppConfig& config);
bool ParseConfigFile(const std::filesystem::path& filePath, AppConfig& loadedConfig);
bool WriteConfigFileToPath(const AppConfig& config, const std::filesystem::path& destinationPath);
std::filesystem::path CorruptConfigCopyPath(const std::filesystem::path& sourcePath);

}  // namespace velo::config
