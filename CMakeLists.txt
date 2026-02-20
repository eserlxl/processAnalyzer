# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Eser KUBALI

cmake_minimum_required(VERSION 3.17)
project(processAnalyzer VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find clang-tidy for static analysis
find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-p;${CMAKE_BINARY_DIR}")
else()
    message(WARNING "clang-tidy not found. Static analysis will be skipped.")
endif()

# Code Coverage
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled.")

    else()
        message(WARNING "Code coverage is not supported for this compiler.")
    endif()
endif()

# Create a library for the core logic to be shared with tests
add_library(processAnalyzerLib STATIC
    src/analyzer/internal/helpers.cpp
    src/analyzer/base.cpp
    src/analyzer/control.cpp
    src/analyzer/details.cpp
    src/analyzer/network.cpp
    src/analyzer/performance.cpp
    src/analyzer/query.cpp
    src/analyzer/system.cpp
    src/utils/file.cpp
    src/utils/filesystem_attributes.cpp
    src/utils/filesystem_operations.cpp
    src/utils/path.cpp
    src/utils/string.cpp
    src/utils/system.cpp
    src/utils/time.cpp
    src/utils/types.cpp
)
# Add include directory correctly to the library target
target_include_directories(processAnalyzerLib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Apply coverage options to the library
if(ENABLE_COVERAGE AND (CMAKE_COMPILER_ID MATCHES "GNU|Clang"))
    target_compile_options(processAnalyzerLib INTERFACE --coverage)
    target_link_libraries(processAnalyzerLib INTERFACE --coverage)
endif()

# Create a library for CLI logic
add_library(processAnalyzerCliLib STATIC
    src/cli/args.cpp
    src/cli/output.cpp
)
target_include_directories(processAnalyzerCliLib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(processAnalyzerCliLib PRIVATE processAnalyzerLib)


add_executable(processAnalyzer src/main.cpp)
target_include_directories(processAnalyzer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(processAnalyzer PRIVATE processAnalyzerCliLib processAnalyzerLib)

# Enable testing
include(FetchContent)

# Reuse an already fetched googletest source tree when available.
# This keeps clean out-of-tree builds usable in offline environments.
if((NOT DEFINED FETCHCONTENT_SOURCE_DIR_GOOGLETEST OR FETCHCONTENT_SOURCE_DIR_GOOGLETEST STREQUAL "") AND
   EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/googletest-src/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST
        "${CMAKE_SOURCE_DIR}/build/_deps/googletest-src"
        CACHE PATH "Path to pre-fetched googletest sources" FORCE)
endif()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0 # Updated to v1.14.0
)

# Temporarily disable clang-tidy and CMake deprecation warnings for third-party deps
set(CMAKE_CXX_CLANG_TIDY_BACKUP ${CMAKE_CXX_CLANG_TIDY})
set(CMAKE_CXX_CLANG_TIDY "")
set(CMAKE_WARN_DEPRECATED_BACKUP ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEPRECATED OFF)

# The following will download and add googletest to the project
FetchContent_MakeAvailable(googletest)

# Restore configuration
set(CMAKE_CXX_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY_BACKUP})
set(CMAKE_WARN_DEPRECATED ${CMAKE_WARN_DEPRECATED_BACKUP})

enable_testing()

# Variable for test executable output path
set(TEST_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests" CACHE PATH "Output directory for test executables")

# Add tests subdirectory
add_subdirectory(tests)

if(ENABLE_COVERAGE)
    find_program(GCOVR_COMMAND gcovr)
    if(GCOVR_COMMAND)
        add_custom_target(coverage
            COMMAND ${GCOVR_COMMAND} --root ${CMAKE_SOURCE_DIR} -e '.*tests/.*' --html --html-details -o coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report..."
        )
    else()
        message(WARNING "gcovr not found. Cannot generate coverage report.")
    endif()
endif()
