# processAnalyzer

[![Build Status](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml/badge.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Code Coverage](https://img.shields.io/badge/Coverage-95%25-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![Static Analysis](https://img.shields.io/badge/Static%20Analysis-Passing-green.svg)](https://github.com/eserlxl/processAnalyzer/actions/workflows/build.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-0.1.0-blue.svg)](https://github.com/eserlxl/processAnalyzer/releases)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake](https://img.shields.io/badge/CMake-3.17%2B-blue.svg)](https://cmake.org/)

**processAnalyzer** is a modern, high-performance system diagnostics tool and C++ library for Linux. Built with **C++23**, it provides a powerful and efficient interface to the `/proc` filesystem, allowing developers and system administrators to inspect, monitor, and analyze processes and system-wide metrics with precision.

---

## 📑 Table of Contents

- [Key Features](#-key-features)
- [Core Technologies](#-core-technologies)
- [Installation](#-installation)
- [Quick Start](#-quick-start)
- [API Usage Example](#-api-usage-example)
- [Documentation](#-documentation)
- [Testing](#-testing)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🚀 Key Features

- **Deep Process Inspection**: Analyze memory maps, environment variables, open files, network connections (TCP/UDP), and resource limits.
- **Advanced Filtering & Sorting**: Precise filtering by PID, user, state, name, or memory usage. Sort by any field (e.g., CPU %, RSS, threads).
- **Thread-Level Analysis**: Inspect individual threads within a process to debug concurrency issues and performance bottlenecks.
- **System-Wide Metrics**: Monitor global CPU load (per-core), memory utilization, disk I/O, network interface statistics, and filesystem usage.
- **Flexible Output Formats**: Export data as **Table**, **Vertical**, **JSON**, or **CSV** for easy integration with external tools (Splunk, ELK, Excel).
- **Process Control**: Send signals, modify process niceness, and set CPU affinity directly from the API.

---

## 🛠 Core Technologies

- **C++23 Features**: Utilizes `std::generator` for lazy-loaded process streaming and `std::expected` for robust error handling.
- **Modern Architecture**: Minimal dependencies, high performance, and thread-safe design.
- **Zero-Cost Abstractions**: Direct interface to Linux `/proc` and `/sys` filesystems without unnecessary overhead.

---

## 📦 Installation

### Prerequisites

- **OS**: Linux (Kernel 5.x+)
- **Compiler**: GCC 12+ or Clang 16+ (C++23 support required)
- **Build Tools**: CMake 3.17+ and Make/Ninja

### Build from Source

```bash
# Clone the repository
git clone https://github.com/eserlxl/processAnalyzer.git
cd processAnalyzer

# Configure and build
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

For more details, see [docs/build.md](docs/build.md).

---

## ⚡ Quick Start

Run `processAnalyzer` from the `build/` directory:

```bash
# List all running processes with default columns
./processAnalyzer list

# Find processes by name and sort by RSS memory (descending)
./processAnalyzer list --name nginx --sort-by rss --sort-order desc

# Show detailed info for a specific PID (children, open files, threads, network)
./processAnalyzer show --pid 1234 --children --open-files --threads --network

# Export high-memory processes to JSON
./processAnalyzer list --sort-by rss --output json > heavy_procs.json

# Display help menu for all options
./processAnalyzer --help
```

For a full command reference, see [docs/usage.md](docs/usage.md).

---

## 💻 API Usage Example

`processAnalyzer` can also be used as a header-only or compiled library in your C++ projects.

```cpp
#include <analyzer/core.h>
#include <iostream>

int main() {
    ProcessAnalyzer analyzer("/proc");

    // Use C++23 generators for efficient streaming
    for (const auto& proc : analyzer.streamProcesses()) {
        std::cout << "PID: " << proc.pid << " Name: " << proc.name << "\n";
    }

    // Query specific process details
    auto result = analyzer.getProcessDetails(1);
    if (result) {
        std::cout << "Init Memory: " << result->rss << " KB\n";
    }

    return 0;
}
```

Refer to [docs/api-reference.md](docs/api-reference.md) for the full API documentation.

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

---

## 🧪 Testing

We use **GoogleTest** for ensuring code reliability and performance.

```bash
cd build
ctest --output-on-failure
```

---

## 🤝 Contributing

Contributions are welcome! Please see [docs/contributing.md](docs/contributing.md) for our contribution guidelines.

---

## 📄 License

This project is open-source software licensed under the [GNU General Public License v3.0](LICENSE).
