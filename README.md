# processAnalyzer

![License](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)

## Table of Contents
* [Features](#features)
* [Quick Start](#quick-start)
  * [Prerequisites](#prerequisites)
  * [Building from Source](#building-from-source)
  * [Running the Analyzer](#running-the-analyzer)
* [Usage](#usage)
* [Project Structure](#project-structure)
* [Utility Library (`Utils` Namespace)](#utility-library-utils-namespace)
* [Contributing](#contributing)
* [License](#license)
* [Copyright](#copyright)

A powerful and lightweight C++ command-line tool designed for in-depth analysis of running processes on your system. `processAnalyzer` provides a robust interface to inspect process details, offering insights into system resource usage and process behavior.

## Features

*   **List Processes**: Enumerate all running processes with key information.
*   **Process Details**: Obtain comprehensive details for a specific process ID (PID), including its child processes and open files.
*   **Process Filtering**: Filter processes by various criteria such as name, user, or state.
*   **Process Sorting**: Sort processes based on various fields like PID, user, name, memory usage, etc.
*   **Customizable Output**: Choose which columns to display and output results in different formats (table, CSV, JSON).
*   **Utility Library**: Leverages a robust C++ utility library for common tasks like file system operations, string manipulation, and system interaction.

## Quick Start

Follow these steps to quickly build and run `processAnalyzer` on your system.

### System Requirements

*   **Operating System**: Linux (relies on `/proc` filesystem)
*   A C++ compiler (e.g., GCC, Clang) supporting C++23
*   CMake (version 3.17 or higher)
*   Make (or Ninja build system)

### Building from Source

To compile the project:

```bash
# Create a build directory
mkdir build
cd build

# Configure the project with CMake
cmake ..

# Build the executable
make
```

### Running the Analyzer

Once built, you can run `processAnalyzer` from the `build` directory:

```bash
# List all running processes
./processAnalyzer list

# Get detailed information for a process with PID 1234
./processAnalyzer pid 1234

# Filter processes by name and sort by RSS memory in descending order
./processAnalyzer name chrome --sort-by rss --desc

# List processes with state 'R' (running) and output in JSON format
./processAnalyzer list --state R --format json

# Show children processes and open files for PID 1
./processAnalyzer pid 1 --children --open-files
```

### Running Tests

To verify the build, you can run the included unit tests:

```bash
# Run all tests using CTest
ctest --output-on-failure

# Or run individual test executables directly
./tests/UtilsTest
./tests/AnalyzerTest
```

## Usage

For comprehensive usage instructions, command-line arguments, and examples, please refer to the [Usage Guide](docs/usage.md).

## Project Structure

The project is organized to promote modularity and maintainability.

*   `src/`: Contains the core C++ source files for `processAnalyzer` and its utilities.
*   `include/`: Contains public header files.
*   `tests/`: Unit tests for various components of the project.
*   `docs/`: Additional documentation, including detailed guides and API references.
*   `CMakeLists.txt`: The primary CMake configuration file for the project.
*   `LICENSE`: Contains the licensing information for the project.

For a detailed breakdown of the project directory structure, see [docs/project-structure.md](docs/project-structure.md).

## Utility Library (`Utils` Namespace)

The project includes a robust `Utils` namespace in `src/utils.cpp` and `include/utils.h`. This library provides a collection of general-purpose utility functions for:

*   File system operations (reading/writing files, directory management)
*   String manipulation (trimming, splitting, joining, replacing, etc.)
*   Numeric parsing and validation
*   System interaction (environment variables, process execution)

For a comprehensive overview and usage examples of the utility library, please see [docs/UTILS.md](docs/UTILS.md).

## Contributing

We welcome contributions to `processAnalyzer`! Please see our [contribution guidelines](docs/contributing.md) for details on how to get started, report bugs, and suggest new features.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for more details.

## Copyright

Copyright (c) Eser KUBALI.
