#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace velo::diagnostics {

class Logger {
public:
    explicit Logger(std::filesystem::path rootDirectory);
    ~Logger();

    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);

    [[nodiscard]] const std::filesystem::path& SessionLogPath() const noexcept;

private:
    void WriteLine(const char* level, const std::string& message);
    void FlushLocked();

    std::filesystem::path rootDirectory_;
    std::filesystem::path sessionLogPath_;
    std::ofstream logFile_;
    mutable std::mutex mutex_;
    size_t bufferedLineCount_ = 0;
    std::chrono::steady_clock::time_point lastFlushTime_ = std::chrono::steady_clock::now();
};

std::filesystem::path DefaultLogRoot();

}  // namespace velo::diagnostics