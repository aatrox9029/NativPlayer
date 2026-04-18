#include "diagnostics/release_gate.h"

#include <fstream>
#include <sstream>

namespace velo::diagnostics {

ReleaseGateResult EvaluateReleaseGate(const ReleaseGateInput& input) {
    ReleaseGateResult result;
    result.passed = input.headlessAutomationPassed && input.uiSmokePassed && input.benchmark.passed &&
                    input.regressionFailures.empty();

    std::ostringstream report;
    report << "headless_automation=" << (input.headlessAutomationPassed ? "pass" : "fail") << '\n';
    report << "ui_smoke=" << (input.uiSmokePassed ? "pass" : "fail") << '\n';
    report << "benchmark=" << (input.benchmark.passed ? "pass" : "fail") << '\n';
    if (input.regressionFailures.empty()) {
        report << "regressions=pass\n";
    } else {
        for (const auto& failure : input.regressionFailures) {
            report << "regression_failure=" << failure << '\n';
        }
    }
    report << "release_gate=" << (result.passed ? "pass" : "fail") << '\n';
    result.report = report.str();
    return result;
}

bool WriteReleaseGateReport(const std::filesystem::path& path, const ReleaseGateResult& result) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << result.report;
    return true;
}

}  // namespace velo::diagnostics
