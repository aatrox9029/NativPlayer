#pragma once

#include <Windows.h>

#include <string>
#include <string_view>
#include <vector>

#include "config/config_service.h"

namespace velo::platform::win32 {

struct InputBindingDefinition {
    std::wstring_view actionId;
    std::wstring_view label;
    unsigned int defaultVirtualKey = 0;
};

struct KeyChoice {
    unsigned int virtualKey = 0;
    std::wstring_view label;
};

const std::vector<InputBindingDefinition>& InputBindingDefinitions();
const std::vector<KeyChoice>& SupportedKeyChoices();
unsigned int ResolveVirtualKey(const velo::config::AppConfig& config, std::wstring_view actionId);
void SetVirtualKey(velo::config::AppConfig& config, std::wstring_view actionId, unsigned int virtualKey);
std::wstring VirtualKeyDisplayName(unsigned int virtualKey);
std::vector<std::wstring> FindBindingConflicts(const velo::config::AppConfig& config);
bool MatchBinding(WPARAM key, const velo::config::AppConfig& config, std::wstring_view actionId);

}  // namespace velo::platform::win32