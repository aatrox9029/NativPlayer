#include "diagnostics/startup_metrics.h"

#include <fstream>
#include <sstream>

namespace velo::diagnostics {

StartupMetrics::StartupMetrics() : start_(std::chrono::steady_clock::now()) {}

void StartupMetrics::Mark(const std::string& phaseName) {
    std::scoped_lock lock(mutex_);
    phases_[phaseName] = std::chrono::steady_clock::now();
}

void StartupMetrics::MarkFirstFrame() {
    std::scoped_lock lock(mutex_);
    hasFirstFrame_ = true;
    firstFrame_ = std::chrono::steady_clock::now();
}

bool StartupMetrics::WriteReport(const std::filesystem::path& filePath) const {
    std::ofstream output(filePath, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << BuildInlineSummary();
    return true;
}

std::string StartupMetrics::BuildInlineSummary() const {
    std::scoped_lock lock(mutex_);
    std::ostringstream output;
    output << "startup_ms_total=";
    const auto now = std::chrono::steady_clock::now();
    output << std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count() << '\n';

    for (const auto& [phaseName, phaseTime] : phases_) {
        output << phaseName << "="
               << std::chrono::duration_cast<std::chrono::milliseconds>(phaseTime - start_).count() << "ms\n";
    }

    if (hasFirstFrame_) {
        output << "first_frame="
               << std::chrono::duration_cast<std::chrono::milliseconds>(firstFrame_ - start_).count() << "ms\n";
    }
    return output.str();
}

}  // namespace velo::diagnostics