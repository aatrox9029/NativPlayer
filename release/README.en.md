# NativPlayer

This is the English Windows x64 user package for NativPlayer.

## Contents

- `nativplayer.exe`
- `nativplayer_headless_testkit.exe`
- `button-style/`
- `README.md`

## First Launch

- This package defaults to English on first launch.
- If `%LOCALAPPDATA%\NativPlayer\runtime\win64\libmpv-2.dll` is missing, NativPlayer first shows an English `libmpv-2.dll` check / license prompt.
- The runtime prompt includes quick language switching for English, Traditional Chinese, and Simplified Chinese.
- You can choose automatic download of the approved official LGPL x64 build, or open the official page and handle the runtime manually.
- User packages do not include `libmpv-2.dll`.

## libmpv Source And License

- Automatic download source: `zhongfly/mpv-winbuild` GitHub Releases
- Asset filter: `mpv-dev-lgpl-x86_64-*.7z`
- Upstream project: `mpv-player/mpv`

References:

- mpv upstream: <https://github.com/mpv-player/mpv>
- Windows build release: <https://github.com/zhongfly/mpv-winbuild/releases/latest>
