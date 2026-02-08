# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2024 Eser KUBALI

cmake_minimum_required(VERSION 3.17)

# Consolidate all tests into a single executable
add_executable(ProcessAnalyzerTests
    utils/Core.cpp
    utils/System.cpp
    analyzer/Core.cpp
    utils/Test.cpp
    utils/TestNewApi.cpp
    utils/FileSystem.cpp
    utils/Types.cpp
    utils/String.cpp
)

# Link the single test executable
target_include_directories(ProcessAnalyzerTests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(ProcessAnalyzerTests PRIVATE processAnalyzerLib GTest::gtest_main)

# Add a single test to CTest
add_test(NAME ProcessAnalyzerTests COMMAND ProcessAnalyzerTests)
