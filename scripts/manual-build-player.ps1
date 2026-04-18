param()

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build-clang'
$DebugDir = Join-Path $BuildDir 'debug'
$PlayerExe = Join-Path $BuildDir 'nativplayer.exe'
$PlayerExeArg = '.\build-clang\nativplayer.exe'
$IconSyncScript = Join-Path $PSScriptRoot 'sync-app-icon.ps1'
$IconResourceScript = Join-Path $Root 'resources\app_icon.rc'
$IconResourceOutput = Join-Path $BuildDir 'nativplayer_icon.res'
$BuildLogo = Join-Path $BuildDir 'logo.png'
$BuildLog = Join-Path $DebugDir 'manual-player-build.log'
$ExitFile = Join-Path $DebugDir 'manual-player-build.exit.txt'

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
if (-not (Test-Path $DebugDir)) {
    New-Item -ItemType Directory -Path $DebugDir | Out-Null
}

$sources = @(
    'app/app_metadata.cpp',
    'app/application.cpp',
    'app/application_bootstrap.cpp',
    'app/mpv_runtime_license_prompt.cpp',
    'app/mpv_runtime_release_asset.cpp',
    'app/mpv_runtime_bootstrap.cpp',
    'app/application_open_commands.cpp',
    'app/application_playlist_runtime.cpp',
    'app/application_screenshot.cpp',
    'app/application_state_sync.cpp',
    'app/application_diagnostics.cpp',
    'app/command_line.cpp',
    'app/media_history.cpp',
    'app/media_file_types.cpp',
    'app/media_playlist.cpp',
    'app/media_source.cpp',
    'app/quick_browse_catalog.cpp',
    'app/main.cpp',
    'app/playlist_storage.cpp',
    'app/single_instance.cpp',
    'config/app_config_defaults.cpp',
    'config/app_config_normalizer.cpp',
    'common/hex_color.cpp',
    'common/http_download.cpp',
    'common/portable_executable.cpp',
    'common/sha256.cpp',
    'config/config_service.cpp',
    'config/config_parser.cpp',
    'config/config_serializer.cpp',
    'diagnostics/crash_dump.cpp',
    'diagnostics/logger.cpp',
    'diagnostics/performance_report.cpp',
    'diagnostics/release_gate.cpp',
    'diagnostics/session_recovery.cpp',
    'diagnostics/system_report.cpp',
    'diagnostics/startup_metrics.cpp',
    'localization/localization.cpp',
    'platform/win32/input_profile.cpp',
    'platform/win32/input_router.cpp',
    'platform/win32/window_host.cpp',
    'playback/command_dispatcher.cpp',
    'playback/mpv_runtime_probe.cpp',
    'playback/mpv_runtime_provenance.cpp',
    'playback/mpv_runtime_paths.cpp',
    'playback/mpv_loader.cpp',
    'playback/mpv_player.cpp',
    'playback/player_thread.cpp',
    'playback/state_normalizer.cpp',
    'playback/subtitle_layout.cpp',
    'playback/video_filter_profile.cpp',
    'ui/button_visuals.cpp',
    'ui/button_style_assets.cpp',
    'ui/combo_box_utils.cpp',
    'ui/app_icon.cpp',
    'ui/control_value_formats.cpp',
    'ui/error_messages.cpp',
    'ui/design_tokens.cpp',
    'ui/main_window.cpp',
    'ui/main_window_layout.cpp',
    'ui/main_window_messages.cpp',
    'ui/main_window_controls.cpp',
    'ui/main_window_layout_runtime.cpp',
    'ui/main_window_menu.cpp',
    'ui/main_window_overlays.cpp',
    'ui/main_window_quick_browse.cpp',
    'ui/main_window_draw.cpp',
    'ui/player_state.cpp',
    'ui/preview_session.cpp',
    'ui/quick_browse_panel.cpp',
    'ui/seek_preview_popup.cpp',
    'ui/shortcut_capture.cpp',
    'ui/shell_thumbnail_provider.cpp',
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
    'ui/themed_slider.cpp',
    'ui/themed_scrollbar.cpp'
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
    ('/Fe:' + $PlayerExeArg),
    '/link',
    '/MACHINE:X64',
    'user32.lib',
    'gdi32.lib',
    'gdiplus.lib',
    'comdlg32.lib',
    'comctl32.lib',
    'shell32.lib',
    'ole32.lib',
    'advapi32.lib',
    'shlwapi.lib',
    'dbghelp.lib',
    'bcrypt.lib',
    'winhttp.lib',
    'winmm.lib',
    'msimg32.lib',
    'uxtheme.lib'
)

Push-Location $Root
try {
    & powershell -ExecutionPolicy Bypass -File $IconSyncScript | Out-Null
    & 'C:\Program Files\LLVM\bin\llvm-rc.exe' /fo $IconResourceOutput $IconResourceScript
    $linkIndex = [Array]::IndexOf($arguments, '/link')
    if ($linkIndex -lt 0) {
        throw 'Unable to locate /link marker in manual player build arguments.'
    }
    $argumentsWithResources = $arguments[0..($linkIndex - 1)] + @($IconResourceOutput) + $arguments[$linkIndex..($arguments.Length - 1)]
    & 'C:\Program Files\LLVM\bin\clang-cl.exe' @argumentsWithResources 2>&1 | Tee-Object -FilePath $BuildLog
    $exitCode = $LASTEXITCODE
    Set-Content -LiteralPath $ExitFile -Value $exitCode
    if ($exitCode -ne 0) {
        exit $exitCode
    }
} finally {
    Pop-Location
}

$runtimeMpvPath = Join-Path $Root 'runtime\mpv-2.dll'
if (Test-Path $runtimeMpvPath) {
    Copy-Item $runtimeMpvPath (Join-Path $BuildDir 'mpv-2.dll') -Force
}
Remove-Item -LiteralPath $BuildLogo -Force -ErrorAction SilentlyContinue

$buildButtonStylePath = Join-Path $BuildDir 'button-style'
$sourceButtonStylePath = Join-Path $Root 'button-style'
if (Test-Path $sourceButtonStylePath) {
    Remove-Item -LiteralPath $buildButtonStylePath -Recurse -Force -ErrorAction SilentlyContinue
    Copy-Item -LiteralPath $sourceButtonStylePath -Destination $buildButtonStylePath -Recurse -Force
} else {
    Remove-Item -LiteralPath $buildButtonStylePath -Recurse -Force -ErrorAction SilentlyContinue
}
