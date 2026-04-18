#include "diagnostics/logger.h"

#include <Windows.h>

#include <ShlObj.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>

#include "config/config_service.h"
#include "common/text_encoding.h"

namespace velo::diagnostics {
namespace {

std::string Narrow(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

std::string TimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);
    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

}  // namespace

std::filesystem::path DefaultLogRoot() {
    std::filesystem::path root = velo::config::DefaultConfigRoot() / "logs";
    std::error_code error;
    std::filesystem::create_directories(root, error);
    return root;
}

Logger::Logger(std::filesystem::path rootDirectory) : rootDirectory_(std::move(rootDirectory)) {
    std::error_code error;
    std::filesystem::create_directories(rootDirectory_, error);

    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);
    std::ostringstream fileName;
    fileName << "session-" << std::put_time(&localTime, "%Y%m%d-%H%M%S") << ".log";
    sessionLogPath_ = rootDirectory_ / fileName.str();
    logFile_.open(sessionLogPath_, std::ios::app);
}

Logger::~Logger() {
    std::scoped_lock lock(mutex_);
    FlushLocked();
}

void Logger::Info(const std::string& message) {
    WriteLine("INFO", message);
}

void Logger::Warn(const std::string& message) {
    WriteLine("WARN", message);
}

void Logger::Error(const std::string& message) {
    WriteLine("ERROR", message);
}

const std::filesystem::path& Logger::SessionLogPath() const noexcept {
    return sessionLogPath_;
}

void Logger::WriteLine(const char* level, const std::string& message) {
    std::scoped_lock lock(mutex_);
    if (!logFile_.is_open()) {
        return;
    }
    const std::string line = TimestampNow() + " [" + level + "] " + message + "\n";
    logFile_ << line;
    ++bufferedLineCount_;
    const auto now = std::chrono::steady_clock::now();
    const bool urgent = std::string_view(level) != "INFO";
    if (urgent || bufferedLineCount_ >= 16 || now - lastFlushTime_ >= std::chrono::seconds(2)) {
        FlushLocked();
    }
    OutputDebugStringA(line.c_str());
}

void Logger::FlushLocked() {
    if (!logFile_.is_open() || bufferedLineCount_ == 0) {
        return;
    }
    logFile_.flush();
    bufferedLineCount_ = 0;
    lastFlushTime_ = std::chrono::steady_clock::now();
}

}  // namespace velo::diagnostics
