# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Eser KUBALI

cmake_minimum_required(VERSION 3.17)

# Adjust file paths to match actual directory structure
add_process_analyzer_test(UtilsTest utils/Core.cpp utils/SystemTest.cpp)
add_process_analyzer_test(AnalyzerTest Analyzer.cpp TestUtils.cpp)
add_process_analyzer_test(TestUtilsNewApiTest TestUtilsNewApi.cpp TestUtils.cpp)
add_process_analyzer_test(UtilsNewApiTest utils/FileSystemTest.cpp)
add_process_analyzer_test(TypesTest utils/TypesTest.cpp)
