#pragma once

#include <string>
#include <string_view>

namespace velo::diagnostics {
class Logger;
}

namespace velo::app {

struct MpvRuntimeBootstrapResult {
    bool ready = false;
    bool downloaded = false;
    std::wstring runtimePath;
    std::wstring detail;
    std::wstring effectiveLanguageCode;
};

MpvRuntimeBootstrapResult EnsureManagedMpvRuntimeAvailable(velo::diagnostics::Logger& logger, int networkTimeoutMs,
                                                           std::wstring_view languageCode);

}  // namespace velo::app
