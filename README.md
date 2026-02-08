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

In a world of complex, containerized, and microservice-based architectures, understanding how processes interact with the system is more critical than ever. `processAnalyzer` solves this by offering a **single, powerful, and user-friendly interface** to the `/proc` filesystem.

It is designed to be **Comprehensive**, **Efficient** (C++23), **User-Friendly**, and **Modern**. Whether you're debugging a memory leak or monitoring containers, `processAnalyzer` provides the insights you need.

## Features

`processAnalyzer` provides a comprehensive suite of features for process monitoring and system diagnostics.

-   **Process Enumeration and Filtering**: Filter by name, user, state, resources, etc.
-   **In-Depth Process Details**: Memory maps, open files, network connections, child processes.
-   **System-Wide Metrics**: CPU load, memory usage, network stats.
-   **Flexible Output**: Table, CSV, JSON.

See [docs/features.md](docs/features.md) for a complete feature list.

## Installation

**Prerequisites:** Linux OS, C++23 Compiler, CMake 3.17+, git.

```bash
git clone https://github.com/eserlxl/processAnalyzer.git
cd processAnalyzer
mkdir build && cd build
cmake .. && make
```

See [docs/build.md](docs/build.md) for detailed build and install instructions.

## Quick Start

The binary is located at `build/bin/processAnalyzer`.

```bash
# List all running processes
./build/bin/processAnalyzer list

# Show help
./build/bin/processAnalyzer --help
```

## Usage Examples

```bash
# Filter by name and output as JSON
./build/bin/processAnalyzer list --name sshd --output json

# Show full details for a specific PID (children, files, network)
./build/bin/processAnalyzer show --pid 1234 --children --open-files --network
```

See [docs/usage.md](docs/usage.md) for more examples and full command reference.

## Documentation

*   **[API Reference](docs/api-reference.md)**: C++ API documentation for developers.
*   **[Project Structure](docs/project-structure.md)**: Overview of the codebase organization.
*   **[Utility Library](docs/UTILS.md)**: Documentation for the `utils` namespace.
*   **[Build & Testing](docs/build.md)**: detailed build instructions and how to run tests.
*   **[Configuration](docs/configuration.md)**: Configuration options details.
*   **[Changelog](docs/changelog.md)**: History of changes.

## Contributing

We welcome contributions! Please see [docs/contributing.md](docs/contributing.md) for guidelines.

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for more details.
