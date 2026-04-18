#pragma once

#include "tests/scenario_runner.h"

#include <Windows.h>

#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

#include "app/app_metadata.h"
#include "app/command_line.h"
#include "app/end_of_playback_policy.h"
#include "app/mpv_runtime_release_asset.h"
#include "app/media_history.h"
#include "app/media_playlist.h"
#include "app/media_source.h"
#include "app/playlist_storage.h"
#include "app/quick_browse_catalog.h"
#include "app/release_update.h"
#include "app/replay_selection_policy.h"
#include "common/hex_color.h"
#include "common/sha256.h"
#include "config/config_service.h"
#include "diagnostics/logger.h"
#include "diagnostics/performance_report.h"
#include "diagnostics/release_gate.h"
#include "diagnostics/session_recovery.h"
#include "diagnostics/startup_metrics.h"
#include "diagnostics/system_report.h"
#include "localization/localization.h"
#include "platform/win32/input_profile.h"
#include "platform/win32/input_router.h"
#include "playback/command_dispatcher.h"
#include "playback/end_file_reason.h"
#include "playback/low_latency_seek.h"
#include "playback/mpv_runtime_provenance.h"
#include "playback/seek_optimization_profile.h"
#include "playback/state_normalizer.h"
#include "playback/subtitle_layout.h"
#include "playback/video_filter_profile.h"
#include "ui/button_visuals.h"
#include "ui/combo_box_utils.h"
#include "ui/error_messages.h"
#include "ui/main_window_layout.h"
#include "ui/settings_dialog.h"
#include "ui/settings_dialog_state.h"
#include "ui/settings_page_surface.h"
#include "ui/shortcut_capture.h"
#include "ui/shortcut_display.h"

namespace velo::tests {
namespace {

inline void AppendHeadlessTrace(std::string_view message) {
    std::error_code error;
    const auto traceRoot = std::filesystem::temp_directory_path(error) / "nativplayer-tests";
    std::filesystem::create_directories(traceRoot, error);
    std::ofstream output(traceRoot / "headless-progress.log", std::ios::app);
    if (!output.is_open()) {
        return;
    }
    output << message << '\n';
}

inline void Expect(const bool condition, const std::string& message, ScenarioResult& result) {
    if (condition) {
        ++result.passed;
        result.report += "[PASS] " + message + "\n";
        return;
    }
    ++result.failed;
    result.report += "[FAIL] " + message + "\n";
}

struct ForwardingProbeState {
    int commandCount = 0;
    int drawItemCount = 0;
    int measureItemCount = 0;
};

}  // namespace

LRESULT CALLBACK ForwardingProbeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsDialogTestWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND CreateSettingsDialogTestOwner();
HWND WaitForSettingsDialogWindow();
HWND FindDescendantById(HWND parent, int controlId);
HWND WaitForDescendantById(HWND parent, int controlId, int attempts = 200, int delayMs = 10);

void RunConfigScenarios(ScenarioResult& result);
void RunSettingsScenarios(ScenarioResult& result);
void RunPlaylistScenarios(ScenarioResult& result);
void RunUiScenarios(ScenarioResult& result);
void RunDiagnosticsScenarios(ScenarioResult& result);
void RunApplicationScenarios(ScenarioResult& result);
void RunSeekOptimizationScenarios(ScenarioResult& result);

}  // namespace velo::tests
