#pragma once

#include <filesystem>
#include <string>

namespace velo::playback {

std::filesystem::path ManagedMpvRuntimeRoot();
std::filesystem::path ManagedMpvRuntimePath(const std::wstring& libraryName);
std::filesystem::path BundledMpvRuntimePath(const std::wstring& libraryName);

}  // namespace velo::playback
