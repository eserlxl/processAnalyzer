# Build Instructions

This document provides detailed instructions on how to build `processAnalyzer` from source, including prerequisites, compilation steps, and how to run tests.

## Prerequisites

To build and run `processAnalyzer`, ensure your system meets the following requirements:

*   **Operating System**: Linux (relies on `/proc` filesystem).
*   **Compiler**: A C++ compiler supporting C++23 (e.g., GCC 12+, Clang 16+).
*   **Build System**: CMake (version 3.17 or higher) and Make (or Ninja).
*   **VCS**: `git` for cloning the repository.
*   **Testing**: `gtest` and `gmock` are required for building and running the test suite. They are automatically fetched by CMake if not found.

### Dependency Installation (Debian-based Systems)

You can install the necessary tools on Debian-based systems (like Ubuntu) with the following command:

```bash
sudo apt-get update && sudo apt-get install -y build-essential g++-12 cmake git
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

1.  **Run all tests**:
    From the `build` directory, use `ctest` to run the entire test suite.
    ```bash
    ctest --output-on-failure
    ```

2.  **Run individual test executables**:
    While `ctest` is recommended for running all tests, you can also run specific test executables directly for more detailed output or debugging.
    
    To find the names of the available test executables, list the contents of the `build/tests/` directory:
    ```bash
    ls tests/
    ```
    
    Then, you can run a specific test executable, for example:
    ```bash
    ./tests/MySpecificTest
    ```
    *Note: The exact names and number of test executables are defined by the project's `CMakeLists.txt` configuration in the `tests/` directory.*

## Installation

After a successful build, you can install the `processAnalyzer` binary to a system-wide location (e.g., `/usr/local/bin`).

From the `build` directory, run:
```bash
sudo make install
```

This will install the `processAnalyzer` executable, making it available from any terminal. You can then run it directly:
```bash
processAnalyzer --help
```
