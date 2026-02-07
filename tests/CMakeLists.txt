# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Eser KUBALI

cmake_minimum_required(VERSION 3.10)

# UtilsTest
add_executable(UtilsTest Utils.cpp)
target_link_libraries(UtilsTest PRIVATE processAnalyzerLib GTest::gtest_main GTest::gtest)
set_target_properties(UtilsTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME UtilsTests COMMAND $<TARGET_FILE:UtilsTest>)

# AnalyzerTest
add_executable(AnalyzerTest AnalyzerTest.cpp)
target_link_libraries(AnalyzerTest PRIVATE processAnalyzerLib GTest::gtest_main GTest::gtest)
set_target_properties(AnalyzerTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
add_test(NAME AnalyzerTests COMMAND $<TARGET_FILE:AnalyzerTest>)
