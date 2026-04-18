#pragma once

#include <filesystem>
#include <string>

namespace velo::common {

bool TryComputeFileSha256Hex(const std::filesystem::path& path, std::wstring& sha256Hex, std::wstring& errorMessage);

}  // namespace velo::common
