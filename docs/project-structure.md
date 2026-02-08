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
│   ├───configuration.md        # Guide to configuring the tool.
│   ├───contributing.md         # Guidelines for contributing to the project.
│   ├───features.md             # List of features.
│   ├───project-structure.md    # This file, detailing the project's directory structure.
│   ├───usage.md                # Detailed guide on using the processAnalyzer tool.
│   ├───utils.md                # Documentation for the internal 'utils' library.
│   └───audit/                  # Directory for audit-related documentation or logs.
├───include/                    # Public header files for the project.
│   ├───utils.h                 # Main header for the utility library.
│   ├───analyzer/               # Headers for the core analysis engine.
│   │   ├───Core.h
│   │   ├───NetworkModel.h
│   │   ├───ProcessModel.h
│   │   └───SystemModel.h
│   ├───cli/                    # Headers for the command-line interface.
│   │   ├───Args.h
│   │   └───Output.h
│   └───utils/                  # Headers for the utility library modules.
│       ├───Core.h
│       ├───File.h
│       ├───Path.h
│       ├───String.h
│       ├───System.h
│       ├───Time.h
│       └───Types.h
├───src/                        # Source code files for the application logic.
│   ├───main.cpp                # Main entry point for the command-line application.
│   ├───analyzer/               # Implementation of the core analysis engine.
│   │   └───Core.cpp
│   ├───cli/                    # Command-line interface logic.
│   │   ├───Args.cpp
│   │   └───Output.cpp
│   └───utils/                  # Implementation of the utility library modules.
│       ├───File.cpp
│       ├───Path.cpp
│       ├───String.cpp
│       ├───System.cpp
│       ├───Time.cpp
│       └───Types.cpp
├───Testing/                    # Directory used by CTest for testing purposes.
└───tests/                      # Source files for unit and integration tests.
    ├───CMakeLists.txt          # CMake configuration for building the tests.
    ├───analyzer/
    │   └───Core.cpp            # Tests for the analysis engine.
    ├───cli/                    # Tests for the CLI components.
    │   ├───ArgsTests.cpp
    │   └───OutputTests.cpp
    └───utils/                  # Tests for the utility library.
        ├───Core.cpp
        ├───FileTests.cpp
        ├───PathTests.cpp
        ├───String.cpp
        ├───System.cpp
        ├───Test.cpp
        ├───Test.h
        ├───TestApi.cpp
        ├───TimeTests.cpp
        └───Types.cpp

