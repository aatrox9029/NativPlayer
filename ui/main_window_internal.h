#pragma once

#include "ui/main_window.h"

#include <Windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string_view>

#include "app/app_metadata.h"
#include "app/media_playlist.h"
#include "app/quick_browse_catalog.h"
#include "common/text_encoding.h"
#include "localization/localization.h"
#include "ui/app_icon.h"
#include "ui/button_visuals.h"
#include "ui/control_value_formats.h"
#include "ui/design_tokens.h"
#include "ui/error_messages.h"
#include "ui/main_window_layout.h"
#include "ui/shortcut_display.h"
#include "ui/window_placement_utils.h"

namespace velo::ui {
namespace {

std::wstring ShortcutForAction(const velo::config::AppConfig& config, std::wstring_view actionId) {
    return ShortcutDisplayName(config, actionId);
}

LRESULT CALLBACK EditInputSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (message) {
        case WM_GETDLGCODE:
            return DefSubclassProc(hwnd, message, wParam, lParam) | DLGC_WANTALLKEYS;
        case WM_KEYDOWN:
        case WM_CHAR:
            if (wParam == VK_RETURN) {
                return 0;
            }
            break;
        default:
            break;
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

constexpr wchar_t kMainWindowClassName[] = L"NativPlayerMainWindow";
constexpr int kControlHeight = 50;
constexpr int kSeekHeight = 32;
constexpr int kPanelHeight = 156;
constexpr int kStatusHeight = 40;
constexpr int kInfoPanelWidth = 420;
constexpr UINT kMenuSeparatorBase = 50000;
constexpr int kSpeedMinimum = 25;
constexpr int kSpeedMaximum = 300;
constexpr int kEmptyDropZoneWidth = 440;
constexpr int kEmptyDropZoneHeight = 136;
constexpr int kMinimumWindowWidth = 960;
constexpr int kMinimumWindowHeight = 600;
constexpr int kWindowResizeBorder = 8;
constexpr int kFullscreenCaptionButtonGap = 2;
constexpr UINT kPopupEndActionMenuCommand = 56001;
constexpr UINT kPopupAudioTrackMenuCommand = 56002;
constexpr UINT kPopupSubtitleTrackMenuCommand = 56003;
constexpr UINT kPopupAudioOutputMenuCommand = 56004;
constexpr UINT kDynamicAudioTrackCommandBase = 57000;
constexpr UINT kDynamicSubtitleTrackCommandBase = 57200;
constexpr UINT kDynamicAudioOutputCommandBase = 57400;

enum ControlId {
    kControlOpen = 1001,
    kControlRecent = 1002,
    kControlQuickBrowse = 10018,
    kControlPreviousTrack = 1018,
    kControlPlayPause = 1003,
    kControlNextTrack = 1019,
    kControlMute = 1004,
    kControlSpeed = 1005,
    kControlAudio = 1006,
    kControlSubtitle = 1007,
    kControlFullscreen = 1008,
    kControlSeek = 1009,
    kControlVolume = 1010,
    kControlReplay = 1011,
    kControlEndNext = 1012,
    kControlSettings = 1013,
    kControlMore = 1014,
    kControlVolumeEdit = 1015,
    kControlSpeedSlider = 1016,
    kControlSpeedEdit = 1017,
    kControlVolumeToggle = 1020,
    kControlFullscreenDownload = 1024,
    kControlFullscreenMinimize = 1021,
    kControlFullscreenWindowed = 1022,
    kControlFullscreenClose = 1023,
};

inline ButtonKind ButtonKindFromControlId(const UINT controlId) {
    switch (controlId) {
        case kControlOpen:
            return ButtonKind::OpenFile;
        case kControlRecent:
            return ButtonKind::RecentFiles;
        case kControlQuickBrowse:
            return ButtonKind::QuickBrowse;
        case kControlPreviousTrack:
            return ButtonKind::PreviousItem;
        case kControlPlayPause:
            return ButtonKind::PlayPause;
        case kControlNextTrack:
            return ButtonKind::NextPlaybackItem;
        case kControlMute:
        case kControlVolumeToggle:
            return ButtonKind::Mute;
        case kControlSpeed:
            return ButtonKind::Speed;
        case kControlAudio:
            return ButtonKind::AudioTrack;
        case kControlSubtitle:
            return ButtonKind::SubtitleTrack;
        case kControlSettings:
            return ButtonKind::Settings;
        case kControlMore:
            return ButtonKind::More;
        case kControlFullscreen:
            return ButtonKind::Fullscreen;
        case kControlFullscreenDownload:
            return ButtonKind::Download;
        case kControlReplay:
            return ButtonKind::Replay;
        case kControlEndNext:
            return ButtonKind::NextItem;
        default:
            return ButtonKind::OpenFile;
    }
}

inline std::wstring Utf8ToWide(const std::string& value) {
    return velo::common::Utf8ToWide(value);
}

inline HBRUSH BackgroundBrush() {
    static HBRUSH brush = CreateSolidBrush(tokens::DarkPalette().bgCanvas);
    return brush;
}

inline HBRUSH InputBrush() {
    static HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    return brush;
}

inline HBRUSH ControlsPanelBrush() {
    static HBRUSH brush = CreateSolidBrush(tokens::DarkPalette().bgSurface1);
    return brush;
}

inline HBRUSH MenuBrush() {
    static HBRUSH brush = CreateSolidBrush(tokens::DarkPalette().bgSurface1);
    return brush;
}

inline DWORD MainWindowStyle() {
    return WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN;
}

inline DWORD FullscreenWindowStyle() {
    return WS_POPUP | WS_CLIPCHILDREN;
}

inline bool IsControlCurrentlyVisible(HWND control) {
    return control != nullptr && (GetWindowLongPtrW(control, GWL_STYLE) & WS_VISIBLE) != 0;
}

inline void SetControlVisibility(HWND control, const bool visible) {
    if (control == nullptr) {
        return;
    }
    if (IsControlCurrentlyVisible(control) == visible) {
        return;
    }
    ShowWindow(control, visible ? SW_SHOWNA : SW_HIDE);
}

inline void SetControlTextIfChanged(HWND control, const std::wstring& value) {
    if (control == nullptr) {
        return;
    }

    const int length = GetWindowTextLengthW(control);
    std::wstring current(length, L'\0');
    if (length > 0) {
        GetWindowTextW(control, current.data(), length + 1);
    }
    if (current == value) {
        return;
    }
    SetWindowTextW(control, value.c_str());
}

inline ATOM RegisterClass(HINSTANCE instance, HICON windowIcon) {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = MainWindow::WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kMainWindowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = windowIcon != nullptr ? windowIcon : LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground = BackgroundBrush();
    atom = ::RegisterClassW(&windowClass);
    return atom;
}

inline void SetControlFont(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

inline std::wstring FormatSpeedLabel(const double speed) {
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(2) << speed << L"x";
    return stream.str();
}

inline std::wstring BuildMediaTitleText(const PlayerState& state) {
    if (!state.mediaTitle.empty()) {
        return Utf8ToWide(state.mediaTitle);
    }
    if (!state.currentPath.empty()) {
        return velo::app::ShortDisplayName(Utf8ToWide(state.currentPath));
    }
    return app::AppDisplayName();
}

inline POINT ContextMenuPointFromLParam(const LPARAM lParam) {
    POINT point{};
    if (lParam == -1) {
        GetCursorPos(&point);
        return point;
    }
    point.x = GET_X_LPARAM(lParam);
    point.y = GET_Y_LPARAM(lParam);
    return point;
}

inline bool IsPointInsideWindowRect(HWND window, const POINT screenPoint) {
    if (window == nullptr) {
        return false;
    }

    RECT rect{};
    return GetWindowRect(window, &rect) != FALSE && PtInRect(&rect, screenPoint) != FALSE;
}

}  // namespace
}  // namespace velo::ui

#define titleBarBackground_ headerControls_.titleBarBackground
#define panelBackground_ headerControls_.panelBackground
#define titleLabel_ headerControls_.titleLabel
#define openButton_ headerControls_.openButton
#define recentButton_ headerControls_.recentButton
#define quickBrowseButton_ headerControls_.quickBrowseButton
#define previousButton_ headerControls_.previousButton
#define playPauseButton_ headerControls_.playPauseButton
#define nextTrackButton_ headerControls_.nextTrackButton
#define muteButton_ headerControls_.muteButton
#define speedButton_ headerControls_.speedButton
#define subtitleButton_ headerControls_.subtitleButton
#define settingsButton_ headerControls_.settingsButton
#define moreButton_ headerControls_.moreButton
#define fullscreenButton_ headerControls_.fullscreenButton
#define currentTimeLabel_ headerControls_.currentTimeLabel
#define durationLabel_ headerControls_.durationLabel
#define mediaInfoLabel_ headerControls_.mediaInfoLabel
#define volumeLabel_ footerControls_.volumeLabel
#define speedLabel_ footerControls_.speedLabel
#define volumeEdit_ footerControls_.volumeEdit
#define speedEdit_ footerControls_.speedEdit
#define statusLabel_ footerControls_.statusLabel
#define emptyTitleLabel_ emptyStateControls_.emptyTitleLabel
#define emptyHintLabel_ emptyStateControls_.emptyHintLabel
#define emptyRecentLabel_ emptyStateControls_.emptyRecentLabel
#define osdLabel_ overlayControls_.osdLabel
#define endPromptTitleLabel_ overlayControls_.endPromptTitleLabel
#define endPromptHintLabel_ overlayControls_.endPromptHintLabel
#define replayButton_ overlayControls_.replayButton
#define nextButton_ overlayControls_.nextButton
#define fullscreenDownloadButton_ fullscreenCaptionControls_.downloadButton
#define fullscreenMinimizeButton_ fullscreenCaptionControls_.minimizeButton
#define fullscreenWindowedButton_ fullscreenCaptionControls_.windowedButton
#define fullscreenCloseButton_ fullscreenCaptionControls_.closeButton
