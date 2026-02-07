# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Eser KUBALI

cmake_minimum_required(VERSION 3.17)
project(processAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Create a library for the core logic to be shared with tests
add_library(processAnalyzerLib STATIC
    src/process_analyzer.cpp
    src/utils.cpp
)

# Main executable
add_executable(processAnalyzer src/main.cpp)
target_link_libraries(processAnalyzer PRIVATE processAnalyzerLib)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
# include_directories(${GTEST_INCLUDE_DIRS}) # Not needed if using targets

# Add tests subdirectory
add_subdirectory(tests)
