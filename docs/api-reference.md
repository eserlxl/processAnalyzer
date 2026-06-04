# API Reference

The `processAnalyzer` project exposes a C++ API primarily through the headers in the `include/` directory. This document provides a high-level overview of the core components and their functionalities.

For the most detailed and up-to-date information, please consult the header files directly.

-   [`include/analyzer/core.h`](../include/analyzer/core.h): Defines the main `ProcessAnalyzer` class.
-   [`include/analyzer/process_model.h`](../include/analyzer/process_model.h): Defines data structures related to individual processes (`ProcessInfo`, `ThreadInfo`, `ProcessFilter`, etc.).
-   [`include/analyzer/system_model.h`](../include/analyzer/system_model.h): Defines data structures for system-wide metrics (`SystemMemoryInfo`, `SystemCpuStats`, etc.).
-   [`include/analyzer/network_model.h`](../include/analyzer/network_model.h): Defines data structures for network information (`NetworkConnection`, etc.).
-   [`include/utils/`](../include/utils/): Contains general-purpose utility headers for tasks like string manipulation, file handling, and system interactions.

---

## `ProcessAnalyzer` Class

The `ProcessAnalyzer` class (from `analyzer/core.h`) is the central entry point for all analysis and monitoring tasks. It provides a unified interface to the underlying `/proc` filesystem data.

```cpp

ProcessAnalyzer analyzer; // Create an instance
```

### Core Process Information
-   `getPids()`: Returns a `std::vector<pid_t>` of all currently running Process IDs (PIDs).
-   `getProcessDetails(pid)`: Fetches a comprehensive `ProcessInfo` struct for a specific PID, containing static and quasi-static data.
-   `snapshot()`: Returns a `std::vector<ProcessInfo>` containing details for all running processes at a single point in time. This is useful for creating a complete system snapshot.

### Process Hierarchy and Relationships
-   `getParentProcess(pid)`: Gets the parent process details for a given PID.
-   `getChildProcesses(pid)`: Gets a list of immediate child processes.
-   `getAllDescendantProcesses(pid)`: Recursively finds all descendant processes (children, grandchildren, etc.).

### Detailed Process Context
-   `getProcessThreads(pid)`: Retrieves detailed information about all threads belonging to a process.
-   `getProcessEnvironment(pid)`: Returns the environment variables of a process as a key-value map.
-   `getProcessMemoryMaps(pid)`: Fetches the memory mapping details, useful for understanding a process's address space.
-   `getProcessResourceLimits(pid)`: Gets the `rlimit` resource limits (e.g., max open files, stack size).
-   `getProcessCgroupInfo(pid)`: Retrieves cgroup membership information, critical for containerized environments.
-   `getProcessOpenFileDetails(pid)`: Lists all file descriptors opened by the process, including files, sockets, and pipes.
-   `getNetworkConnections(pid)`: Shows network connections (TCP, UDP, UNIX) associated with the process.

### Process Performance Metrics
-   `getProcessCpuUsage(pid, duration)`: Calculates the CPU usage of a single process as a percentage over a specified `std::chrono::duration`.
-   `getAllProcessesCpuUsage(duration)`: Calculates CPU usage for all currently running processes.
-   `getProcessDiskIoUsage(pid, duration)`: Measures the disk I/O (bytes read/written) of a single process over a duration.
-.  `getAllProcessesDiskIoUsage(duration)`: Measures disk I/O for all processes.

### Process Control & Manipulation
*Note: These functions often require elevated privileges.*
-   `sendSignal(pid, signal)`: Sends a signal (e.g., `SIGTERM`, `SIGKILL`) to a process.
-   `setProcessPriority(pid, niceValue)`: Adjusts the scheduling priority (niceness) of a process.
-   `getProcessCpuAffinity(pid)`: Returns the CPU affinity of a process as a `CpuSet` (list of CPU core IDs), parsed from `/proc/<pid>/status`.
-   `setProcessCpuAffinity(pid, affinity)`: Sets the CPU affinity, binding a process to specific CPU cores via `sched_setaffinity`.

