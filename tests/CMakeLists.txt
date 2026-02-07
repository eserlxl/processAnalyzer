# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Eser KUBALI

cmake_minimum_required(VERSION 3.10)

add_executable(utils_test utils_test.cpp)

# Link against the library we created, and GTest
target_link_libraries(utils_test PRIVATE processAnalyzerLib GTest::gtest_main GTest::gtest)

# Set output directory
set_target_properties(utils_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

add_test(NAME UtilsTests COMMAND $<TARGET_FILE:utils_test>)
