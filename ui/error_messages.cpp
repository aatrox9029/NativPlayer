#include "ui/error_messages.h"

#include <Windows.h>

#include <algorithm>

#include "common/text_encoding.h"
#include "localization/localization.h"

namespace velo::ui {
namespace {

std::wstring Utf8ToWide(const std::string& value) {
    return velo::common::Utf8ToWide(value);
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        if (ch >= 'A' && ch <= 'Z') {
            return static_cast<char>(ch - 'A' + 'a');
        }
        return static_cast<char>(ch);
    });
    return value;
}

bool Contains(const std::string& text, const std::string& needle) {
    return text.find(needle) != std::string::npos;
}

}  // namespace

std::wstring FriendlyPlaybackError(const std::string& rawError, const std::wstring_view languageCode) {
    if (rawError.empty()) {
        return {};
    }

    const std::string normalized = ToLowerAscii(rawError);
    const auto language = velo::localization::ResolveLanguage(languageCode);
    if (Contains(normalized, "permission denied") || Contains(normalized, "access denied")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorAccessDenied);
    }
    if (Contains(normalized, "no such file") || Contains(normalized, "file not found") || Contains(normalized, "cannot open file") ||
        Contains(normalized, "failed to open")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorFileNotFound);
    }
    if (Contains(normalized, "decoder") || Contains(normalized, "decode") || Contains(normalized, "unsupported") ||
        Contains(normalized, "codec")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorDecoderFailure);
    }
    if (Contains(normalized, "gpu") || Contains(normalized, "d3d") || Contains(normalized, "vo") || Contains(normalized, "opengl") ||
        Contains(normalized, "vulkan")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorGpuFailure);
    }
    if (Contains(normalized, "audio") || Contains(normalized, "ao") || Contains(normalized, "wasapi")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorAudioFailure);
    }
    if (Contains(normalized, "subtitle") || Contains(normalized, "sub")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorSubtitleFailure);
    }
    if (Contains(normalized, "network") || Contains(normalized, "http") || Contains(normalized, "timed out")) {
        return velo::localization::Text(language, velo::localization::TextId::ErrorNetworkFailure);
    }
    if (Contains(normalized, "libmpv")) {
        return Utf8ToWide(rawError);
    }
    return velo::localization::Text(language, velo::localization::TextId::ErrorGenericPlaybackFailure);
}

}  // namespace velo::ui
