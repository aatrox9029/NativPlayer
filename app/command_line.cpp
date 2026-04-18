#include "app/command_line.h"

#include <string_view>

namespace velo::app {

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]) {
    CommandLineOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::wstring_view token = argv[index];
        if (token == L"--no-single-instance") {
            options.noSingleInstance = true;
            continue;
        }
        if (token == L"--headless") {
            options.headless = true;
            continue;
        }
        options.filesToOpen.emplace_back(token);
    }
    return options;
}

}  // namespace velo::app