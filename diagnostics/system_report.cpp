#include "diagnostics/system_report.h"

#include <Windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "common/portable_executable.h"
#include "common/text_encoding.h"
#include "diagnostics/process_memory.h"
#include "playback/mpv_runtime_probe.h"

namespace velo::diagnostics {
namespace {

std::wstring QueryRegistryString(const wchar_t* subKey, const wchar_t* valueName) {
    DWORD bytes = 0;
    if (RegGetValueW(HKEY_LOCAL_MACHINE, subKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &bytes) != ERROR_SUCCESS ||
        bytes == 0) {
        return {};
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    if (RegGetValueW(HKEY_LOCAL_MACHINE, subKey, valueName, RRF_RT_REG_SZ, nullptr, value.data(), &bytes) != ERROR_SUCCESS) {
        return {};
    }
    if (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return value;
}

std::string Narrow(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

std::string NarrowPath(const std::filesystem::path& value) {
    return velo::common::WideToUtf8(value.wstring());
}

std::string NowTimestampForPath() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);
    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y%m%d-%H%M%S");
    return stream.str();
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<char>(ch - 'A' + 'a');
        }
        return static_cast<char>(ch);
    });
    return value;
}

std::string ArchitectureName(WORD processorArchitecture) {
    switch (processorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return "x64";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return "arm64";
        case PROCESSOR_ARCHITECTURE_INTEL:
            return "x86";
        default:
            return "unknown";
    }
}

void AppendLoadedMpvSummary(std::ostringstream& output) {
    HMODULE loadedModule = nullptr;
    std::wstring loadedModuleName;
    for (const auto& candidate : velo::playback::MpvRuntimeCandidateNames()) {
        loadedModule = GetModuleHandleW(candidate.c_str());
        if (loadedModule != nullptr) {
            loadedModuleName = candidate;
            break;
        }
    }

    if (loadedModule == nullptr) {
        output << "mpv_loaded_name=not_loaded\n";
        output << "mpv_loaded_path=\n";
        output << "mpv_loaded_arch=not_loaded\n";
        return;
    }

    velo::common::PortableExecutableInfo loadedInfo;
    if (!velo::common::TryGetLoadedModuleInfo(loadedModule, loadedInfo)) {
        output << "mpv_loaded_name=" << Narrow(loadedModuleName) << '\n';
        output << "mpv_loaded_path=\n";
        output << "mpv_loaded_arch=unknown\n";
        return;
    }

    output << "mpv_loaded_name=" << Narrow(loadedModuleName) << '\n';
    output << "mpv_loaded_path=" << NarrowPath(loadedInfo.path) << '\n';
    output << "mpv_loaded_arch=" << velo::common::BinaryArchitectureName(loadedInfo.architecture) << '\n';
}

void AppendResolvedMpvCandidateSummary(std::ostringstream& output) {
    const auto candidate = velo::playback::ProbeFirstMpvRuntimeCandidate();
    output << "mpv_candidate_name=" << Narrow(candidate.libraryName) << '\n';
    output << "mpv_candidate_path=" << (candidate.found ? NarrowPath(candidate.libraryPath) : std::string()) << '\n';
    output << "mpv_candidate_arch="
           << (candidate.libraryImage.valid ? velo::common::BinaryArchitectureName(candidate.libraryImage.architecture)
                                            : (candidate.found ? "unknown" : "missing"))
           << '\n';
    output << "mpv_candidate_compatible="
           << ((candidate.found && candidate.libraryImage.valid && !candidate.architectureMismatch) ? "yes" : "no") << '\n';
    if (!candidate.diagnosticMessage.empty()) {
        output << "mpv_candidate_diagnostic=" << candidate.diagnosticMessage << '\n';
    }
}

}  // namespace

