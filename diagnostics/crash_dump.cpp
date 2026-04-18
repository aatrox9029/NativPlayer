#include "diagnostics/crash_dump.h"

#include <Windows.h>
#include <DbgHelp.h>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace velo::diagnostics {
namespace {

std::filesystem::path g_dumpDirectory;

LONG WINAPI HandleUnhandledException(EXCEPTION_POINTERS* exceptionPointers) {
    std::error_code error;
    std::filesystem::create_directories(g_dumpDirectory, error);

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream fileName;
    fileName << "crash-" << std::put_time(&localTime, "%Y%m%d-%H%M%S") << ".dmp";
    const auto dumpPath = g_dumpDirectory / fileName.str();

    HANDLE dumpFile = CreateFileW(dumpPath.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION info{};
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = exceptionPointers;
        info.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpNormal, &info, nullptr,
                          nullptr);
        CloseHandle(dumpFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

void InstallCrashDumpHandler(const std::filesystem::path& dumpDirectory) {
    g_dumpDirectory = dumpDirectory;
    SetUnhandledExceptionFilter(HandleUnhandledException);
}

}  // namespace velo::diagnostics