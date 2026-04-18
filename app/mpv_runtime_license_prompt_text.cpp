#include "app/mpv_runtime_license_prompt_text.h"

#include "localization/localization.h"

namespace velo::app {

MpvRuntimeLicensePromptText BuildMpvRuntimeLicensePromptText(const std::wstring_view languageCode) {
    using velo::localization::AppLanguage;
    using velo::localization::ResolveLanguage;

    switch (ResolveLanguage(languageCode)) {
        case AppLanguage::ZhCn:
            return {
                .windowTitle = L"NativPlayer - libmpv Runtime Setup",
                .headline = L"开始播放前需要先准备 libmpv-2.dll",
                .body =
                    L"NativPlayer 没有在 runtime 文件夹中找到可用的 libmpv-2.dll。\r\n"
                    L"你可以让程序自动下载官方 LGPL x64 版本，或手动打开官方页面处理。",
                .pathLabel = L"runtime 文件夹",
                .footer = L"",
                .autoButton = L"下载 libmpv-2.dll（推荐）",
                .manualButton = L"打开官方页面",
                .restartButton = L"重新检查",
                .cancelButton = L"取消",
                .runtimeMissingStatus = L"仍然找不到 libmpv-2.dll，建议使用自动下载按钮。",
                .fallbackText =
                    L"开始播放前需要先准备 libmpv-2.dll。\n\n"
                    L"选择“是”会自动下载官方 LGPL x64 版本。\n"
                    L"选择“否”会打开官方页面和 runtime 文件夹，方便你手动处理。",
            };
        case AppLanguage::EnUs:
            return {
                .windowTitle = L"NativPlayer - libmpv Runtime Setup",
                .headline = L"libmpv-2.dll is required before playback can start",
                .body =
                    L"NativPlayer could not find a usable libmpv-2.dll in the runtime folder.\r\n"
                    L"You can download the official LGPL x64 build now, or open the official page and handle it manually.",
                .pathLabel = L"Runtime folder",
                .footer = L"",
                .autoButton = L"Download libmpv-2.dll",
                .manualButton = L"Open Official Page",
                .restartButton = L"Check Again",
                .cancelButton = L"Cancel",
                .runtimeMissingStatus = L"libmpv-2.dll is still missing. Use the download button for the fastest fix.",
                .fallbackText =
                    L"libmpv-2.dll is required before playback can start.\n\n"
                    L"Choose Yes to download the official LGPL x64 build automatically.\n"
                    L"Choose No to open the official page and runtime folder for manual setup.",
            };
        case AppLanguage::ZhTw:
        default:
            return {
                .windowTitle = L"NativPlayer - libmpv Runtime Setup",
                .headline = L"開始播放前需要先準備 libmpv-2.dll",
                .body =
                    L"NativPlayer 沒有在 runtime 資料夾中找到可用的 libmpv-2.dll。\r\n"
                    L"你可以讓程式自動下載官方 LGPL x64 版本，或手動開啟官方頁面處理。",
                .pathLabel = L"runtime 資料夾",
                .footer = L"",
                .autoButton = L"下載 libmpv-2.dll（建議）",
                .manualButton = L"開啟官方頁面",
                .restartButton = L"重新檢查",
                .cancelButton = L"取消",
                .runtimeMissingStatus = L"目前仍找不到 libmpv-2.dll，建議直接使用下載按鈕。",
                .fallbackText =
                    L"開始播放前需要先準備 libmpv-2.dll。\n\n"
                    L"選擇「是」會自動下載官方 LGPL x64 版本。\n"
                    L"選擇「否」會開啟官方頁面與 runtime 資料夾，方便你手動處理。",
            };
    }
}

std::wstring BuildManualDownloadFailureText(const std::wstring_view languageCode) {
    using velo::localization::AppLanguage;
    using velo::localization::ResolveLanguage;

    switch (ResolveLanguage(languageCode)) {
        case AppLanguage::ZhCn:
            return L"请从窗口打开的官方页面手动下载 libmpv-2.dll，然后放到 %LOCALAPPDATA%\\NativPlayer\\runtime\\win64。";
        case AppLanguage::EnUs:
            return L"Download libmpv-2.dll manually from the official page opened by NativPlayer, then place it in %LOCALAPPDATA%\\NativPlayer\\runtime\\win64.";
        case AppLanguage::ZhTw:
        default:
            return L"請從視窗開啟的官方頁面手動下載 libmpv-2.dll，然後放到 %LOCALAPPDATA%\\NativPlayer\\runtime\\win64。";
    }
}

std::wstring BuildCancelledDownloadFailureText(const std::wstring_view languageCode) {
    using velo::localization::AppLanguage;
    using velo::localization::ResolveLanguage;

    switch (ResolveLanguage(languageCode)) {
        case AppLanguage::ZhCn:
            return L"已取消 libmpv-2.dll 安装。";
        case AppLanguage::EnUs:
            return L"Cancelled libmpv-2.dll setup.";
        case AppLanguage::ZhTw:
        default:
            return L"已取消 libmpv-2.dll 安裝。";
    }
}

}  // namespace velo::app
