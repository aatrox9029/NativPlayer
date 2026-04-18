#pragma once

#include <Windows.h>

#include <functional>

#include "app/quick_browse_catalog.h"
#include "ui/player_state.h"
#include "ui/preview_session.h"
#include "ui/shell_thumbnail_provider.h"
#include "ui/themed_scrollbar.h"

namespace velo::ui {

class QuickBrowsePanel {
public:
    struct Callbacks {
        std::function<void(const std::wstring&)> openFile;
        std::function<void(const std::wstring&)> navigateFolder;
        std::function<void()> closePanel;
    };

    bool Create(HINSTANCE instance, HWND parent, Callbacks callbacks);
    void SetBounds(const RECT& bounds) const;
    void SetCatalog(velo::app::QuickBrowseCatalog catalog);
    void SetLanguageCode(std::wstring languageCode);
    void SetVisible(bool visible);
    [[nodiscard]] bool Visible() const noexcept;
    [[nodiscard]] bool IsPointInside(POINT parentClientPoint) const;
    [[nodiscard]] bool IsScreenPointInside(POINT screenPoint) const;
    [[nodiscard]] HWND WindowHandle() const noexcept;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void Paint();
    void UpdateScrollbar();
    void UpdateThumbnailBudget();
    void SetScrollOffset(int offset);
    void ScrollToActiveEntry();
    void UpdateHoverIndex(POINT clientPoint);
    void InvalidateEntry(int index) const;
    void StartPreview(int index);
    void StopPreview(bool releaseSession = false);
    void SchedulePreviewShutdown();
    void UpdatePreviewBounds();
    bool UpdatePreviewProgressFromPoint(POINT clientPoint);
    void DrawThumbnail(HDC dc, const RECT& entryRect, const velo::app::QuickBrowseEntry& entry, bool previewActive);
    void DrawBitmap(HDC dc, HBITMAP bitmap, const RECT& targetRect) const;
    void DrawCloseButton(HDC dc) const;
    void ActivateEntry(int index) const;
    [[nodiscard]] RECT HeaderRect() const;
    [[nodiscard]] RECT CloseButtonRect() const;
    [[nodiscard]] RECT ContentRect() const;
    [[nodiscard]] RECT EntryRect(int index) const;
    [[nodiscard]] RECT ThumbnailRect(const RECT& entryRect) const;
    [[nodiscard]] RECT ProgressBarRect(const RECT& entryRect) const;
    [[nodiscard]] RECT PreviewRect(const RECT& entryRect) const;
    [[nodiscard]] RECT TextRect(const RECT& entryRect) const;
    [[nodiscard]] int EntryStride() const;
    [[nodiscard]] int ContentHeight() const;
    [[nodiscard]] int MaxScrollOffset() const;
    [[nodiscard]] int HitTestEntry(POINT clientPoint) const;
    [[nodiscard]] bool IsPreviewProgressHit(POINT clientPoint, int* hitIndex = nullptr) const;

    static constexpr UINT_PTR kPreviewTimer = 1;
    static constexpr UINT_PTR kPreviewShutdownTimer = 2;

    HINSTANCE instance_ = nullptr;
    HWND parent_ = nullptr;
    HWND hwnd_ = nullptr;
    HWND previewWindow_ = nullptr;
    HFONT titleFont_ = nullptr;
    HFONT bodyFont_ = nullptr;
    HFONT captionFont_ = nullptr;
    Callbacks callbacks_;
    velo::app::QuickBrowseCatalog catalog_;
    ShellThumbnailProvider thumbnailProvider_;
    ThemedScrollbar scrollbar_;
    PreviewSession previewSession_;
    int scrollOffset_ = 0;
    int hoveredIndex_ = -1;
    int previewIndex_ = -1;
    bool visible_ = false;
    bool previewInitialized_ = false;
    bool previewProgressDragging_ = false;
    bool suppressActivateOnRelease_ = false;
    std::wstring languageCode_ = L"zh-TW";
    std::wstring previewPath_;
    int thumbnailBudget_ = 24;
};

}  // namespace velo::ui
