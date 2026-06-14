# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **CLI capability expansion:**
    - `show --descendants` prints a process's full descendant subtree, and `show --affinity` prints its CPU affinity mask.
    - `list --output tree` renders the listed processes as an indented parent/child forest keyed on ppid.
    - The `top` command now honors the standard process filters (`--name`, `--user`, `--state`, ...) to narrow the ranking.
    - `--name-regex` and `--cmdline-regex` filter processes by regular expression (validated at parse time).
    - The built-in `--help` now documents the full `top` option set, and `top`/`system` reject `--output` formats other than `json`.
    - The `summary` command aggregates the process population — total count, breakdown by state, zombie tally, and total thread, resident, and virtual memory footprint — with `--output json` for automation.
    - `--field <NAME>` emits a single column's raw value, one process per line (no header or quoting), for scripting; it accepts the same field names as `--columns`.
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
- **Real-Time Monitoring CLI:**
    - Added the `top` command, ranking processes by live CPU usage (via `getAllProcessesCpuUsage`) or disk I/O (`--io`, via `getAllProcessesDiskIoUsage`), with a configurable row limit (`--count N`).
    - Added `system --watch [SECONDS]` to continuously refresh the system report until interrupted.
    - Added `system --output json` to emit the full system metrics as a single machine-readable JSON object.
    - Added `getSystemActivityRates` (per-second context switches, interrupts, and forks) and its `System Activity Rates` section in the `system` command.
    - Added direct unit tests for the `Internal` process filter helpers and the JSON string escaper.

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
