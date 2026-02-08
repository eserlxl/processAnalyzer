# processAnalyzer

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-95%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)
[![Maintenance](https://img.shields.io/badge/Maintained-yes-green.svg)](https://github.com/eserlxl/processAnalyzer)

## Why processAnalyzer?

In a world of complex, containerized, and microservice-based architectures, understanding how processes interact with the system is more critical than ever. While Linux offers a wealth of diagnostic tools (`ps`, `top`, `lsof`, `netstat`), they often provide either too little or too much information, requiring complex combinations of commands and parsing to get a clear picture.

`processAnalyzer` was built to solve this problem by offering a **single, powerful, and user-friendly interface** to the `/proc` filesystem.

It is designed to be:
- **Comprehensive**: Get a holistic view of a process—from memory maps and open files to network connections and child processes—all in one place.
- **Efficient**: Written in modern C++23 for high performance and low overhead, making it suitable for production environments.
- **User-Friendly**: Features a clean command-line interface with clear, structured output (including JSON for easy parsing).
- **Modern**: Leverages the latest C++ features for robust, maintainable, and extensible code.

Whether you're a developer debugging a memory leak, a DevOps engineer monitoring containerized applications, or a system administrator troubleshooting a performance issue, `processAnalyzer` provides the detailed insights you need to resolve issues quickly and effectively.

## Table of Contents
* [Why processAnalyzer?](#why-processanalyzer)
* [Features](#features)
* [Installation](#installation)
* [Quick Start](#quick-start)
* [Usage Examples](#usage-examples)
* [API Reference](#api-reference)
* [Project Structure](#project-structure)
* [Utility Library (`utils` Namespace)](#utility-library-utils-namespace)
* [Build Details](#build-details)
* [Testing](#testing)
* [Configuration](#configuration)
* [Contributing](#contributing)
* [Changelog](#changelog)
* [License](#license)

## Features

`processAnalyzer` provides a comprehensive suite of features for process monitoring, system diagnostics, and performance analysis.

Key capabilities include:
-   **Process Enumeration and Filtering**: List, filter, and sort processes by name, user, state, and resource consumption.
-   **In-Depth Process Details**: Inspect critical process properties, including memory maps, open files, network connections, environment variables, and child processes.
-   **System-Wide Metrics**: Monitor overall system health, including CPU load, memory usage, and detailed network statistics.
-   **Flexible Output Formats**: Display data in human-readable tables or structured **JSON** for easy integration with other tools.
-   **Performance Analysis**: Analyze CPU and memory usage patterns to identify bottlenecks and optimize resource utilization.

For a complete list of features, please see the [Features documentation](docs/features.md).

## Installation

To get `processAnalyzer` up and running, you'll need to build it from source.

**Prerequisites:**
*   Linux OS (relies on the `/proc` filesystem)
*   C++23 compatible compiler (e.g., GCC 12+, Clang 16+)
*   CMake 3.17 or higher
*   `git` for cloning the repository.

**Build Steps:**

```bash
# 1. Clone the repository
git clone https://github.com/eserlxl/processAnalyzer.git
cd processAnalyzer

# 2. Configure and build the project
mkdir build && cd build
cmake ..
make

# 3. (Optional) Install the binary
sudo make install
```

## Quick Start

Once built, the binary is located at `build/bin/processAnalyzer`. If you ran `sudo make install`, it will be in your system's path.

To verify it works, list all running processes:

```bash
./build/bin/processAnalyzer list
```
> **Note:** Some process information may be restricted. Run with `sudo` for full system visibility.

To see all available commands and options, run:
```bash
./build/bin/processAnalyzer --help
```

## Usage Examples

The command-line interface follows a `processAnalyzer [command] [options]` structure. Here are some common examples.

### Process Listing and Filtering
```bash
# List all running processes in a tree-like view
./build/bin/processAnalyzer list --tree

# Filter by process name and user, and output as JSON
./build/bin/processAnalyzer list --name sshd --user root --output json

# List processes sorted by memory usage (RSS) in descending order
./build/bin/processAnalyzer list --sort-by rss --sort-order desc

# List all 'systemd' processes, showing only pid, name, and state
./build/bin/processAnalyzer list --name systemd --columns pid,name,state

# Find processes using more than 500MB of memory
./build/bin/processAnalyzer list --min-rss 500M
```

### Detailed Process Inspection
```bash
# Show full details for a specific PID, including children, open files, and network connections
./build/bin/processAnalyzer show --pid 1234 --children --open-files --network

# Inspect a process's memory maps
./build/bin/processAnalyzer show --pid 1234 --memory-maps

# Display the environment variables of a process
./build/bin/processAnalyzer show --pid 1234 --environment
```

For comprehensive usage instructions and all available command-line arguments, refer to the [**Usage Guide**](docs/usage.md).

## API Reference

`processAnalyzer` exposes a C++ API for programmatic access to its process and system inspection capabilities. The primary interface is the `ProcessAnalyzer` class, which provides methods for process enumeration, detailed inspection, and system metric retrieval.

For a detailed breakdown of the classes, functions, and data structures, see the [**API Reference**](docs/api-reference.md).

## Project Structure

The project is organized to promote modularity and maintainability.

*   `src/`: Contains the core C++ source files for `processAnalyzer` and its utilities.
*   `include/`: Contains public header files.
*   `tests/`: Unit tests for various components of the project.
*   `docs/`: Additional documentation, including detailed guides and API references.

For a detailed breakdown, see the [**Project Structure Guide**](docs/project-structure.md).

## Utility Library (`utils` Namespace)

`processAnalyzer` includes a modern C++23 utility library (`utils` namespace) with robust, general-purpose functions for file systems, string manipulation, system interaction, and more. All utilities are accessible via the `<utils.h>` header.

For complete documentation, including functions, error codes, and usage examples, refer to the **[Utils Library Documentation](docs/UTILS.md)**.

## Build Details

For detailed build steps, including how to run tests and generate coverage reports, please refer to the [**Build Instructions**](docs/build.md).

## Testing

`processAnalyzer` includes a comprehensive suite of unit tests built with Google Test to ensure reliability and correctness.

For instructions on how to build and run the tests, refer to the [**Build Instructions**](docs/build.md#running-tests).

## Configuration

`processAnalyzer` is primarily configured via command-line arguments. While a `--config-file` option exists, support for external configuration files is currently **experimental** and under development.

For details on planned configuration options, see the [**Configuration Documentation**](docs/configuration.md).

## Contributing

We welcome contributions to `processAnalyzer`! Please see our [**Contribution Guidelines**](docs/contributing.md) for details on how to get started, report bugs, and suggest new features.

## Changelog

See the [**Changelog**](docs/changelog.md) for a history of changes to the project.

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for more details.

