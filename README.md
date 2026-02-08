# processAnalyzer

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/L-L-M/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)
[![Maintenance](https://img.shields.io/badge/Maintained-yes-green.svg)](https://github.com/L-L-M/processAnalyzer)


`processAnalyzer` is a high-performance, lightweight C++ command-line utility designed for the real-time inspection and monitoring of system processes. 

It provides a robust interface to deliver detailed resource usage metrics and execution statistics, facilitating efficient system diagnostics and performance optimization. Offering in-depth analysis of running processes, `processAnalyzer` helps you gain insights into system resource consumption and process behavior.

## Table of Contents
* [Features](#features)
* [Build](#build)
* [Quick Start](#quick-start)
* [Usage](#usage)
* [Project Structure](#project-structure)
* [Utility Library (`Utils` Namespace)](#utility-library-utils-namespace)
* [Contributing](#contributing)
* [API Reference](#api-reference)
* [Changelog](#changelog)
* [License](#license)

## Features

*   **List Processes**: Enumerate all running processes with key information.
*   **Process Details**: Obtain comprehensive details for a specific process ID (PID), including its child processes, parent process, descendants, environment variables, memory maps, resource limits, cgroup information, and open files/sockets/pipes.
*   **Process Filtering**: Filter processes by various criteria such as name, user, state, parent process ID (PPID), memory usage, executable path, command line arguments, CPU usage, memory percentage, or network connection attributes.
*   **Process Sorting**: Sort processes based on various fields like PID, user, name, memory usage, CPU time, start time, executable path, CPU usage percentage, memory usage percentage, etc.
*   **Customizable Output**: Choose which columns to display and output results in different formats (table, CSV, JSON).
*   **Real-time Monitoring**: Provides dynamic, real-time updates on process resource consumption and status, similar to 'top' or 'htop', including CPU and disk I/O usage per process and system-wide.
*   **System Metrics**: Monitor system-wide metrics such as total memory usage, load average, CPU statistics (user, system, idle), per-CPU usage, disk I/O per device, network interface statistics, and system activity (interrupts, context switches, forks).
*   **Network Activity**: Inspect detailed network connections (TCP, UDP, IPv4, IPv6) for individual processes.
*   **Thread Details**: Enumerate and inspect individual threads within a process.
*   **Process Control**: Send POSIX signals to processes, modify process niceness, and set CPU affinity.
*   **System Information**: Retrieve system uptime, kernel version, OS name, hostname, and mounted filesystem disk usage.
*   **C++23 Streaming API**: Utilize a modern C++23 `std::generator`-based API for efficient, lazy-loaded streaming of process information.
*   **Utility Library**: Leverages a robust C++ utility library for common tasks like file system operations, string manipulation, and system interaction.

## Build

To get `processAnalyzer` up and running, you'll need to build it from source.

**Prerequisites:**
*   Linux OS (relies on `/proc` filesystem)
*   C++23 compatible compiler (e.g., GCC, Clang)
*   CMake 3.17 or higher

For detailed build steps, please refer to the [Build Instructions](docs/build.md).

## Quick Start

Once `processAnalyzer` is built, you can quickly run it from the `build` directory.

To list all running processes:

```bash
./processAnalyzer list
```

> **Note:** Some process information may be restricted to the owner or the root user. If you don't see expected details, try running with `sudo`.

To see available commands and options:

```bash
./processAnalyzer --help
```

For more detailed usage examples and commands, please refer to the [Usage Guide](docs/usage.md).

## Usage

For comprehensive usage instructions, command-line arguments, and examples, please refer to the [Usage Guide](docs/usage.md).

## Project Structure

The project is organized to promote modularity and maintainability.

*   `src/`: Contains the core C++ source files for `processAnalyzer` and its utilities.
*   `include/`: Contains public header files.
*   `tests/`: Unit tests for various components of the project.
*   `docs/`: Additional documentation, including detailed guides and API references.
*   `CMakeLists.txt`: The primary CMake configuration file for the project.
*   `LICENSE`: Contains the licensing information for the project.

For a detailed breakdown of the project directory structure, see [docs/project-structure.md](docs/project-structure.md).

## Utility Library (Utils)

The project includes a robust `Utils` namespace in `src/utils.cpp` and `include/utils.h`. This library provides a collection of general-purpose utility functions for:

*   File system operations (reading/writing files, directory management)
*   String manipulation (trimming, splitting, joining, replacing, etc.)
*   Numeric parsing and validation
*   System interaction (environment variables, process execution)

**Recent updates include enhancements and a new API for improved functionality.** For a comprehensive overview and usage examples of the utility library, including details on the new API, please see [docs/UTILS.md](docs/UTILS.md).

## Contributing

We welcome contributions to `processAnalyzer`! Please see our [contribution guidelines](docs/contributing.md) for details on how to get started, report bugs, and suggest new features.

## API Reference

The core `processAnalyzer` API, including key classes and functions such as those defined in `include/Analyzer.h`, is detailed in the API Reference documentation. For comprehensive information on `processAnalyzer`'s C++ API, including classes, functions, and data structures, please refer to the [API Reference documentation](docs/api-reference.md).

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for a history of changes to the project.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for more details.


