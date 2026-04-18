#pragma once

#include <filesystem>
#include <string>

namespace velo::app {

enum class MediaSourceKind {
    LocalFile,
    NetworkUrl,
    PlaylistContainer,
    DiscImage,
    DiscProtocol,
    Unsupported,
};

bool IsLikelyUrl(const std::wstring& value);
bool IsPlaylistContainer(const std::filesystem::path& path);
bool IsDiscImage(const std::filesystem::path& path);
bool IsDiscProtocol(const std::wstring& value);
MediaSourceKind DetectMediaSourceKind(const std::wstring& value);
bool SupportsDirectPlayback(const std::wstring& value);
std::wstring SourceKindDescription(MediaSourceKind kind);

}  // namespace velo::app
