# processAnalyzer

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-95%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)
[![Maintenance](https://img.shields.io/badge/Maintained-yes-green.svg)](https://github.com/eserlxl/processAnalyzer)

## Table of Contents
* [Overview](#overview)
* [Features](#features)
* [Installation](#installation)
* [Quick Start](#quick-start)
* [Usage](#usage)
* [API Reference](#api-reference)
* [Project Structure](#project-structure)
* [Utility Library (`utils` Namespace)](#utility-library-utils-namespace)
* [Testing](#testing)
* [Configuration](#configuration)
* [Contributing](#contributing)
* [Changelog](#changelog)
* [License](#license)

## Overview

`processAnalyzer` is a high-performance, lightweight C++ command-line utility designed for real-time inspection and monitoring of system processes on Linux. It provides developers, system administrators, and performance engineers with a powerful tool to gain deep insights into process behavior and resource consumption.

By leveraging the `/proc` filesystem, it offers a robust interface to deliver detailed resource usage metrics and execution statistics, facilitating efficient system diagnostics, performance optimization, and debugging. Whether you're troubleshooting a memory leak, analyzing CPU bottlenecks, or simply exploring the system's process landscape, `processAnalyzer` provides the clarity you need.

## Features

`processAnalyzer` provides a comprehensive suite of features for process monitoring, system diagnostics, and performance analysis.

Key capabilities include:
-   **Process Enumeration and Filtering**: List, filter, and sort processes by various criteria (name, user, usage, etc.).
-   **In-Depth Process Details**: Inspect process properties, including memory maps, open files, network connections, and environment variables.
-   **System-Wide Metrics**: Monitor overall system health, including CPU load, memory usage, and network statistics.
-   **Performance Analysis**: Analyze CPU and memory usage patterns.

For a complete list of features, please see the [Features documentation](docs/features.md).

## Installation

To get `processAnalyzer` up and running, you'll need to build it from source.

**Prerequisites:**
*   Linux OS (relies on `/proc` filesystem)
*   C++23 compatible compiler (e.g., GCC 12+, Clang 16+)
*   CMake 3.17 or higher
*   `git` for cloning the repository.
*   (Optional) `gtest` and `gmock` for running tests.

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

For detailed build steps, including running tests and coverage, please refer to the [Build Instructions](docs/build.md).

## Quick Start

Once built, the binary is located in `build/processAnalyzer`. If you ran `make install`, it will be in your system's path.

To verify it works, list the running processes:

```bash
./build/processAnalyzer list
```

> **Note:** Some process information may be restricted. Run with `sudo` if you need full system visibility.

To see all available commands and options:

```bash
./build/processAnalyzer --help
```

## Usage

The command-line interface follows a `processAnalyzer [command] [options]` structure.

Here are some common commands:

```bash
# List all running processes
./build/processAnalyzer list

# Filter by name and output as JSON
./build/processAnalyzer name chrome --format json

# Show details for a specific PID, including children and open files
./build/processAnalyzer pid 1234 --children --open-files

# Show details for a specific PID, including thread information
./build/processAnalyzer pid 1234 --threads

# List processes sorted by memory usage (RSS) in descending order
./build/processAnalyzer list --sort-by rss --desc
```

For comprehensive usage instructions, command-line arguments, and detailed examples, please refer to the [Usage Guide](docs/usage.md).

## API Reference

`processAnalyzer` exposes a C++ API for programmatic access to its process and system inspection capabilities. The primary interface is the `Analyzer` class, which provides methods for process enumeration, detailed inspection, and system metric retrieval.

For a detailed breakdown of the classes, functions, and data structures, please see the [API Reference documentation](docs/api-reference.md).

## Project Structure

The project is organized to promote modularity and maintainability.

*   `src/`: Contains the core C++ source files for `processAnalyzer` and its utilities.
*   `include/`: Contains public header files.
*   `tests/`: Unit tests for various components of the project.
*   `docs/`: Additional documentation, including detailed guides and API references.

For a detailed breakdown of the project directory structure, see [docs/project-structure.md](docs/project-structure.md).

## Utility Library (`utils` Namespace)

`processAnalyzer` includes a modern C++23 utility library (`utils` namespace) with robust, general-purpose functions for file systems, string manipulation, system interaction, and more. All utilities are accessible via the `<utils.h>` header.

For complete documentation, including functions, error codes, and usage examples, please refer to the **[Utils Library Documentation](docs/UTILS.md)**.

## Testing

`processAnalyzer` includes a comprehensive suite of unit tests to ensure reliability and correctness.

For instructions on how to build and run the tests, please refer to the [Build Instructions](docs/build.md#running-tests).

## Configuration

`processAnalyzer` is primarily configured via command-line arguments. While a `--config` option exists, support for external configuration files is currently **experimental** and under development.

For details on the planned configuration options and future file formats, please refer to the [Configuration documentation](docs/configuration.md).

## Contributing

We welcome contributions to `processAnalyzer`! Please see our [contribution guidelines](docs/contributing.md) for details on how to get started, report bugs, and suggest new features.

## Changelog

See [docs/changelog.md](docs/changelog.md) for a history of changes to the project.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for more details.

