#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace velo::app {

bool SavePlaylist(const std::filesystem::path& path, const std::vector<std::wstring>& items);
std::vector<std::wstring> LoadPlaylist(const std::filesystem::path& path);
bool MovePlaylistItem(std::vector<std::wstring>& items, int fromIndex, int toIndex);
bool RemovePlaylistItem(std::vector<std::wstring>& items, int index);
void InsertIntoPlaylist(std::vector<std::wstring>& items, const std::vector<std::wstring>& newItems, int insertIndex);

}  // namespace velo::app
