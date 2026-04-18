#include "app/mpv_runtime_license_prompt.h"

#include <Windows.h>
#include <uxtheme.h>
#include <shellapi.h>

#include <filesystem>
#include <string>

#include "app/mpv_runtime_license_prompt_text.h"
#include "localization/localization.h"
#include "playback/mpv_runtime_probe.h"
#include "playback/mpv_runtime_paths.h"

namespace velo::app {
namespace {

constexpr int kAutoDownloadButtonId = 1001;
constexpr int kManualDownloadButtonId = 1002;
constexpr int kRestartButtonId = 1003;
constexpr int kCancelButtonId = 1004;
constexpr int kLanguageEnglishButtonId = 1011;
constexpr int kLanguageTraditionalButtonId = 1012;
constexpr int kLanguageSimplifiedButtonId = 1013;
constexpr wchar_t kPromptClassName[] = L"NativPlayerMpvLicensePromptWindow";
constexpr wchar_t kManualDownloadUrl[] = L"https://github.com/zhongfly/mpv-winbuild/releases/latest";

constexpr COLORREF kWindowBackground = RGB(245, 247, 250);
constexpr COLORREF kHeaderBackground = RGB(33, 92, 176);
constexpr COLORREF kPrimaryText = RGB(30, 41, 59);
constexpr COLORREF kPathBackground = RGB(255, 255, 255);
constexpr COLORREF kPathBorder = RGB(203, 213, 225);
constexpr COLORREF kErrorText = RGB(185, 28, 28);

struct PromptState {
    MpvRuntimeLicenseChoice choice = MpvRuntimeLicenseChoice::Cancel;
    std::wstring languageCode = L"en-US";
    MpvRuntimeLicensePromptText text;
    std::wstring runtimePath = velo::playback::ManagedMpvRuntimeRoot().wstring();
    HWND headline = nullptr;
    HWND body = nullptr;
    HWND pathLabel = nullptr;
    HWND pathValue = nullptr;
    HWND statusLabel = nullptr;
    HWND autoButton = nullptr;
    HWND manualButton = nullptr;
    HWND restartButton = nullptr;
    HWND cancelButton = nullptr;
    HWND englishButton = nullptr;
    HWND traditionalButton = nullptr;
    HWND simplifiedButton = nullptr;
    HFONT uiFont = nullptr;
    HBRUSH backgroundBrush = CreateSolidBrush(kWindowBackground);
    HBRUSH pathBrush = CreateSolidBrush(kPathBackground);
};

HFONT CreatePromptFont(const int pointSize, const int weight) {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
    LOGFONTW font = metrics.lfMessageFont;

    HDC desktopDc = GetDC(nullptr);
    const int dpi = desktopDc != nullptr ? GetDeviceCaps(desktopDc, LOGPIXELSY) : 96;
    if (desktopDc != nullptr) {
        ReleaseDC(nullptr, desktopDc);
    }

    font.lfHeight = -MulDiv(pointSize, dpi, 72);
    font.lfWeight = weight;
    wcscpy_s(font.lfFaceName, L"Segoe UI");
    return CreateFontIndirectW(&font);
}

bool IsRuntimeAvailableNow() {
    const auto runtimePath = velo::playback::ManagedMpvRuntimePath(L"libmpv-2.dll");
    const auto probe = velo::playback::ProbeMpvRuntimeLibrary(runtimePath, L"libmpv-2.dll");
    return probe.found && !probe.architectureMismatch && probe.libraryImage.valid;
}

std::wstring PromptLanguageButtonLabel(const std::wstring_view languageCode) {
    if (languageCode == L"zh-TW") {
        return L"\u7E41\u9AD4\u4E2D\u6587";
    }
    if (languageCode == L"zh-CN") {
        return L"\u7B80\u4F53\u4E2D\u6587";
    }
    return L"English";
}

void SetStatusMessage(PromptState& state, const std::wstring& message) {
    if (state.statusLabel != nullptr) {
        SetWindowTextW(state.statusLabel, message.c_str());
        ShowWindow(state.statusLabel, message.empty() ? SW_HIDE : SW_SHOW);
        RedrawWindow(state.statusLabel, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void OpenManualDownloadResources() {
    ShellExecuteW(nullptr, L"open", kManualDownloadUrl, nullptr, nullptr, SW_SHOWNORMAL);

    const auto runtimeFolder = velo::playback::ManagedMpvRuntimeRoot();
    std::error_code error;
    std::filesystem::create_directories(runtimeFolder, error);
    ShellExecuteW(nullptr, L"open", runtimeFolder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void ApplyFont(const HWND hwnd, const HFONT font) {
    if (hwnd != nullptr && font != nullptr) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

void ApplyWindowThemeIfPresent(const HWND hwnd) {
    if (hwnd != nullptr) {
        SetWindowTheme(hwnd, L"Explorer", nullptr);
    }
}

void UpdatePromptTexts(HWND hwnd, PromptState& state) {
    state.text = BuildMpvRuntimeLicensePromptText(state.languageCode);
    SetWindowTextW(hwnd, state.text.windowTitle.c_str());
    SetWindowTextW(state.body, state.text.body.c_str());
    SetWindowTextW(state.pathLabel, state.text.pathLabel.c_str());
    SetWindowTextW(state.autoButton, state.text.autoButton.c_str());
    SetWindowTextW(state.manualButton, state.text.manualButton.c_str());
    SetWindowTextW(state.restartButton, state.text.restartButton.c_str());
    SetWindowTextW(state.cancelButton, state.text.cancelButton.c_str());
    SetWindowTextW(state.englishButton, PromptLanguageButtonLabel(L"en-US").c_str());
    SetWindowTextW(state.traditionalButton, PromptLanguageButtonLabel(L"zh-TW").c_str());
    SetWindowTextW(state.simplifiedButton, PromptLanguageButtonLabel(L"zh-CN").c_str());

    const bool statusVisible = state.statusLabel != nullptr && IsWindowVisible(state.statusLabel) != FALSE;
    if (statusVisible) {
        SetWindowTextW(state.statusLabel, state.text.runtimeMissingStatus.c_str());
    }
    if (state.englishButton != nullptr) {
        EnableWindow(state.englishButton, state.languageCode != L"en-US");
    }
    if (state.traditionalButton != nullptr) {
        EnableWindow(state.traditionalButton, state.languageCode != L"zh-TW");
    }
    if (state.simplifiedButton != nullptr) {
        EnableWindow(state.simplifiedButton, state.languageCode != L"zh-CN");
    }
    RedrawWindow(hwnd, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void CreatePromptFonts(PromptState& state) {
    state.uiFont = CreatePromptFont(10, FW_NORMAL);
}

void CreatePromptControls(const HWND hwnd, PromptState& state) {
    constexpr int kWindowWidth = 700;
    constexpr int kMargin = 24;
    constexpr int kHeaderHeight = 72;
    constexpr int kBodyTop = 92;
    constexpr int kContentWidth = kWindowWidth - (kMargin * 2);
    constexpr int kLanguageButtonWidth = 118;
    constexpr int kLanguageButtonHeight = 30;
    constexpr int kLanguageButtonGap = 10;
    const int languageRowWidth = (kLanguageButtonWidth * 3) + (kLanguageButtonGap * 2);
    const int languageRowLeft = kWindowWidth - kMargin - languageRowWidth;

    state.body = CreateWindowExW(
        0, L"STATIC", state.text.body.c_str(), WS_CHILD | WS_VISIBLE, kMargin, kBodyTop, kContentWidth, 48, hwnd, nullptr, nullptr, nullptr);

    state.pathLabel = CreateWindowExW(
        0, L"STATIC", state.text.pathLabel.c_str(), WS_CHILD | WS_VISIBLE, kMargin, 148, kContentWidth, 22, hwnd, nullptr, nullptr, nullptr);

    state.pathValue = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state.runtimePath.c_str(),
                                      WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, kMargin, 174, kContentWidth, 28, hwnd, nullptr,
                                      nullptr, nullptr);

    state.statusLabel = CreateWindowExW(
        0, L"STATIC", L"", WS_CHILD, kMargin, 212, kContentWidth, 22, hwnd, nullptr, nullptr, nullptr);

    state.englishButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                          languageRowLeft, 20, kLanguageButtonWidth, kLanguageButtonHeight, hwnd,
                                          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLanguageEnglishButtonId)), nullptr, nullptr);
    state.traditionalButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                              languageRowLeft + kLanguageButtonWidth + kLanguageButtonGap, 20, kLanguageButtonWidth,
                                              kLanguageButtonHeight, hwnd,
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLanguageTraditionalButtonId)), nullptr, nullptr);
    state.simplifiedButton = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                             languageRowLeft + ((kLanguageButtonWidth + kLanguageButtonGap) * 2), 20, kLanguageButtonWidth,
                                             kLanguageButtonHeight, hwnd,
                                             reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLanguageSimplifiedButtonId)), nullptr, nullptr);

