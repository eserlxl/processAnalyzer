# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2024 Eser KUBALI

cmake_minimum_required(VERSION 3.17)

# Automatically discover all test source files
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Consolidate all tests into a single executable
add_executable(ProcessAnalyzerTests ${TEST_SOURCES})

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
