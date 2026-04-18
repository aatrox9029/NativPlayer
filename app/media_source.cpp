#include "app/media_source.h"

#include <algorithm>
#include <cwctype>

#include "app/media_file_types.h"

namespace velo::app {
namespace {

std::wstring Lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const wchar_t ch) {
        return static_cast<wchar_t>(towlower(ch));
    });
    return value;
}

bool HasPrefixInsensitive(const std::wstring& value, const std::wstring& prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    return Lowercase(value.substr(0, prefix.size())) == Lowercase(prefix);
}

}  // namespace

bool IsLikelyUrl(const std::wstring& value) {
    return HasPrefixInsensitive(value, L"http://") || HasPrefixInsensitive(value, L"https://") ||
           HasPrefixInsensitive(value, L"rtsp://") || HasPrefixInsensitive(value, L"rtmp://") ||
           HasPrefixInsensitive(value, L"mms://") || HasPrefixInsensitive(value, L"udp://") ||
           HasPrefixInsensitive(value, L"tcp://") || HasPrefixInsensitive(value, L"smb://") ||
           HasPrefixInsensitive(value, L"ftp://") || HasPrefixInsensitive(value, L"magnet:");
}

bool IsPlaylistContainer(const std::filesystem::path& path) {
    return HasKnownExtension(path, PlaylistContainerExtensions());
}

bool IsDiscImage(const std::filesystem::path& path) {
    return HasKnownExtension(path, DiscImageExtensions());
}

bool IsDiscProtocol(const std::wstring& value) {
    return HasPrefixInsensitive(value, L"dvd://") || HasPrefixInsensitive(value, L"bd://") ||
           HasPrefixInsensitive(value, L"bluray://") || HasPrefixInsensitive(value, L"cdda://");
}

MediaSourceKind DetectMediaSourceKind(const std::wstring& value) {
    if (value.empty()) {
        return MediaSourceKind::Unsupported;
    }
    if (IsLikelyUrl(value)) {
        return MediaSourceKind::NetworkUrl;
    }
    if (IsDiscProtocol(value)) {
        return MediaSourceKind::DiscProtocol;
    }

    const std::filesystem::path path(value);
    if (IsPlaylistContainer(path)) {
        return MediaSourceKind::PlaylistContainer;
    }
    if (IsDiscImage(path)) {
        return MediaSourceKind::DiscImage;
    }
    if (std::filesystem::exists(path)) {
        return MediaSourceKind::LocalFile;
    }
    return MediaSourceKind::Unsupported;
}

bool SupportsDirectPlayback(const std::wstring& value) {
    switch (DetectMediaSourceKind(value)) {
        case MediaSourceKind::LocalFile:
        case MediaSourceKind::NetworkUrl:
        case MediaSourceKind::DiscProtocol:
            return true;
        case MediaSourceKind::PlaylistContainer:
        case MediaSourceKind::DiscImage:
        case MediaSourceKind::Unsupported:
        default:
            return false;
    }
}

std::wstring SourceKindDescription(const MediaSourceKind kind) {
    switch (kind) {
        case MediaSourceKind::LocalFile:
            return L"local_file";
        case MediaSourceKind::NetworkUrl:
            return L"network_url";
        case MediaSourceKind::PlaylistContainer:
            return L"playlist_container";
        case MediaSourceKind::DiscImage:
            return L"disc_image";
        case MediaSourceKind::DiscProtocol:
            return L"disc_protocol";
        case MediaSourceKind::Unsupported:
        default:
            return L"unsupported";
    }
}

}  // namespace velo::app
