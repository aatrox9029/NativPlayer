#include "ui/settings_page_surface.h"

#include <Windows.h>
#include <commctrl.h>

#include "ui/design_tokens.h"

namespace velo::ui {
namespace {

LRESULT CALLBACK SettingsPageSurfaceProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData) {
    const auto* theme = reinterpret_cast<const SettingsPageSurfaceTheme*>(refData);
    if (theme == nullptr) {
        return DefSubclassProc(hwnd, message, wParam, lParam);
    }

    switch (message) {
        case WM_COMMAND:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
            if (theme->owner != nullptr) {
                return SendMessageW(theme->owner, message, wParam, lParam);
            }
            break;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            const auto& palette = tokens::DarkPalette();
            SetTextColor(dc, palette.textPrimary);
            SetBkColor(dc, tokens::MixColor(palette.bgSurface2, palette.bgCanvas, 0.10));
            SetBkMode(dc, OPAQUE);
            return reinterpret_cast<LRESULT>(theme->pageBrush);
        }

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wParam);
            const auto& palette = tokens::DarkPalette();
            SetTextColor(dc, palette.textPrimary);
            SetBkColor(dc, tokens::MixColor(palette.bgSurface2, palette.bgCanvas, 0.18));
            SetBkMode(dc, OPAQUE);
            return reinterpret_cast<LRESULT>(theme->controlBrush);
        }

        case WM_HSCROLL:
            if (theme->owner != nullptr) {
                SendMessageW(theme->owner, WM_HSCROLL, wParam, lParam);
            }
            return 0;

        case WM_ERASEBKGND: {
            RECT client{};
            GetClientRect(hwnd, &client);
            FillRect(reinterpret_cast<HDC>(wParam), &client, theme->pageBrush);
            return 1;
        }

        default:
            break;
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

}  // namespace

void AttachSettingsPageSurface(HWND page, const SettingsPageSurfaceTheme* theme) {
    if (page == nullptr || theme == nullptr) {
        return;
    }
    SetWindowSubclass(page, SettingsPageSurfaceProc, 1, reinterpret_cast<DWORD_PTR>(theme));
}

}  // namespace velo::ui
