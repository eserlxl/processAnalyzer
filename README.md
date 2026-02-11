# processAnalyzer

<div align="center">

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-100%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Version](https://img.shields.io/badge/Version-1.0.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Maintenance](https://img.shields.io/badge/Maintained-Yes-green.svg)](https://github.com/eserlxl/processAnalyzer/pulse)

</div>

**processAnalyzer** is a high-performance system diagnostics tool and C++23 library for Linux. It offers a powerful and efficient interface to the `/proc` filesystem, allowing developers and administrators to monitor, analyze, and manage processes and system metrics with precision.

---

## 📑 Table of Contents

- [Key Features](#-key-features)
- [Project Structure](#️-project-structure)
- [System Requirements](#-system-requirements)
- [Build and Installation](#-build-and-installation)
  - [Prerequisites](#prerequisites)
  - [Basic Build Steps](#basic-build-steps)
- [CLI Quick Start](#-cli-quick-start)
- [Library Quick Start](#-library-quick-start)
- [Documentation](#-documentation)
- [Security Considerations](#-security-considerations)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🚀 Key Features

`processAnalyzer` offers a robust set of functionalities for in-depth system and process analysis:

*   **Lazy-Loaded Process Streaming**: Efficiently stream process data using C++23 `std::generator`.
*   **Deep Process Inspection**: Access detailed process context, including CPU, memory, disk I/O, threads, open files, network connections, and more.
*   **Process Hierarchy Traversal**: Navigate parent, child, and descendant process relationships.
*   **Advanced Filtering & Sorting**: Query processes by name, user, resource usage, and other attributes.
*   **Comprehensive System Metrics**: Monitor global and per-CPU usage, memory statistics, load average, and network interfaces.
*   **Process Control (API Only)**: Programmatically send signals to processes and adjust their priorities.

For a complete list of features, see [docs/features.md](docs/features.md).

---

## 🏗️ Project Structure

The project follows a standard CMake structure. For a detailed breakdown of the source code, headers, and build files, see the [Project Structure Guide](docs/project-structure.md).

---

## 💻 System Requirements

- **Operating System**: Linux (Kernel version 5.x or newer recommended)
- **Compiler**: A C++23 compatible compiler. This project uses `std::generator`, which requires:
  - GCC 14 or newer
  - Clang 17 or newer (with `libc++`)
- **Build Tools**:
  - CMake 3.17 or newer
  - Make or Ninja

---

## 🛠️ Build and Installation

To get started with `processAnalyzer`, you'll need to build it from source.

### Prerequisites
Ensure you have met the [System Requirements](#-system-requirements) before proceeding.

### Basic Build Steps

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release # Or Debug
cmake --build .
sudo cmake --install . # Optional: Install to system paths
```

For detailed instructions on building, installing, running tests, and advanced configuration, please refer to the [Build Guide](docs/build.md).

---
---

## ⚡ CLI Quick Start

Once built and installed, you can use the `processAnalyzer` command-line tool.

### Example: List all processes

To list all running processes with their PID and name:

```bash
processAnalyzer list
```

For a comprehensive guide to the command-line interface, including detailed commands, options, and practical examples, see the [Usage Guide](docs/usage.md).


## ⚡ Library Quick Start

The C++ API allows you to integrate process and system monitoring directly into your applications. You can link against the library by adding the project as a subdirectory in your CMake configuration.

**Example `main.cpp`:**
```cpp
#include <iostream>
#include "analyzer/analyzer.h"

int main() {
    try {
        processAnalyzer analyzer;
        // Stream all running processes and print their PID and name
        for (const auto& process : analyzer.streamProcesses()) {
            std.cout << "PID: " << process.pid
                      << ", Name: " << process.name << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

**Example `CMakeLists.txt`:**
```cmake
cmake_minimum_required(VERSION 3.17)
project(MyMonitor)

set(CMAKE_CXX_STANDARD 23)

# Add processAnalyzer as a subdirectory
add_subdirectory(path/to/processAnalyzer)

add_executable(MyMonitor main.cpp)
target_link_libraries(MyMonitor PRIVATE processAnalyzerLib)
```

For more examples, see the [Code Examples](docs/code-examples.md) and the complete [API Reference](docs/api-reference.md).

---

## 📚 Documentation

Detailed documentation is available in the [docs/](docs/) folder:

| Document | Description |
| :--- | :--- |
| [**Usage Guide**](docs/usage.md) | Command-line reference and examples. |
| [**API Reference**](docs/api-reference.md) | Comprehensive C++ API documentation. |
| [**Code Examples**](docs/code-examples.md) | C++ API integration examples. |
| [**Build Guide**](docs/build.md) | Detailed compilation and installation instructions. |
| [**Features**](docs/features.md) | An exhaustive list of all capabilities. |
| [**Configuration**](docs/configuration.md) | Instructions for customizing tool behavior. |
| [**Project Structure**](docs/project-structure.md) | A guide to the codebase organization. |
| [**Utility Library**](docs/utils.md) | A guide to the internal `utils` library. |
| [**Changelog**](docs/changelog.md) | A history of all version changes. |
| [**Testing**](docs/testing.md) | The project's testing strategy and instructions. |

---

## 🔒 Security Considerations

`processAnalyzer` interacts directly with the Linux `/proc` filesystem, which requires careful handling of permissions and data.

-   **Permissions**: Running the tool with `sudo` may be necessary to access detailed information for all processes.
-   **Output Handling**: Be cautious when exporting data (e.g., to JSON or CSV), as it may contain sensitive process or environment details.
-   **Integration**: When integrating the library, validate and sanitize all inputs to prevent potential security vulnerabilities.

---

## 🤝 Contributing

Contributions are welcome! Please read our [Contributing Guidelines](docs/contributing.md) to get started.

---

## 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
