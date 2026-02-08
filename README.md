# processAnalyzer

[![Build Status](https://img.shields.io/badge/Build-Passing-green.svg)](https://github.com/L-L-M/processAnalyzer/actions)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/L-L-M/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)
[![Maintenance](https://img.shields.io/badge/Maintained-yes-green.svg)](https://github.com/L-L-M/processAnalyzer)

## Table of Contents
* [Overview](#overview)
* [Features](#features)
* [Installation](#installation)
* [Quick Start](#quick-start)
* [Usage](#usage)
* [API Reference](#api-reference)
* [Project Structure](#project-structure)
* [Utility Library (`utils` Namespace)](#utility-library-utils-namespace)
* [Configuration](#configuration)
* [Contributing](#contributing)
* [Changelog](#changelog)
* [License](#license)

## Overview

`processAnalyzer` is a high-performance, lightweight C++ command-line utility designed for the real-time inspection and monitoring of system processes. 

It provides a robust interface to deliver detailed resource usage metrics and execution statistics, facilitating efficient system diagnostics and performance optimization. Offering in-depth analysis of running processes, `processAnalyzer` helps you gain insights into system resource consumption and process behavior.

## Features

`processAnalyzer` provides a comprehensive suite of features for process monitoring, system diagnostics, and real-time performance analysis.

Key capabilities include:
-   **Process Enumeration and Filtering**: List, filter, and sort processes by various criteria.
-   **In-Depth Process Details**: Inspect process properties, including memory maps, open files, and network connections.
-   **Real-time Monitoring**: Get live updates on CPU, memory, and I/O usage, similar to `htop`.
-   **System-Wide Metrics**: Monitor overall system health, including CPU load, memory usage, and network statistics.

For a complete list of features, please see the [Features documentation](docs/features.md).

## Installation

To get `processAnalyzer` up and running, you'll need to build it from source.

**Prerequisites:**
*   Linux OS (relies on `/proc` filesystem)
*   C++23 compatible compiler (e.g., GCC, Clang)
*   CMake 3.17 or higher

**Quick Build:**

```bash
git clone https://github.com/L-L-M/processAnalyzer.git
cd processAnalyzer
mkdir build && cd build
cmake ..
make
```

For detailed build steps, including running tests and coverage, please refer to the [Build Instructions](docs/build.md).

## Quick Start

Once built, the binary is located in `build/processAnalyzer`.

To verify it works, list the running processes:

```bash
./processAnalyzer list
```

> **Note:** Some process information may be restricted. Run with `sudo` if you need full system visibility.

To see all available commands and options:

```bash
./processAnalyzer --help
```

## Usage

Here are some common commands to get you started:

```bash
# List all running processes
./processAnalyzer list

# Filter by name and output as JSON
./processAnalyzer name chrome --format json

# Show details for a specific PID, including children and open files
./processAnalyzer pid 1234 --children --open-files

# List processes sorted by memory usage (RSS) in descending order
./processAnalyzer list --sort-by rss --desc
```

For comprehensive usage instructions, command-line arguments, and detailed examples, please refer to the [Usage Guide](docs/usage.md).

## API Reference

The core `processAnalyzer` API, including key classes and functions such as those defined in `include/Analyzer.h`, is detailed in the API Reference documentation.

For comprehensive information on `processAnalyzer`'s C++ API, including classes, functions, and data structures, please refer to the [API Reference documentation](docs/api-reference.md).

## Project Structure

The project is organized to promote modularity and maintainability.

*   `src/`: Contains the core C++ source files for `processAnalyzer` and its utilities.
*   `include/`: Contains public header files.
*   `tests/`: Unit tests for various components of the project.
*   `docs/`: Additional documentation, including detailed guides and API references.

For a detailed breakdown of the project directory structure, see [docs/project-structure.md](docs/project-structure.md).

## Utility Library (`utils` Namespace)

`processAnalyzer` includes a powerful, modern C++23 utility library that provides a robust foundation for the application. It features a `std::expected`-based error handling model and offers a rich set of functions for:

-   File system and path manipulation
-   String processing and encoding (Base64, URL)
-   System interaction and command execution
-   Time and hashing utilities

For complete documentation of all functions, error codes, and usage examples, please refer to the **[Utils Library Documentation](docs/UTILS.md)**.

## Configuration

`processAnalyzer` supports loading configuration settings from a specified file. This allows for persistent customization of behavior and default parameters.

For details on available configuration options and file formats, please refer to the [Configuration documentation](docs/configuration.md).

## Contributing

We welcome contributions to `processAnalyzer`! Please see our [contribution guidelines](docs/contributing.md) for details on how to get started, report bugs, and suggest new features.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for a history of changes to the project.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for more details.

