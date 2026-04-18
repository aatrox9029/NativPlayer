#pragma once

#include <string>

namespace velo::ui {

std::wstring FriendlyPlaybackError(const std::string& rawError, std::wstring_view languageCode = L"zh-TW");

}  // namespace velo::ui
