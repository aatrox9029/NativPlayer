#include "playback/mpv_loader.h"

#include <string>

#include "common/text_encoding.h"
#include "playback/mpv_runtime_probe.h"

namespace velo::playback {
namespace {

std::string FormatWin32ErrorMessage(const DWORD errorCode) {
    LPSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageA(flags, nullptr, errorCode, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    if (length == 0 || buffer == nullptr) {
        return "Win32 error " + std::to_string(errorCode);
    }

    std::string message(buffer, length);
    LocalFree(buffer);
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n' || message.back() == ' ' || message.back() == '.')) {
        message.pop_back();
    }
    return message;
}

std::string ToUtf8(const std::wstring& value) {
    return velo::common::WideToUtf8(value);
}

}  // namespace

MpvLoader::~MpvLoader() {
    Unload();
}

bool MpvLoader::Load() {
    if (module_ != nullptr) {
        return true;
    }

    lastLoadDiagnostic_.clear();
    processArchitecture_ = velo::common::BinaryArchitectureName(velo::common::CurrentProcessArchitecture());

    for (const auto& candidate : MpvRuntimeCandidateNames()) {
        const auto probe = ProbeMpvRuntimeCandidate(candidate);
        processArchitecture_ = velo::common::BinaryArchitectureName(probe.processArchitecture);
        if (!probe.found) {
            continue;
        }
        if (probe.architectureMismatch) {
            lastLoadDiagnostic_ = probe.diagnosticMessage;
            continue;
        }

        module_ = LoadLibraryW(probe.libraryPath.c_str());
        if (module_ != nullptr) {
            loadedLibraryName_ = candidate;
            loadedLibraryPath_ = probe.libraryPath;
            loadedLibraryArchitecture_ = velo::common::BinaryArchitectureName(probe.libraryImage.architecture);
            return ResolveExports();
        }

        loadedLibraryName_.clear();
        loadedLibraryPath_.clear();
        loadedLibraryArchitecture_.clear();

        const DWORD loadError = GetLastError();
        if (loadError == ERROR_BAD_EXE_FORMAT && !probe.diagnosticMessage.empty()) {
            lastLoadDiagnostic_ = probe.diagnosticMessage;
        } else {
            lastLoadDiagnostic_ = "Failed to load libmpv from " + velo::common::WideToUtf8(probe.libraryPath.wstring()) +
                                  ": " + FormatWin32ErrorMessage(loadError);
        }
    }

    if (lastLoadDiagnostic_.empty()) {
        lastLoadDiagnostic_ =
            "Failed to locate libmpv. Searched application output and standard DLL search paths for mpv-2.dll, libmpv-2.dll, and mpv-1.dll.";
    }
    return false;
}

void MpvLoader::Unload() {
    if (module_ != nullptr) {
        FreeLibrary(module_);
        module_ = nullptr;
    }
    create = nullptr;
    initialize = nullptr;
    terminateDestroy = nullptr;
    setOptionString = nullptr;
    commandAsync = nullptr;
    setProperty = nullptr;
    getProperty = nullptr;
    observeProperty = nullptr;
    waitEvent = nullptr;
    errorString = nullptr;
    freeNodeContents = nullptr;
    loadedLibraryName_.clear();
    loadedLibraryPath_.clear();
    loadedLibraryArchitecture_.clear();
}

bool MpvLoader::IsLoaded() const noexcept {
    return module_ != nullptr;
}

std::string MpvLoader::LibraryName() const {
    return ToUtf8(loadedLibraryName_);
}

std::string MpvLoader::LibraryPath() const {
    return velo::common::WideToUtf8(loadedLibraryPath_.wstring());
}

std::string MpvLoader::LibraryArchitecture() const {
    return loadedLibraryArchitecture_;
}

std::string MpvLoader::ProcessArchitecture() const {
    return processArchitecture_;
}

std::string MpvLoader::LastLoadDiagnostic() const {
    return lastLoadDiagnostic_;
}

bool MpvLoader::ResolveExports() {
    create = reinterpret_cast<CreateFn>(GetProcAddress(module_, "mpv_create"));
    initialize = reinterpret_cast<InitializeFn>(GetProcAddress(module_, "mpv_initialize"));
    terminateDestroy = reinterpret_cast<TerminateDestroyFn>(GetProcAddress(module_, "mpv_terminate_destroy"));
    setOptionString = reinterpret_cast<SetOptionStringFn>(GetProcAddress(module_, "mpv_set_option_string"));
    commandAsync = reinterpret_cast<CommandAsyncFn>(GetProcAddress(module_, "mpv_command_async"));
    setProperty = reinterpret_cast<SetPropertyFn>(GetProcAddress(module_, "mpv_set_property"));
    getProperty = reinterpret_cast<GetPropertyFn>(GetProcAddress(module_, "mpv_get_property"));
    observeProperty = reinterpret_cast<ObservePropertyFn>(GetProcAddress(module_, "mpv_observe_property"));
    waitEvent = reinterpret_cast<WaitEventFn>(GetProcAddress(module_, "mpv_wait_event"));
    errorString = reinterpret_cast<ErrorStringFn>(GetProcAddress(module_, "mpv_error_string"));
    freeNodeContents = reinterpret_cast<FreeNodeContentsFn>(GetProcAddress(module_, "mpv_free_node_contents"));
    if (create == nullptr || initialize == nullptr || terminateDestroy == nullptr || setOptionString == nullptr ||
        commandAsync == nullptr || setProperty == nullptr || getProperty == nullptr || observeProperty == nullptr || waitEvent == nullptr ||
        errorString == nullptr || freeNodeContents == nullptr) {
        const std::string currentPath = LibraryPath();
        lastLoadDiagnostic_ = "Loaded libmpv from " + currentPath + " but required exports are missing.";
        Unload();
        return false;
    }
    return true;
}

}  // namespace velo::playback