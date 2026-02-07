cmake_minimum_required(VERSION 3.17)
project(processAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/process_analyzer.cpp
    src/utils.cpp
)

# Add the executable
add_executable(processAnalyzer ${SOURCES})
