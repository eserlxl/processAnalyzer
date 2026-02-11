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
├───docs/                       # Contains supplementary project documentation.
│   ├───api-reference.md        # Detailed API documentation.
│   ├───build.md                # Detailed instructions on how to build the project.
│   ├───changelog.md            # Project change history.
│   ├───code-examples.md        # C++ API integration examples.
│   ├───configuration.md        # Guide to configuring the tool.
│   ├───contributing.md         # Guidelines for contributing to the project.
│   ├───features.md             # List of features.
│   ├───project-structure.md    # This file, detailing the project's directory structure.
│   ├───testing.md              # The project's testing strategy and instructions.
│   ├───usage.md                # Detailed guide on using the processAnalyzer tool.
│   ├───utils.md                # Documentation for the internal 'utils' library.
│   └───audit/                  # Directory for audit-related documentation or logs.
├───include/                    # Public header files for the project.
│   ├───analyzer/               # Headers for the core analysis engine.
│   │   ├───core.h
│   │   ├───network_model.h
│   │   ├───process_model.h
│   │   └───system_model.h
│   ├───cli/                    # Headers for the command-line interface.
│   │   ├───args.h
│   │   └───output.h
│   └───utils/                  # Headers for the utility library modules.
│       ├───core.h
│       ├───file.h
│       ├───general.h
│       ├───path.h
│       ├───string.h
│       ├───system.h
│       ├───time.h
│       └───types.h
├───src/                        # Source code files for the application logic.
│   ├───main.cpp                # Main entry point for the command-line application.
│   ├───analyzer/               # Implementation of the core analysis engine.
│   │   ├───core.cpp
│   │   ├───network.cpp
│   │   ├───process.cpp
│   │   └───system.cpp
│   ├───cli/                    # Command-line interface logic.
│   │   ├───args.cpp
│   │   └───output.cpp
│   └───utils/                  # Implementation of the utility library modules.
│       ├───file.cpp
│       ├───filesystem.cpp
│       ├───path.cpp
│       ├───string.cpp
│       ├───system.cpp
│       ├───time.cpp
│       └───types.cpp
├───Testing/                    # Directory used by CTest for testing purposes.
└───tests/                      # Source files for unit and integration tests.
    ├───CMakeLists.txt          # CMake configuration for building the tests.
    ├───analyzer/
    │   └───network.cpp
    ├───cli/                    # Tests for the CLI components.
    │   ├───args.cpp
    │   └───output.cpp
    └───utils/                  # Tests for the utility library.
        ├───core.cpp
        ├───file.cpp
        ├───filesystem.cpp
        ├───path.cpp
        ├───string.cpp
        ├───system.cpp
        ├───test.cpp
        ├───test.h
        ├───time.cpp
        ├───types.cpp
        └───mock_proc/
            ├───basic.cpp
            ├───process.cpp
            └───system.cpp
```
