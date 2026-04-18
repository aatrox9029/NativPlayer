# NativPlayer

這是 NativPlayer 的 Windows x64 繁體中文使用者版本。

## 內容

- `nativplayer.exe`
- `nativplayer_headless_testkit.exe`
- `button-style/`
- `README.md`

## 第一次啟動

- 此版本預設語言為繁體中文。
- 若 `%LOCALAPPDATA%\NativPlayer\runtime\win64\libmpv-2.dll` 尚未存在，程式會先顯示繁體中文的 `libmpv-2.dll` 檢查 / 授權提示視窗。
- 你可以選擇自動下載官方 LGPL x64 版本，或開啟官方頁面手動下載。
- 使用者版不附帶 `libmpv-2.dll`。

## libmpv 來源與授權

- 自動下載來源：`zhongfly/mpv-winbuild` GitHub Releases
- 套件篩選：`mpv-dev-lgpl-x86_64-*.7z`
- 上游專案：`mpv-player/mpv`

參考連結：

- mpv upstream: <https://github.com/mpv-player/mpv>
- Windows build release: <https://github.com/zhongfly/mpv-winbuild/releases/latest>
