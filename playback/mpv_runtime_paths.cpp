#include "playback/mpv_runtime_paths.h"

#include <array>

#include "common/portable_executable.h"
#include "config/config_service.h"

namespace velo::playback {

std::filesystem::path ManagedMpvRuntimeRoot() {
    return velo::config::DefaultConfigRoot() / "runtime" / "win64";
}

std::filesystem::path ManagedMpvRuntimePath(const std::wstring& libraryName) {
    return ManagedMpvRuntimeRoot() / libraryName;
}

std::filesystem::path BundledMpvRuntimePath(const std::wstring& libraryName) {
    const auto processPath = velo::common::CurrentProcessPath();
    if (processPath.empty()) {
        return {};
    }

    const auto processDir = processPath.parent_path();
    const auto repoRoot = processDir.parent_path();
    const std::array<std::filesystem::path, 4> candidates = {
        repoRoot / "runtime" / "win64" / libraryName,
        repoRoot / "runtime" / libraryName,
        processDir / "runtime" / "win64" / libraryName,
        processDir / "runtime" / libraryName,
    };
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

}  // namespace velo::playback
