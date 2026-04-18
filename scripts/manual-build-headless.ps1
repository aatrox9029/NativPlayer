param(
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build-clang'
$DebugDir = Join-Path $BuildDir 'debug'
$HeadlessExe = if ([string]::IsNullOrWhiteSpace($OutputPath)) { Join-Path $BuildDir 'nativplayer_headless_testkit.exe' } else { $OutputPath }
$BuildLog = Join-Path $DebugDir 'manual-headless-build.log'
$ExitFile = Join-Path $DebugDir 'manual-headless-build.exit.txt'

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
if (-not (Test-Path $DebugDir)) {
    New-Item -ItemType Directory -Path $DebugDir | Out-Null
}

$sources = @(
    'tests/headless_test_kit.cpp',
    'app/app_metadata.cpp',
    'app/command_line.cpp',
    'app/media_history.cpp',
    'app/media_file_types.cpp',
    'app/media_playlist.cpp',
    'app/media_source.cpp',
    'app/quick_browse_catalog.cpp',
    'app/playlist_storage.cpp',
    'config/app_config_defaults.cpp',
    'config/app_config_normalizer.cpp',
    'common/hex_color.cpp',
    'common/portable_executable.cpp',
    'config/config_service.cpp',
    'config/config_parser.cpp',
    'config/config_serializer.cpp',
    'diagnostics/logger.cpp',
    'diagnostics/performance_report.cpp',
    'diagnostics/release_gate.cpp',
    'diagnostics/session_recovery.cpp',
    'diagnostics/system_report.cpp',
    'diagnostics/startup_metrics.cpp',
    'localization/localization.cpp',
    'platform/win32/input_profile.cpp',
    'platform/win32/input_router.cpp',
    'playback/command_dispatcher.cpp',
    'playback/mpv_runtime_probe.cpp',
    'playback/state_normalizer.cpp',
    'playback/subtitle_layout.cpp',
    'playback/video_filter_profile.cpp',
    'ui/button_visuals.cpp',
    'ui/combo_box_utils.cpp',
    'ui/control_value_formats.cpp',
    'ui/error_messages.cpp',
    'ui/design_tokens.cpp',
    'ui/main_window_layout.cpp',
    'ui/shortcut_capture.cpp',
    'ui/shortcut_display.cpp',
    'ui/settings_dialog.cpp',
    'ui/settings_dialog_state.cpp',
    'ui/settings_dialog_runtime.cpp',
    'ui/settings_dialog_playback_page.cpp',
    'ui/settings_dialog_subtitle_page.cpp',
    'ui/settings_dialog_audio_page.cpp',
    'ui/settings_dialog_shortcuts_page.cpp',
    'ui/settings_dialog_advanced_page.cpp',
    'ui/settings_dialog_draw.cpp',
    'ui/settings_dialog_audio_outputs.cpp',
    'ui/settings_page_surface.cpp',
    'tests/scenario_runner.cpp',
    'tests/config_scenarios.cpp',
    'tests/settings_scenarios.cpp',
    'tests/playlist_scenarios.cpp',
    'tests/ui_scenarios.cpp',
    'tests/diagnostics_scenarios.cpp',
    'tests/application_scenarios.cpp',
    'ui/player_state.cpp'
)

$arguments = @(
    '/std:c++20',
    '/EHsc',
    '/DUNICODE',
    '/D_UNICODE',
    '/DWIN32_LEAN_AND_MEAN',
    '/DNOMINMAX',
    '/I', $Root,
    '/I', (Join-Path $Root 'include')
) + $sources + @(
    ('/Fe:' + $HeadlessExe),
    '/link',
    '/MACHINE:X64',
    'user32.lib',
    'gdi32.lib',
    'shell32.lib',
    'ole32.lib',
    'shlwapi.lib',
    'advapi32.lib',
    'winmm.lib',
    'msimg32.lib',
    'comdlg32.lib',
    'comctl32.lib',
    'uxtheme.lib'
)

& 'C:\Program Files\LLVM\bin\clang-cl.exe' @arguments 2>&1 | Tee-Object -FilePath $BuildLog
$exitCode = $LASTEXITCODE
Set-Content -LiteralPath $ExitFile -Value $exitCode
exit $exitCode
