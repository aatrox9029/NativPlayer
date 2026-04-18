#include "ui/button_style_assets.h"

#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace velo::ui {
namespace {

class GdiplusSession {
public:
    GdiplusSession() {
        Gdiplus::GdiplusStartupInput startupInput;
        ready_ = Gdiplus::GdiplusStartup(&token_, &startupInput, nullptr) == Gdiplus::Ok;
    }

    ~GdiplusSession() {
        if (ready_) {
            Gdiplus::GdiplusShutdown(token_);
        }
    }

    [[nodiscard]] bool Ready() const noexcept {
        return ready_;
    }

private:
    ULONG_PTR token_ = 0;
    bool ready_ = false;
};

GdiplusSession& Session() {
    static GdiplusSession session;
    return session;
}

std::filesystem::path ResolveExecutableDirectory(HINSTANCE instance) {
    std::vector<wchar_t> buffer(MAX_PATH, L'\0');
    while (true) {
        const DWORD length = GetModuleFileNameW(instance, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            return std::filesystem::path(buffer.data()).parent_path();
        }
        buffer.resize(buffer.size() * 2, L'\0');
    }
}

std::vector<std::filesystem::path> ResolveAssetSearchRoots(HINSTANCE instance) {
    const auto executableDirectory = ResolveExecutableDirectory(instance);
    if (executableDirectory.empty()) {
        return {};
    }

    return {
        executableDirectory / L"button-style",
        executableDirectory.parent_path() / L"button-style",
    };
}

RECT FitBitmapToBounds(const RECT& bounds, const UINT sourceWidth, const UINT sourceHeight) {
    RECT fitted = bounds;
    const int availableWidth = std::max(0, static_cast<int>(bounds.right - bounds.left));
    const int availableHeight = std::max(0, static_cast<int>(bounds.bottom - bounds.top));
    if (availableWidth == 0 || availableHeight == 0 || sourceWidth == 0 || sourceHeight == 0) {
        fitted.right = fitted.left;
        fitted.bottom = fitted.top;
        return fitted;
    }

    const double scale = std::min(static_cast<double>(availableWidth) / static_cast<double>(sourceWidth),
                                  static_cast<double>(availableHeight) / static_cast<double>(sourceHeight));
    const int drawWidth = std::max(1, static_cast<int>(std::lround(static_cast<double>(sourceWidth) * scale)));
    const int drawHeight = std::max(1, static_cast<int>(std::lround(static_cast<double>(sourceHeight) * scale)));
    fitted.left = bounds.left + (availableWidth - drawWidth) / 2;
    fitted.top = bounds.top + (availableHeight - drawHeight) / 2;
    fitted.right = fitted.left + drawWidth;
    fitted.bottom = fitted.top + drawHeight;
    return fitted;
}

class ButtonStyleBitmapCache {
public:
    Gdiplus::Bitmap* Load(HINSTANCE instance, std::wstring_view assetStem) {
        if (assetStem.empty() || !Session().Ready()) {
            return nullptr;
        }

        const std::wstring key(assetStem);
        const auto assetPath = ResolveAssetPath(instance, key);
        if (assetPath.empty()) {
            bitmaps_.erase(key);
            return nullptr;
        }

        const auto updatedAt = ResolveAssetTimestamp(assetPath);
        if (const auto existing = bitmaps_.find(key); existing != bitmaps_.end()) {
            if (existing->second.path == assetPath && existing->second.updatedAt == updatedAt && existing->second.bitmap != nullptr) {
                existing->second.lastAccessTick = NextAccessTick();
                return existing->second.bitmap.get();
            }
        }

        auto bitmap = std::make_unique<Gdiplus::Bitmap>(assetPath.c_str(), FALSE);
        if (bitmap->GetLastStatus() != Gdiplus::Ok) {
            bitmaps_.erase(key);
            return nullptr;
        }

        auto* result = bitmap.get();
        bitmaps_[key] = BitmapEntry{
            .path = assetPath,
            .updatedAt = updatedAt,
            .bitmap = std::move(bitmap),
            .lastAccessTick = NextAccessTick(),
        };
        EvictIfNeeded();
        return result;
    }

private:
    static constexpr size_t kMaxEntries = 12;

    struct BitmapEntry {
        std::filesystem::path path;
        std::optional<std::filesystem::file_time_type> updatedAt;
        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        uint64_t lastAccessTick = 0;
    };

    uint64_t NextAccessTick() {
        ++accessTick_;
        return accessTick_;
    }

    void EvictIfNeeded() {
        while (bitmaps_.size() > kMaxEntries) {
            auto eviction = bitmaps_.end();
            for (auto it = bitmaps_.begin(); it != bitmaps_.end(); ++it) {
                if (eviction == bitmaps_.end() || it->second.lastAccessTick < eviction->second.lastAccessTick) {
                    eviction = it;
                }
            }
            if (eviction == bitmaps_.end()) {
                return;
            }
            bitmaps_.erase(eviction);
        }
    }

    std::filesystem::path ResolveAssetPath(HINSTANCE instance, const std::wstring& assetStem) const {
        std::error_code error;
        for (const auto& root : ResolveAssetSearchRoots(instance)) {
            const auto path = root / (assetStem + L".png");
            if (std::filesystem::exists(path, error)) {
                return path;
            }
            error.clear();
        }
        return {};
    }

    std::optional<std::filesystem::file_time_type> ResolveAssetTimestamp(const std::filesystem::path& assetPath) const {
        std::error_code error;
        const auto updatedAt = std::filesystem::last_write_time(assetPath, error);
        if (error) {
            return std::nullopt;
        }
        return updatedAt;
    }

    std::unordered_map<std::wstring, BitmapEntry> bitmaps_;
    uint64_t accessTick_ = 0;
};

ButtonStyleBitmapCache& BitmapCache() {
    static ButtonStyleBitmapCache cache;
    return cache;
}

}  // namespace

bool DrawButtonStyleIcon(HINSTANCE instance, const std::wstring_view assetStem, HDC deviceContext, const RECT& bounds, const bool disabled) {
    auto* bitmap = BitmapCache().Load(instance, assetStem);
    if (bitmap == nullptr) {
        return false;
    }

    const RECT fitted = FitBitmapToBounds(bounds, bitmap->GetWidth(), bitmap->GetHeight());
    if (fitted.right <= fitted.left || fitted.bottom <= fitted.top) {
        return false;
    }

    Gdiplus::Graphics graphics(deviceContext);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

    Gdiplus::ImageAttributes imageAttributes;
    Gdiplus::ColorMatrix colorMatrix = {{
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, disabled ? 0.42f : 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    }};
    imageAttributes.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

    const Gdiplus::Rect destinationRect(fitted.left, fitted.top, fitted.right - fitted.left, fitted.bottom - fitted.top);
    graphics.DrawImage(bitmap,
                       destinationRect,
                       0,
                       0,
                       static_cast<INT>(bitmap->GetWidth()),
                       static_cast<INT>(bitmap->GetHeight()),
                       Gdiplus::UnitPixel,
                       &imageAttributes,
                       nullptr,
                       nullptr);
    return true;
}

}  // namespace velo::ui