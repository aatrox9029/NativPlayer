#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "diagnostics/startup_metrics.h"
#include "ui/player_state.h"

namespace velo::diagnostics {

struct PerformanceSample {
    std::string name;
    double value = 0.0;
    std::string unit;
    double threshold = 0.0;
    bool pass = true;
};

struct BenchmarkReport {
    std::vector<PerformanceSample> samples;
    bool passed = true;
    std::string text;
};

BenchmarkReport BuildBenchmarkReport(const StartupMetrics& metrics, const velo::ui::PlayerState& state);
bool WriteBenchmarkReport(const std::filesystem::path& path, const BenchmarkReport& report);

}  // namespace velo::diagnostics
