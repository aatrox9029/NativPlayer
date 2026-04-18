#include "localization/localization.h"

#include <array>

namespace velo::localization {
namespace {

struct TextEntry {
    TextId id;
    const wchar_t* zhTw;
    const wchar_t* zhCn;
    const wchar_t* enUs;
};

constexpr TextEntry kTexts[] = {
    {TextId::AppLanguageTraditionalChinese, L"繁體中文", L"繁體中文", L"Traditional Chinese"},
    {TextId::AppLanguageSimplifiedChinese, L"簡體中文", L"简体中文", L"Simplified Chinese"},
    {TextId::AppLanguageEnglish, L"English", L"English", L"English"},
    {TextId::MediaFileFilter, L"媒體檔案", L"媒体文件", L"Media Files"},
    {TextId::AllFilesFilter, L"所有檔案", L"所有文件", L"All Files"},
    {TextId::RecoveredAfterUncleanShutdown, L"已從非正常關閉中恢復", L"已从异常关闭中恢复", L"Recovered after an unclean shutdown"},
    {TextId::PlaylistContainerIsEmpty, L"播放清單容器是空的", L"播放列表容器为空", L"Playlist container is empty"},
    {TextId::DiscImageDetected, L"偵測到光碟映像。請先掛載，或使用 dvd:// / bd://", L"检测到光盘镜像。请先挂载，或使用 dvd:// / bd://", L"Disc image detected. Mount it or use dvd:// / bd://"},
    {TextId::MediaSourceNotFound, L"找不到媒體來源", L"找不到媒体来源", L"Media source not found"},
    {TextId::UnsupportedMediaFile, L"不支援的媒體檔案", L"不支持的媒体文件", L"Unsupported media file"},
    {TextId::NoSupportedFilesInSelection, L"選取內容中沒有支援的檔案", L"所选内容中没有受支持的文件", L"No supported files in the selection"},
    {TextId::TemporaryPlaylistReady, L"暫時播放清單已建立", L"临时播放列表已建立", L"Temporary playlist ready"},
    {TextId::NoPlayableFilesFoundInFolder, L"資料夾中沒有可播放檔案", L"文件夹中没有可播放文件", L"No playable files found in folder"},
    {TextId::FolderPlaylistReady, L"資料夾播放清單已建立", L"文件夹播放列表已建立", L"Folder playlist ready"},
    {TextId::SubtitleFileNotFound, L"找不到字幕檔", L"找不到字幕文件", L"Subtitle file not found"},
    {TextId::SubtitleLoadedPrefix, L"已載入字幕: ", L"已加载字幕: ", L"Subtitle loaded: "},
    {TextId::NoNextItem, L"沒有下一個項目", L"没有下一个项目", L"No next item"},
    {TextId::RestartedCurrentItem, L"已重新播放目前項目", L"已重新播放当前项目", L"Restarted current item"},
    {TextId::NoPreviousItem, L"沒有上一個項目", L"没有上一个项目", L"No previous item"},
    {TextId::OpeningPrefix, L"正在開啟: ", L"正在打开: ", L"Opening: "},
    {TextId::AutoLoadedSidecarSubtitle, L"已自動載入同名字幕", L"已自动加载同名字幕", L"Auto-loaded sidecar subtitle"},
    {TextId::ResumedFromPrefix, L"已從這裡繼續播放: ", L"已从这里继续播放: ", L"Resumed from "},
    {TextId::PauseStatePreserved, L"已保留暫停狀態", L"已保留暂停状态", L"Pause state preserved"},
    {TextId::RepeatCurrentItem, L"重播目前項目", L"重播当前项目", L"Repeat current item"},
    {TextId::AutoplayingNextItem, L"自動播放下一個項目", L"自动播放下一个项目", L"Autoplaying next item"},
    {TextId::PlaybackFinished, L"播放已結束", L"播放已结束", L"Playback finished"},
    {TextId::MainStatusReady, L"拖曳影片到這裡開始播放", L"拖拽视频到这里开始播放", L"Drag media here to start playback"},
    {TextId::MainEmptyHint, L"拖放影片/資料夾至此", L"拖放视频/文件夹到此", L"Drop video or folder here"},
    {TextId::MainEndPromptTitle, L"播放結束", L"播放结束", L"Playback ended"},
    {TextId::MainEndPromptReplayNext, L"可重播目前檔案或播放下一個", L"可重播当前文件或播放下一个", L"Replay the current file or play the next one"},
    {TextId::MainEndPromptAutoplayNext, L"1.8 秒後自動播放下一個檔案", L"1.8 秒后自动播放下一个文件", L"Autoplaying the next file in 1.8 seconds"},
    {TextId::MainEndPromptReplayOnly, L"播放清單已結束，可重播目前檔案", L"播放列表已结束，可重播当前文件", L"Playlist ended. You can replay the current file"},
    {TextId::NoRecentFiles, L"尚無最近檔案", L"暂无最近文件", L"No recent files"},
    {TextId::Open, L"開啟", L"打开", L"Open"},
    {TextId::Recent, L"最近", L"最近", L"Recent"},
    {TextId::QuickBrowse, L"快覽", L"快览", L"Browse"},
    {TextId::PreviousItem, L"上一部", L"上一项", L"Previous"},
    {TextId::Play, L"播放", L"播放", L"Play"},
    {TextId::Pause, L"暫停", L"暂停", L"Pause"},
    {TextId::NextItem, L"下一部", L"下一项", L"Next"},
    {TextId::Mute, L"靜音", L"静音", L"Mute"},
    {TextId::Unmute, L"取消靜音", L"取消静音", L"Unmute"},
    {TextId::Speed, L"倍速", L"倍速", L"Speed"},
    {TextId::Subtitle, L"字幕", L"字幕", L"Subtitle"},
    {TextId::Settings, L"設定", L"设置", L"Settings"},
    {TextId::More, L"更多", L"更多", L"More"},
    {TextId::Fullscreen, L"全螢幕", L"全屏", L"Fullscreen"},
    {TextId::EnterFullscreen, L"進入全螢幕", L"进入全屏", L"Enter fullscreen"},
    {TextId::ExitFullscreen, L"退出全螢幕", L"退出全屏", L"Exit fullscreen"},
    {TextId::Volume, L"音量", L"音量", L"Volume"},
    {TextId::MediaInfoShown, L"顯示媒體資訊", L"显示媒体信息", L"Show media info"},
    {TextId::MediaInfoHidden, L"隱藏媒體資訊", L"隐藏媒体信息", L"Hide media info"},
    {TextId::AudioDeviceRecovered, L"已偵測到音訊裝置變更，正在重新套用輸出", L"检测到音频设备变化，正在重新应用输出", L"Detected audio device change. Reapplying output"},
    {TextId::DisplayConfigApplied, L"已套用新的顯示器配置", L"已应用新的显示器配置", L"Applied new display configuration"},
    {TextId::DpiUpdated, L"已更新 DPI 縮放", L"已更新 DPI 缩放", L"DPI scaling updated"},
    {TextId::WakeRecoveredAudio, L"系統喚醒後已嘗試恢復音訊輸出", L"系统唤醒后已尝试恢复音频输出", L"Tried to recover audio output after wake"},
    {TextId::CycleAudioTrack, L"切換音軌", L"切换音轨", L"Cycle audio track"},
    {TextId::CycleSubtitleTrack, L"切換字幕", L"切换字幕", L"Cycle subtitle track"},
    {TextId::DiagnosticsExportFailed, L"診斷資料匯出失敗", L"诊断数据导出失败", L"Diagnostics export failed"},
    {TextId::DiagnosticsExported, L"已匯出診斷資料", L"已导出诊断数据", L"Diagnostics exported"},
    {TextId::EndActionReplay, L"重播", L"重播", L"Replay"},
    {TextId::EndActionPlayNext, L"播放下一個", L"播放下一个", L"Play next"},
    {TextId::EndActionStop, L"停止", L"停止", L"Stop"},
    {TextId::EndActionCloseWindow, L"關閉視窗", L"关闭窗口", L"Close window"},
    {TextId::ReplayCurrentFile, L"重播目前檔案", L"重播当前文件", L"Replay current file"},
    {TextId::ShowMediaInfo, L"顯示媒體資訊", L"显示媒体信息", L"Show media info"},
    {TextId::HideMediaInfo, L"隱藏媒體資訊", L"隐藏媒体信息", L"Hide media info"},
    {TextId::AboutAndIssueReport, L"關於 / 問題回報", L"关于 / 问题反馈", L"About / Issue report"},
    {TextId::ShortcutHelp, L"快捷鍵說明", L"快捷键说明", L"Shortcut help"},
    {TextId::OpenFile, L"開啟檔案", L"打开文件", L"Open file"},
    {TextId::OpenFolder, L"開啟資料夾", L"打开文件夹", L"Open folder"},
    {TextId::LoadSubtitle, L"載入字幕", L"加载字幕", L"Load subtitle"},
    {TextId::ToggleSpeed, L"切換倍速", L"切换倍速", L"Toggle speed"},
    {TextId::SlowerSpeed, L"降低倍速", L"降低倍速", L"Slower speed"},
    {TextId::FasterSpeed, L"提高倍速", L"提高倍速", L"Faster speed"},
    {TextId::ResetSpeed, L"重設倍速", L"重置倍速", L"Reset speed"},
    {TextId::ExportDiagnostics, L"匯出診斷資料", L"导出诊断数据", L"Export diagnostics"},
    {TextId::ShowRecentFiles, L"最近檔案", L"最近文件", L"Recent files"},
    {TextId::QuickBrowseUnavailable, L"目前影片無法使用快速瀏覽", L"当前视频无法使用快速浏览", L"Quick browse is unavailable for this item"},
    {TextId::QuickBrowseTitle, L"快速瀏覽", L"快速浏览", L"Quick Browse"},
    {TextId::QuickBrowseNotLoadedLocalMedia, L"尚未載入本機影片", L"尚未加载本地视频", L"No local media loaded"},
    {TextId::QuickBrowseNoSwitchableVideos, L"目前資料夾沒有可快速切換的影片", L"当前文件夹没有可快速切换的视频", L"No switchable videos in the current folder"},
    {TextId::QuickBrowseNavigateUp, L"回到上層資料夾", L"返回上级文件夹", L"Go to parent folder"},
    {TextId::QuickBrowseFolder, L"資料夾", L"文件夹", L"Folder"},
    {TextId::QuickBrowsePreview, L"預覽", L"预览", L"Preview"},
    {TextId::QuickBrowseVideoCountSuffix, L" 部影片", L" 个视频", L" videos"},
    {TextId::SettingsUpdated, L"設定已更新", L"设置已更新", L"Settings updated"},
    {TextId::PlaybackEnded, L"播放結束後", L"播放结束后", L"After playback"},
    {TextId::MediaInfoTitle, L"標題", L"标题", L"Title"},
    {TextId::MediaInfoTime, L"時間", L"时间", L"Time"},
    {TextId::MediaInfoFormat, L"格式", L"格式", L"Format"},
    {TextId::MediaInfoHardwareDecode, L"硬體解碼", L"硬件解码", L"Hardware decode"},
    {TextId::MediaInfoUnknown, L"未知", L"未知", L"Unknown"},
    {TextId::MediaInfoSoftwareAuto, L"軟體 / 自動", L"软件 / 自动", L"Software / Auto"},
    {TextId::MediaInfoNotLoaded, L"尚未載入媒體", L"尚未加载媒体", L"No media loaded"},
    {TextId::MediaInfoNotLoadedHint, L"拖曳影片到這裡，或按 O 開啟", L"拖拽视频到这里，或按 O 打开", L"Drag media here, or press O to open"},
    {TextId::MediaInfoDebugPath, L"[Debug] 路徑", L"[Debug] 路径", L"[Debug] Path"},
    {TextId::MediaInfoDebugAudio, L"[Debug] 音軌", L"[Debug] 音轨", L"[Debug] Audio"},
    {TextId::MediaInfoDebugSubtitle, L"[Debug] 字幕", L"[Debug] 字幕", L"[Debug] Subtitle"},
    {TextId::ShortcutHelpTitle, L"快捷鍵總覽", L"快捷键总览", L"Shortcut Overview"},
    {TextId::ShortcutEscExitFullscreen, L"Esc  退出全螢幕", L"Esc  退出全屏", L"Esc  Exit fullscreen"},
    {TextId::ShortcutDoubleClick, L"雙擊", L"双击", L"Double Click"},
    {TextId::ShortcutMiddleClick, L"中鍵", L"中键", L"Middle Click"},
    {TextId::ActionPauseResume, L"播放 / 暫停", L"播放 / 暂停", L"Pause / Resume"},
    {TextId::ActionSeekBackward, L"快退", L"快退", L"Seek backward"},
    {TextId::ActionSeekForward, L"快進", L"快进", L"Seek forward"},
    {TextId::ActionVolumeUp, L"提高音量", L"提高音量", L"Volume up"},
    {TextId::ActionVolumeDown, L"降低音量", L"降低音量", L"Volume down"},
    {TextId::ActionToggleMute, L"靜音", L"静音", L"Mute"},
    {TextId::ActionOpenFile, L"開啟檔案", L"打开文件", L"Open file"},
    {TextId::ActionShowRecent, L"顯示最近檔案", L"显示最近文件", L"Show recent files"},
    {TextId::ActionCycleAudio, L"切換音軌", L"切换音轨", L"Cycle audio track"},
    {TextId::ActionCycleSubtitle, L"切換字幕", L"切换字幕", L"Cycle subtitle track"},
    {TextId::ActionTakeScreenshot, L"截圖", L"截图", L"Take screenshot"},
    {TextId::ActionSlowerSpeed, L"降低倍速", L"降低倍速", L"Slower speed"},
    {TextId::ActionFasterSpeed, L"提高倍速", L"提高倍速", L"Faster speed"},
    {TextId::ActionResetSpeed, L"重設倍速", L"重置倍速", L"Reset speed"},
    {TextId::ActionToggleFullscreen, L"切換全螢幕", L"切换全屏", L"Toggle fullscreen"},
    {TextId::ActionPlayPrevious, L"上一個", L"上一个", L"Previous"},
    {TextId::ActionPlayNext, L"下一個", L"下一个", L"Next"},
    {TextId::ActionShowMediaInfo, L"顯示媒體資訊", L"显示媒体信息", L"Show media info"},
    {TextId::MouseActionNext, L"下一個", L"下一个", L"Next"},
    {TextId::MouseActionShowInfo, L"顯示媒體資訊", L"显示媒体信息", L"Show media info"},
    {TextId::MouseActionNone, L"無", L"无", L"None"},
    {TextId::MouseActionFullscreen, L"全螢幕", L"全屏", L"Fullscreen"},
    {TextId::SettingsTitle, L"設定", L"设置", L"Settings"},
    {TextId::SettingsPagePlayback, L"播放", L"播放", L"Playback"},
    {TextId::SettingsPageSubtitle, L"字幕", L"字幕", L"Subtitle"},
    {TextId::SettingsPageAudio, L"音訊", L"音频", L"Audio"},
    {TextId::SettingsPageShortcuts, L"快捷鍵", L"快捷键", L"Shortcuts"},
    {TextId::SettingsPageAdvanced, L"語言/高級", L"语言/高级", L"Language/Advanced"},
    {TextId::SettingsRememberPlaybackPosition, L"記住播放位置", L"记住播放位置", L"Remember playback position"},
    {TextId::SettingsAutoplayNextFile, L"自動播放下一個檔案", L"自动播放下一个文件", L"Autoplay next file"},
    {TextId::SettingsPreservePauseOnOpen, L"開啟新檔案時保留目前暫停狀態", L"打开新文件时保留当前暂停状态", L"Preserve pause state when opening a new file"},
    {TextId::SettingsSeekPreview, L"進度條停留時顯示縮圖預覽", L"停留在进度条上时显示缩略图预览", L"Show thumbnail preview when hovering the seek bar"},
    {TextId::SettingsEndOfPlayback, L"播放完後", L"播放结束后", L"After playback"},
    {TextId::SettingsRepeatMode, L"重播模式", L"重播模式", L"Repeat mode"},
    {TextId::SettingsAutoLoadSubtitle, L"自動載入同名字幕", L"自动加载同名字幕", L"Auto-load matching subtitle"},
    {TextId::SettingsSubtitleFont, L"字型", L"字体", L"Font"},
    {TextId::SettingsSubtitleSize, L"字幕大小:", L"字幕大小:", L"Subtitle size:"},
    {TextId::SettingsSubtitleDelay, L"字幕延遲:", L"字幕延迟:", L"Subtitle delay:"},
    {TextId::SettingsSubtitleBorderSize, L"字幕邊線粗細:", L"字幕边线粗细:", L"Subtitle border size:"},
    {TextId::SettingsShowSubtitleBackground, L"顯示字幕背景", L"显示字幕背景", L"Show subtitle background"},
    {TextId::SettingsSubtitleBackgroundOpacity, L"背景透明度:", L"背景透明度:", L"Background opacity:"},
    {TextId::SettingsSubtitlePosition, L"字幕位置", L"字幕位置", L"Subtitle position"},
    {TextId::SettingsSubtitleEncoding, L"字幕編碼", L"字幕编码", L"Subtitle encoding"},
    {TextId::SettingsOffsetUp, L"上", L"上", L"Up"},
    {TextId::SettingsOffsetDown, L"下", L"下", L"Down"},
    {TextId::SettingsOffsetLeft, L"左", L"左", L"Left"},
    {TextId::SettingsOffsetRight, L"右", L"右", L"Right"},
    {TextId::SettingsAudioOutput, L"音訊輸出喇叭", L"音频输出扬声器", L"Audio output device"},
    {TextId::SettingsCurrentVolume, L"目前音量:", L"当前音量:", L"Current volume:"},
    {TextId::SettingsRememberCurrentVolume, L"記住目前音量", L"记住当前音量", L"Remember current volume"},
    {TextId::SettingsKeyboardVolumeStep, L"鍵盤音量步進:", L"键盘音量步进:", L"Keyboard volume step:"},
    {TextId::SettingsWheelVolumeStep, L"滾輪音量步進:", L"滚轮音量步进:", L"Wheel volume step:"},
    {TextId::SettingsAudioDelay, L"音訊延遲:", L"音频延迟:", L"Audio delay:"},
    {TextId::SettingsDoubleClickAction, L"雙擊動作", L"双击动作", L"Double-click action"},
    {TextId::SettingsMiddleClickAction, L"中鍵動作", L"中键动作", L"Middle-click action"},
    {TextId::SettingsShortcutWarningIdle, L"目前快捷鍵沒有衝突", L"当前快捷键没有冲突", L"No shortcut conflicts detected"},
    {TextId::SettingsShortcutWarningCapturing, L"等待輸入鍵盤或滑鼠按鍵，方向鍵不會被記錄，按 Esc 取消", L"等待输入键盘或鼠标按键，方向键不会被记录，按 Esc 取消", L"Waiting for keyboard or mouse input. Arrow keys are ignored. Press Esc to cancel"},
    {TextId::SettingsHardwareDecode, L"硬體解碼", L"硬件解码", L"Hardware decode"},
    {TextId::SettingsShowDebugInfo, L"顯示 Debug 資訊", L"显示 Debug 信息", L"Show debug info"},
    {TextId::SettingsAspectRatio, L"畫面比例", L"画面比例", L"Aspect ratio"},
    {TextId::SettingsRotate, L"旋轉", L"旋转", L"Rotate"},
    {TextId::SettingsMirrorVideo, L"鏡像顯示影片", L"镜像显示视频", L"Mirror video"},
    {TextId::SettingsEnableDeinterlace, L"啟用去交錯", L"启用去交错", L"Enable deinterlace"},
    {TextId::SettingsEqualizer, L"等化器", L"均衡器", L"Equalizer"},
    {TextId::SettingsSharpenStrength, L"銳化強度:", L"锐化强度:", L"Sharpen strength:"},
    {TextId::SettingsDenoiseStrength, L"降噪強度:", L"降噪强度:", L"Denoise strength:"},
    {TextId::SettingsNetworkTimeout, L"網路逾時:", L"网络超时:", L"Network timeout:"},
    {TextId::SettingsReconnectAttempts, L"重連次數:", L"重连次数:", L"Reconnect attempts:"},
    {TextId::SettingsScreenshotFormat, L"截圖格式", L"截图格式", L"Screenshot format"},
    {TextId::SettingsAdvancedHint, L"進階選項會直接影響播放核心，只有在需要排查相容性時才建議調整。", L"高级选项会直接影响播放核心，仅在需要排查兼容性时才建议调整。", L"Advanced options affect the playback core directly. Change them only when troubleshooting compatibility issues."},
    {TextId::SettingsImport, L"匯入設定", L"导入设置", L"Import settings"},
    {TextId::SettingsExport, L"匯出設定", L"导出设置", L"Export settings"},
    {TextId::SettingsResetDefaults, L"還原預設", L"恢复默认", L"Reset defaults"},
    {TextId::SettingsOk, L"確定", L"确定", L"OK"},
    {TextId::SettingsCancel, L"取消", L"取消", L"Cancel"},
    {TextId::SettingsImportFailed, L"匯入設定失敗", L"导入设置失败", L"Failed to import settings"},
    {TextId::SettingsExportFailed, L"匯出設定失敗", L"导出设置失败", L"Failed to export settings"},
    {TextId::SettingsLanguage, L"語言", L"语言", L"Language"},
    {TextId::SettingsControlHideDelay, L"控制列自動隱藏:", L"控制栏自动隐藏:", L"Controls auto-hide:"},
    {TextId::SettingsSeekStepMode, L"快進 / 快退單位:", L"快进 / 快退单位:", L"Seek step unit:"},
    {TextId::SettingsSeekStepModeSeconds, L"秒", L"秒", L"Seconds"},
    {TextId::SettingsSeekStepModePercent, L"百分比", L"百分比", L"Percent"},
    {TextId::SettingsSeekStepSeconds, L"快進 / 快退秒數:", L"快进 / 快退秒数:", L"Seek step:"},
    {TextId::SettingsSeekStepPercent, L"快進 / 快退比例:", L"快进 / 快退比例:", L"Seek step:"},
    {TextId::SettingsJpegQuality, L"JPG 品質:", L"JPG 质量:", L"JPG quality:"},
    {TextId::SettingsColorPrimary, L"主要顏色", L"主要颜色", L"Primary color"},
    {TextId::SettingsColorBorder, L"邊線顏色", L"边线颜色", L"Border color"},
    {TextId::SettingsColorShadow, L"陰影顏色", L"阴影颜色", L"Shadow color"},
    {TextId::SettingsColorBackground, L"背景顏色", L"背景颜色", L"Background color"},
    {TextId::SettingsKeyPrompt, L"請按鍵...", L"请按键...", L"Press a key..."},
    {TextId::SettingsChooseFont, L"選擇字型", L"选择字体", L"Choose font"},
    {TextId::SettingsAudioDefault, L"系統預設", L"系统默认", L"System default"},
    {TextId::SettingsAudioSpeakerPrefix, L"喇叭 ", L"扬声器 ", L"Speaker "},
    {TextId::SettingsAudioOutputPrefix, L"輸出裝置 ", L"输出设备 ", L"Output device "},
    {TextId::SettingsColorWhite, L"白色", L"白色", L"White"},
    {TextId::SettingsColorBlack, L"黑色", L"黑色", L"Black"},
    {TextId::SettingsColorRed, L"紅色", L"红色", L"Red"},
    {TextId::SettingsColorOrange, L"橘色", L"橙色", L"Orange"},
    {TextId::SettingsColorYellow, L"黃色", L"黄色", L"Yellow"},
    {TextId::SettingsColorGreen, L"綠色", L"绿色", L"Green"},
    {TextId::SettingsColorBlue, L"藍色", L"蓝色", L"Blue"},
    {TextId::SettingsColorPurple, L"紫色", L"紫色", L"Purple"},
    {TextId::SettingsColorGray, L"灰色", L"灰色", L"Gray"},
    {TextId::SettingsColorPink, L"粉紅色", L"粉红色", L"Pink"},
    {TextId::SettingsCustomColor, L"自訂顏色", L"自定义颜色", L"Custom color"},
    {TextId::SettingsEndActionReplay, L"重播", L"重播", L"Replay"},
    {TextId::SettingsEndActionPlayNext, L"播放下一個", L"播放下一个", L"Play next"},
    {TextId::SettingsEndActionStop, L"停止", L"停止", L"Stop"},
    {TextId::SettingsEndActionCloseWindow, L"關閉視窗", L"关闭窗口", L"Close window"},
    {TextId::SettingsRepeatModeOff, L"關閉", L"关闭", L"Off"},
    {TextId::SettingsRepeatModeOne, L"單曲重播", L"单曲重播", L"Repeat one"},
    {TextId::SettingsRepeatModeAll, L"全部重播", L"全部重播", L"Repeat all"},
    {TextId::SettingsSubtitlePositionTop, L"上方", L"上方", L"Top"},
    {TextId::SettingsSubtitlePositionMiddle, L"中間", L"中间", L"Middle"},
    {TextId::SettingsSubtitlePositionBottom, L"下方", L"下方", L"Bottom"},
    {TextId::SettingsMouseActionPauseResume, L"播放 / 暫停", L"播放 / 暂停", L"Pause / Resume"},
    {TextId::SettingsMouseActionPlayNext, L"播放下一個", L"播放下一个", L"Play next"},
    {TextId::SettingsMouseActionShowInfo, L"顯示媒體資訊", L"显示媒体信息", L"Show media info"},
    {TextId::SettingsMouseActionNone, L"無", L"无", L"None"},
    {TextId::SettingsMouseActionFullscreen, L"全螢幕", L"全屏", L"Fullscreen"},
    {TextId::SettingsHwdecAutoSafe, L"自動", L"自动", L"Auto"},
    {TextId::SettingsHwdecSoftwareOnly, L"僅軟體解碼", L"仅软件解码", L"Software Only"},
    {TextId::SettingsHwdecAutoCopy, L"自動複製", L"自动复制", L"Auto Copy"},
    {TextId::SettingsAspectRatioDefault, L"預設", L"默认", L"Default"},
    {TextId::SettingsEqualizerOff, L"關閉", L"关闭", L"Off"},
    {TextId::SettingsEqualizerVoice, L"人聲", L"人声", L"Voice"},
    {TextId::SettingsEqualizerCinema, L"影院", L"影院", L"Cinema"},
    {TextId::SettingsEqualizerMusic, L"音樂", L"音乐", L"Music"},
    {TextId::SettingsScreenshotFormatPng, L"PNG", L"PNG", L"PNG"},
    {TextId::SettingsScreenshotFormatJpg, L"JPG", L"JPG", L"JPG"},
    {TextId::StateOff, L"關閉", L"关闭", L"Off"},
    {TextId::PlaylistLabel, L"播放清單", L"播放列表", L"Playlist"},
    {TextId::ScreenshotSavedPrefix, L"已儲存截圖: ", L"已保存截图: ", L"Screenshot saved: "},
    {TextId::ErrorAccessDenied, L"目前沒有權限存取該媒體來源，請確認檔案權限或改用可讀取的位置。", L"当前没有权限访问该媒体源，请检查文件权限或改用可读取的位置。", L"Permission denied while accessing this media source. Check file permissions or use a readable location."},
    {TextId::ErrorFileNotFound, L"無法開啟媒體來源，請確認檔案仍存在且路徑有效。", L"无法打开媒体源，请确认文件仍存在且路径有效。", L"Unable to open the media source. Check that the file still exists and the path is valid."},
    {TextId::ErrorDecoderFailure, L"解碼器初始化失敗，請嘗試切換不同的硬體解碼模式或改用軟體解碼。", L"解码器初始化失败，请尝试切换不同的硬件解码模式或改用软件解码。", L"Decoder initialization failed. Try another hardware decode mode or switch to software decoding."},
    {TextId::ErrorGpuFailure, L"顯示輸出初始化失敗，請嘗試重新開啟程式或關閉硬體加速。", L"显示输出初始化失败，请尝试重新打开程序或关闭硬件加速。", L"Display output initialization failed. Try reopening the app or disabling hardware acceleration."},
    {TextId::ErrorAudioFailure, L"音訊輸出初始化失敗，請確認系統音訊裝置可用後再試一次。", L"音频输出初始化失败，请确认系统音频设备可用后再试一次。", L"Audio output initialization failed. Make sure a system audio device is available and try again."},
    {TextId::ErrorSubtitleFailure, L"字幕載入失敗，請確認字幕檔格式正確且檔案可讀。", L"字幕加载失败，请确认字幕文件格式正确且文件可读。", L"Failed to load subtitles. Check that the subtitle file is readable and uses a supported format."},
    {TextId::ErrorNetworkFailure, L"網路串流連線失敗，請確認來源可連線後再試一次。", L"网络串流连接失败，请确认源可连接后再试一次。", L"Network stream connection failed. Verify that the source is reachable and try again."},
    {TextId::ErrorGenericPlaybackFailure, L"播放時發生問題，請檢查媒體內容與輸出裝置後重試。", L"播放时发生问题，请检查媒体内容与输出设备后重试。", L"A playback error occurred. Check the media source and output devices, then try again."},
    {TextId::ConflictAlreadyUsedBy, L"已被以下功能使用", L"已被以下功能使用", L"is already used by"},
    {TextId::AboutConfig, L"設定", L"设置", L"Config"},
    {TextId::AboutLog, L"記錄", L"日志", L"Log"},
    {TextId::AboutRecoveryMarker, L"恢復標記", L"恢复标记", L"Recovery marker"},
    {TextId::AboutDiagnosticsHint, L"回報播放或 UI 問題時，請使用診斷資料匯出。", L"反馈播放或 UI 问题时，请使用诊断数据导出。", L"Use diagnostics export when reporting playback or UI issues."},
};

const TextEntry* FindTextEntry(const TextId id) {
    for (const auto& entry : kTexts) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

}  // namespace

AppLanguage ResolveLanguage(const std::wstring_view languageCode) {
    if (languageCode == L"zh-CN") {
        return AppLanguage::ZhCn;
    }
    if (languageCode == L"en-US" || languageCode == L"en") {
        return AppLanguage::EnUs;
    }
    return AppLanguage::ZhTw;
}

std::wstring LanguageCode(const AppLanguage language) {
    switch (language) {
        case AppLanguage::ZhCn:
            return L"zh-CN";
        case AppLanguage::EnUs:
            return L"en-US";
        case AppLanguage::ZhTw:
        default:
            return L"zh-TW";
    }
}

std::wstring LanguageDisplayName(const AppLanguage language) {
    switch (language) {
        case AppLanguage::ZhCn:
            return Text(language, TextId::AppLanguageSimplifiedChinese);
        case AppLanguage::EnUs:
            return Text(language, TextId::AppLanguageEnglish);
        case AppLanguage::ZhTw:
        default:
            return Text(language, TextId::AppLanguageTraditionalChinese);
    }
}

std::wstring LanguageMenuLabel(const AppLanguage language) {
    switch (language) {
        case AppLanguage::ZhCn:
            return L"\u7B80  " + LanguageDisplayName(language);
        case AppLanguage::EnUs:
            return L"EN  " + LanguageDisplayName(language);
        case AppLanguage::ZhTw:
        default:
            return L"\u7E41  " + LanguageDisplayName(language);
    }
}

std::wstring Text(const AppLanguage language, const TextId id) {
    if (id == TextId::OpeningPrefix) {
        switch (language) {
            case AppLanguage::ZhCn:
                return L"正在打开: ";
            case AppLanguage::EnUs:
                return L"Opening: ";
            case AppLanguage::ZhTw:
            default:
                return L"正在開啟: ";
        }
    }

    const auto* entry = FindTextEntry(id);
    if (entry == nullptr) {
        return {};
    }

    switch (language) {
        case AppLanguage::ZhCn:
            return entry->zhCn;
        case AppLanguage::EnUs:
            return entry->enUs;
        case AppLanguage::ZhTw:
        default:
            return entry->zhTw;
    }
}

std::wstring Text(const std::wstring_view languageCode, const TextId id) {
    return Text(ResolveLanguage(languageCode), id);
}

std::wstring ActionLabel(const AppLanguage language, const std::wstring_view actionId) {
    if (actionId == L"toggle_pause") {
        return Text(language, TextId::ActionPauseResume);
    }
    if (actionId == L"seek_backward") {
        return Text(language, TextId::ActionSeekBackward);
    }
    if (actionId == L"seek_forward") {
        return Text(language, TextId::ActionSeekForward);
    }
    if (actionId == L"volume_up") {
        return Text(language, TextId::ActionVolumeUp);
    }
    if (actionId == L"volume_down") {
        return Text(language, TextId::ActionVolumeDown);
    }
    if (actionId == L"toggle_mute") {
        return Text(language, TextId::ActionToggleMute);
    }
    if (actionId == L"open_file") {
        return Text(language, TextId::ActionOpenFile);
    }
    if (actionId == L"show_recent") {
        return Text(language, TextId::ActionShowRecent);
    }
    if (actionId == L"cycle_audio") {
        return Text(language, TextId::ActionCycleAudio);
    }
    if (actionId == L"cycle_subtitle") {
        return Text(language, TextId::ActionCycleSubtitle);
    }
    if (actionId == L"take_screenshot") {
        return Text(language, TextId::ActionTakeScreenshot);
    }
    if (actionId == L"slower_speed") {
        return Text(language, TextId::ActionSlowerSpeed);
    }
    if (actionId == L"faster_speed") {
        return Text(language, TextId::ActionFasterSpeed);
    }
    if (actionId == L"reset_speed") {
        return Text(language, TextId::ActionResetSpeed);
    }
    if (actionId == L"toggle_fullscreen") {
        return Text(language, TextId::ActionToggleFullscreen);
    }
    if (actionId == L"play_previous") {
        return Text(language, TextId::ActionPlayPrevious);
    }
    if (actionId == L"play_next") {
        return Text(language, TextId::ActionPlayNext);
    }
    if (actionId == L"show_media_info") {
        return Text(language, TextId::ActionShowMediaInfo);
    }
    return std::wstring(actionId);
}

}  // namespace velo::localization
