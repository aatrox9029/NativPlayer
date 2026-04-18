#include "tests/scenario_runner_internal.h"

namespace velo::tests {

LRESULT CALLBACK ForwardingProbeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<ForwardingProbeState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message) {
        case WM_NCCREATE: {
            const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            return TRUE;
        }
        case WM_COMMAND:
            if (state != nullptr) {
                ++state->commandCount;
            }
            return 101;
        case WM_DRAWITEM:
            if (state != nullptr) {
                ++state->drawItemCount;
            }
            return 102;
        case WM_MEASUREITEM:
            if (state != nullptr) {
                ++state->measureItemCount;
            }
            return 103;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK SettingsDialogTestWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        return TRUE;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

HWND CreateSettingsDialogTestOwner() {
    constexpr wchar_t kOwnerClassName[] = L"NativPlayerSettingsDialogTestOwner";
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = SettingsDialogTestWindowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = kOwnerClassName;
        RegisterClassW(&windowClass);
        classRegistered = true;
    }
    return CreateWindowExW(0, kOwnerClassName, L"", WS_OVERLAPPED, 0, 0, 16, 16, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
}

HWND WaitForSettingsDialogWindow() {
    for (int attempt = 0; attempt < 200; ++attempt) {
        HWND hwnd = FindWindowW(velo::ui::SettingsDialogClassNameForTesting(), nullptr);
        if (hwnd != nullptr && velo::ui::SettingsDialogWindowReadyForTesting(hwnd)) {
            return hwnd;
        }
        Sleep(10);
    }
    return nullptr;
}

namespace {

struct DescendantLookupState {
    int controlId = 0;
    HWND result = nullptr;
};

BOOL CALLBACK FindDescendantByIdProc(HWND hwnd, LPARAM lParam) {
    auto* state = reinterpret_cast<DescendantLookupState*>(lParam);
    if (state == nullptr || state->result != nullptr) {
        return FALSE;
    }
    if (GetDlgCtrlID(hwnd) == state->controlId) {
        state->result = hwnd;
        return FALSE;
    }
    return TRUE;
}

}  // namespace

HWND FindDescendantById(HWND parent, const int controlId) {
    if (parent == nullptr) {
        return nullptr;
    }
    DescendantLookupState state{.controlId = controlId};
    EnumChildWindows(parent, FindDescendantByIdProc, reinterpret_cast<LPARAM>(&state));
    return state.result;
}

HWND WaitForDescendantById(HWND parent, const int controlId, const int attempts, const int delayMs) {
    for (int attempt = 0; attempt < attempts; ++attempt) {
        HWND child = FindDescendantById(parent, controlId);
        if (child != nullptr) {
            return child;
        }
        Sleep(delayMs);
    }
    return nullptr;
}

ScenarioResult RunAllScenarios() {
    ScenarioResult result;
    RunConfigScenarios(result);
    RunSettingsScenarios(result);
    RunPlaylistScenarios(result);
    RunUiScenarios(result);
    RunDiagnosticsScenarios(result);
    RunApplicationScenarios(result);
    RunSeekOptimizationScenarios(result);
    return result;
}

}  // namespace velo::tests
