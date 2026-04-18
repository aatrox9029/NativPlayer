#include "app/media_file_types.h"

#include <algorithm>
#include <array>
#include <cwctype>

namespace velo::app {
namespace {

constexpr std::array<std::wstring_view, 41> kPlayableMediaExtensions = {
    L".mp4",  L".mkv",  L".avi",  L".mov",  L".wmv",  L".flv",  L".webm", L".m4v",  L".mpg",  L".mpeg", L".ts",
    L".m2ts", L".mts",  L".m2v",  L".vob",  L".ogv",  L".ogm",  L".asf",  L".3gp",  L".3g2",  L".f4v",  L".rm",
    L".rmvb", L".divx", L".mxf",  L".nut",  L".mp3",  L".m4a",  L".aac",  L".flac", L".wav",  L".ogg",  L".opus",
    L".wma",  L".aif",  L".aiff", L".ape",  L".ac3",  L".dts",  L".mka",
};

constexpr std::array<std::wstring_view, 10> kSubtitleExtensions = {
    L".srt", L".ass", L".ssa", L".vtt", L".sub", L".txt", L".smi", L".smil", L".lrc", L".sup",
};

constexpr std::array<std::wstring_view, 7> kPlaylistContainerExtensions = {
    L".m3u", L".m3u8", L".pls", L".cue", L".mpls", L".asx", L".xspf",
};

constexpr std::array<std::wstring_view, 4> kDiscImageExtensions = {
    L".iso", L".img", L".udf", L".nrg",
};

std::wstring Lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const wchar_t ch) {
        return static_cast<wchar_t>(towlower(ch));
    });
    return value;
}

}  // namespace

std::span<const std::wstring_view> PlayableMediaExtensions() {
    return kPlayableMediaExtensions;
}

std::span<const std::wstring_view> SubtitleExtensions() {
    return kSubtitleExtensions;
}

std::span<const std::wstring_view> PlaylistContainerExtensions() {
    return kPlaylistContainerExtensions;
}

std::span<const std::wstring_view> DiscImageExtensions() {
    return kDiscImageExtensions;
}

bool HasKnownExtension(const std::filesystem::path& path, const std::span<const std::wstring_view> extensions) {
    const std::wstring extension = Lowercase(path.extension().wstring());
    return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

std::wstring BuildDialogExtensionPattern(const std::span<const std::wstring_view> extensions) {
    std::wstring pattern;
    for (size_t index = 0; index < extensions.size(); ++index) {
        if (index > 0) {
            pattern += L';';
        }
        pattern += L'*';
        pattern += extensions[index];
    }
    return pattern;
}

std::wstring BuildMediaOpenDialogPattern() {
    std::wstring pattern = BuildDialogExtensionPattern(PlayableMediaExtensions());
    const std::wstring playlistPattern = BuildDialogExtensionPattern(PlaylistContainerExtensions());
    if (!playlistPattern.empty()) {
        pattern += L';';
        pattern += playlistPattern;
    }
    return pattern;
}

std::wstring BuildSubtitleOpenDialogPattern() {
    return BuildDialogExtensionPattern(SubtitleExtensions());
}

}  // namespace velo::app