std::string BuildSystemSummary() {
    std::ostringstream output;

    const std::wstring productName = QueryRegistryString(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName");
    const std::wstring buildNumber = QueryRegistryString(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuild");
    const std::wstring displayVersion = QueryRegistryString(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
    output << "windows_product=" << Narrow(productName) << '\n';
    output << "windows_build=" << Narrow(buildNumber) << '\n';
    output << "windows_display_version=" << Narrow(displayVersion) << '\n';

    SYSTEM_INFO systemInfo{};
    GetNativeSystemInfo(&systemInfo);
    output << "cpu_arch=" << ArchitectureName(systemInfo.wProcessorArchitecture) << '\n';
    output << "process_arch=" << velo::common::BinaryArchitectureName(velo::common::CurrentProcessArchitecture()) << '\n';
    output << "process_path=" << NarrowPath(velo::common::CurrentProcessPath()) << '\n';
    output << "logical_processors=" << systemInfo.dwNumberOfProcessors << '\n';

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        output << "memory_total_mb=" << (memory.ullTotalPhys / (1024ull * 1024ull)) << '\n';
    }

    const ProcessMemorySnapshot processMemory = QueryCurrentProcessMemorySnapshot();
    if (processMemory.available) {
        output << "process_working_set_mb=" << processMemory.workingSetMb << '\n';
        output << "process_private_bytes_mb=" << processMemory.privateBytesMb << '\n';
        output << "process_peak_working_set_mb=" << processMemory.peakWorkingSetMb << '\n';
    }

    output << "system_dpi=" << GetDpiForSystem() << '\n';
    output << "monitor_count=" << GetSystemMetrics(SM_CMONITORS) << '\n';

    HDC screenDc = GetDC(nullptr);
    if (screenDc != nullptr) {
        output << "primary_refresh_hz=" << GetDeviceCaps(screenDc, VREFRESH) << '\n';
        ReleaseDC(nullptr, screenDc);
    }

    int displayIndex = 0;
    DISPLAY_DEVICEW displayDevice{};
    displayDevice.cb = sizeof(displayDevice);
    while (EnumDisplayDevicesW(nullptr, displayIndex, &displayDevice, 0)) {
        if ((displayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0) {
            output << "display_adapter_" << displayIndex << "=" << Narrow(displayDevice.DeviceString) << '\n';
        }
        ++displayIndex;
        displayDevice = DISPLAY_DEVICEW{};
        displayDevice.cb = sizeof(displayDevice);
    }

    output << "audio_output_device_count=" << waveOutGetNumDevs() << '\n';
    AppendLoadedMpvSummary(output);
    AppendResolvedMpvCandidateSummary(output);
    return output.str();
}

std::string ClassifyPlaybackFailure(const std::string& rawError) {
    const std::string normalized = ToLowerAscii(rawError);
    if (normalized.find("permission denied") != std::string::npos || normalized.find("access denied") != std::string::npos) {
        return "permission_denied";
    }
    if (normalized.find("no such file") != std::string::npos || normalized.find("file not found") != std::string::npos ||
        normalized.find("cannot open file") != std::string::npos) {
        return "missing_file";
    }
    if (normalized.find("decoder") != std::string::npos || normalized.find("codec") != std::string::npos ||
        normalized.find("unsupported") != std::string::npos) {
        return "decoder_unavailable";
    }
    if (normalized.find("gpu") != std::string::npos || normalized.find("vulkan") != std::string::npos ||
        normalized.find("d3d") != std::string::npos || normalized.find("opengl") != std::string::npos ||
        normalized.find("hwdec") != std::string::npos) {
        return "video_output_failure";
    }
    if (normalized.find("audio") != std::string::npos || normalized.find("ao") != std::string::npos ||
        normalized.find("wasapi") != std::string::npos) {
        return "audio_output_failure";
    }
    if (normalized.find("subtitle") != std::string::npos || normalized.find("sub") != std::string::npos) {
        return "subtitle_error";
    }
    return "unknown_failure";
}

std::filesystem::path ExportDiagnosticsBundle(const std::filesystem::path& exportRoot,
                                             const std::filesystem::path& configPath,
                                             const std::filesystem::path& backupConfigPath,
                                             const std::filesystem::path& sessionLogPath,
                                             const std::string& aboutText,
                                             const StartupMetrics& metrics,
                                             const std::unordered_map<std::string, int>& failureCounts) {
    std::error_code error;
    std::filesystem::create_directories(exportRoot, error);
    const std::filesystem::path bundleRoot = exportRoot / ("diagnostics-" + NowTimestampForPath());
    std::filesystem::create_directories(bundleRoot, error);
    if (error) {
        return {};
    }

    std::ofstream report(bundleRoot / "report.txt", std::ios::trunc);
    if (!report.is_open()) {
        return {};
    }

    report << aboutText << "\n\n";
    report << "[system]\n" << BuildSystemSummary() << '\n';
    report << "[startup]\n" << metrics.BuildInlineSummary() << '\n';
    report << "[playback_failures]\n";
    if (failureCounts.empty()) {
        report << "none=0\n";
    } else {
        for (const auto& [category, count] : failureCounts) {
            report << category << '=' << count << '\n';
        }
    }
    report.close();

    if (!configPath.empty() && std::filesystem::exists(configPath)) {
        std::filesystem::copy_file(configPath, bundleRoot / configPath.filename(), std::filesystem::copy_options::overwrite_existing, error);
    }
    if (!backupConfigPath.empty() && std::filesystem::exists(backupConfigPath)) {
        std::filesystem::copy_file(backupConfigPath, bundleRoot / backupConfigPath.filename(),
                                   std::filesystem::copy_options::overwrite_existing, error);
    }
    if (!sessionLogPath.empty() && std::filesystem::exists(sessionLogPath)) {
        std::filesystem::copy_file(sessionLogPath, bundleRoot / sessionLogPath.filename(),
                                   std::filesystem::copy_options::overwrite_existing, error);
    }
    return bundleRoot;
}

}  // namespace velo::diagnostics