# Project Structure of processAnalyzer

This document provides a detailed overview of the directory and file structure of the `processAnalyzer` project.

```
processAnalyzer/
├───.clang-format               # Configuration for ClangFormat to ensure consistent code style.
├───.clang-tidy                 # Configuration for ClangTidy for static analysis.
├───.gitignore                  # Specifies intentionally untracked files for Git to ignore.
├───CMakeLists.txt              # Primary CMake build configuration for the project.
├───CMakePresets.json           # Defines presets for CMake configuration and builds.
├───LICENSE                     # Project license information (GNU General Public License v3.0).
├───MISSION.yaml                # Project mission statement or high-level goals.
├───README.md                   # Main project documentation and quick-start guide.
├───.git/                       # Internal Git directory for version control.
├───build/                      # Output directory for compiled binaries and build artifacts.
│   ├───CMakeFiles/             # CMake-generated internal build files.
│   └───tests/                  # Compiled test executables.
├───docs/                       # Contains supplementary project documentation.
│   ├───build.md                # Detailed instructions on how to build the project.
│   ├───contributing.md         # Guidelines for contributing to the project.
│   ├───project-structure.md    # This file, detailing the project's directory structure.
│   ├───usage.md                # Detailed guide on using the processAnalyzer tool.
│   ├───UTILS.md                # Documentation for the internal 'Utils' library.
│   └───audit/                  # Directory for audit-related documentation or logs.
│       └───gemini-cli/         # Specific audit info, possibly related to Gemini CLI usage.
├───include/                    # Public header files for the project.
│   ├───Analyzer.h              # Header for the main Analyzer class and related functions.
│   └───utils.h                 # Header for general utility functions (Utils namespace).
├───src/                        # Source code files for the application logic.
│   ├───Analyzer.cpp            # Implementation of the Analyzer class.
│   ├───main.cpp                # Main entry point for the command-line application.
│   └───utils.cpp               # Implementation of general utility functions.
├───Testing/                    # Directory used by CTest for testing purposes.
│   └───Temporary/              # Temporary files generated during the testing process.
└───tests/                      # Source files for unit and integration tests.
    ├───AnalyzerTest.cpp        # Unit tests for the Analyzer class.
    ├───CMakeLists.txt          # CMake configuration for building the tests.
    ├───TestUtils.cpp           # Implementation of test utility helpers.
    ├───TestUtils.h             # Utility functions and helpers for the test suite.
    ├───TestUtilsNewApiTest.cpp # Tests for new APIs within TestUtils.
    └───Utils.cpp               # Unit tests for the Utils library.
```
