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

## ⭐ Motivation

`processAnalyzer` is a high-performance C++ command-line utility designed for the real-time inspection and monitoring of system processes. It delivers detailed resource usage metrics and execution statistics to facilitate efficient system diagnostics and performance optimization.

---

## 📑 Table of Contents

- [Motivation](#-motivation)
- [Key Features](#-key-features)
- [Core Technologies](#-core-technologies)
- [Installation](#-installation)
- [Quick Start](#-quick-start)
- [API Usage Example](#-api-usage-example)
- [Documentation](#-documentation)
- [Testing](#-testing)
- [Security](#-security)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🚀 Key Features

`processAnalyzer` provides a rich set of features for deep process inspection, advanced filtering, and system-wide monitoring.

For a complete list of features, see [docs/features.md](docs/features.md).

---

## 🛠 Core Technologies

- **C++23 Features**: Utilizes `std::generator` for lazy-loaded process streaming and `std::expected` for robust error handling.
- **Modern Architecture**: Minimal dependencies, high performance, and thread-safe design.
- **Zero-Cost Abstractions**: Direct interface to Linux `/proc` and `/sys` filesystems without unnecessary overhead.

---

## 📦 Installation

For detailed build and installation instructions, please see [docs/build.md](docs/build.md).

### Prerequisites

- **OS**: Linux (Kernel 5.x+)
- **Compiler**: GCC 12+ or Clang 16+ (C++23 support required)
- **Build Tools**: CMake 3.17+ and Make/Ninja

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

## 💻 API Usage

`processAnalyzer` can be used as a C++ library in your own projects. For details on how to use the API and code examples, please refer to the [API Reference](docs/api-reference.md).

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

## 🔒 Security

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
