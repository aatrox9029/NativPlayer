cmake_minimum_required(VERSION 3.21)

project(NativPlayer LANGUAGES CXX)
enable_language(RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(FATAL_ERROR "NativPlayer is Windows-only and officially supports Windows 10/11 x64.")
endif()

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    if(NOT DEFINED CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "")
        message(FATAL_ERROR
            "Visual Studio builds must be configured explicitly for Win64. Re-run CMake with -A x64.")
    endif()

    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" VELO_GENERATOR_PLATFORM)
    if(NOT VELO_GENERATOR_PLATFORM STREQUAL "x64")
        message(FATAL_ERROR "NativPlayer only supports Win64 builds. Re-run CMake with -A x64.")
    endif()
endif()

if(CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "NativPlayer only supports 64-bit Windows builds.")
endif()

add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

set(VELO_RUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
set(VELO_RUNTIME_WIN64_ROOT "${VELO_RUNTIME_ROOT}/win64")
set(VELO_LIBMPV_DLL "")
set(VELO_MPV_DLL "")

if(EXISTS "${VELO_RUNTIME_WIN64_ROOT}/libmpv-2.dll")
    set(VELO_LIBMPV_DLL "${VELO_RUNTIME_WIN64_ROOT}/libmpv-2.dll")
elseif(EXISTS "${VELO_RUNTIME_ROOT}/libmpv-2.dll")
    set(VELO_LIBMPV_DLL "${VELO_RUNTIME_ROOT}/libmpv-2.dll")
endif()

if(EXISTS "${VELO_RUNTIME_WIN64_ROOT}/mpv-2.dll")
    set(VELO_MPV_DLL "${VELO_RUNTIME_WIN64_ROOT}/mpv-2.dll")
elseif(EXISTS "${VELO_RUNTIME_ROOT}/mpv-2.dll")
    set(VELO_MPV_DLL "${VELO_RUNTIME_ROOT}/mpv-2.dll")
endif()

set(VELO_PLAYER_SOURCES
    app/app_metadata.cpp
    app/application.cpp
    app/application_bootstrap.cpp
    app/application_update.cpp
    app/mpv_runtime_license_prompt.cpp
    app/mpv_runtime_license_prompt_text.cpp
    app/mpv_runtime_release_asset.cpp
    app/mpv_runtime_bootstrap.cpp
    app/application_open_commands.cpp
    app/application_playlist_runtime.cpp
    app/application_screenshot.cpp
    app/application_state_sync.cpp
    app/application_diagnostics.cpp
    app/end_of_playback_policy.cpp
    app/replay_selection_policy.cpp
    app/command_line.cpp
    app/media_history.cpp
    app/media_file_types.cpp
    app/media_playlist.cpp
    app/media_source.cpp
    app/quick_browse_catalog.cpp
    app/main.cpp
    app/playlist_storage.cpp
    app/runtime_playlist.cpp
    app/release_update.cpp
    app/single_instance.cpp
    config/app_config_defaults.cpp
    config/default_language_hint.cpp
    config/app_config_normalizer.cpp
    config/subtitle_position_utils.cpp
    common/hex_color.cpp
    common/http_download.cpp
    common/portable_executable.cpp
    common/sha256.cpp
    config/config_service.cpp
    config/config_parser.cpp
    config/config_serializer.cpp
    diagnostics/crash_dump.cpp
    diagnostics/logger.cpp
    diagnostics/performance_report.cpp
    diagnostics/process_memory.cpp
    diagnostics/release_gate.cpp
    diagnostics/session_recovery.cpp
    diagnostics/system_report.cpp
    diagnostics/startup_metrics.cpp
    localization/localization.cpp
    platform/win32/input_profile.cpp
    platform/win32/input_router.cpp
    platform/win32/window_host.cpp
    playback/command_dispatcher.cpp
    playback/end_file_reason.cpp
    playback/low_latency_seek.cpp
    playback/mpv_runtime_probe.cpp
    playback/mpv_runtime_provenance.cpp
    playback/mpv_runtime_paths.cpp
    playback/mpv_loader.cpp
    playback/mpv_player.cpp
    playback/player_thread.cpp
    playback/seek_optimization_profile.cpp
    playback/state_normalizer.cpp
    playback/subtitle_layout.cpp
    playback/video_filter_profile.cpp
    ui/button_visuals.cpp
    ui/button_style_assets.cpp
    ui/combo_box_utils.cpp
    ui/app_icon.cpp
    ui/control_value_formats.cpp
    ui/error_messages.cpp
    ui/design_tokens.cpp
    ui/main_window.cpp
    ui/main_window_layout.cpp
    ui/main_window_messages.cpp
    ui/main_window_controls.cpp
    ui/main_window_layout_runtime.cpp
    ui/main_window_menu.cpp
    ui/main_window_overlays.cpp
    ui/main_window_quick_browse.cpp
    ui/main_window_draw.cpp
    ui/player_state.cpp
    ui/preview_session.cpp
    ui/quick_browse_panel.cpp
    ui/seek_preview_popup.cpp
    ui/shortcut_capture.cpp
    ui/shell_thumbnail_provider.cpp
    ui/shortcut_display.cpp
    ui/settings_dialog.cpp
    ui/settings_dialog_state.cpp
    ui/settings_dialog_runtime.cpp
    ui/settings_dialog_playback_page.cpp
    ui/settings_dialog_subtitle_page.cpp
    ui/settings_dialog_audio_page.cpp
    ui/settings_dialog_shortcuts_page.cpp
    ui/settings_dialog_advanced_page.cpp
    ui/settings_dialog_draw.cpp
    ui/settings_dialog_audio_outputs.cpp
    ui/settings_page_surface.cpp
    ui/themed_slider.cpp
    ui/themed_scrollbar.cpp
    ui/window_placement_utils.cpp
)

set(VELO_COMMON_SOURCES
    app/app_metadata.cpp
    app/command_line.cpp
    app/end_of_playback_policy.cpp
    app/media_history.cpp
    app/media_file_types.cpp
    app/media_playlist.cpp
    app/media_source.cpp
    app/quick_browse_catalog.cpp
    app/playlist_storage.cpp
    app/mpv_runtime_license_prompt.cpp
    app/mpv_runtime_license_prompt_text.cpp
    app/mpv_runtime_release_asset.cpp
    app/replay_selection_policy.cpp
    app/runtime_playlist.cpp
    app/release_update.cpp
    config/app_config_defaults.cpp
    config/default_language_hint.cpp
    config/app_config_normalizer.cpp
    config/subtitle_position_utils.cpp
    common/hex_color.cpp
    common/http_download.cpp
    common/portable_executable.cpp
    common/sha256.cpp
    config/config_service.cpp
    config/config_parser.cpp
    config/config_serializer.cpp
    diagnostics/logger.cpp
    diagnostics/performance_report.cpp
    diagnostics/process_memory.cpp
    diagnostics/release_gate.cpp
    diagnostics/session_recovery.cpp
    diagnostics/system_report.cpp
    diagnostics/startup_metrics.cpp
    localization/localization.cpp
    platform/win32/input_profile.cpp
    platform/win32/input_router.cpp
    playback/command_dispatcher.cpp
    playback/end_file_reason.cpp
    playback/low_latency_seek.cpp
    playback/mpv_runtime_probe.cpp
    playback/mpv_runtime_provenance.cpp
    playback/mpv_runtime_paths.cpp
    playback/seek_optimization_profile.cpp
    playback/state_normalizer.cpp
    playback/subtitle_layout.cpp
    playback/video_filter_profile.cpp
    ui/button_visuals.cpp
    ui/combo_box_utils.cpp
    ui/control_value_formats.cpp
    ui/error_messages.cpp
    ui/design_tokens.cpp
    ui/main_window_layout.cpp
    ui/shortcut_capture.cpp
    ui/shortcut_display.cpp
    ui/settings_dialog.cpp
    ui/settings_dialog_state.cpp
    ui/settings_dialog_runtime.cpp
    ui/settings_dialog_playback_page.cpp
    ui/settings_dialog_subtitle_page.cpp
    ui/settings_dialog_audio_page.cpp
    ui/settings_dialog_shortcuts_page.cpp
    ui/settings_dialog_advanced_page.cpp
    ui/settings_dialog_draw.cpp
    ui/settings_dialog_audio_outputs.cpp
    ui/settings_page_surface.cpp
    ui/window_placement_utils.cpp
    tests/scenario_runner.cpp
    tests/config_scenarios.cpp
    tests/settings_scenarios.cpp
    tests/playlist_scenarios.cpp
    tests/ui_scenarios.cpp
    tests/diagnostics_scenarios.cpp
    tests/application_scenarios.cpp
    tests/seek_optimization_scenarios.cpp
    ui/player_state.cpp
)

add_executable(nativplayer WIN32 ${VELO_PLAYER_SOURCES})
target_sources(nativplayer PRIVATE resources/app_icon.rc)
target_include_directories(nativplayer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nativplayer PRIVATE user32 gdi32 gdiplus comdlg32 comctl32 shell32 ole32 advapi32 shlwapi dbghelp bcrypt winhttp winmm msimg32 uxtheme psapi)

if(VELO_LIBMPV_DLL)
    add_custom_command(TARGET nativplayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VELO_LIBMPV_DLL}"
                "$<TARGET_FILE_DIR:nativplayer>/libmpv-2.dll")
endif()

if(VELO_MPV_DLL)
    add_custom_command(TARGET nativplayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VELO_MPV_DLL}"
                "$<TARGET_FILE_DIR:nativplayer>/mpv-2.dll")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/button-style")
    add_custom_command(TARGET nativplayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "$<TARGET_FILE_DIR:nativplayer>/button-style"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/button-style"
                "$<TARGET_FILE_DIR:nativplayer>/button-style")
endif()

add_executable(nativplayer_headless_testkit
    tests/headless_test_kit.cpp
    ${VELO_COMMON_SOURCES}
)
target_include_directories(nativplayer_headless_testkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(nativplayer_headless_testkit PRIVATE user32 gdi32 shell32 ole32 shlwapi advapi32 bcrypt winhttp winmm msimg32 comctl32 uxtheme psapi)

enable_testing()
add_test(NAME nativplayer_headless_testkit COMMAND nativplayer_headless_testkit)
