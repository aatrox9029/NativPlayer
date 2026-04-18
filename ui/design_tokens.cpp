#include "ui/design_tokens.h"

#include <Windows.h>

#include <algorithm>

namespace velo::ui::tokens {
namespace {

constexpr Palette kDarkPalette{
    .bgCanvas = RGB(7, 12, 18),
    .bgSurface1 = RGB(15, 21, 30),
    .bgSurface2 = RGB(22, 30, 41),
    .bgOverlay = RGB(10, 16, 24),
    .strokeSoft = RGB(54, 66, 79),
    .strokeFocus = RGB(92, 214, 192),
    .textPrimary = RGB(247, 249, 252),
    .textSecondary = RGB(186, 196, 208),
    .textTertiary = RGB(131, 144, 159),
    .brandPrimary = RGB(78, 198, 176),
    .brandHover = RGB(113, 221, 201),
    .success = RGB(59, 203, 144),
    .warning = RGB(247, 189, 94),
    .error = RGB(255, 107, 107),
};

int FontHeight(FontRole role) {
    switch (role) {
        case FontRole::Display:
            return -28;
        case FontRole::H1:
            return -22;
        case FontRole::H2:
            return -18;
        case FontRole::H3:
            return -15;
        case FontRole::Icon:
            return -20;
        case FontRole::Caption:
            return -12;
        case FontRole::Numeric:
            return -13;
        default:
            return -14;
    }
}

int FontWeight(FontRole role) {
    switch (role) {
        case FontRole::Display:
        case FontRole::H1:
            return FW_BOLD;
        case FontRole::H2:
        case FontRole::H3:
        case FontRole::Icon:
        case FontRole::BodyStrong:
        case FontRole::Numeric:
            return FW_SEMIBOLD;
        default:
            return FW_NORMAL;
    }
}

const wchar_t* FontFamily(FontRole role) {
    if (role == FontRole::Numeric) {
        return L"Bahnschrift";
    }
    if (role == FontRole::Icon) {
        return L"Segoe UI Symbol";
    }
    return L"Microsoft JhengHei UI";
}

}  // namespace

const Palette& DarkPalette() {
    return kDarkPalette;
}

HFONT CreateAppFont(const FontRole role) {
    return CreateFontW(FontHeight(role), 0, 0, 0, FontWeight(role), FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       FontFamily(role));
}

COLORREF MixColor(const COLORREF first, const COLORREF second, const double factor) {
    const double clamped = std::clamp(factor, 0.0, 1.0);
    const auto mix = [clamped](const int left, const int right) {
        return static_cast<int>(left + (right - left) * clamped);
    };
    return RGB(mix(GetRValue(first), GetRValue(second)), mix(GetGValue(first), GetGValue(second)),
               mix(GetBValue(first), GetBValue(second)));
}

void FillRoundedRect(HDC dc, const RECT& rect, const COLORREF color, const int radius) {
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    const auto oldBrush = SelectObject(dc, brush);
    const auto oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawRoundedOutline(HDC dc, const RECT& rect, const COLORREF color, const int radius, const int width) {
    HPEN pen = CreatePen(PS_SOLID, width, color);
    const auto oldPen = SelectObject(dc, pen);
    const auto oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void FillVerticalGradient(HDC dc, const RECT& rect, const COLORREF topColor, const COLORREF bottomColor) {
    TRIVERTEX vertices[2] = {
        {rect.left, rect.top, static_cast<COLOR16>(GetRValue(topColor) << 8), static_cast<COLOR16>(GetGValue(topColor) << 8),
         static_cast<COLOR16>(GetBValue(topColor) << 8), 0x0000},
        {rect.right, rect.bottom, static_cast<COLOR16>(GetRValue(bottomColor) << 8), static_cast<COLOR16>(GetGValue(bottomColor) << 8),
         static_cast<COLOR16>(GetBValue(bottomColor) << 8), 0x0000},
    };
    GRADIENT_RECT gradient{0, 1};
    GradientFill(dc, vertices, 2, &gradient, 1, GRADIENT_FILL_RECT_V);
}

}  // namespace velo::ui::tokens
