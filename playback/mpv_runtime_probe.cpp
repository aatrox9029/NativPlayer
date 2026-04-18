#include "playback/mpv_runtime_probe.h"

#include <Windows.h>

#include <array>
#include <filesystem>

#include "common/text_encoding.h"
#include "playback/mpv_runtime_paths.h"

namespace velo::playback {
namespace {

std::string NarrowPath(const std::filesystem::path& path) {
    return velo::common::WideToUtf8(path.wstring());
}

std::filesystem::path ResolveMpvRuntimePath(const std::wstring& libraryName) {
    const std::filesystem::path directPath(libraryName);
    if (directPath.has_parent_path() && std::filesystem::exists(directPath)) {
        return directPath;
    }

    const std::array<std::filesystem::path, 2> candidates = {
        ManagedMpvRuntimePath(libraryName),
        BundledMpvRuntimePath(libraryName),
    };
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

std::string BuildArchitectureMismatchMessage(const MpvRuntimeProbeResult& probe) {
    return "Incompatible libmpv architecture: process=" +
           velo::common::BinaryArchitectureName(probe.processArchitecture) +
           " library=" + velo::common::BinaryArchitectureName(probe.libraryImage.architecture) +
           " path=" + NarrowPath(probe.libraryPath);
}

}  // namespace

std::vector<std::wstring> MpvRuntimeCandidateNames() {
    return {L"mpv-2.dll", L"libmpv-2.dll", L"mpv-1.dll"};
}

MpvRuntimeProbeResult ProbeMpvRuntimeLibrary(const std::filesystem::path& libraryPath, const std::wstring& libraryName) {
    MpvRuntimeProbeResult probe;
    probe.libraryName = libraryName.empty() ? libraryPath.filename().wstring() : libraryName;
    probe.libraryPath = libraryPath;
    probe.processArchitecture = velo::common::CurrentProcessArchitecture();
    probe.found = !libraryPath.empty() && std::filesystem::exists(libraryPath);

    if (!probe.found) {
        probe.diagnosticMessage = "libmpv candidate not found: " + velo::common::WideToUtf8(probe.libraryName);
        return probe;
    }

    if (!velo::common::TryReadPortableExecutableInfo(libraryPath, probe.libraryImage)) {
        probe.diagnosticMessage = "Found libmpv candidate but could not read PE machine type: " + NarrowPath(libraryPath);
        return probe;
    }

    probe.architectureMismatch = !velo::common::AreArchitecturesCompatible(probe.processArchitecture, probe.libraryImage.architecture);
    if (probe.architectureMismatch) {
        probe.diagnosticMessage = BuildArchitectureMismatchMessage(probe);
    }

    return probe;
}

MpvRuntimeProbeResult ProbeMpvRuntimeCandidate(const std::wstring& libraryName) {
    const auto resolvedPath = ResolveMpvRuntimePath(libraryName);
    return ProbeMpvRuntimeLibrary(resolvedPath, libraryName);
}

MpvRuntimeProbeResult ProbeFirstMpvRuntimeCandidate() {
    for (const auto& candidate : MpvRuntimeCandidateNames()) {
        const auto probe = ProbeMpvRuntimeCandidate(candidate);
        if (probe.found) {
            return probe;
        }
    }

    MpvRuntimeProbeResult probe;
    probe.libraryName = L"libmpv-2.dll";
    probe.processArchitecture = velo::common::CurrentProcessArchitecture();
    probe.diagnosticMessage = "libmpv candidate not found";
    return probe;
}

}  // namespace velo::playback
