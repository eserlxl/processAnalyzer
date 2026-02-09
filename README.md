# processAnalyzer

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-95%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)

## Overview

**processAnalyzer** is a modern, high-performance system diagnostics tool for Linux. Built with C++23, it provides a powerful interface to the `/proc` filesystem, allowing developers and system administrators to inspect, monitor, and analyze processes with precision.

Whether you are debugging complex microservices, analyzing memory footprints, or tracing process hierarchies, `processAnalyzer` delivers the insights you need through a user-friendly command-line interface.

## Key Features

-   **Deep Process Inspection**: Analyze memory maps, open files, network connections (TCP/UDP), and thread details.
-   **Advanced Filtering**: precise filtering by PID, user, state, memory usage, and more.
-   **System-Wide Metrics**: Monitor global CPU load, memory utilization, and I/O statistics.
-   **Flexible Output Formats**: Export data as **table**, **vertical**, **JSON**, or **CSV** for easy integration with external tools.
-   **Modern Architecture**: Written in C++23 for maximum performance and efficiency.

For a detailed list of features, see [docs/features.md](docs/features.md).

## Installation

### Prerequisites

-   **Operating System**: Linux (Kernel 5.x+ recommended for full feature support via `/proc`).
-   **Compiler**: C++23 compatible compiler (GCC 12+ or Clang 16+).
-   **Build System**: CMake 3.17+ and a build tool (Make or Ninja).
-   **Version Control**: Git.

### Build from Source

```bash
# Clone the repository
git clone https://github.com/eserlxl/processAnalyzer.git
cd processAnalyzer

# Configure and build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The executable will be available at `build/processAnalyzer`.

For detailed build instructions and troubleshooting, see [docs/build.md](docs/build.md).

## Quick Start

Get started immediately with these common commands:

```bash
# List all running processes
./build/processAnalyzer list

# Find processes by name (e.g., 'sshd')
./build/processAnalyzer list --name sshd

# Show detailed info for a specific PID (including children and open files)
./build/processAnalyzer show --pid <PID> --children --open-files

# Equivalent positional PID command
./build/processAnalyzer pid <PID> --children --open-files

# Display help menu
./build/processAnalyzer --help
```

## Documentation

Comprehensive documentation is available in the `docs/` directory:

| Document | Description |
| :--- | :--- |
| [**Usage Guide**](docs/usage.md) | detailed command reference and examples. |
| [**API Reference**](docs/api-reference.md) | C++ API documentation for library integrators. |
| [**Project Structure**](docs/project-structure.md) | Overview of the codebase organization. |
| [**Configuration**](docs/configuration.md) | Configuration file options (Roadmap). |
| [**Utility Library**](docs/utils.md) | Guide to the internal `utils` library. |
| [**Changelog**](docs/changelog.md) | History of version changes. |

## Project Structure

The project follows a standard C++ directory layout:

-   `src/`: Source code (`main.cpp`, CLI logic, Analyzer core).
-   `include/`: Header files (API definitions).
-   `tests/`: Unit and integration tests (GoogleTest).
-   `docs/`: Project documentation.

## Testing

We use GoogleTest for ensuring code reliability.

```bash
cd build
ctest --output-on-failure
```

## Contributing

Contributions are welcome! Whether it's reporting bugs, suggesting features, or submitting pull requests.

Please read [docs/contributing.md](docs/contributing.md) for our contribution guidelines.

## License

This project is open-source software licensed under the [GNU General Public License v3.0](LICENSE).
