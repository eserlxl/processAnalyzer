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

In a world of complex, containerized, and microservice-based architectures, understanding how processes interact with the system is critical. `processAnalyzer` solves this by offering a **single, powerful, and user-friendly interface** to the `/proc` filesystem.

It is designed to be **Comprehensive**, **Efficient** (C++23), and **Modern**. Whether you're debugging a memory leak, analyzing performance bottlenecks, or monitoring containers, `processAnalyzer` provides the deep insights you need.

## Features

`processAnalyzer` provides a suite of tools for process monitoring and system diagnostics:

-   **Process Enumeration & Filtering**: List processes and filter by name, user, state, memory usage, and more.
-   **Deep Inspection**: Get detailed info including memory maps, open files, network connections (TCP/UDP), and process hierarchy.
-   **System Metrics**: Monitor global stats like CPU load, memory usage, disk I/O, and network interface traffic.
-   **Flexible Output**: Export data as formatted tables, CSV, or JSON for easy integration with other tools.

See [docs/features.md](docs/features.md) for a complete feature list.

## Installation

**Prerequisites:**
-   Linux OS (relies on `/proc` filesystem)
-   C++23 Compiler (GCC 12+ / Clang 16+)
-   CMake 3.17+
-   git

### Build from Source

```bash
git clone https://github.com/eserlxl/processAnalyzer.git
cd processAnalyzer
mkdir build && cd build
cmake ..
make
```

The executable will be located at `build/processAnalyzer`.

See [docs/build.md](docs/build.md) for detailed build and install instructions.

## Quick Start

```bash
# List all running processes
./build/processAnalyzer list

# Show help
./build/processAnalyzer --help
```

## Usage Examples

```bash
# Filter by name and output as JSON
./build/processAnalyzer list --name sshd --output json

# Show full details for a specific PID (children, files, network)
./build/processAnalyzer show --pid 1234 --children --open-files --network

# Sort by resident memory usage in descending order
./build/processAnalyzer list --sort-by rss --sort-order desc
```

See [docs/usage.md](docs/usage.md) for more examples and full command reference.

## Documentation

*   **[API Reference](docs/api-reference.md)**: C++ API documentation for developers.
*   **[Project Structure](docs/project-structure.md)**: Overview of the codebase organization.
*   **[Utility Library](docs/utils.md)**: Documentation for the `utils` namespace.
*   **[Build & Testing](docs/build.md)**: Detailed build instructions and test execution.
*   **[Configuration](docs/configuration.md)**: Details on configuration options (Roadmap).
*   **[Changelog](docs/changelog.md)**: History of changes.

## Testing

`processAnalyzer` includes a comprehensive test suite using Google Test.

To run the tests after building:

```bash
cd build
ctest --output-on-failure
```

For more details on running specific tests or generating coverage reports, see [docs/build.md](docs/build.md).

## Configuration

Support for external configuration files is currently **experimental**. The tool is primarily designed to be configured via command-line arguments.

See [docs/configuration.md](docs/configuration.md) for the roadmap and current status.

## Contributing

We welcome contributions! Please see [docs/contributing.md](docs/contributing.md) for guidelines.

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for more details.
