#pragma once

#include <filesystem>

namespace velo::diagnostics {

class SessionRecoveryTracker {
public:
    explicit SessionRecoveryTracker(std::filesystem::path rootDirectory);

    bool PrepareForStartup();
    void MarkCleanExit();
    [[nodiscard]] bool HadPreviousUncleanExit() const noexcept;
    [[nodiscard]] const std::filesystem::path& MarkerPath() const noexcept;

private:
    std::filesystem::path markerPath_;
    bool hadPreviousUncleanExit_ = false;
};

}  // namespace velo::diagnostics
