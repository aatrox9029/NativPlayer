#include <iostream>

#include "tests/scenario_runner.h"

int main() {
    const auto result = velo::tests::RunAllScenarios();
    std::cout << result.report;
    std::cout << "passed=" << result.passed << " failed=" << result.failed << std::endl;
    return result.failed == 0 ? 0 : 1;
}