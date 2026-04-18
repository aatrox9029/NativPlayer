#include "diagnostics/session_recovery.h"

#include <fstream>

namespace velo::diagnostics {

SessionRecoveryTracker::SessionRecoveryTracker(std::filesystem::path rootDirectory)
    : markerPath_(std::move(rootDirectory) / "session.pending") {}

bool SessionRecoveryTracker::PrepareForStartup() {
    hadPreviousUncleanExit_ = std::filesystem::exists(markerPath_);
    std::error_code error;
    std::filesystem::create_directories(markerPath_.parent_path(), error);
    std::ofstream output(markerPath_, std::ios::trunc);
    return output.is_open();
}

void SessionRecoveryTracker::MarkCleanExit() {
    std::error_code error;
    std::filesystem::remove(markerPath_, error);
}

bool SessionRecoveryTracker::HadPreviousUncleanExit() const noexcept {
    return hadPreviousUncleanExit_;
}

const std::filesystem::path& SessionRecoveryTracker::MarkerPath() const noexcept {
    return markerPath_;
}

}  // namespace velo::diagnostics
