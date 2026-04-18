#include "app/runtime_playlist.h"

#include <Windows.h>

#include <filesystem>

namespace velo::app {
namespace {

bool SamePathInsensitive(const std::wstring& left, const std::wstring& right) {
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()), right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

}  // namespace

void RuntimePlaylist::Clear() {
    explicitItems_.clear();
    folderRoot_.clear();
    folderFileNames_.clear();
}

void RuntimePlaylist::SetExplicit(std::vector<std::wstring> items) {
    explicitItems_ = std::move(items);
    folderRoot_.clear();
    folderFileNames_.clear();
}

void RuntimePlaylist::SetFolderBacked(std::wstring folderRoot, std::vector<std::wstring> fileNames) {
    explicitItems_.clear();
    folderRoot_ = std::move(folderRoot);
    folderFileNames_ = std::move(fileNames);
}

bool RuntimePlaylist::Empty() const noexcept {
    return Size() == 0;
}

size_t RuntimePlaylist::Size() const noexcept {
    return UsesFolderBacking() ? folderFileNames_.size() : explicitItems_.size();
}

std::wstring RuntimePlaylist::PathAt(const int index) const {
    if (index < 0 || index >= static_cast<int>(Size())) {
        return {};
    }
    if (!UsesFolderBacking()) {
        return explicitItems_[index];
    }

    return (std::filesystem::path(folderRoot_) / folderFileNames_[index]).wstring();
}

int RuntimePlaylist::FindIndexByPath(const std::wstring& path) const {
    if (path.empty()) {
        return -1;
    }

    if (!UsesFolderBacking()) {
        for (size_t index = 0; index < explicitItems_.size(); ++index) {
            if (SamePathInsensitive(explicitItems_[index], path)) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    for (size_t index = 0; index < folderFileNames_.size(); ++index) {
        if (SamePathInsensitive((std::filesystem::path(folderRoot_) / folderFileNames_[index]).wstring(), path)) {
            return static_cast<int>(index);
        }
    }

    const std::filesystem::path inputPath(path);
    if (!inputPath.has_filename()) {
        return -1;
    }
    const std::wstring fileName = inputPath.filename().wstring();
    for (size_t index = 0; index < folderFileNames_.size(); ++index) {
        if (SamePathInsensitive(folderFileNames_[index], fileName)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

bool RuntimePlaylist::UsesFolderBacking() const noexcept {
    return !folderRoot_.empty() && !folderFileNames_.empty();
}

}  // namespace velo::app
