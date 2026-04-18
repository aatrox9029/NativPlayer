#include <Windows.h>

#include <shellapi.h>

#include "app/application.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr) {
        return 1;
    }

    const auto options = velo::app::ParseCommandLine(argc, argv);
    LocalFree(argv);

    velo::app::Application application(instance, options);
    return application.Run(showCommand);
}