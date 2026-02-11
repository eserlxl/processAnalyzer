# Code Examples

This document provides a set of code examples demonstrating how to use the `processAnalyzer` C++ library to monitor and analyze system processes.

## Basic Usage: Stream and Print All Processes

This example shows how to iterate through all running processes and print their PID and command. This is the simplest way to get started with the library.

```cpp
#include <iostream>
#include "analyzer/core.h"

int main() {
    try {
        // Stream all running processes and print their PID and command
        for (const auto& process : analyzer::processes()) {
            std::cout << "PID: " << process.pid()
                      << ", Command: " << process.comm() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

## Find a Process by Name

This example demonstrates how to find a specific process by its name.

```cpp
#include <iostream>
#include "analyzer/core.h"

int main() {
    try {
        const std::string processName = "systemd";
        auto process = analyzer::process::find_by_name(processName);

        if (process) {
            std::cout << "Found process '" << processName << "' with PID: " << process->pid() << std::endl;
            std::cout << "  - CPU Usage: " << process->cpu_usage_short() << "%" << std::endl;
            std::cout << "  - Memory Usage: " << process->resident_set_size() / 1024 << " MB" << std::endl;
        } else {
            std::cout << "Process '" << processName << "' not found." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

## Get System-Wide Statistics

This example shows how to retrieve and display system-wide metrics like CPU usage, memory, and uptime.

```cpp
#include <iostream>
#include <iomanip>
#include "analyzer/system.h"

int main() {
    try {
        auto system_stats = analyzer::system::get_system_stats();

        std::cout << "System-Wide Statistics:" << std::endl;
        std::cout << "  - Uptime: " << std::fixed << std::setprecision(2) << system_stats.uptime / 3600.0 << " hours" << std::endl;
        std::cout << "  - Total Memory: " << system_stats.mem_total / (1024 * 1024) << " GB" << std::endl;
        std::cout << "  - Free Memory: " << system_stats.mem_free / (1024 * 1024) << " GB" << std::endl;
        std::cout << "  - CPU Usage: " << std::fixed << std::setprecision(2) << system_stats.total_cpu_usage << "%" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

For more advanced examples and a complete API reference, please see the [API Reference](api-reference.md).
