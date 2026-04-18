#include "ui/shortcut_display.h"

#include "localization/localization.h"
#include "platform/win32/input_profile.h"

namespace velo::ui {
namespace {

std::wstring MouseActionText(const velo::localization::AppLanguage language, std::wstring_view actionId) {
    if (actionId == L"toggle_pause") {
        return velo::localization::Text(language, velo::localization::TextId::SettingsMouseActionPauseResume);
    }
    if (actionId == L"play_next") {
        return velo::localization::Text(language, velo::localization::TextId::SettingsMouseActionPlayNext);
    }
    if (actionId == L"none" || actionId == L"show_info") {
        return velo::localization::Text(language, velo::localization::TextId::SettingsMouseActionNone);
    }
    return velo::localization::Text(language, velo::localization::TextId::SettingsMouseActionFullscreen);
}

void AppendBindingLine(std::wstring& text, const velo::config::AppConfig& config, std::wstring_view actionId, const std::wstring& label) {
    text += ShortcutDisplayName(config, actionId);
    text += L"  ";
    text += label;
    text += L'\n';
}

}  // namespace

std::wstring ShortcutDisplayName(const velo::config::AppConfig& config, std::wstring_view actionId) {
    return velo::platform::win32::VirtualKeyDisplayName(velo::platform::win32::ResolveVirtualKey(config, actionId));
}

std::wstring BuildShortcutHelpText(const velo::config::AppConfig& config) {
    const auto language = velo::localization::ResolveLanguage(config.languageCode);
    std::wstring text = velo::localization::Text(language, velo::localization::TextId::ShortcutHelpTitle) + L"\n\n";
    AppendBindingLine(text, config, L"toggle_pause", velo::localization::ActionLabel(language, L"toggle_pause"));
    AppendBindingLine(text, config, L"seek_backward", velo::localization::ActionLabel(language, L"seek_backward"));
    AppendBindingLine(text, config, L"seek_forward", velo::localization::ActionLabel(language, L"seek_forward"));
    AppendBindingLine(text, config, L"volume_up", velo::localization::ActionLabel(language, L"volume_up"));
    AppendBindingLine(text, config, L"volume_down", velo::localization::ActionLabel(language, L"volume_down"));
    AppendBindingLine(text, config, L"toggle_mute", velo::localization::ActionLabel(language, L"toggle_mute"));
    AppendBindingLine(text, config, L"cycle_audio", velo::localization::ActionLabel(language, L"cycle_audio"));
    AppendBindingLine(text, config, L"cycle_subtitle", velo::localization::ActionLabel(language, L"cycle_subtitle"));
    AppendBindingLine(text, config, L"take_screenshot", velo::localization::ActionLabel(language, L"take_screenshot"));
    AppendBindingLine(text, config, L"slower_speed", velo::localization::ActionLabel(language, L"slower_speed"));
    AppendBindingLine(text, config, L"faster_speed", velo::localization::ActionLabel(language, L"faster_speed"));
    AppendBindingLine(text, config, L"reset_speed", velo::localization::ActionLabel(language, L"reset_speed"));
    AppendBindingLine(text, config, L"play_previous", velo::localization::ActionLabel(language, L"play_previous"));
    AppendBindingLine(text, config, L"play_next", velo::localization::ActionLabel(language, L"play_next"));
    AppendBindingLine(text, config, L"open_file", velo::localization::ActionLabel(language, L"open_file"));
    AppendBindingLine(text, config, L"toggle_fullscreen", velo::localization::ActionLabel(language, L"toggle_fullscreen"));
    text += velo::localization::Text(language, velo::localization::TextId::ShortcutEscExitFullscreen) + L"\n";
    text += velo::localization::Text(language, velo::localization::TextId::ShortcutDoubleClick) + L"  " +
            MouseActionText(language, config.doubleClickAction) + L"\n";
    text += velo::localization::Text(language, velo::localization::TextId::ShortcutMiddleClick) + L"  " +
            MouseActionText(language, config.middleClickAction);
    return text;
}

}  // namespace velo::ui
