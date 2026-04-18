#pragma once

#include <string>
#include <vector>

namespace velo::app {

struct CommandLineOptions {
    bool noSingleInstance = false;
    bool headless = false;
    std::vector<std::wstring> filesToOpen;
};

CommandLineOptions ParseCommandLine(int argc, wchar_t* argv[]);

}  // namespace velo::app