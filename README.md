# NativPlayer

<p align="center">
  <img src="logo.png" alt="NativPlayer logo" width="180">
</p>

<p align="center">
  A native Windows media player built with C++20 and libmpv.
</p>

<p align="center">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-0A7E8C">
  <img alt="Language" src="https://img.shields.io/badge/C%2B%2B-20-1F6FEB">
</p>

<h1 align="center">Just a video player.<br>No forced auto-update popups. No sponsor nags.</h1>

## Why It Stands Out

- Pure playback-first design: no forced update dialog, no donation interruption, no extra launcher layer.
- Native desktop app instead of a web wrapper.
- `libmpv` playback engine with a dedicated player thread to keep UI and playback responsibilities separated.
- Built-in Headless Automation Test Kit for regression checks.
- Managed runtime flow for `libmpv-2.dll`, including verification before downloaded runtime use.
- A passive release check can show a download button in the title bar when a newer GitHub release exists.
- Supports English, Traditional Chinese, and Simplified Chinese.

## Highlights

- Video, audio, subtitle, playlist, and streaming playback support.
- Quick browse and playlist-oriented playback workflow.
- Settings surfaces for playback, audio, subtitles, shortcuts, and advanced options.
<h4>Tips: Click the speed icon to return to 1× speed.</h4>

## Quick Start
- General users can go to [Releases](https://github.com/aatrox9029/NativPlayer/releases) to download either the installer or portable version and start using it right away.
- `quick_build.bat`
  Builds the latest player EXE and headless test kit with the standard local build flow.
- `setup.bat`
  Downloads the approved `libmpv` archive for local runtime/setup preparation and verifies its SHA-256.

## Tech Stack

- C++20
- `libmpv`
- CMake
- PowerShell build and release automation
- Clang/LLVM-based Windows build flow
<img width="958" height="596" alt="image" src="https://github.com/user-attachments/assets/ec158698-588e-4ac7-a799-8d9298a6072d" /><p>
<img width="1270" height="680" alt="image" src="https://github.com/user-attachments/assets/91ea92c2-c005-4606-b275-65eb1aefea56" />

## License And Upstream References

NativPlayer uses the LGPL build of `libmpv`. If you redistribute builds that include `libmpv-2.dll`, make sure you comply with the relevant LGPL obligations.

You may replace the libmpv-2.dll used by this project with your own version.
- mpv upstream: <https://github.com/mpv-player/mpv>
- Windows build source used by the runtime flow: <https://github.com/zhongfly/mpv-winbuild/releases/latest>
