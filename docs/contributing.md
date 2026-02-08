# Contributing to processAnalyzer

We welcome and appreciate contributions to the `processAnalyzer` project! Whether it's reporting a bug, suggesting a new feature, improving documentation, or submitting code, your help makes `processAnalyzer` better for everyone.

## How to Contribute

1.  **Report Bugs**: If you find a bug, please open an issue on our GitHub repository. Provide a clear description of the bug, steps to reproduce it, and your environment details (OS, compiler, `processAnalyzer` version).
2.  **Suggest Features**: Have an idea for a new feature or enhancement? Open an issue to discuss your idea. This allows for feedback and ensures alignment with the project's goals.
3.  **Improve Documentation**: Spotted a typo, an unclear explanation, or missing information? Documentation improvements are always welcome.
4.  **Submit Code**:
    *   Fork the repository.
    *   Create a new branch for your feature or bug fix (`git checkout -b feature/your-feature-name`).
    *   Make your changes, ensuring code adheres to the project's coding style (e.g., using `.clang-format`).
    *   Write or update unit tests to cover your changes.
    *   Ensure all existing tests pass.
    *   Commit your changes with a clear and concise commit message.
    *   Push your branch to your fork.
    *   Open a Pull Request to the `main` branch of the original repository. Describe your changes thoroughly.

## Coding Style

*   We use `.clang-format` for C++ code formatting. Please ensure your code is formatted correctly before submitting a Pull Request.
*   Follow best practices for C++ development, including clear naming conventions, robust error handling, and efficient algorithms.

## Development Setup

To get your development environment ready, follow the "Building from Source" instructions in the main [README.md](README.md) file.

## Testing

Unit tests are a crucial part of `processAnalyzer` and are located in the `tests/` directory. We use Google Test as our testing framework.

When writing tests for components that interact with the `/proc` filesystem, you should use the `MockProc` utility class found in `tests/TestUtils.h`. This class provides a convenient way to create a mock `/proc` directory structure for your tests, ensuring they are hermetic and repeatable.

The `MockProc` class offers several helpful methods for setting up your test environment, including:
*   `addProcess(pid, name, ...)`: Quickly creates a directory and basic files for a mock process.
*   `createStatus(pid, data)`: Creates a mock `/proc/<pid>/status` file.
*   `createCmdline(pid, args)`: Creates a mock `/proc/<pid>/cmdline` file with null-separated arguments.
*   `createEnviron(pid, env_vars)`: Creates a mock `/proc/<pid>/environ` file.
*   `createFdDir(pid, fds)`: Creates a mock `/proc/<pid>/fd` directory with symlinked file descriptors.

Using these utilities is highly encouraged to simplify test creation and maintain consistency across the test suite.

## Code of Conduct

Please note that this project is released with a Contributor Code of Conduct. By participating in this project, you agree to abide by its terms.

Thank you for contributing to `processAnalyzer`!
