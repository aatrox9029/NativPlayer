#pragma once

#include <filesystem>

namespace velo::diagnostics {

void InstallCrashDumpHandler(const std::filesystem::path& dumpDirectory);

}  // namespace velo::diagnostics