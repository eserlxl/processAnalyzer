# API Reference

The `processAnalyzer` project exposes a C++ API primarily through the headers in the `include/` directory:

*   [`include/analyzer/Core.h`](../include/analyzer/Core.h): Defines the core `ProcessAnalyzer` class and related structures for process inspection and analysis.
*   [`include/utils.h`](../include/utils.h): Provides a collection of general-purpose utility functions used throughout the project, often within the `Utils` namespace.

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

The `utils` namespace offers a robust set of helper functions. The main header is `include/utils.h`, which includes the modular components from the `include/utils/` directory. The implementations are located in the `src/utils/` directory. The library includes:

*   File system operations.
*   String manipulation.
*   System interactions.
*   Time and Type utilities.

For a comprehensive overview and usage examples of the utility library, please refer to **[docs/UTILS.md](UTILS.md)**.

## Future Enhancements

We plan to integrate Doxygen or a similar tool in the future to generate comprehensive, browsable API documentation automatically.
