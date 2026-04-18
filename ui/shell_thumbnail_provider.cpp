#include "ui/shell_thumbnail_provider.h"

#include <algorithm>
#include <shobjidl.h>

namespace velo::ui {

ShellThumbnailProvider::~ShellThumbnailProvider() {
    Clear();
}

HBITMAP ShellThumbnailProvider::GetThumbnail(const std::wstring& path, const SIZE desiredSize) {
    if (path.empty() || desiredSize.cx <= 0 || desiredSize.cy <= 0) {
        return nullptr;
    }

    const std::wstring key = CacheKey(path, desiredSize);
    const auto existing = cache_.find(key);
    if (existing != cache_.end()) {
        TouchKey(key);
        return existing->second.bitmap;
    }

    CacheEntry entry;
    entry.size = desiredSize;
    entry.bitmap = LoadThumbnail(path, desiredSize);
    if (entry.bitmap == nullptr) {
        return nullptr;
    }
    cache_.emplace(key, entry);
    TouchKey(key);
    EvictIfNeeded();
    return entry.bitmap;
}

void ShellThumbnailProvider::Clear() {
    for (auto& [_, entry] : cache_) {
        if (entry.bitmap != nullptr) {
            DeleteObject(entry.bitmap);
            entry.bitmap = nullptr;
        }
    }
    cache_.clear();
    insertionOrder_.clear();
}

void ShellThumbnailProvider::SetMaxEntries(const size_t maxEntries) {
    maxEntries_ = std::clamp<size_t>(maxEntries, 8, 64);
    EvictIfNeeded();
}

std::wstring ShellThumbnailProvider::CacheKey(const std::wstring& path, const SIZE desiredSize) const {
    return path + L"|" + std::to_wstring(desiredSize.cx) + L"x" + std::to_wstring(desiredSize.cy);
}

void ShellThumbnailProvider::TouchKey(const std::wstring& key) {
    insertionOrder_.erase(std::remove(insertionOrder_.begin(), insertionOrder_.end(), key), insertionOrder_.end());
    insertionOrder_.push_back(key);
}

HBITMAP ShellThumbnailProvider::LoadThumbnail(const std::wstring& path, const SIZE desiredSize) const {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(init);

    IShellItem* shellItem = nullptr;
    const HRESULT createResult = SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&shellItem));
    if (FAILED(createResult) || shellItem == nullptr) {
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return nullptr;
    }

    IShellItemImageFactory* imageFactory = nullptr;
    HBITMAP bitmap = nullptr;
    if (SUCCEEDED(shellItem->QueryInterface(IID_PPV_ARGS(&imageFactory))) && imageFactory != nullptr) {
        HRESULT imageResult = imageFactory->GetImage(desiredSize, SIIGBF_BIGGERSIZEOK | SIIGBF_THUMBNAILONLY, &bitmap);
        if (FAILED(imageResult) || bitmap == nullptr) {
            imageResult = imageFactory->GetImage(desiredSize, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, &bitmap);
        }
        imageFactory->Release();
    }

    shellItem->Release();
    if (shouldUninitialize) {
        CoUninitialize();
    }
    return bitmap;
}

void ShellThumbnailProvider::EvictIfNeeded() {
    while (cache_.size() > maxEntries_ && !insertionOrder_.empty()) {
        const std::wstring key = insertionOrder_.front();
        insertionOrder_.pop_front();
        const auto existing = cache_.find(key);
        if (existing == cache_.end()) {
            continue;
        }
        if (existing->second.bitmap != nullptr) {
            DeleteObject(existing->second.bitmap);
        }
        cache_.erase(existing);
    }
}

}  // namespace velo::ui