# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add the existing test files
# Assuming FileTests.cpp, PathTests.cpp, String.cpp, System.cpp, Types.cpp are already here
# If these files are not directly listed, they might be included from other CMakeLists.txt
# For now, we explicitly list them as sources.
set(TEST_SOURCES
    FileTests.cpp
    PathTests.cpp
    String.cpp
    System.cpp
    Types.cpp
)

# Define the test executable
add_executable(utils_tests ${TEST_SOURCES})

# Link against the utils library and GTest
# We assume 'utils' is a library target defined elsewhere in the project
target_link_libraries(utils_tests PRIVATE utils ${GTEST_LIBRARIES})

# Set output directory for tests
set_target_properties(utils_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
