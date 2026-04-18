#include "common/hex_color.h"

#include <cwctype>
#include <iomanip>
#include <sstream>

namespace velo::common {
namespace {

bool IsHexDigit(const wchar_t ch) {
    return std::iswxdigit(ch) != 0;
}

std::wstring NormalizeEightDigitHex(std::wstring_view value) {
    std::wstring normalized;
    normalized.reserve(8);
    for (const wchar_t ch : value) {
        if (!IsHexDigit(ch)) {
            return {};
        }
        normalized.push_back(static_cast<wchar_t>(std::towupper(ch)));
    }
    return normalized;
}

int ParseHexByte(const wchar_t high, const wchar_t low) {
    auto parseDigit = [](const wchar_t ch) -> int {
        if (ch >= L'0' && ch <= L'9') {
            return ch - L'0';
        }
        if (ch >= L'A' && ch <= L'F') {
            return 10 + (ch - L'A');
        }
        if (ch >= L'a' && ch <= L'f') {
            return 10 + (ch - L'a');
        }
        return -1;
    };

    const int hi = parseDigit(high);
    const int lo = parseDigit(low);
    if (hi < 0 || lo < 0) {
        return -1;
    }
    return (hi << 4) | lo;
}

}  // namespace

std::wstring NormalizeRgbaHexColor(const std::wstring_view value, const std::wstring_view fallback) {
    const auto fallbackNormalized = NormalizeEightDigitHex(fallback);
    if (value.size() == 8) {
        const auto normalized = NormalizeEightDigitHex(value);
        return normalized.empty() ? fallbackNormalized : normalized;
    }
    if (value.size() == 6) {
        const auto normalized = NormalizeEightDigitHex(value);
        return normalized.empty() ? fallbackNormalized : normalized + L"FF";
    }
    return fallbackNormalized;
}

std::optional<RgbaColor> TryParseRgbaColor(const std::wstring_view value) {
    const std::wstring normalized = NormalizeRgbaHexColor(value, L"");
    if (normalized.size() != 8) {
        return std::nullopt;
    }

    const int red = ParseHexByte(normalized[0], normalized[1]);
    const int green = ParseHexByte(normalized[2], normalized[3]);
    const int blue = ParseHexByte(normalized[4], normalized[5]);
    const int alpha = ParseHexByte(normalized[6], normalized[7]);
    if (red < 0 || green < 0 || blue < 0 || alpha < 0) {
        return std::nullopt;
    }

    return RgbaColor{static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue),
                     static_cast<std::uint8_t>(alpha)};
}

std::wstring FormatRgbaHexColor(const RgbaColor& value) {
    std::wostringstream stream;
    stream << std::uppercase << std::hex << std::setfill(L'0')
           << std::setw(2) << static_cast<int>(value.red)
           << std::setw(2) << static_cast<int>(value.green)
           << std::setw(2) << static_cast<int>(value.blue)
           << std::setw(2) << static_cast<int>(value.alpha);
    return stream.str();
}

std::optional<COLORREF> TryParseRgbColor(const std::wstring_view value) {
    const auto parsed = TryParseRgbaColor(value);
    if (!parsed.has_value()) {
        return std::nullopt;
    }
    return RGB(parsed->red, parsed->green, parsed->blue);
}

}  // namespace velo::common
