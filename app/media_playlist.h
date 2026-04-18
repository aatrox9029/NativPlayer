#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace velo::app {

struct PlaylistBuildResult {
    std::vector<std::wstring> items;
    std::wstring compactRootFolder;
    std::vector<std::wstring> compactItemNames;
    bool isFolderBacked = false;
    int currentIndex = -1;
};

bool IsPlayableMediaFile(const std::filesystem::path& path);
bool IsVideoFile(const std::filesystem::path& path);
bool IsSubtitleFile(const std::filesystem::path& path);
PlaylistBuildResult BuildPlaylistFromFile(const std::filesystem::path& filePath);
PlaylistBuildResult BuildPlaylistFromFolder(const std::filesystem::path& folderPath);
PlaylistBuildResult BuildTemporaryPlaylist(const std::vector<std::wstring>& paths);
std::optional<std::filesystem::path> FindSidecarSubtitle(const std::filesystem::path& videoPath);
std::wstring ShortDisplayName(const std::wstring& path);

}  // namespace velo::app