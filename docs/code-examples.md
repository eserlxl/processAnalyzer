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
#include "analyzer/core.h"

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

## Monitoring Process Performance

This example shows how to measure CPU and disk I/O usage for a specific process over a short
sampling interval using the delta-based performance APIs.

```cpp
#include <iostream>
#include <chrono>
#include <unistd.h>
#include "analyzer/core.h"

int main() {
    ProcessAnalyzer analyzer;
    const int pid = static_cast<int>(::getpid());

    // Sample CPU usage over 500 ms
    auto cpuResult = analyzer.getProcessCpuUsage(pid, std::chrono::milliseconds(500));
    if (cpuResult) {
        std::cout << "CPU usage: " << cpuResult->cpuPercentage << "%\n";
    } else {
        std::cerr << "CPU error: " << cpuResult.error().message() << "\n";
    }

    // Sample disk I/O rate over 500 ms
    auto ioResult = analyzer.getProcessDiskIoUsage(pid, std::chrono::milliseconds(500));
    if (ioResult) {
        std::cout << "Read:  " << ioResult->readBytesPerSec  << " bytes/s\n";
        std::cout << "Write: " << ioResult->writeBytesPerSec << " bytes/s\n";
    } else {
        std::cerr << "I/O error: " << ioResult.error().message() << "\n";
    }

    return 0;
}
```

For bulk snapshots across all processes, use `getAllProcessesCpuUsage(duration)` and
`getAllProcessesDiskIoUsage(duration)` — both complete in a single sleep interval.

## Monitoring Network Throughput

Use `getNetworkInterfaceStats()` for cumulative byte/packet counters, or
`getNetworkInterfaceRates(duration)` for live bytes-per-second rates (sampled over
`duration`).

```cpp
#include "analyzer/core.h"
#include <chrono>
#include <iostream>

int main() {
    ProcessAnalyzer analyzer;

    // Cumulative counters (total since boot)
    auto statsResult = analyzer.getNetworkInterfaceStats();
    if (statsResult) {
        for (const auto& iface : *statsResult) {
            std::cout << iface.interfaceName
                      << "  RX: " << iface.rxBytes << " bytes"
                      << "  TX: " << iface.txBytes << " bytes\n";
        }
    }

    // Live throughput over 200 ms
    auto ratesResult = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(200));
    if (ratesResult) {
        for (const auto& r : *ratesResult) {
            std::cout << r.interfaceName
                      << "  RX: " << r.rxBytesPerSec << " bytes/s"
                      << "  TX: " << r.txBytesPerSec << " bytes/s\n";
        }
    }

    return 0;
}
```

## Monitoring System Disk I/O

Use `getSystemDiskIoStats()` for cumulative block-device counters, or
`getSystemDiskIoRates(duration)` for live reads/writes per second.

```cpp
#include "analyzer/core.h"
#include <chrono>
#include <iostream>

int main() {
    ProcessAnalyzer analyzer;

    // Cumulative counters (total since boot)
    auto statsResult = analyzer.getSystemDiskIoStats();
    if (statsResult) {
        for (const auto& dev : *statsResult) {
            std::cout << dev.deviceName
                      << "  reads: "  << dev.readsCompleted
                      << "  writes: " << dev.writesCompleted << "\n";
        }
    }

    // Live I/O rate over 200 ms
    auto ratesResult = analyzer.getSystemDiskIoRates(std::chrono::milliseconds(200));
    if (ratesResult) {
        for (const auto& r : *ratesResult) {
            std::cout << r.deviceName
                      << "  reads/s: "  << r.readsPerSec
                      << "  writes/s: " << r.writesPerSec << "\n";
        }
    }

    return 0;
}
```

## ⚡ Library Quick Start

The C++ API allows you to integrate process and system monitoring directly into your applications. You can link against the library by adding the project as a subdirectory in your CMake configuration.

**Example `main.cpp`:**
```cpp
#include <iostream>
#include "analyzer/core.h"

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
