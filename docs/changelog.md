# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **New Analyzer Features (Iterations 5, 7, 9, 13, 14):**
    - Enhanced `ProcessInfo` with fields for `startTimeUnix`, `elapsedTime`, `executablePath`, `currentWorkingDirectory`, `environmentVariables`, `cpuUserTimeTicks`, `cpuKernelTimeTicks`, `ioReadBytes`, `ioWriteBytes`, `priority`, `cpuUsage`, and `memoryPercentage`.
    - Implemented C++23 `std::generator` for lazy-loaded process streaming (`streamPids`, `streamProcesses`, `streamQueryProcesses`).
    - Added system-wide statistics: `SystemMemoryInfo`, `SystemLoadAverage`, `SystemCpuStats`, `SystemInfo` (uptime, kernel, OS, hostname), `SystemCpuUsage`, `PerCpuUsage`, `SystemDiskIoStats`, `NetworkInterfaceStats`, `SystemActivityStats`.
    - Introduced detailed process context: `getProcessMemoryMaps`, `getProcessResourceLimits`, `getProcessCgroupInfo`, `getProcessOpenFileDetails`.
    - Enhanced process navigation: `getChildProcesses`, `getParentProcess`, `getAllDescendantProcesses`, `getProcessEnvironment`.
    - Integrated network activity monitoring: `NetworkConnection` struct and `getNetworkConnections` method.
    - Added process disk I/O monitoring: `ProcessDiskIoUsage` struct and `getProcessDiskIoUsage`, `getAllProcessesDiskIoUsage` methods.
    - Implemented thread enumeration: `ThreadInfo` struct and `getProcessThreads` method.
    - Included system disk usage information: `MountPointInfo` struct and `getSystemDiskUsage` method.
    - Added process control capabilities: `sendSignal`, `setProcessNiceness`, `setProcessCpuAffinity`.
    - Extended `ProcessFilter` with regex capabilities, min/max for various metrics, network connection filtering, and CPU/memory usage percentage filtering.
    - Expanded `ProcessSortField` options to include `cpuTime`, `cpuUsage`, `memoryPercentage`, and more.

### Changed
- Refactored `Utils` library to use `std::expected` (as `Result<T>`) for robust error handling.
- Deprecated older utility functions using `std::optional` or out-parameters for errors.
- Enhanced `docs/utils.md` to reflect the new API and error handling patterns.
- Added `TestUtilsNewApiTest` for verifying the new Utility API.

## [0.1.0] - 2026-02-08

### Added
- Initial release of `processAnalyzer`.
- Feature: List all running processes.
- Feature: Get detailed information for a specific process.
- Feature: Filter processes by name and user.
- Feature: Sort processes by various fields.
- Feature: Customizable output columns and formats (CSV, JSON).
- Feature: Utility library for file system, string, and numeric operations.
- Added unit tests for core functionalities.
- Added comprehensive documentation.
