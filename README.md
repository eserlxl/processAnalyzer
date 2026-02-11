# processAnalyzer

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-95%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Maintenance](https://img.shields.io/badge/Maintained-Yes-green.svg)](https://github.com/eserlxl/processAnalyzer/pulse)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)

**processAnalyzer** is a high-performance system diagnostics tool and C++23 library for Linux. It offers a powerful and efficient interface to the `/proc` filesystem, allowing developers and administrators to monitor, analyze, and manage processes and system metrics with precision.

---

## 📑 Table of Contents

- [Key Features](#-key-features)
- [Project Structure](#️-project-structure)
- [System Requirements](#-system-requirements)
- [Build and Installation](#-build-and-installation)
- [Quick Start](#-quick-start)
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

Ensure the [system requirements](#-system-requirements) are met. All major dependencies are fetched automatically by CMake during the build process.

1.  **Clone the Repository**
    ```bash
    git clone https://github.com/eserlxl/processAnalyzer.git
    cd processAnalyzer
    ```

2.  **Configure and Build**

    You can use standard CMake commands:
    ```bash
    # Configure using the Release preset
    cmake --preset release
    # Build
    cmake --build --preset release
    ```
    The executable will be located at `build/processAnalyzer`.

For advanced build options, see the [Build Guide](docs/build.md).

---

## ⚡ Quick Start

### Command-Line Interface (CLI)

After building, you can run `processAnalyzer` from the `build` directory.

```bash
# List all running processes
./build/processAnalyzer list

# Find processes by name and sort by RSS memory (descending)
./build/processAnalyzer list --name nginx --sort-by rss --sort-order desc

# Show detailed info for a specific PID (may require sudo)
# This example includes children and open files for the given process
sudo ./build/processAnalyzer show --pid 1 --children --open-files

# Display the help menu
./build/processAnalyzer --help
```

For a full command reference, see the [Usage Guide](docs/usage.md).

### C++ Library Usage

The C++ API allows you to integrate process and system monitoring directly into your applications. You can link against the library by adding the project as a subdirectory in your CMake configuration.

**Example `main.cpp`:**
```cpp
#include <iostream>
#include "analyzer/core.h"

int main() {
    try {
        ProcessAnalyzer analyzer;
        // Stream all running processes and print their PID and name
        for (const auto& process : analyzer.streamProcesses()) {
            std::cout << "PID: " << process.pid
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
