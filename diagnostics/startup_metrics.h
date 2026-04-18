#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace velo::diagnostics {

class StartupMetrics {
public:
    StartupMetrics();

    void Mark(const std::string& phaseName);
    void MarkFirstFrame();
    bool WriteReport(const std::filesystem::path& filePath) const;
    [[nodiscard]] std::string BuildInlineSummary() const;

private:
    std::chrono::steady_clock::time_point start_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> phases_;
    std::chrono::steady_clock::time_point firstFrame_{};
    bool hasFirstFrame_ = false;
};

}  // namespace velo::diagnostics