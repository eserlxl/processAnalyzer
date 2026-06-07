# Features

`processAnalyzer` offers a comprehensive suite of features for process monitoring and system diagnostics:

*   **List Processes**: Enumerate all running processes with key information.
*   **Process Details**: Obtain comprehensive details for a specific process ID (PID), including its child processes, parent process, descendants, environment variables, memory maps, resource limits, cgroup information, CPU affinity, and open files/sockets/pipes. The `show` command exposes these via `--children`, `--descendants`, `--affinity`, and the other inspection flags.
*   **Process Filtering**: Filter processes by various criteria such as name, user, state, parent process ID (PPID), memory usage, executable path, command line arguments, CPU usage, memory percentage, or network connection attributes. Name and command line also accept regular expressions via `--name-regex`/`--cmdline-regex`.
*   **Process Sorting**: Sort processes based on various fields like PID, user, name, memory usage, CPU time, start time, executable path, CPU usage percentage, memory usage percentage, etc.
*   **Customizable Output**: Choose which columns to display and output results in different formats (table, vertical, CSV, JSON, or an indented `tree` process forest).
*   **Performance Monitoring**: Provides both instantaneous snapshots of process resource consumption and metrics calculated over a specific duration. The API includes functions to measure CPU and Disk I/O usage for single processes or all processes over a defined time interval (e.g., 500ms), enabling precise performance analysis.
*   **Real-Time Monitoring CLI**: The `top` command ranks processes by live CPU usage (or disk I/O with `--io`, or resident memory with `--mem`), limited with `--count N`, narrowed by the standard process filters (e.g. `--user`, `--name`), refreshable with `--watch`, and emittable as JSON with `--output json`. The `system --watch [SECONDS]` flag continuously refreshes the system report (clearing the screen each cycle) until interrupted, and `system --output json` emits the full system metrics as a single machine-readable JSON object for automation and diagnostics tooling.
*   **System Metrics**: Monitor system-wide metrics such as total memory usage, load average, CPU statistics (user, system, idle), per-CPU usage, disk I/O per device, network interface statistics, and system activity (interrupts, context switches, forks).
*   **Network Activity**: Inspect detailed network connections (TCP, UDP, IPv4, IPv6) for individual processes.
*   **Thread Details**: Enumerate and inspect individual threads within a process.
*   **Process Control** (API Only): Send POSIX signals, modify process niceness, and set CPU affinity programmatically via the C++ API.
*   **System Information**: Retrieve system uptime, kernel version, OS name, hostname, and mounted filesystem disk usage.
*   **C++23 Streaming API**: Utilize a modern C++23 `std::generator`-based API for efficient, lazy-loaded streaming of process information.
*   **Utility Library**: Leverages a robust, modern C++23 utility library for common tasks. It features a `std::expected`-based error handling model and provides a comprehensive suite of functions for file system operations (including atomic writes and advanced directory traversal), string manipulation (Unicode-aware, Base64, URL encoding), numeric parsing, system interaction (command execution, environment variables), time utilities, and file hashing (SHA256/512, MD5, CRC32).
