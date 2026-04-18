# Media Sample Library

本目錄保留給固定回歸與 benchmark 樣本。

建議至少準備以下類型:

- `local-1080p-h264.mp4`
- `local-4k-hevc.mkv`
- `local-4k-hdr10.mkv`
- `multi-audio-multi-subtitle.mkv`
- `damaged-subtitle-encoding.srt`
- `playlist-basic.m3u8`
- `network-live-url.txt`
- `long-form-two-hour.mkv`
- `vfr-sample.mp4`
- `corrupt-tail-sample.mp4`

使用規則:

- 每次 release gate 固定跑同一批樣本。
- 樣本檔名不可隨意變更，避免 benchmark 比對失真。
- 若樣本不適合提交到 repo，請在 CI / lab 機器以相同檔名掛載。
