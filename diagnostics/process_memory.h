#pragma once

namespace velo::diagnostics {

struct ProcessMemorySnapshot {
    double workingSetMb = 0.0;
    double privateBytesMb = 0.0;
    double peakWorkingSetMb = 0.0;
    bool available = false;
};

ProcessMemorySnapshot QueryCurrentProcessMemorySnapshot();

}  // namespace velo::diagnostics