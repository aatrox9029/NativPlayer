#pragma once

#include <Windows.h>

#include <deque>
#include <string>
#include <unordered_map>

namespace velo::ui {

class ShellThumbnailProvider {
public:
    ~ShellThumbnailProvider();

    [[nodiscard]] HBITMAP GetThumbnail(const std::wstring& path, SIZE desiredSize);
    void Clear();
    void SetMaxEntries(size_t maxEntries);

private:
    static constexpr size_t kDefaultMaxEntries = 24;

    struct CacheEntry {
        HBITMAP bitmap = nullptr;
        SIZE size{};
    };

    [[nodiscard]] std::wstring CacheKey(const std::wstring& path, SIZE desiredSize) const;
    [[nodiscard]] HBITMAP LoadThumbnail(const std::wstring& path, SIZE desiredSize) const;
    void TouchKey(const std::wstring& key);
    void EvictIfNeeded();

    std::unordered_map<std::wstring, CacheEntry> cache_;
    std::deque<std::wstring> insertionOrder_;
    size_t maxEntries_ = kDefaultMaxEntries;
};

}  // namespace velo::ui