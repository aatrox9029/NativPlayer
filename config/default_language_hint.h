#pragma once

#include <optional>
#include <string>

namespace velo::config {

std::optional<std::wstring> TryResolvePackagedDefaultLanguage();

}  // namespace velo::config
