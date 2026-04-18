#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace velo::app {

enum class QuickBrowseEntryKind {
    NavigateUp,
    Folder,
    Video,
};

struct QuickBrowseEntry {
    QuickBrowseEntryKind kind = QuickBrowseEntryKind::Video;
    std::wstring path;
    std::wstring label;
    int videoCount = 0;
    bool active = false;
};

class QuickBrowseCatalog {
public:
    std::wstring rootFolder;
    std::wstring currentFolder;
    int activeEntryIndex = -1;

    [[nodiscard]] size_t EntryCount() const noexcept;
    [[nodiscard]] bool Empty() const noexcept;
    [[nodiscard]] QuickBrowseEntry EntryAt(int index) const;

    void SetNavigateUpPath(std::wstring path);
    void SetFolderPaths(std::vector<std::wstring> paths);
    void SetVideoPaths(std::vector<std::wstring> paths, std::wstring activeVideoPath);

private:
    std::wstring navigateUpPath_;
    std::vector<std::wstring> folderPaths_;
    std::vector<std::wstring> videoPaths_;
    std::wstring activeVideoPath_;
};

bool CanBuildQuickBrowseCatalog(const std::wstring& mediaPath);
QuickBrowseCatalog BuildQuickBrowseCatalog(const std::wstring& mediaPath, const std::wstring& currentFolder = L"",
                                           const std::wstring& rootFolder = L"");

}  // namespace velo::app