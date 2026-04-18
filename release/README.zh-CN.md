# NativPlayer

这是 NativPlayer 的 Windows x64 简体中文用户版本。

## 内容

- `nativplayer.exe`
- `nativplayer_headless_testkit.exe`
- `button-style/`
- `README.md`

## 首次启动

- 此版本默认语言为简体中文。
- 如果 `%LOCALAPPDATA%\NativPlayer\runtime\win64\libmpv-2.dll` 尚不存在，程序会先显示简体中文的 `libmpv-2.dll` 检查 / 授权提示窗口。
- 你可以选择自动下载官方 LGPL x64 版本，或打开官方页面手动下载。
- 用户版不附带 `libmpv-2.dll`。

## libmpv 来源与授权

- 自动下载来源：`zhongfly/mpv-winbuild` GitHub Releases
- 套件筛选：`mpv-dev-lgpl-x86_64-*.7z`
- 上游项目：`mpv-player/mpv`

参考链接：

- mpv upstream: <https://github.com/mpv-player/mpv>
- Windows build release: <https://github.com/zhongfly/mpv-winbuild/releases/latest>
