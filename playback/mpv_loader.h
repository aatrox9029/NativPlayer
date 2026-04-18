#pragma once

#include <Windows.h>

#include <filesystem>
#include <string>

#include "mpv/client.h"

namespace velo::playback {

class MpvLoader {
public:
    using CreateFn = mpv_handle* (*)();
    using InitializeFn = int (*)(mpv_handle*);
    using TerminateDestroyFn = void (*)(mpv_handle*);
    using SetOptionStringFn = int (*)(mpv_handle*, const char*, const char*);
    using CommandAsyncFn = int (*)(mpv_handle*, uint64_t, const char* const[]);
    using SetPropertyFn = int (*)(mpv_handle*, const char*, mpv_format, void*);
    using GetPropertyFn = int (*)(mpv_handle*, const char*, mpv_format, void*);
    using ObservePropertyFn = int (*)(mpv_handle*, uint64_t, const char*, mpv_format);
    using WaitEventFn = const mpv_event* (*)(mpv_handle*, double);
    using ErrorStringFn = const char* (*)(int);
    using FreeNodeContentsFn = void (*)(mpv_node* node);

    ~MpvLoader();

    bool Load();
    void Unload();
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] std::string LibraryName() const;
    [[nodiscard]] std::string LibraryPath() const;
    [[nodiscard]] std::string LibraryArchitecture() const;
    [[nodiscard]] std::string ProcessArchitecture() const;
    [[nodiscard]] std::string LastLoadDiagnostic() const;

    CreateFn create = nullptr;
    InitializeFn initialize = nullptr;
    TerminateDestroyFn terminateDestroy = nullptr;
    SetOptionStringFn setOptionString = nullptr;
    CommandAsyncFn commandAsync = nullptr;
    SetPropertyFn setProperty = nullptr;
    GetPropertyFn getProperty = nullptr;
    ObservePropertyFn observeProperty = nullptr;
    WaitEventFn waitEvent = nullptr;
    ErrorStringFn errorString = nullptr;
    FreeNodeContentsFn freeNodeContents = nullptr;

private:
    bool ResolveExports();

    HMODULE module_ = nullptr;
    std::wstring loadedLibraryName_;
    std::filesystem::path loadedLibraryPath_;
    std::string loadedLibraryArchitecture_;
    std::string processArchitecture_;
    std::string lastLoadDiagnostic_;
};

}  // namespace velo::playback