### System-wide Information & Statistics
-   `getSystemInfo()`: Gets static system information (e.g., hostname, OS version, kernel version).
-   `getSystemBootTimeUnix()`: Returns the system boot time as a Unix timestamp.
-   `getSystemClockTicksPerSecond()`: Returns the number of clock ticks per second (`USER_HZ`), essential for interpreting certain `/proc` values.
-   `getSystemMemoryInfo()`: Retrieves system-wide memory and swap usage (total, free, available, cached).
-   `getSystemLoadAverage()`: Gets the 1, 5, and 15-minute system load averages.
-   `getSystemDiskUsage()`: Returns usage statistics for all mounted filesystems.
-   `getSystemDiskIoStats()`: Gets aggregated device-level disk I/O statistics.
-   `getNetworkInterfaceStats()`: Retrieves I/O statistics for all network interfaces.
-   `getSystemActivityStats()`: Provides system-wide statistics like context switches and total processes created since boot.
-   `getSystemCpuStats()`: Returns raw aggregate CPU time counters (user, nice, system, idle, iowait, irq, softirq, steal, guest, guestNice) from the first `cpu` line in `/proc/stat`.

### System Performance Metrics
-   `getSystemCpuStats()`: Gets raw CPU time statistics (user, system, idle, etc.) for all cores combined since boot.
-   `getSystemCpuUsage(duration)`: Calculates the overall system CPU utilization as a percentage over a given duration.
-   `getPerCpuUsage(duration)`: Calculates the CPU utilization for each CPU core individually.

### Process Query & Filtering
-   `queryProcesses(filter, sortBy, sortOrder)`: A powerful method to find, filter, and sort processes based on flexible criteria. The `filter` is a `ProcessFilter` struct, and sorting can be done on attributes like `cpu`, `memory`, `pid`, etc.

### C++23 Streaming API (Generators)
These methods provide an efficient, lazy-loaded way to iterate over processes without collecting them all in memory at once. This is ideal for performance-critical applications or tools that need to handle a very large number of processes.

**⚠️ Warning**: The returned `std::generator` holds a reference to the `ProcessAnalyzer` instance and its internal state. It must not be used after the `ProcessAnalyzer` is destroyed.

-   `streamPids()`: Streams all running PIDs one by one.
-   `streamProcesses()`: Streams `ProcessInfo` structs for all running processes.
-   `streamQueryProcesses(filter, sortBy, sortOrder)`: Lazily streams processes that match the given filter and sort criteria.



---

## Error Handling

The library uses `utils::Result<T>` (an alias for `std::expected`) to report errors. Most functions return a `utils::Result` which contains either the requested value or a `std::error_code`.

You should check the result before accessing the value.

```cpp
auto result = analyzer.getProcessDetails(99999); // A PID that likely doesn't exist
if (result) {
    ProcessInfo p = *result;
    // ...
} else {
    std::cerr << "Error fetching process details: " << result.error().message() << std::endl;
}
```

---

## Key Data Structures

The API uses several Plain Old Data (POD) structures to represent process and system information. These are defined in the `_model.h` headers.

*   `ProcessInfo`: A comprehensive struct holding most of the details for a single process (PID, name, state, memory, CPU times, owner, etc.).
*   `ProcessFilter`: A struct used with `queryProcesses` to specify filtering criteria (e.g., by name, user ID, memory usage, or a regex pattern).
*   `SystemMemoryInfo`: Contains total, free, available, and cached memory/swap values.
*   `SystemLoadAverage`: Holds the 1, 5, and 15-minute load averages.
*   `NetworkConnection`: Details of a network socket (protocol, local/remote addresses, TCP state).
*   For more details, please consult the header files in `include/analyzer/`.

---

## `utils` Namespace

The `utils` namespace offers a robust set of helper functions for common tasks, with headers located in the `include/utils/` directory. These must be included directly (e.g., `#include "utils/string.h"`).

Key utilities include string trimming, splitting, type conversions, file reading/writing, and path manipulation. For a comprehensive overview, please refer to **[docs/utils.md](utils.md)**.
