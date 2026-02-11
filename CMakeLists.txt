# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Eser KUBALI

cmake_minimum_required(VERSION 3.17)
project(processAnalyzer LANGUAGES CXX)
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
    src/analyzer/analyzer_internal_helpers.cpp
    src/analyzer/process_analyzer_base.cpp
    src/analyzer/process_analyzer_control.cpp
    src/analyzer/process_analyzer_details.cpp
    src/analyzer/process_analyzer_network.cpp
    src/analyzer/process_analyzer_performance.cpp
    src/analyzer/process_analyzer_query.cpp
    src/analyzer/process_analyzer_system.cpp
    src/utils/file.cpp
    src/utils/filesystem.cpp
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

# Main executable
add_executable(processAnalyzer src/main.cpp)
target_link_libraries(processAnalyzer PRIVATE processAnalyzerLib processAnalyzerCliLib)

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
  GIT_TAG        release-1.12.1 # Using a specific release tag for reproducibility
)
# The following will download and add googletest to the project
FetchContent_MakeAvailable(googletest)

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
