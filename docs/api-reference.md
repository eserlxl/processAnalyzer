# API Reference

The `processAnalyzer` project exposes a C++ API primarily through the headers in the `include/` directory. This document provides an overview of the core components.

While formal Doxygen-generated documentation is not currently provided, you can examine the header files directly for detailed information on available classes, methods, and data structures.

-   [`include/analyzer/core.h`](../include/analyzer/core.h): Defines the main `ProcessAnalyzer` class.
-   [`include/analyzer/process_model.h`](../include/analyzer/process_model.h): Defines data structures related to processes (`ProcessInfo`, `ThreadInfo`, etc.).
-   [`include/analyzer/system_model.h`](../include/analyzer/system_model.h): Defines data structures for system-wide metrics (`SystemMemoryInfo`, `SystemCpuStats`, etc.).
-   [`include/analyzer/network_model.h`](../include/analyzer/network_model.h): Defines data structures for network information (`NetworkConnection`, etc.).
-   [`include/utils/`](../include/utils/): Contains general-purpose utility headers for tasks like string manipulation, file handling, and system interactions.

---

## `ProcessAnalyzer` Class

The `ProcessAnalyzer` class (from `analyzer/core.h`) is the central entry point for all analysis and monitoring tasks.

```cpp
#include "analyzer/core.h"

ProcessAnalyzer analyzer; // Create an instance
```

### Core Process Information
-   `getPids()`: Returns a list of all currently running Process IDs (PIDs).
-   `getProcessDetails(pid)`: Fetches detailed `ProcessInfo` for a specific PID.
-   `snapshot()`: Returns a `std::vector<ProcessInfo>` containing details for all running processes at a single point in time.

### Process Hierarchy and Relationships
-   `getParentProcess(pid)`: Gets the parent process details for a given PID.
-   `getChildProcesses(pid)`: Gets a list of immediate child processes.
-   `getAllDescendantProcesses(pid)`: Recursively finds all descendant processes.

### Detailed Process Context
-   `getProcessThreads(pid)`: Retrieves information about all threads belonging to a process.
-alalyzer   `getProcessEnvironment(pid)`: Returns the environment variables of a process.
-   `getProcessMemoryMaps(pid)`: Fetches the memory mapping details for a process.
-   `getProcessResourceLimits(pid)`: Gets the `rlimit` resource limits.
-   `getProcessCgroupInfo(pid)`: Retrieves cgroup membership information.
-   `getProcessOpenFileDetails(pid)`: Lists all file descriptors opened by the process.
-   `getNetworkConnections(pid)`: Shows network connections associated with the process.

### Process Performance Metrics
-   `getProcessCpuUsage(pid, duration)`: Calculates the CPU usage of a single process over a specified duration.
-   `getAllProcessesCpuUsage(duration)`: Calculates CPU usage for all processes.
-   `getProcessDiskIoUsage(pid, duration)`: Measures the disk I/O of a single process over a duration.
-   `getAllProcessesDiskIoUsage(duration)`: Measures disk I/O for all processes.

### Process Control & Manipulation
-   `sendSignal(pid, signal)`: Sends a signal (e.g., `SIGTERM`) to a process.
-   `setProcessNiceness(pid, niceness)`: Adjusts the scheduling priority (niceness) of a process.
-   `setProcessCpuAffinity(pid, affinity)`: Sets the CPU affinity for a process.

### System-wide Information & Statistics
-   `getSystemInfo()`: Gets static system information (e.g., hostname, OS version).
-   `getSystemBootTimeUnix()`: Returns the system boot time as a Unix timestamp.
-   `getSystemClockTicksPerSecond()`: Returns the number of clock ticks per second (`USER_HZ`).
-   `getSystemMemoryInfo()`: Retrieves system-wide memory and swap usage.
-   `getSystemLoadAverage()`: Gets the 1, 5, and 15-minute load averages.
-   `getSystemDiskUsage()`: Returns usage statistics for all mounted filesystems.
-   `getSystemDiskIoStats()`: Gets device-level disk I/O statistics.
-   `getNetworkInterfaceStats()`: Retrieves statistics for all network interfaces.
-   `getSystemActivityStats()`: Provides system-wide context switches and process creation counts.

### System Performance Metrics
-   `getSystemCpuStats()`: Gets raw CPU time statistics (user, system, idle, etc.) for all cores combined.
-   `getSystemCpuUsage(duration)`: Calculates the overall system CPU utilization over a duration.
-   `getPerCpuUsage(duration)`: Calculates the CPU utilization for each core individually.

### Process Query & Filtering
-   `queryProcesses(filter, sortBy, sortOrder)`: A powerful method to find, filter, and sort processes based on flexible criteria defined in the `ProcessFilter` struct.

### C++23 Streaming API (Generators)
These methods provide an efficient, lazy-loaded way to iterate over processes without loading them all into memory at once.

**⚠️ Warning**: The returned `std::generator` holds a reference to the `ProcessAnalyzer` instance. It must not be used after the analyzer is destroyed.

-   `streamPids()`: Streams all running PIDs one by one.
-   `streamProcesses()`: Streams `ProcessInfo` for all running processes.
-   `streamQueryProcesses(filter, sortBy, sortOrder)`: Streams processes that match the given filter and sort criteria.

---

## Key Data Structures

The API uses several data structures to represent process and system information. These are defined in the `_model.h` headers.

*   `ProcessInfo`: A comprehensive struct holding most of the details for a single process (PID, name, state, memory, CPU, owner, etc.).
*   `ProcessFilter`: A struct used with `queryProcesses` to specify filtering criteria (e.g., by name, user, memory usage, or a regex pattern).
*   `SystemMemoryInfo`: Contains total, free, and available memory/swap.
*   `SystemLoadAverage`: Holds the 1, 5, and 15-minute load averages.
*   `NetworkConnection`: Details of a network socket (protocol, local/remote addresses, state).
*   For more details, please consult the header files in `include/analyzer/`.

---

## `utils` Namespace

The `utils` namespace offers a robust set of helper functions, with headers located in the `include/utils/` directory. These must be included directly (e.g., `#include "utils/string.h"`).

For a comprehensive overview of the utility library, please refer to **[docs/utils.md](utils.md)**.
