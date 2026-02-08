# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2024 Eser KUBALI

cmake_minimum_required(VERSION 3.17)

# Adjust file paths to match actual directory structure
add_process_analyzer_test(UtilsTest utils/Core.cpp utils/System.cpp)
add_process_analyzer_test(AnalyzerTest analyzer/Core.cpp utils/Test.cpp)
add_process_analyzer_test(TestUtilsNewApiTest utils/TestNewApi.cpp utils/Test.cpp)
add_process_analyzer_test(UtilsNewApiTest utils/FileSystem.cpp)
add_process_analyzer_test(TypesTest utils/Types.cpp)
add_process_analyzer_test(StringTest utils/String.cpp)
