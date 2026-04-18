#pragma once

#include <Windows.h>

namespace velo::ui::tokens {

enum class FontRole {
    Display,
    H1,
    H2,
    H3,
    Icon,
    Body,
    BodyStrong,
    Caption,
    Numeric,
};

struct Palette {
    COLORREF bgCanvas;
    COLORREF bgSurface1;
    COLORREF bgSurface2;
    COLORREF bgOverlay;
    COLORREF strokeSoft;
    COLORREF strokeFocus;
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF textTertiary;
    COLORREF brandPrimary;
    COLORREF brandHover;
    COLORREF success;
    COLORREF warning;
    COLORREF error;
};

const Palette& DarkPalette();
HFONT CreateAppFont(FontRole role);
COLORREF MixColor(COLORREF first, COLORREF second, double factor);
void FillRoundedRect(HDC dc, const RECT& rect, COLORREF color, int radius);
void DrawRoundedOutline(HDC dc, const RECT& rect, COLORREF color, int radius, int width = 1);
void FillVerticalGradient(HDC dc, const RECT& rect, COLORREF topColor, COLORREF bottomColor);

constexpr int Space2() { return 4; }
constexpr int Space3() { return 8; }
constexpr int Space4() { return 12; }
constexpr int Space5() { return 16; }
constexpr int Space6() { return 20; }
constexpr int Space8() { return 32; }
constexpr int RadiusSm() { return 8; }
constexpr int RadiusMd() { return 12; }
constexpr int RadiusLg() { return 16; }
constexpr int MotionFastMs() { return 80; }
constexpr int MotionBaseMs() { return 140; }
constexpr int MotionPanelMs() { return 180; }

}  // namespace velo::ui::tokens
