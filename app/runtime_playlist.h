#pragma once

#include <string>
#include <vector>

namespace velo::app {

class RuntimePlaylist {
public:
    void Clear();
    void SetExplicit(std::vector<std::wstring> items);
    void SetFolderBacked(std::wstring folderRoot, std::vector<std::wstring> fileNames);

    [[nodiscard]] bool Empty() const noexcept;
    [[nodiscard]] size_t Size() const noexcept;
    [[nodiscard]] std::wstring PathAt(int index) const;
    [[nodiscard]] int FindIndexByPath(const std::wstring& path) const;

private:
    [[nodiscard]] bool UsesFolderBacking() const noexcept;

    std::vector<std::wstring> explicitItems_;
    std::wstring folderRoot_;
    std::vector<std::wstring> folderFileNames_;
};

}  // namespace velo::app