#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "diagnostics/performance_report.h"

namespace velo::diagnostics {

struct ReleaseGateInput {
    bool headlessAutomationPassed = false;
    bool uiSmokePassed = false;
    BenchmarkReport benchmark;
    std::vector<std::string> regressionFailures;
};

struct ReleaseGateResult {
    bool passed = false;
    std::string report;
};

ReleaseGateResult EvaluateReleaseGate(const ReleaseGateInput& input);
bool WriteReleaseGateReport(const std::filesystem::path& path, const ReleaseGateResult& result);

}  // namespace velo::diagnostics
