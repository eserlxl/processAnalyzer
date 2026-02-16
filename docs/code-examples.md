# Code Examples

This document provides examples of how to integrate `processAnalyzer` into your C++ applications.

## Streaming, Filtering, and Sorting Processes

The library uses C++23 features like `std::generator` to efficiently stream process data. The following example demonstrates how to:
1.  Define a filter (e.g., find processes with "bash" in their name).
2.  Lazily stream all processes that match the filter.
3.  Sort the results by resident memory usage.
4.  Iterate through the stream and print details.

```cpp
#include <iostream>
#include <ranges>

int main() {
    ProcessAnalyzer analyzer;

    // 1. Define a filter to find processes with "bash" in their name
    ProcessFilter filter;
    filter.nameContains = "bash";

    // 2. Lazily stream, filter, and sort processes by memory usage
    // Note: The actual streaming happens when you iterate over processStream
    auto processStream = analyzer.streamQueryProcesses(
        filter,
        ProcessSortField::rss, // Sort by Resident Set Size (RSS)
        SortOrder::desc        // Sort in descending order
    );

    // 3. Iterate through the stream and print details
    std::cout << "--- Finding 'bash' processes, sorted by memory usage ---\\n";
    for (const auto& proc : processStream) {
        std::cout << "PID: " << proc.pid
                  << ", Name: " << proc.name
                  << ", Memory: " << proc.residentMemory << " KB\\n";
    }

    return 0;
}
```

## Creating a System Snapshot

If you prefer a static snapshot of the system state rather than a stream, you can use the `snapshot()` method.

```cpp
#include <iostream>

int main() {
    ProcessAnalyzer analyzer; 
    
    // Get a complete snapshot of all running processes
    auto result = analyzer.snapshot();
    
    if (result) {
        const auto& processes = *result;
        std::cout << "Total processes: " << processes.size() << "\\n";
        
        for (const auto& proc : processes) {
            std::cout << "PID: " << proc.pid << ", Name: " << proc.name << std::endl;
        }
    } else {
        std::cerr << "Error getting processes: " << result.error().message() << std::endl;
    }
    
    return 0;
}
```

## Get System-Wide Statistics

This example shows how to retrieve and display system-wide metrics like memory usage and load average.

```cpp
#include <iostream>
#include <iomanip>

int main() {
    try {
        ProcessAnalyzer analyzer;
        
        // System Memory Info
        auto memInfoResult = analyzer.getSystemMemoryInfo();
        if (memInfoResult) {
            const auto& mem = *memInfoResult;
            std::cout << "System Memory:" << std::endl;
            std::cout << "  - Total: " << mem.memTotal / 1024 << " MB" << std::endl;
            std::cout << "  - Free:  " << mem.memFree / 1024 << " MB" << std::endl;
            std::cout << "  - Available: " << mem.memAvailable / 1024 << " MB" << std::endl;
        }

        // System Load Average
        auto loadResult = analyzer.getSystemLoadAverage();
        if (loadResult) {
            const auto& load = *loadResult;
            std::cout << "Load Average: " 
                      << load.oneMin << " (1m), " 
                      << load.fiveMin << " (5m), " 
                      << load.fifteenMin << " (15m)" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

For more details on the API, refer to the [API Reference](api-reference.md).

## ⚡ Library Quick Start

The C++ API allows you to integrate process and system monitoring directly into your applications. You can link against the library by adding the project as a subdirectory in your CMake configuration.

**Example `main.cpp`:**
```cpp
#include <iostream>
#include "analyzer/process_analyzer.h"

int main() {
    try {
        ProcessAnalyzer analyzer;
        // Stream all running processes and print their PID and name
        for (const auto& process : analyzer.streamProcesses()) {
            std::cout << "PID: " << process.pid
                      << ", Name: " << process.name << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

**Example `CMakeLists.txt`:**
```cmake
cmake_minimum_required(VERSION 3.17)
project(MyMonitor)

set(CMAKE_CXX_STANDARD 23)

# Add processAnalyzer as a subdirectory
add_subdirectory(path/to/processAnalyzer)

add_executable(MyMonitor main.cpp)
target_link_libraries(MyMonitor PRIVATE processAnalyzerLib)
```
