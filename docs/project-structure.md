# Project Structure of processAnalyzer

This document provides a detailed overview of the directory and file structure of the `processAnalyzer` project.

```
/opt/lxl/c++/processAnalyzer/
├───.clang-format               # Configuration for ClangFormat, ensuring consistent code style.
├───.clang-tidy                 # Configuration for ClangTidy, a static analysis tool.
├───.gitignore                  # Specifies intentionally untracked files to ignore.
├───CMakeLists.txt              # Primary CMake build configuration for the entire project.
├───CMakePresets.json           # Presets for CMake configuration and build.
├───LICENSE                     # Project licensing information (GNU General Public License v3.0).
├───MISSION.yaml                # (Potentially internal or related to mission planning/description)
├───README.md                   # Main project documentation and quick start guide.
├───.git/                       # Git version control metadata.
├───build/                      # Directory for compiled binaries, object files, and CMake build artifacts.
│   ├───CMakeFiles/             # CMake internal build files.
│   └───tests/                  # Compiled test executables.
├───docs/                       # Supplementary documentation files.
│   ├───UTILS.md                # Documentation for the internal 'Utils' library.
│   ├───audit/                  # Audit-related documentation or logs.
│   │   └───gemini-cli/         # Specific audit information, possibly related to Gemini CLI usage.
│   ├───usage.md                # Detailed guide on how to use the processAnalyzer tool.
│   ├───contributing.md         # Guidelines for contributing to the project.
│   └───project-structure.md    # This document, detailing the project's directory structure.
├───include/                    # Public header files.
│   ├───Analyzer.h              # Declarations for the main Analyzer class and related functionalities.
│   └───utils.h                 # Declarations for general utility functions (part of the Utils namespace).
├───src/                        # Source code files for the application.
│   ├───Analyzer.cpp            # Implementations for the Analyzer class.
│   ├───main.cpp                # The main entry point of the processAnalyzer command-line application.
│   └───utils.cpp               # Implementations for general utility functions.
├───Testing/                    # Directory used by CMake/CTest for testing purposes.
│   └───Temporary/              # Temporary files generated during testing.
└───tests/                      # Unit and integration tests for the project.
    ├───CMakeLists.txt          # CMake configuration for building the tests.
    └───Utils.cpp               # Unit tests specifically for the Utils library.
```
