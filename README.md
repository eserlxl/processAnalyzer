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
- [Core Technologies](#-core-technologies)
- [Project Structure](#-project-structure)
- [Installation](#-installation)
- [Quick Start](#-quick-start)
- [API Usage Example](#-api-usage-example)
- [Documentation](#-documentation)
- [Testing](#-testing)
- [Security Considerations](#-security-considerations)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🚀 Key Features

`processAnalyzer` offers a robust set of functionalities for in-depth system and process analysis:

*   **Lazy-Loaded Process Streaming**: Use C++23's `std::generator` to efficiently stream and query process data on-the-fly, minimizing memory overhead.
*   **Deep Process Inspection**: Access detailed information for any process, including:
    *   CPU, memory, and disk I/O usage (including live rate monitoring).
    *   Process state, priority, user, and start time.
    *   Executable path, command line, and environment variables.
    *   Parent/child relationships and full descendant trees.
    *   Detailed thread-level statistics.
*   **Advanced Filtering & Sorting**: Dynamically query processes with complex filters (name, user, memory, etc.) and sort results by any metric.
*   **Comprehensive System Metrics**: Monitor system-wide statistics:
    *   Overall CPU load, memory usage (RAM/swap), and uptime.
    *   Per-CPU core usage percentages.
    *   Network interface and disk I/O statistics.
*   **Process Control Interface**:
    *   Send signals to processes (e.g., `SIGTERM`, `SIGKILL`).
    *   Adjust process niceness (priority) and CPU affinity.
*   **Detailed Context Retrieval**:
    *   Inspect open file descriptors, memory maps, and resource limits.
    *   Analyze network connections (TCP/UDP) per process.

For a complete list of features and detailed explanations, see [docs/features.md](docs/features.md).

---

## 🛠 Core Technologies

- **C++23**: Leverages the latest standard for high-performance, modern code.
  - `std::generator`: Enables efficient, lazy-loaded streaming of process data, minimizing memory footprint.
  - `std::expected`: Provides robust, clear, and explicit error handling without exceptions.
- **CMake**: Modern, cross-platform build system for easy configuration and compilation.
- **High-Performance Design**:
  - **Zero-Cost Abstractions**: Direct, low-level interface to Linux `/proc` and `/sys` filesystems, avoiding unnecessary overhead.
  - **Minimal Dependencies**: Keeps the project lightweight and easy to deploy.
  - **Thread-Safe**: Designed for safe concurrent use in multithreaded applications.

---

## 🏗️ Project Structure

The project is organized into several key directories:

-   `src/`: Contains the main application source code.
    -   `analyzer/`: Core library for process and system analysis.
    -   `cli/`: Command-line interface logic.
    -   `utils/`: Shared utility functions.
-   `include/`: Public headers for the `processAnalyzer` library.
-   `tests/`: Unit and integration tests.
-   `docs/`: Detailed documentation files.
-   `CMakeLists.txt`: Main CMake build script.

A more detailed overview is available in [docs/project-structure.md](docs/project-structure.md).

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

2.  **Configure with CMake**
    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    ```

3.  **Build the Project**
    ```bash
    cmake --build build
    ```
The executable will be available at `build/bin/processAnalyzer`.

For more advanced build options, such as building with debug symbols or running sanitizers, please see the [detailed build guide](docs/build.md).

---

## ⚡ Quick Start

After building, you can run `processAnalyzer` from the `build/bin` directory.

```bash
# List all running processes with default columns
./build/bin/processAnalyzer list

# Find processes by name and sort by RSS memory (descending)
./build/bin/processAnalyzer list --name nginx --sort-by rss --sort-order desc

# Show detailed info for a specific PID (children, open files, threads, network)
./build/bin/processAnalyzer show --pid 1234 --children --open-files --threads --network

# Export high-memory processes to JSON
./build/bin/processAnalyzer list --sort-by rss --output json > heavy_procs.json

# Display help menu for all options
./build/bin/processAnalyzer --help
```

For a full command reference, see [docs/usage.md](docs/usage.md).

---

## 📚 API Usage Example

Integrate `processAnalyzer` into your C++ applications. The library's C++23-based design makes it easy to perform complex queries efficiently.

The example below demonstrates how to use `streamQueryProcesses` to lazily stream all processes, filter them for names containing "bash", and print their details sorted by resident memory usage.

```cpp
#include "analyzer/core.h"
#include <iostream>

int main() {
    ProcessAnalyzer analyzer;

    // 1. Define a filter to find processes with "bash" in their name
    ProcessFilter filter;
    filter.nameContains = "bash";

    // 2. Lazily stream, filter, and sort processes by memory usage
    auto processStream = analyzer.streamQueryProcesses(
        filter,
        ProcessSortField::rss, // Sort by Resident Set Size (RSS)
        SortOrder::desc        // Sort in descending order
    );

    // 3. Iterate through the stream and print details
    std::cout << "--- Finding 'bash' processes, sorted by memory usage ---\n";
    for (const auto& proc : processStream) {
        std::cout << "PID: " << proc.pid
                  << ", Name: " << proc.name
                  << ", Memory: " << proc.residentMemory << " KB\n";
    }

    return 0;
}
```

For comprehensive details on the library's classes, functions, and advanced usage, please refer to the [API Reference](docs/api-reference.md).

To compile and run this example, you would typically link against the `processAnalyzer` library (`libanalyzer.a`) and ensure the headers from the `include/` directory are available to your compiler.

---

## 📚 Documentation

Detailed documentation is available in the [docs/](docs/) folder:

| Document | Description |
| :--- | :--- |
| [**Usage Guide**](docs/usage.md) | Command-line reference and examples. |
| [**API Reference**](docs/api-reference.md) | Comprehensive C++ API details. |
| [**Build Details**](docs/build.md) | Compilation and installation guide. |
| [**Features**](docs/features.md) | Exhaustive list of capabilities. |
| [**Configuration**](docs/configuration.md) | Customizing tool behavior via config files. |
| [**Project Structure**](docs/project-structure.md) | Codebase organization and architecture. |
| [**Utility Library**](docs/utils.md) | Guide to the internal `utils` library. |
| [**Changelog**](docs/changelog.md) | History of version changes. |

## 🧪 Testing

See [docs/testing.md](docs/testing.md) for details on testing.

---

## 🔒 Security Considerations

`processAnalyzer` interacts directly with the Linux `/proc` filesystem to gather system and process information. While designed with security in mind, users should be aware of the following:

-   **Permissions**: Running `processAnalyzer` with elevated privileges (e.g., `sudo`) will grant it access to sensitive process information that might otherwise be restricted. Use `sudo` only when necessary and understand the implications.
-   **Output Handling**: When exporting data to files (especially JSON or CSV), ensure the destination is secure if the data contains sensitive information (e.g., command-line arguments, environment variables of certain processes).
-   **External Integration**: If integrating `processAnalyzer`'s output with other tools or scripts, validate and sanitize inputs to prevent injection vulnerabilities.

Always follow best security practices when using system diagnostic tools.

---

## 🤝 Contributing

Contributions are welcome! Please see [docs/contributing.md](docs/contributing.md) for our contribution guidelines.

---

## 📄 License

This project is open-source software licensed under the [GNU General Public License v3.0](LICENSE).
