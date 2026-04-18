#include "common/portable_executable.h"

#include <fstream>
#include <vector>

namespace velo::common {
namespace {

BinaryArchitecture ArchitectureFromMachine(const WORD machine) {
    switch (machine) {
        case IMAGE_FILE_MACHINE_I386:
            return BinaryArchitecture::X86;
        case IMAGE_FILE_MACHINE_AMD64:
            return BinaryArchitecture::X64;
        case IMAGE_FILE_MACHINE_ARM64:
            return BinaryArchitecture::Arm64;
        default:
            return BinaryArchitecture::Unknown;
    }
}

std::filesystem::path QueryModulePath(const HMODULE module) {
    std::vector<wchar_t> buffer(MAX_PATH);
    while (true) {
        const DWORD copied = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0) {
            return {};
        }
        if (copied < buffer.size() - 1) {
            return std::filesystem::path(std::wstring(buffer.data(), copied));
        }
        buffer.resize(buffer.size() * 2);
    }
}

}  // namespace

std::string BinaryArchitectureName(const BinaryArchitecture architecture) {
    switch (architecture) {
        case BinaryArchitecture::X86:
            return "x86";
        case BinaryArchitecture::X64:
            return "x64";
        case BinaryArchitecture::Arm64:
            return "arm64";
        default:
            return "unknown";
    }
}

bool Is64BitArchitecture(const BinaryArchitecture architecture) {
    return architecture == BinaryArchitecture::X64 || architecture == BinaryArchitecture::Arm64;
}

bool AreArchitecturesCompatible(const BinaryArchitecture processArchitecture, const BinaryArchitecture binaryArchitecture) {
    if (processArchitecture == BinaryArchitecture::Unknown || binaryArchitecture == BinaryArchitecture::Unknown) {
        return false;
    }
    return processArchitecture == binaryArchitecture;
}

std::filesystem::path CurrentProcessPath() {
    return QueryModulePath(nullptr);
}

BinaryArchitecture CurrentProcessArchitecture() {
    PortableExecutableInfo info;
    const auto processPath = CurrentProcessPath();
    if (!processPath.empty() && TryReadPortableExecutableInfo(processPath, info)) {
        return info.architecture;
    }

#if defined(_M_X64)
    return BinaryArchitecture::X64;
#elif defined(_M_IX86)
    return BinaryArchitecture::X86;
#elif defined(_M_ARM64)
    return BinaryArchitecture::Arm64;
#else
    return BinaryArchitecture::Unknown;
#endif
}

bool TryReadPortableExecutableInfo(const std::filesystem::path& path, PortableExecutableInfo& info) {
    info = PortableExecutableInfo{};
    info.path = path;

    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    IMAGE_DOS_HEADER dosHeader{};
    input.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
    if (!input.good() || dosHeader.e_magic != IMAGE_DOS_SIGNATURE || dosHeader.e_lfanew < 0) {
        return false;
    }

    input.seekg(dosHeader.e_lfanew, std::ios::beg);
    DWORD signature = 0;
    input.read(reinterpret_cast<char*>(&signature), sizeof(signature));
    if (!input.good() || signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    IMAGE_FILE_HEADER fileHeader{};
    input.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    if (!input.good()) {
        return false;
    }

    info.machine = fileHeader.Machine;
    info.architecture = ArchitectureFromMachine(fileHeader.Machine);
    info.valid = true;
    return true;
}

bool TryGetLoadedModuleInfo(const HMODULE module, PortableExecutableInfo& info) {
    const auto path = QueryModulePath(module);
    if (path.empty()) {
        info = PortableExecutableInfo{};
        return false;
    }
    return TryReadPortableExecutableInfo(path, info);
}

}  // namespace velo::common