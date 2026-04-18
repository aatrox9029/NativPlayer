#include "diagnostics/process_memory.h"

#include <Windows.h>
#include <psapi.h>

namespace velo::diagnostics {
namespace {

double BytesToMb(const SIZE_T bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

}  // namespace

ProcessMemorySnapshot QueryCurrentProcessMemorySnapshot() {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);

    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
        return {};
    }

    return ProcessMemorySnapshot{
        .workingSetMb = BytesToMb(counters.WorkingSetSize),
        .privateBytesMb = BytesToMb(counters.PrivateUsage),
        .peakWorkingSetMb = BytesToMb(counters.PeakWorkingSetSize),
        .available = true,
    };
}

}  // namespace velo::diagnostics