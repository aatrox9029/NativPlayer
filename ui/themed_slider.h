#pragma once

#include <Windows.h>

namespace velo::ui {

class ThemedSlider {
public:
    static constexpr WORD kNotificationValueChanged = 1;
    static constexpr WORD kNotificationHoverChanged = 2;
    static constexpr WORD kNotificationHoverEnded = 3;

    bool Create(HINSTANCE instance, HWND parent, int controlId);
    void SetRange(int minimum, int maximum);
    void SetValue(int value, bool notifyParent = false);
    [[nodiscard]] int Value() const noexcept;
    [[nodiscard]] int HoverValue() const noexcept;
    [[nodiscard]] bool IsDragging() const noexcept;
    [[nodiscard]] HWND WindowHandle() const noexcept;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void NotifyParent(WORD notificationCode) const;
    void Paint() const;
    int ValueFromPoint(int x) const;

    HINSTANCE instance_ = nullptr;
    HWND parent_ = nullptr;
    HWND hwnd_ = nullptr;
    int controlId_ = 0;
    int minimum_ = 0;
    int maximum_ = 100;
    int value_ = 0;
    int hoverValue_ = 0;
    bool hovered_ = false;
    bool dragging_ = false;
};

}  // namespace velo::ui
