#pragma once

#include <Windows.h>

namespace velo::ui {

class ThemedScrollbar {
public:
    static constexpr WORD kNotificationValueChanged = 1;

    bool Create(HINSTANCE instance, HWND parent, int controlId);
    void SetMetrics(int viewportSize, int contentSize);
    void SetValue(int value, bool notifyParent = false);
    [[nodiscard]] int Value() const noexcept;
    [[nodiscard]] int MaxValue() const noexcept;
    [[nodiscard]] HWND WindowHandle() const noexcept;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void NotifyParent() const;
    void Paint() const;
    void UpdateThumbRect();
    int ValueFromPoint(int y) const;
    [[nodiscard]] bool Enabled() const noexcept;

    HINSTANCE instance_ = nullptr;
    HWND parent_ = nullptr;
    HWND hwnd_ = nullptr;
    int controlId_ = 0;
    int viewportSize_ = 0;
    int contentSize_ = 0;
    int value_ = 0;
    bool hovered_ = false;
    bool dragging_ = false;
    int dragOffsetY_ = 0;
    RECT thumbRect_{};
};

}  // namespace velo::ui