    state.autoButton = CreateWindowExW(0, L"BUTTON", state.text.autoButton.c_str(),
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | BS_FLAT, kMargin, 248, 248, 36, hwnd,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAutoDownloadButtonId)), nullptr, nullptr);

    state.manualButton = CreateWindowExW(0, L"BUTTON", state.text.manualButton.c_str(),
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT, kMargin + 260, 248, 156, 36, hwnd,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(kManualDownloadButtonId)), nullptr, nullptr);

    state.restartButton = CreateWindowExW(0, L"BUTTON", state.text.restartButton.c_str(),
                                          WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT, kMargin + 428, 248, 108, 36, hwnd,
                                          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRestartButtonId)), nullptr, nullptr);

    state.cancelButton = CreateWindowExW(0, L"BUTTON", state.text.cancelButton.c_str(),
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT, kWindowWidth - kMargin - 92, 248, 92, 36,
                                         hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelButtonId)), nullptr, nullptr);

    ApplyFont(state.body, state.uiFont);
    ApplyFont(state.pathLabel, state.uiFont);
    ApplyFont(state.pathValue, state.uiFont);
    ApplyFont(state.statusLabel, state.uiFont);
    ApplyFont(state.autoButton, state.uiFont);
    ApplyFont(state.manualButton, state.uiFont);
    ApplyFont(state.restartButton, state.uiFont);
    ApplyFont(state.cancelButton, state.uiFont);
    ApplyFont(state.englishButton, state.uiFont);
    ApplyFont(state.traditionalButton, state.uiFont);
    ApplyFont(state.simplifiedButton, state.uiFont);

    ApplyWindowThemeIfPresent(state.autoButton);
    ApplyWindowThemeIfPresent(state.manualButton);
    ApplyWindowThemeIfPresent(state.restartButton);
    ApplyWindowThemeIfPresent(state.cancelButton);
    ApplyWindowThemeIfPresent(state.englishButton);
    ApplyWindowThemeIfPresent(state.traditionalButton);
    ApplyWindowThemeIfPresent(state.simplifiedButton);
    SendMessageW(state.pathValue, EM_SETSEL, 0, 0);
    ShowWindow(state.statusLabel, SW_HIDE);
    ShowWindow(state.restartButton, SW_HIDE);
    UpdatePromptTexts(hwnd, state);
}

