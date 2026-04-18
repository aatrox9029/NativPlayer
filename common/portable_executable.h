#pragma once

#include <Windows.h>

#include <filesystem>
#include <string>

namespace velo::common {

enum class BinaryArchitecture {
    Unknown,
    X86,
    X64,
    Arm64,
};

struct PortableExecutableInfo {
    std::filesystem::path path;
    BinaryArchitecture architecture = BinaryArchitecture::Unknown;
    WORD machine = 0;
    bool valid = false;
};

std::string BinaryArchitectureName(BinaryArchitecture architecture);
bool Is64BitArchitecture(BinaryArchitecture architecture);
bool AreArchitecturesCompatible(BinaryArchitecture processArchitecture, BinaryArchitecture binaryArchitecture);
std::filesystem::path CurrentProcessPath();
BinaryArchitecture CurrentProcessArchitecture();
bool TryReadPortableExecutableInfo(const std::filesystem::path& path, PortableExecutableInfo& info);
bool TryGetLoadedModuleInfo(HMODULE module, PortableExecutableInfo& info);

}  // namespace velo::common