#include "app/playlist_storage.h"

#include <algorithm>
#include <fstream>

namespace velo::app {

bool SavePlaylist(const std::filesystem::path& path, const std::vector<std::wstring>& items) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::wofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << L"#EXTM3U\n";
    for (const auto& item : items) {
        if (!item.empty()) {
            output << item << L'\n';
        }
    }
    return true;
}

std::vector<std::wstring> LoadPlaylist(const std::filesystem::path& path) {
    std::vector<std::wstring> items;
    std::wifstream input(path);
    if (!input.is_open()) {
        return items;
    }

    std::wstring line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == L'#') {
            continue;
        }
        items.push_back(line);
    }
    return items;
}

bool MovePlaylistItem(std::vector<std::wstring>& items, const int fromIndex, const int toIndex) {
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= static_cast<int>(items.size()) || toIndex >= static_cast<int>(items.size()) ||
        fromIndex == toIndex) {
        return false;
    }

    const std::wstring moving = items[static_cast<size_t>(fromIndex)];
    items.erase(items.begin() + fromIndex);
    items.insert(items.begin() + toIndex, moving);
    return true;
}

bool RemovePlaylistItem(std::vector<std::wstring>& items, const int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) {
        return false;
    }
    items.erase(items.begin() + index);
    return true;
}

void InsertIntoPlaylist(std::vector<std::wstring>& items, const std::vector<std::wstring>& newItems, int insertIndex) {
    insertIndex = std::clamp(insertIndex, 0, static_cast<int>(items.size()));
    items.insert(items.begin() + insertIndex, newItems.begin(), newItems.end());
}

}  // namespace velo::app
