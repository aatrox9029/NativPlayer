#pragma once

#include <string>

namespace velo::tests {

struct ScenarioResult {
    int passed = 0;
    int failed = 0;
    std::string report;
};

ScenarioResult RunAllScenarios();

}  // namespace velo::tests