void DeleteGdiObjectIfPresent(HGDIOBJ object) {
    if (object != nullptr) {
        DeleteObject(object);
    }
}

void PaintPrompt(const HWND hwnd, const PromptState& state) {
    PAINTSTRUCT paint{};
    const HDC dc = BeginPaint(hwnd, &paint);
    if (dc == nullptr) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd, &client);
    FillRect(dc, &client, state.backgroundBrush);

    RECT headerRect = client;
    headerRect.bottom = 72;
    HBRUSH headerBrush = CreateSolidBrush(kHeaderBackground);
    FillRect(dc, &headerRect, headerBrush);
    DeleteObject(headerBrush);

    HPEN dividerPen = CreatePen(PS_SOLID, 1, RGB(226, 232, 240));
    HGDIOBJ oldPen = SelectObject(dc, dividerPen);
    MoveToEx(dc, 24, 236, nullptr);
    LineTo(dc, client.right - 24, 236);
    if (oldPen != nullptr) {
        SelectObject(dc, oldPen);
    }
    DeleteObject(dividerPen);

    RECT pathRect{};
    if (state.pathValue != nullptr && GetWindowRect(state.pathValue, &pathRect) != FALSE) {
        MapWindowPoints(HWND_DESKTOP, hwnd, reinterpret_cast<LPPOINT>(&pathRect), 2);
        HBRUSH borderBrush = CreateSolidBrush(kPathBorder);
        FrameRect(dc, &pathRect, borderBrush);
        DeleteObject(borderBrush);
    }

    EndPaint(hwnd, &paint);
}

