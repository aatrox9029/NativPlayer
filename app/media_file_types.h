#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace velo::app {

std::span<const std::wstring_view> PlayableMediaExtensions();
std::span<const std::wstring_view> SubtitleExtensions();
std::span<const std::wstring_view> PlaylistContainerExtensions();
std::span<const std::wstring_view> DiscImageExtensions();
bool HasKnownExtension(const std::filesystem::path& path, std::span<const std::wstring_view> extensions);
std::wstring BuildDialogExtensionPattern(std::span<const std::wstring_view> extensions);
std::wstring BuildMediaOpenDialogPattern();
std::wstring BuildSubtitleOpenDialogPattern();

}  // namespace velo::app