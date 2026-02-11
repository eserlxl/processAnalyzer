# Build Instructions

This document provides detailed instructions on how to build `processAnalyzer` from source, including prerequisites, compilation steps, and how to run tests.

## Prerequisites

To build and run `processAnalyzer`, ensure your system meets the following requirements:

*   **Operating System**: Linux (relies on `/proc` filesystem).
*   **Compiler**: A C++ compiler supporting C++23 with `std::generator` support (e.g., GCC 14+, Clang 17+ with libc++).
*   **Build System**: CMake (version 3.17 or higher) and Make (or Ninja).
*   **VCS**: `git` for cloning the repository.
*   **Testing**: `gtest` and `gmock` are required for building and running the test suite. They are automatically fetched by CMake if not found.

### Dependency Installation (Debian-based Systems)

You can install the necessary tools on Debian-based systems (like Ubuntu 24.04+) with the following command:

```bash
sudo apt-get update && sudo apt-get install -y build-essential g++-14 cmake git
```

## Building from Source

Follow these steps to clone the repository and compile the project:

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/eserlxl/processAnalyzer.git
    cd processAnalyzer
    ```

2.  **Create a build directory**:
    It is best practice to create a separate build directory to keep the source tree clean.
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure the project with CMake**:
    This step generates the build files for Make (or your chosen build system).
    ```bash
    cmake ..
    ```

4.  **Build the executable**:
    This compiles the source code and creates the `processAnalyzer` executable in the `build` directory.
    ```bash
    make
    ```
    The main executable will be located at `build/processAnalyzer`.

## Running Tests

To verify the correctness of the application, you can run the included unit tests. The tests are automatically built if `gtest` is found or fetched.

1.  **Run all tests via CTest**:
    From the `build` directory, use `ctest` to run the entire test suite. This is the recommended way to run all tests.
    ```bash
    ctest --output-on-failure
    ```

2.  **Run tests directly**:
    The project builds a single test executable named `ProcessAnalyzerTests` that contains all unit tests. Running this executable directly is useful for debugging or for more granular control over which tests are run.

    You can run it from the `build` directory:
    ```bash
    ./tests/ProcessAnalyzerTests
    ```

    You can also use GoogleTest flags to filter tests. For example, to run only the tests related to `Core`:
    ```bash
    ./tests/ProcessAnalyzerTests --gtest_filter="Core*"
    ```

## Installation

The project does not currently provide an automatic install target (e.g., `make install`). To install the application, simply copy the executable to a directory in your system's `PATH`.

From the `build` directory:
```bash
sudo cp processAnalyzer /usr/local/bin/
```

You can then run it from any terminal:
```bash
processAnalyzer --help
```
