# processAnalyzer

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-95%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)

processAnalyzer is a modern, high-performance system diagnostics tool and C++ library for Linux. Built with **C++23**, it provides a powerful and efficient interface to the `/proc` filesystem, allowing developers and system administrators to inspect, monitor, and analyze processes and system-wide metrics with precision.

It delivers detailed resource usage metrics and execution statistics to facilitate efficient system diagnostics and performance optimization.

---

## 📑 Table of Contents

- [Key Features](#-key-features)
- [Installation](#-installation)
- [Quick Start](#-quick-start)
- [Documentation](#-documentation)
- [Security Considerations](#-security-considerations)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🚀 Key Features

`processAnalyzer` offers a robust set of functionalities for in-depth system and process analysis:

*   **Lazy-Loaded Process Streaming**: Efficiently stream process data using C++23 `std::generator`.
*   **Deep Process Inspection**: Access CPU, memory, disk I/O, threads, open files, and network connections.
*   **Advanced Filtering & Sorting**: Query processes by name, user, usage, etc.
*   **System Metrics**: Monitor global CPU, memory, and uptime.
*   **Process Control**: Send signals and adjust niceness/affinity.

For a complete list of features, see [docs/features.md](docs/features.md).

---

## 🏗️ Project Structure

The project follows a standard CMake structure. Detailed organization of the source code, headers, and build files can be found in [docs/project-structure.md](docs/project-structure.md).

---

## 📦 Installation

### Prerequisites

- **OS**: Linux (Kernel 5.x+)
- **Compiler**: GCC 12+ or Clang 16+ (C++23 support required)
- **Build Tools**: CMake 3.17+ and Make/Ninja

### Build Steps

1.  **Clone the Repository**
    ```bash
    git clone https://github.com/eserlxl/processAnalyzer.git
    cd processAnalyzer
    ```

2.  **Configure and Build**
    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ```

The executable will be available at `build/bin/processAnalyzer`. See [docs/build.md](docs/build.md) for advanced build options.

---

## ⚡ Quick Start

After building, you can run `processAnalyzer` from the `build/bin` directory.

```bash
# List all running processes
./build/bin/processAnalyzer list

# Find processes by name and sort by RSS memory (descending)
./build/bin/processAnalyzer list --name nginx --sort-by rss --sort-order desc

# Show detailed info for a specific PID
./build/bin/processAnalyzer show --pid 1234 --children --open-files

# Display help menu
./build/bin/processAnalyzer --help
```

For a full command reference, see [docs/usage.md](docs/usage.md).

---

## 📚 Documentation

Detailed documentation is available in the [docs/](docs/) folder:

| Document | Description |
| :--- | :--- |
| [**Usage Guide**](docs/usage.md) | Command-line reference and examples. |
| [**API Reference**](docs/api-reference.md) | Comprehensive C++ API details. |
| [**Code Examples**](docs/examples.md) | C++ API integration examples. |
| [**Build Details**](docs/build.md) | Compilation and installation guide. |
| [**Features**](docs/features.md) | Exhaustive list of capabilities. |
| [**Configuration**](docs/configuration.md) | Customizing tool behavior. |
| [**Project Structure**](docs/project-structure.md) | Codebase organization. |
| [**Utility Library**](docs/utils.md) | Guide to the internal `utils` library. |
| [**Changelog**](docs/changelog.md) | History of version changes. |
| [**Testing**](docs/testing.md) | Testing strategy and instructions. |

---

## 🔒 Security Considerations

`processAnalyzer` interacts directly with the Linux `/proc` filesystem.

-   **Permissions**: Running with `sudo` grants access to sensitive process info.
-   **Output Handling**: Secure exported data (JSON/CSV) containing sensitive info.
-   **Integration**: Validate inputs when using with external scripts.

---

## 🤝 Contributing

Contributions are welcome! Please see [docs/contributing.md](docs/contributing.md) for guidelines.

---

## 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
