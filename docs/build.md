# Build Instructions

This document details how to build `processAnalyzer` from source.

## Prerequisites

To build and run `processAnalyzer`, ensure your system meets the following requirements:

*   **Operating System**: Linux (relies on `/proc` filesystem)
*   A C++ compiler (e.g., GCC, Clang) supporting C++23
*   CMake (version 3.17 or higher)
*   Make (or Ninja build system)

## Building from Source

Follow these steps to compile the project:

1.  **Create a build directory**:
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project with CMake**:
    ```bash
    cmake ..
    ```

3.  **Build the executable**:
    ```bash
    make
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
