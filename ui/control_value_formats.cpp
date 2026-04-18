#include "ui/control_value_formats.h"

#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <optional>
#include <sstream>

namespace velo::ui {
namespace {

std::wstring Trimmed(std::wstring text) {
    while (!text.empty() && std::iswspace(text.front()) != 0) {
        text.erase(text.begin());
    }
    while (!text.empty() && std::iswspace(text.back()) != 0) {
        text.pop_back();
    }
    return text;
}

}  // namespace

std::wstring FormatVolumeInput(const int volume) {
    return std::to_wstring(std::clamp(volume, 0, 100));
}

std::wstring FormatSpeedInput(const double speed) {
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(2) << std::clamp(speed, 0.25, 3.0) << L"x";
    return stream.str();
}

std::optional<int> ParseVolumeInput(const std::wstring& text) {
    const std::wstring trimmed = Trimmed(text);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    size_t consumed = 0;
    const int value = std::stoi(trimmed, &consumed);
    if (consumed != trimmed.size()) {
        return std::nullopt;
    }
    return std::clamp(value, 0, 100);
}

std::optional<double> ParseSpeedInput(const std::wstring& textValue) {
    std::wstring text = Trimmed(textValue);
    if (text.empty()) {
        return std::nullopt;
    }
    if (!text.empty() && (text.back() == L'x' || text.back() == L'X')) {
        text.pop_back();
    }
    text = Trimmed(std::move(text));
    if (text.empty()) {
        return std::nullopt;
    }

    size_t consumed = 0;
    const double value = std::stod(text, &consumed);
    if (consumed != text.size()) {
        return std::nullopt;
    }
    return std::clamp(value, 0.25, 3.0);
}

}  // namespace velo::ui