LRESULT CALLBACK PromptWindowProc(const HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    auto* state = reinterpret_cast<PromptState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message) {
        case WM_NCCREATE: {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return TRUE;
        }
        case WM_CREATE:
            if (state != nullptr) {
                CreatePromptFonts(*state);
                CreatePromptControls(hwnd, *state);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_CTLCOLORSTATIC:
            if (state == nullptr) {
                break;
            }
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            if (reinterpret_cast<HWND>(lParam) == state->statusLabel) {
                SetTextColor(reinterpret_cast<HDC>(wParam), kErrorText);
            } else {
                SetTextColor(reinterpret_cast<HDC>(wParam), kPrimaryText);
            }
            return reinterpret_cast<INT_PTR>(state->backgroundBrush);
        case WM_CTLCOLOREDIT:
            if (state != nullptr && reinterpret_cast<HWND>(lParam) == state->pathValue) {
                SetBkColor(reinterpret_cast<HDC>(wParam), kPathBackground);
                SetTextColor(reinterpret_cast<HDC>(wParam), kPrimaryText);
                return reinterpret_cast<INT_PTR>(state->pathBrush);
            }
            break;
        case WM_PAINT:
            if (state != nullptr) {
                PaintPrompt(hwnd, *state);
                return 0;
            }
            break;
        case WM_COMMAND:
            if (state == nullptr) {
                return 0;
            }
            switch (LOWORD(wParam)) {
                case kAutoDownloadButtonId:
                    state->choice = MpvRuntimeLicenseChoice::AutoDownload;
                    DestroyWindow(hwnd);
                    return 0;
                case kLanguageEnglishButtonId:
                    state->languageCode = L"en-US";
                    UpdatePromptTexts(hwnd, *state);
                    return 0;
                case kLanguageTraditionalButtonId:
                    state->languageCode = L"zh-TW";
                    UpdatePromptTexts(hwnd, *state);
                    return 0;
                case kLanguageSimplifiedButtonId:
                    state->languageCode = L"zh-CN";
                    UpdatePromptTexts(hwnd, *state);
                    return 0;
                case kManualDownloadButtonId:
                    OpenManualDownloadResources();
                    ShowWindow(state->restartButton, SW_SHOW);
                    SetStatusMessage(*state, L"");
                    return 0;
                case kRestartButtonId:
                    if (IsRuntimeAvailableNow()) {
                        state->choice = MpvRuntimeLicenseChoice::RuntimeReady;
                        DestroyWindow(hwnd);
                        return 0;
                    }
                    SetStatusMessage(*state, state->text.runtimeMissingStatus);
                    return 0;
                case kCancelButtonId:
                    state->choice = MpvRuntimeLicenseChoice::Cancel;
                    DestroyWindow(hwnd);
                    return 0;
                default:
                    return 0;
            }
        case WM_CLOSE:
            if (state != nullptr) {
                state->choice = MpvRuntimeLicenseChoice::Cancel;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            return 0;
        case WM_NCDESTROY:
            if (state != nullptr) {
                DeleteGdiObjectIfPresent(state->uiFont);
                DeleteGdiObjectIfPresent(state->backgroundBrush);
                DeleteGdiObjectIfPresent(state->pathBrush);
            }
            return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

ATOM EnsurePromptClassRegistered() {
    static ATOM atom = 0;
    if (atom != 0) {
        return atom;
    }

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = PromptWindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(windowClass.hInstance, MAKEINTRESOURCEW(101));
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kPromptClassName;
    atom = RegisterClassW(&windowClass);
    return atom;
}

RECT CenteredWindowRect(const int width, const int height) {
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int left = workArea.left + ((workArea.right - workArea.left) - width) / 2;
    const int top = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;
    return RECT{left, top, left + width, top + height};
}

MpvRuntimeLicenseChoice ShowFallbackMessageBox(const std::wstring_view languageCode) {
    const auto text = BuildMpvRuntimeLicensePromptText(languageCode);
    const int result = MessageBoxW(nullptr, text.fallbackText.c_str(), text.windowTitle.c_str(), MB_ICONINFORMATION | MB_YESNOCANCEL);
    if (result == IDYES) {
        return MpvRuntimeLicenseChoice::AutoDownload;
    }
    if (result == IDNO) {
        OpenManualDownloadResources();
        return MpvRuntimeLicenseChoice::Cancel;
    }
    return MpvRuntimeLicenseChoice::Cancel;
}

}  // namespace

MpvRuntimeLicenseChoice PromptForMpvRuntimeDownloadConsent(std::wstring& languageCode) {
    if (EnsurePromptClassRegistered() == 0) {
        return ShowFallbackMessageBox(languageCode);
    }

    constexpr int kWindowWidth = 700;
    constexpr int kWindowHeight = 340;
    PromptState state;
    state.languageCode = languageCode;
    state.text = BuildMpvRuntimeLicensePromptText(state.languageCode);

    const RECT rect = CenteredWindowRect(kWindowWidth, kWindowHeight);
    const HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kPromptClassName, state.text.windowTitle.c_str(),
                                        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_VISIBLE, rect.left, rect.top, kWindowWidth, kWindowHeight,
                                        nullptr, nullptr, GetModuleHandleW(nullptr), &state);
    if (window == nullptr) {
        return ShowFallbackMessageBox(languageCode);
    }

    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);

    MSG message{};
    while (IsWindow(window) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    languageCode = state.languageCode;
    return state.choice;
}

}  // namespace velo::app
