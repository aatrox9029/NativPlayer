#pragma once

#include <string>
#include <string_view>

#include "config/config_service.h"

namespace velo::ui {

std::wstring ShortcutDisplayName(const velo::config::AppConfig& config, std::wstring_view actionId);
std::wstring BuildShortcutHelpText(const velo::config::AppConfig& config);

}  // namespace velo::ui
