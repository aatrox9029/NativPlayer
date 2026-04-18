#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "common/portable_executable.h"

namespace velo::playback {

struct MpvRuntimeProbeResult {
    std::wstring libraryName;
    std::filesystem::path libraryPath;
    velo::common::PortableExecutableInfo libraryImage;
    velo::common::BinaryArchitecture processArchitecture = velo::common::BinaryArchitecture::Unknown;
    bool found = false;
    bool architectureMismatch = false;
    std::string diagnosticMessage;
};

std::vector<std::wstring> MpvRuntimeCandidateNames();
MpvRuntimeProbeResult ProbeMpvRuntimeLibrary(const std::filesystem::path& libraryPath, const std::wstring& libraryName = L"");
MpvRuntimeProbeResult ProbeMpvRuntimeCandidate(const std::wstring& libraryName);
MpvRuntimeProbeResult ProbeFirstMpvRuntimeCandidate();

}  // namespace velo::playback