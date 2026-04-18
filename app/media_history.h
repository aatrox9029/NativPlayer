#pragma once

#include <string>
#include <vector>

#include "config/config_service.h"

namespace velo::app {

uint64_t UnixTimestampNow();
bool MediaPathEqualsInsensitive(const std::wstring& left, const std::wstring& right);
void RememberRecentItem(std::vector<velo::config::RecentItem>& items, const std::wstring& path, uint64_t openedAtUnixSeconds);
void RemoveRecentItem(std::vector<velo::config::RecentItem>& items, const std::wstring& path);
void TogglePinnedRecent(std::vector<velo::config::RecentItem>& items, const std::wstring& path);
void ClearUnpinnedRecentItems(std::vector<velo::config::RecentItem>& items);
void RememberHistoryEntry(std::vector<velo::config::HistoryEntry>& items, const std::wstring& path, const std::wstring& title,
                          double positionSeconds, bool isUrl, uint64_t openedAtUnixSeconds);
void RememberBookmark(std::vector<velo::config::BookmarkEntry>& items, const std::wstring& path, double positionSeconds,
                      std::wstring label);
std::vector<std::wstring> BuildRecentPaths(const std::vector<velo::config::RecentItem>& items);

}  // namespace velo::app
