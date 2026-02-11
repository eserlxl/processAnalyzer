# API Reference

The `processAnalyzer` project exposes a C++ API primarily through the headers in the `include/` directory:

*   [`include/analyzer/core.h`](../include/analyzer/core.h): Defines the core `ProcessAnalyzer` class and related structures for process inspection and analysis.
*   [`include/utils/`](../include/utils/): This directory contains a collection of general-purpose utility headers for tasks like string manipulation, file handling, and system interactions. Each header is self-contained and must be included individually.

While formal Doxygen-generated documentation is not currently provided, you can examine these header files directly for detailed information on available classes, methods, and functions.

## Key API Components

### `ProcessAnalyzer` Class

The `ProcessAnalyzer` class (defined in `include/analyzer/Core.h`) is the primary interface for interacting with system processes. It provides methods for:

*   **Process Enumeration**: `getPids()`, `streamPids()`, `streamProcesses()`, `queryProcesses()`.
*   **Process Inspection**: `getProcessDetails()`, `getChildProcesses()`, `getParentProcess()`, `getAllDescendantProcesses()`.
*   **Process Context**: `getProcessMemoryMaps()`, `getProcessResourceLimits()`, `getProcessCgroupInfo()`, `getProcessOpenFileDetails()`, `getProcessEnvironment()`.
*   **System Metrics**: `getSystemMemoryInfo()`, `getSystemLoadAverage()`, `getSystemCpuStats()`, `getSystemDiskUsage()`.
*   **Network & I/O**: `getNetworkConnections()`, `getProcessDiskIoUsage()`, `getSystemDiskIoStats()`, `getNetworkInterfaceStats()`.
*   **Process Control**: `sendSignal()`, `setProcessNiceness()`, `setProcessCpuAffinity()`.

### Key Data Structures

*   `ProcessInfo`: Detailed information about a single process (PID, name, state, memory, CPU, etc.).
*   `ProcessFilter`: Criteria for filtering processes (by name, user, usage, regex, etc.).
*   `SystemMemoryInfo` & `SystemLoadAverage`: Overall system health metrics.
*   `NetworkConnection`: Details of process network activity (protocol, addresses, ports).
*   `ProcessDiskIoUsage`: Read/write rates for a process.
*   `ThreadInfo`: Details about threads within a process.
*   `MemoryMapInfo`: Process memory layout.
*   `OpenFileDescriptorInfo`: Details on files opened by a process.

### `utils` Namespace

The `utils` namespace offers a robust set of helper functions, with headers located in the `include/utils/` directory. These must be included directly (e.g., `#include "utils/string.h"`). Implementations are located in `src/utils/`. The library includes utilities for:

*   File system operations (`file.h`, `path.h`)
*   String manipulation (`string.h`)
*   System interactions (`system.h`)
*   Time and Type utilities (`time.h`, `types.h`)

For a comprehensive overview and usage examples of the utility library, please refer to **[docs/utils.md](utils.md)**.

## Usage Example

`processAnalyzer` can be used as a header-only or compiled library in your C++ projects. The following example demonstrates how to get a "snapshot" of all running processes.

```cpp
#include "analyzer/core.hh"
#include <iostream>

int main() {
    ProcessAnalyzer analyzer; // Manages access to /proc
    auto result = analyzer.snapshot();
    if (result.has_value()) {
        for (const auto& proc : result.value()) {
            std::cout << "PID: " << proc.pid << ", Name: " << proc.name << std::endl;
        }
    } else {
        std::cerr << "Error getting processes: " << result.error().message << std::endl;
    }
    return 0;
}
```

## Future Enhancements

We plan to integrate Doxygen or a similar tool in the future to generate comprehensive, browsable API documentation automatically.
