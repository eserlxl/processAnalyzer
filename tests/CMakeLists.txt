# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2024 Eser KUBALI

cmake_minimum_required(VERSION 3.17)

# Consolidate all tests into a single executable
add_executable(ProcessAnalyzerTests
    analyzer/core_additional.cpp
    analyzer/core.cpp
    analyzer/network.cpp
    analyzer/process.cpp
    analyzer/system.cpp
    cli/args.cpp
    cli/output.cpp
    utils/core.cpp
    utils/file.cpp
    utils/mock_proc/basic.cpp
    utils/mock_proc/process.cpp
    utils/mock_proc/system.cpp
    utils/path/core.cpp
    utils/path/fs.cpp
    utils/path.cpp
    utils/string.cpp
    utils/system.cpp
    utils/test.cpp
    utils/time.cpp
    utils/types.cpp
)

# Link the single test executable
target_include_directories(ProcessAnalyzerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(ProcessAnalyzerTests PRIVATE processAnalyzerLib processAnalyzerCliLib GTest::gtest_main)

set_target_properties(ProcessAnalyzerTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

# Add a single test to CTest
include(GoogleTest)
gtest_discover_tests(ProcessAnalyzerTests)
