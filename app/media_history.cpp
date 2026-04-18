#include "app/media_history.h"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace velo::app {
namespace {

constexpr size_t kMaxRecentItems = 20;
constexpr size_t kMaxHistoryItems = 120;
constexpr size_t kMaxBookmarkItems = 80;

}  // namespace

uint64_t UnixTimestampNow() {
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
}

bool MediaPathEqualsInsensitive(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()), right.c_str(), static_cast<int>(right.size()), TRUE) ==
           CSTR_EQUAL;
}

void RememberRecentItem(std::vector<velo::config::RecentItem>& items, const std::wstring& path, const uint64_t openedAtUnixSeconds) {
    if (path.empty()) {
        return;
    }

    auto existing = std::find_if(items.begin(), items.end(), [&](const velo::config::RecentItem& item) {
        return MediaPathEqualsInsensitive(item.path, path);
    });

    const bool pinned = existing != items.end() ? existing->pinned : false;
    if (existing != items.end()) {
        items.erase(existing);
    }
    items.insert(items.begin(), {.path = path, .pinned = pinned, .openedAtUnixSeconds = openedAtUnixSeconds});

    std::stable_sort(items.begin(), items.end(), [](const velo::config::RecentItem& left, const velo::config::RecentItem& right) {
        if (left.pinned != right.pinned) {
            return left.pinned && !right.pinned;
        }
        return left.openedAtUnixSeconds > right.openedAtUnixSeconds;
    });
    if (items.size() > kMaxRecentItems) {
        items.resize(kMaxRecentItems);
    }
}

void RemoveRecentItem(std::vector<velo::config::RecentItem>& items, const std::wstring& path) {
    items.erase(std::remove_if(items.begin(), items.end(), [&](const velo::config::RecentItem& item) {
        return MediaPathEqualsInsensitive(item.path, path);
    }), items.end());
}

void TogglePinnedRecent(std::vector<velo::config::RecentItem>& items, const std::wstring& path) {
    const auto existing = std::find_if(items.begin(), items.end(), [&](const velo::config::RecentItem& item) {
        return MediaPathEqualsInsensitive(item.path, path);
    });
    if (existing == items.end()) {
        return;
    }
    existing->pinned = !existing->pinned;
    std::stable_sort(items.begin(), items.end(), [](const velo::config::RecentItem& left, const velo::config::RecentItem& right) {
        if (left.pinned != right.pinned) {
            return left.pinned && !right.pinned;
        }
        return left.openedAtUnixSeconds > right.openedAtUnixSeconds;
    });
}

void ClearUnpinnedRecentItems(std::vector<velo::config::RecentItem>& items) {
    items.erase(std::remove_if(items.begin(), items.end(), [](const velo::config::RecentItem& item) {
        return !item.pinned;
    }), items.end());
}

void RememberHistoryEntry(std::vector<velo::config::HistoryEntry>& items, const std::wstring& path, const std::wstring& title,
                          const double positionSeconds, const bool isUrl, const uint64_t openedAtUnixSeconds) {
    if (path.empty()) {
        return;
    }

    items.erase(std::remove_if(items.begin(), items.end(), [&](const velo::config::HistoryEntry& item) {
        return MediaPathEqualsInsensitive(item.path, path);
    }), items.end());

    items.insert(items.begin(), {.path = path,
                                 .title = title,
                                 .positionSeconds = std::max(0.0, positionSeconds),
                                 .isUrl = isUrl,
                                 .openedAtUnixSeconds = openedAtUnixSeconds});
    if (items.size() > kMaxHistoryItems) {
        items.resize(kMaxHistoryItems);
    }
}

void RememberBookmark(std::vector<velo::config::BookmarkEntry>& items, const std::wstring& path, const double positionSeconds,
                      std::wstring label) {
    if (path.empty()) {
        return;
    }

    if (label.empty()) {
        label = L"Bookmark @" + std::to_wstring(static_cast<int>(positionSeconds));
    }
    items.erase(std::remove_if(items.begin(), items.end(), [&](const velo::config::BookmarkEntry& item) {
        return MediaPathEqualsInsensitive(item.path, path) && std::abs(item.positionSeconds - positionSeconds) < 0.5;
    }), items.end());

    items.insert(items.begin(), {.path = path, .label = std::move(label), .positionSeconds = std::max(0.0, positionSeconds)});
    if (items.size() > kMaxBookmarkItems) {
        items.resize(kMaxBookmarkItems);
    }
}

std::vector<std::wstring> BuildRecentPaths(const std::vector<velo::config::RecentItem>& items) {
    std::vector<std::wstring> paths;
    for (const auto& item : items) {
        if (!item.path.empty()) {
            paths.push_back(item.path);
        }
    }
    return paths;
}

}  // namespace velo::app
