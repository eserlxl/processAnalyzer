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

2.  **Configure and Build**

    You can use standard CMake commands with presets to configure and build the project:
    ```bash
    # Configure using the Release preset (creates a 'build' directory if it doesn't exist)
    cmake --preset default
    # Build the project
    cmake --build --preset default
    ```
    The executable will be located at `build/processAnalyzer`.

    For advanced options, including debug builds and running tests, refer to the relevant sections below.

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

To install `processAnalyzer` system-wide (e.g., to `/usr/local/bin`), use the CMake install target after building.

1.  **Configure and Build** (if not already done, execute the steps above)
    ```bash
    cmake --preset default
    cmake --build --preset default
    ```

2.  **Install**
    ```bash
    # This may require sudo depending on the install prefix (default is /usr/local)
    sudo cmake --install build
    ```

3.  **Verify Installation**:
    After successful installation, the `processAnalyzer` executable should be available in your system's PATH.
    ```bash
    processAnalyzer --help
    ```
