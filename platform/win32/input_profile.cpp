#include "platform/win32/input_profile.h"

#include <algorithm>
#include <map>

#include "localization/localization.h"

namespace velo::platform::win32 {
namespace {

bool SameTextInsensitive(std::wstring_view left, std::wstring_view right) {
    return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) ==
           CSTR_EQUAL;
}

const std::vector<InputBindingDefinition> kDefinitions = {
    {L"toggle_pause", L"", VK_SPACE},       {L"seek_backward", L"", VK_LEFT},      {L"seek_forward", L"", VK_RIGHT},
    {L"volume_up", L"", VK_UP},             {L"volume_down", L"", VK_DOWN},        {L"toggle_mute", L"", 'M'},
    {L"open_file", L"", 'O'},               {L"cycle_audio", L"", 'A'},
    {L"cycle_subtitle", L"", 'S'},          {L"take_screenshot", L"", VK_F12},      {L"slower_speed", L"", VK_OEM_4},
    {L"faster_speed", L"", VK_OEM_6},
    {L"reset_speed", L"", '0'},             {L"toggle_fullscreen", L"", 'F'},      {L"play_previous", L"", 'P'},
    {L"play_next", L"", 'N'},
};

const std::vector<KeyChoice> kChoices = {
    {0, L"None"},         {VK_SPACE, L"Space"}, {VK_LEFT, L"Left"},   {VK_RIGHT, L"Right"},
    {VK_UP, L"Up"},       {VK_DOWN, L"Down"},   {'A', L"A"},          {'F', L"F"},
    {'I', L"I"},          {'M', L"M"},          {'N', L"N"},          {'O', L"O"},
    {'P', L"P"},          {'R', L"R"},          {'S', L"S"},          {'0', L"0"},
    {VK_F12, L"F12"},
    {VK_OEM_4, L"["},     {VK_OEM_6, L"]"},     {VK_F11, L"F11"},     {VK_HOME, L"Home"},
    {VK_END, L"End"},     {VK_PRIOR, L"PageUp"}, {VK_NEXT, L"PageDown"}, {VK_RETURN, L"Enter"},
};

const InputBindingDefinition* FindDefinition(std::wstring_view actionId) {
    const auto it = std::find_if(kDefinitions.begin(), kDefinitions.end(), [&](const InputBindingDefinition& definition) {
        return SameTextInsensitive(definition.actionId, actionId);
    });
    return it == kDefinitions.end() ? nullptr : &(*it);
}

}  // namespace

const std::vector<InputBindingDefinition>& InputBindingDefinitions() {
    return kDefinitions;
}

const std::vector<KeyChoice>& SupportedKeyChoices() {
    return kChoices;
}

unsigned int ResolveVirtualKey(const velo::config::AppConfig& config, std::wstring_view actionId) {
    const auto configured = std::find_if(config.keyBindings.begin(), config.keyBindings.end(), [&](const velo::config::KeyBindingEntry& entry) {
        return SameTextInsensitive(entry.actionId, actionId);
    });
    if (configured != config.keyBindings.end()) {
        return configured->virtualKey;
    }

    const auto* definition = FindDefinition(actionId);
    return definition != nullptr ? definition->defaultVirtualKey : 0;
}

void SetVirtualKey(velo::config::AppConfig& config, std::wstring_view actionId, const unsigned int virtualKey) {
    auto existing = std::find_if(config.keyBindings.begin(), config.keyBindings.end(), [&](const velo::config::KeyBindingEntry& entry) {
        return SameTextInsensitive(entry.actionId, actionId);
    });
    if (existing == config.keyBindings.end()) {
        config.keyBindings.push_back({std::wstring(actionId), virtualKey});
        return;
    }
    existing->virtualKey = virtualKey;
}

std::wstring VirtualKeyDisplayName(const unsigned int virtualKey) {
    const auto choice = std::find_if(kChoices.begin(), kChoices.end(), [&](const KeyChoice& entry) {
        return entry.virtualKey == virtualKey;
    });
    if (choice != kChoices.end()) {
        return std::wstring(choice->label);
    }

    wchar_t buffer[32] = {};
    UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC) << 16;
    if (GetKeyNameTextW(static_cast<LONG>(scanCode), buffer, 32) > 0) {
        return buffer;
    }
    return std::to_wstring(virtualKey);
}

std::vector<std::wstring> FindBindingConflicts(const velo::config::AppConfig& config) {
    const auto language = velo::localization::ResolveLanguage(config.languageCode);
    std::map<unsigned int, std::wstring> usedBindings;
    std::vector<std::wstring> conflicts;
    for (const auto& definition : kDefinitions) {
        const unsigned int virtualKey = ResolveVirtualKey(config, definition.actionId);
        if (virtualKey == 0) {
            continue;
        }

        const std::wstring actionLabel = velo::localization::ActionLabel(language, definition.actionId);
        const auto existing = usedBindings.find(virtualKey);
        if (existing == usedBindings.end()) {
            usedBindings.emplace(virtualKey, actionLabel);
            continue;
        }

        conflicts.push_back(VirtualKeyDisplayName(virtualKey) + L" " +
                            velo::localization::Text(language, velo::localization::TextId::ConflictAlreadyUsedBy) + L" " +
                            existing->second + L" / " + actionLabel);
    }
    return conflicts;
}

bool MatchBinding(const WPARAM key, const velo::config::AppConfig& config, std::wstring_view actionId) {
    return static_cast<unsigned int>(key) == ResolveVirtualKey(config, actionId);
}

}  // namespace velo::platform::win32
