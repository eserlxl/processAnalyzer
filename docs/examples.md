# Code Examples

This document provides examples of how to integrate `processAnalyzer` into your C++ applications.

## Streaming, Filtering, and Sorting Processes

The library uses C++23 features like `std::generator` to efficiently stream process data. The following example demonstrates how to:
1.  Define a filter (e.g., find processes with "bash" in their name).
2.  Lazily stream all processes that match the filter.
3.  Sort the results by resident memory usage.
4.  Iterate through the stream and print details.

```cpp
#include "analyzer/core.h"
#include <iostream>
#include <ranges>

// Ensure you link against the processAnalyzer library (libanalyzer)

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
#include "analyzer/core.h"
#include <iostream>

int main() {
    ProcessAnalyzer analyzer; 
    
    // Get a complete snapshot of all running processes
    auto result = analyzer.snapshot();
    
    if (result.has_value()) {
        const auto& processes = result.value();
        std::cout << "Total processes: " << processes.size() << "\\n";
        
        for (const auto& proc : processes) {
            std::cout << "PID: " << proc.pid << ", Name: " << proc.name << std::endl;
        }
    } else {
        std::cerr << "Error getting processes: " << result.error().message << std::endl;
    }
    
    return 0;
}
```

For more details on the API, refer to the [API Reference](api-reference.md).

```