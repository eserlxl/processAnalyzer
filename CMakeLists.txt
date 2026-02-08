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
        # add_compile_options(--coverage) # Replaced with target-specific options
        # add_link_options(--coverage)
    else()
        message(WARNING "Code coverage is not supported for this compiler.")
    endif()
endif()

# Create a library for the core logic to be shared with tests
add_library(processAnalyzerLib STATIC
    src/Analyzer.cpp
    src/utils.cpp
)
# Add include directory correctly to the library target
target_include_directories(processAnalyzerLib PUBLIC include)

# Apply coverage options to the library
if(ENABLE_COVERAGE AND (CMAKE_COMPILER_ID MATCHES "GNU|Clang"))
    target_compile_options(processAnalyzerLib INTERFACE --coverage)
    target_link_libraries(processAnalyzerLib INTERFACE --coverage)
endif()

# Main executable
add_executable(processAnalyzer src/main.cpp)
target_link_libraries(processAnalyzer PRIVATE processAnalyzerLib)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
# include_directories(${GTEST_INCLUDE_DIRS}) # Not needed if using targets

# Variable for test executable output path
set(TEST_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests" CACHE PATH "Output directory for test executables")

# Helper function for adding tests
function(add_process_analyzer_test test_name)
    set(sources ${ARGN})

    add_executable(${test_name} ${sources})

    target_link_libraries(${test_name} PRIVATE processAnalyzerLib GTest::gtest_main GTest::gtest)

    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_DIRECTORY}")

    add_test(NAME ${test_name}s COMMAND $<TARGET_FILE:${test_name}>)
endfunction()

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
