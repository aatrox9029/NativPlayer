#include "app/quick_browse_catalog.h"

#include <Windows.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <system_error>

#include "app/media_playlist.h"

namespace velo::app {
namespace {

std::wstring Lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const wchar_t ch) {
        return static_cast<wchar_t>(towlower(ch));
    });
    return value;
}

std::filesystem::path NormalizePath(const std::filesystem::path& path) {
    std::error_code error;
    const auto canonical = std::filesystem::weakly_canonical(path, error);
    if (!error) {
        return canonical;
    }
    return std::filesystem::absolute(path, error).lexically_normal();
}

bool PathEqualsInsensitive(const std::filesystem::path& left, const std::filesystem::path& right) {
    const std::wstring leftText = NormalizePath(left).wstring();
    const std::wstring rightText = NormalizePath(right).wstring();
    return CompareStringOrdinal(leftText.c_str(), static_cast<int>(leftText.size()), rightText.c_str(), static_cast<int>(rightText.size()), TRUE) ==
           CSTR_EQUAL;
}

bool IsSameOrChildFolder(std::filesystem::path folder, const std::filesystem::path& root) {
    folder = NormalizePath(folder);
    const auto normalizedRoot = NormalizePath(root);
    while (!folder.empty()) {
        if (PathEqualsInsensitive(folder, normalizedRoot)) {
            return true;
        }

        const auto parent = folder.parent_path();
        if (parent.empty() || PathEqualsInsensitive(parent, folder)) {
            break;
        }
        folder = parent;
    }

    return false;
}

struct FolderContents {
    std::vector<std::filesystem::path> folders;
    std::vector<std::filesystem::path> videos;
};

FolderContents CollectFolderContents(const std::filesystem::path& folderPath) {
    FolderContents contents;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath, error)) {
        if (error) {
            continue;
        }

        if (entry.is_directory(error)) {
            if (!error) {
                contents.folders.push_back(entry.path());
            }
            error.clear();
            continue;
        }

        if (!entry.is_regular_file(error) || error) {
            error.clear();
            continue;
        }

        if (IsPlayableMediaFile(entry.path())) {
            contents.videos.push_back(entry.path());
        }
    }

    std::sort(contents.folders.begin(), contents.folders.end(), [](const auto& left, const auto& right) {
        return Lowercase(left.filename().wstring()) < Lowercase(right.filename().wstring());
    });
    std::sort(contents.videos.begin(), contents.videos.end(), [](const auto& left, const auto& right) {
        return Lowercase(left.filename().wstring()) < Lowercase(right.filename().wstring());
    });
    return contents;
}

}  // namespace

size_t QuickBrowseCatalog::EntryCount() const noexcept {
    return (navigateUpPath_.empty() ? 0u : 1u) + folderPaths_.size() + videoPaths_.size();
}

bool QuickBrowseCatalog::Empty() const noexcept {
    return EntryCount() == 0;
}

QuickBrowseEntry QuickBrowseCatalog::EntryAt(const int index) const {
    if (index < 0 || index >= static_cast<int>(EntryCount())) {
        return {};
    }

    int offset = 0;
    if (!navigateUpPath_.empty()) {
        if (index == 0) {
            return QuickBrowseEntry{
                .kind = QuickBrowseEntryKind::NavigateUp,
                .path = navigateUpPath_,
                .label = L"返回上一層",
            };
        }
        offset = 1;
    }

    const int folderCount = static_cast<int>(folderPaths_.size());
    if (index - offset < folderCount) {
        const std::wstring& path = folderPaths_[index - offset];
        const std::filesystem::path folder(path);
        const std::wstring label = folder.filename().wstring().empty() ? path : folder.filename().wstring();
        return QuickBrowseEntry{
            .kind = QuickBrowseEntryKind::Folder,
            .path = path,
            .label = label,
        };
    }

    const std::wstring& path = videoPaths_[index - offset - folderCount];
    return QuickBrowseEntry{
        .kind = QuickBrowseEntryKind::Video,
        .path = path,
        .label = ShortDisplayName(path),
        .active = PathEqualsInsensitive(std::filesystem::path(path), std::filesystem::path(activeVideoPath_)),
    };
}

void QuickBrowseCatalog::SetNavigateUpPath(std::wstring path) {
    navigateUpPath_ = std::move(path);
}

void QuickBrowseCatalog::SetFolderPaths(std::vector<std::wstring> paths) {
    folderPaths_ = std::move(paths);
}

void QuickBrowseCatalog::SetVideoPaths(std::vector<std::wstring> paths, std::wstring activeVideoPath) {
    videoPaths_ = std::move(paths);
    activeVideoPath_ = std::move(activeVideoPath);
}

bool CanBuildQuickBrowseCatalog(const std::wstring& mediaPath) {
    if (mediaPath.empty()) {
        return false;
    }

    const std::filesystem::path path(mediaPath);
    std::error_code error;
    return std::filesystem::exists(path, error) && std::filesystem::is_regular_file(path, error) &&
            IsPlayableMediaFile(path) && std::filesystem::exists(path.parent_path(), error);
}

QuickBrowseCatalog BuildQuickBrowseCatalog(const std::wstring& mediaPath, const std::wstring& currentFolder, const std::wstring& rootFolder) {
    QuickBrowseCatalog catalog;
    if (!CanBuildQuickBrowseCatalog(mediaPath)) {
        return catalog;
    }

    const std::filesystem::path mediaFile(mediaPath);
    std::filesystem::path resolvedRootFolder = rootFolder.empty() ? NormalizePath(mediaFile.parent_path()) : NormalizePath(std::filesystem::path(rootFolder));
    std::filesystem::path browseFolder = currentFolder.empty() ? resolvedRootFolder : NormalizePath(std::filesystem::path(currentFolder));
    std::error_code error;
    if (!std::filesystem::exists(resolvedRootFolder, error) || !std::filesystem::is_directory(resolvedRootFolder, error) ||
        !IsSameOrChildFolder(mediaFile.parent_path(), resolvedRootFolder)) {
        resolvedRootFolder = NormalizePath(mediaFile.parent_path());
    }
    error.clear();
    if (!std::filesystem::exists(browseFolder, error) || !std::filesystem::is_directory(browseFolder, error) ||
        !IsSameOrChildFolder(browseFolder, resolvedRootFolder)) {
        browseFolder = resolvedRootFolder;
    }

    catalog.rootFolder = resolvedRootFolder.wstring();
    catalog.currentFolder = browseFolder.wstring();

    if (!PathEqualsInsensitive(browseFolder, resolvedRootFolder)) {
        catalog.SetNavigateUpPath(browseFolder.parent_path().wstring());
    }

    const FolderContents contents = CollectFolderContents(browseFolder);

    std::vector<std::wstring> folderPaths;
    folderPaths.reserve(contents.folders.size());
    for (const auto& folder : contents.folders) {
        folderPaths.push_back(folder.wstring());
    }
    catalog.SetFolderPaths(std::move(folderPaths));

    std::vector<std::wstring> videoPaths;
    videoPaths.reserve(contents.videos.size());
    for (const auto& video : contents.videos) {
        if (PathEqualsInsensitive(video, mediaFile)) {
            const int navigateUpOffset = browseFolder == resolvedRootFolder ? 0 : 1;
            catalog.activeEntryIndex = navigateUpOffset + static_cast<int>(contents.folders.size()) + static_cast<int>(videoPaths.size());
        }
        videoPaths.push_back(video.wstring());
    }
    catalog.SetVideoPaths(std::move(videoPaths), mediaFile.wstring());

    return catalog;
}

}  // namespace velo::app