#include "app/media_playlist.h"

#include <Windows.h>

#include <algorithm>
#include <cwctype>

#include "app/media_file_types.h"

namespace velo::app {
namespace {

std::wstring Lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
    return value;
}

bool PathEqualsInsensitive(const std::filesystem::path& left, const std::filesystem::path& right) {
    return CompareStringOrdinal(left.wstring().c_str(), -1, right.wstring().c_str(), -1, TRUE) == CSTR_EQUAL;
}

std::vector<std::filesystem::path> CollectVideoFiles(const std::filesystem::path& folderPath) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath, error)) {
        if (error || !entry.is_regular_file()) {
            continue;
        }
        if (IsPlayableMediaFile(entry.path())) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end(), [](const auto& left, const auto& right) {
        return Lowercase(left.filename().wstring()) < Lowercase(right.filename().wstring());
    });
    return files;
}

std::vector<std::wstring> BuildFileNames(const std::vector<std::filesystem::path>& items) {
    std::vector<std::wstring> names;
    names.reserve(items.size());
    for (const auto& item : items) {
        names.push_back(item.filename().wstring());
    }
    return names;
}

}  // namespace

bool IsPlayableMediaFile(const std::filesystem::path& path) {
    return HasKnownExtension(path, PlayableMediaExtensions());
}

bool IsVideoFile(const std::filesystem::path& path) {
    return IsPlayableMediaFile(path);
}

bool IsSubtitleFile(const std::filesystem::path& path) {
    return HasKnownExtension(path, SubtitleExtensions());
}

PlaylistBuildResult BuildPlaylistFromFile(const std::filesystem::path& filePath) {
    PlaylistBuildResult result;
    if (!std::filesystem::exists(filePath)) {
        return result;
    }

    auto items = CollectVideoFiles(filePath.parent_path());
    if (items.empty()) {
        result.items.push_back(filePath.wstring());
        result.currentIndex = 0;
        return result;
    }

    for (size_t index = 0; index < items.size(); ++index) {
        result.items.push_back(items[index].wstring());
        if (PathEqualsInsensitive(items[index], filePath)) {
            result.currentIndex = static_cast<int>(index);
        }
    }

    result.compactRootFolder = filePath.parent_path().wstring();
    result.compactItemNames = BuildFileNames(items);
    result.isFolderBacked = !result.compactRootFolder.empty() && !result.compactItemNames.empty();

    if (result.currentIndex < 0) {
        result.items.insert(result.items.begin(), filePath.wstring());
        result.currentIndex = 0;
        result.compactRootFolder.clear();
        result.compactItemNames.clear();
        result.isFolderBacked = false;
    }
    return result;
}

PlaylistBuildResult BuildPlaylistFromFolder(const std::filesystem::path& folderPath) {
    PlaylistBuildResult result;
    auto items = CollectVideoFiles(folderPath);
    for (const auto& item : items) {
        result.items.push_back(item.wstring());
    }
    result.compactRootFolder = folderPath.wstring();
    result.compactItemNames = BuildFileNames(items);
    result.isFolderBacked = !result.compactRootFolder.empty() && !result.compactItemNames.empty();
    result.currentIndex = result.items.empty() ? -1 : 0;
    return result;
}

PlaylistBuildResult BuildTemporaryPlaylist(const std::vector<std::wstring>& paths) {
    PlaylistBuildResult result;
    for (const auto& path : paths) {
        if (path.empty() || !IsVideoFile(path)) {
            continue;
        }
        const bool alreadyExists = std::any_of(result.items.begin(), result.items.end(), [&](const std::wstring& existing) {
            return PathEqualsInsensitive(existing, path);
        });
        if (!alreadyExists) {
            result.items.push_back(path);
        }
    }
    result.currentIndex = result.items.empty() ? -1 : 0;
    return result;
}

std::optional<std::filesystem::path> FindSidecarSubtitle(const std::filesystem::path& videoPath) {
    if (!std::filesystem::exists(videoPath)) {
        return std::nullopt;
    }

    const auto stem = Lowercase(videoPath.stem().wstring());
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(videoPath.parent_path(), error)) {
        if (error || !entry.is_regular_file() || !IsSubtitleFile(entry.path())) {
            continue;
        }
        if (Lowercase(entry.path().stem().wstring()) == stem) {
            return entry.path();
        }
    }
    return std::nullopt;
}

std::wstring ShortDisplayName(const std::wstring& path) {
    const std::filesystem::path fsPath(path);
    return fsPath.filename().empty() ? path : fsPath.filename().wstring();
}

}  // namespace velo::app