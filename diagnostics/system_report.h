#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "diagnostics/startup_metrics.h"

namespace velo::diagnostics {

std::string BuildSystemSummary();
std::string ClassifyPlaybackFailure(const std::string& rawError);
std::filesystem::path ExportDiagnosticsBundle(const std::filesystem::path& exportRoot,
                                             const std::filesystem::path& configPath,
                                             const std::filesystem::path& backupConfigPath,
                                             const std::filesystem::path& sessionLogPath,
                                             const std::string& aboutText,
                                             const StartupMetrics& metrics,
                                             const std::unordered_map<std::string, int>& failureCounts);

}  // namespace velo::diagnostics