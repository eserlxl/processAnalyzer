# Usage Guide for processAnalyzer

This document provides detailed instructions for using the `processAnalyzer` command-line interface (CLI). For C++ library usage, see the [Code Examples](code-examples.md).

---

## 📑 Table of Contents

- [Getting Started](#-getting-started)
- [Commands](#-commands)
  - [`list`](#list-command)
  - [`show`](#show-command)
  - [`system`](#system-command)
- [Options Reference](#-options-reference)
  - [Filtering Options](#filtering-options)
  - [Sorting Options](#sorting-options)
  - [Output Control](#output-control)
  - [Column Reference](#column-reference)
  - [Inspection Options](#inspection-options)
  - [General Options](#general-options)
- [Practical Examples](#-practical-examples)
  - [Basic Process Listing](#basic-process-listing)
  - [Advanced Filtering and Sorting](#advanced-filtering-and-sorting)
  - [Detailed Process Inspection](#detailed-process-inspection)
  - [Custom Output Formats](#custom-output-formats)

---

## 🚀 Getting Started

The `processAnalyzer` CLI allows you to monitor and inspect running processes. The basic syntax is:

```bash
processAnalyzer [command] [options]
```

-   **`[command]`**: The action to perform, such as `list` or `show`. If no command is provided, `list` is used by default.
-   **`[options]`**: Flags to filter, sort, or format the output.

**Common Commands:**

-   `./processAnalyzer list`: List all running processes.
-   `./processAnalyzer show --pid <PID>`: Get detailed information about a specific process.
-   `./processAnalyzer system`: Show system-wide information (memory, CPU load, disk, network).
-   `./processAnalyzer --help`: Display all available commands and options.

---

## 🔎 Commands

### `list` Command

The `list` command enumerates, filters, and sorts running processes. It is the default command.

**Syntax:**
```bash
./processAnalyzer list [options]
```

**Description:**
Use this command to get an overview of system activity. You can customize the output by combining filtering, sorting, and formatting options.

**Example:**
```bash
# List all processes owned by the user "www-data", sorted by memory usage
./processAnalyzer list --user www-data --sort-by rss --sort-order desc
```

### `show` Command

The `show` command provides a deep dive into a single process, identified by its PID.

**Syntax:**
```bash
./processAnalyzer show --pid <PID> [options]
```

**Description:**
Use this command for detailed diagnostics of a specific process. You can include information about its children, open files, network connections, and threads. Accessing certain details may require `sudo` privileges.

The vertical output includes: PID, PPID, UID, user, name, state, nice value, RSS memory (KB), virtual memory (KB), thread count, start time, elapsed time, executable path, working directory, CPU user time (ticks), CPU kernel time (ticks), I/O read bytes, I/O write bytes, and full command line.

**Example:**
```bash
# Show detailed info for PID 1, including children and open files
sudo ./processAnalyzer show --pid 1 --children --open-files
```

### `system` Command

The `system` command prints a snapshot of system-wide resource usage.

**Syntax:**
```bash
./processAnalyzer system
```

**Output sections:**
- **System Information** — hostname, OS name, kernel version, and uptime.
- **Load Average** — 1-minute, 5-minute, and 15-minute load averages.
- **Memory (MiB)** — total, free, available, buffers, cached RAM; swap total and free (if swap is present).
- **Disk Usage** — for each mounted filesystem: device, mount point, total space (GiB), free space (GiB).
- **Network Interfaces** — for each network interface: RX/TX bytes and packets.
- **Disk I/O Stats** — for each block device: reads, writes, sectors read, sectors written.
- **System Activity** — total context switches, interrupts, and process forks since boot.

**Example:**
```bash
./processAnalyzer system
```

---

## ⚙️ Options Reference

### Filtering Options

Apply these options with the `list` command to narrow down results.

| Option | Description | Example |
| :--- | :--- | :--- |
| `--ppid <pid>` | Filter by Parent Process ID. | `--ppid 1` |
| `--name <name>` | Filter by process name (case-insensitive substring). | `--name nginx` |
| `--user <user>` | Filter by username. | `--user root` |
| `--uid <N>` | Filter by numeric User ID (non-negative integer). | `--uid 1000` |
| `--state <char>` | Filter by process state (e.g., 'R', 'S', 'Z'). | `--state Z` |
| `--cmdline <pattern>` | Filter by command-line substring. | `--cmdline --config` |
| `--min-rss <KB>` | Minimum resident set size in KB. | `--min-rss 51200` |
| `--max-rss <KB>` | Maximum resident set size in KB. | `--max-rss 102400` |
| `--min-vm <KB>` | Minimum virtual memory size in KB. | `--min-vm 1024` |
| `--max-vm <KB>` | Maximum virtual memory size in KB. | `--max-vm 524288` |
| `--min-threads <N>` | Minimum thread count. | `--min-threads 4` |
| `--max-threads <N>` | Maximum thread count. | `--max-threads 16` |
| `--min-priority <N>` | Minimum process priority (signed; lower = higher priority). | `--min-priority 0` |
| `--max-priority <N>` | Maximum process priority. | `--max-priority 19` |
| `--network <port>` | Show only processes with an active connection on the given local port. | `--network 8080` |

### Sorting Options

Apply these options with the `list` command to order the results.

| Option | Description |
| :--- | :--- |
| `--sort-by <field>` | Field to sort by. Valid fields: `pid`, `ppid`, `uid`, `user`, `name`, `state`, `rss`, `vm`, `threads`, `cpu`, `start-time`, `mem`, `cmdline`, `cwd`, `cpu-time`, `elapsed-time`, `exec-path`, `nice`. |
| `--sort-order <order>` | Sort order. Valid values: `asc` (ascending) or `desc` (descending). Default is `asc`. |

### Output Control

Customize the appearance of the output for the `list` command.

| Option | Description |
| :--- | :--- |
| `--output <format>` | Output format. Options: `table` (default), `csv`, `json`, `vertical`. |
| `--columns <c1,c2...>`| Comma-separated list of columns to display. See [Column Reference](#column-reference) for valid names. |
| `--no-truncate-cmdline`| Prevents truncating long command line arguments in the output. |
| `--brief`, `-b` | Use a brief, single-line output format. |

### Column Reference

Use these names with `--columns`:

| Column | Description |
| :--- | :--- |
| `pid` | Process ID |
| `ppid` | Parent process ID |
| `uid` | Numeric user ID |
| `user` | Username |
| `name` | Process name |
| `state` | State character (R, S, D, Z, T, …) |
| `rss` | Resident set size (KB) |
| `vm` | Virtual memory size (KB) |
| `threads` | Thread count |
| `cmdline` | Full command line |
| `start-time` | Process start timestamp |
| `elapsed-time` | Elapsed wall-clock time since start |
| `exec-path` | Path to the executable |
| `nice` | Nice value (priority offset) |
| `cwd` | Current working directory |

### Inspection Options

Use these options with the `show` command to include additional details. May require `sudo`.

| Option | Description |
| :--- | :--- |
| `--children` | Show child processes recursively. |
| `--threads` | Show detailed information for each thread. |
| `--open-files` | List all files opened by the process. |
| `--network` | Display active network connections (TCP/UDP/TCP6/UDP6). |
| `--env`, `--environment` | List the process's environment variables. |
| `--maps` | Show the process's memory map (address ranges, permissions, pathnames). |
| `--limits` | Show resource limits (soft/hard limits for CPU, memory, files, etc.). |
| `--cgroup` | Show cgroup membership (id, controllers, hierarchy path). |

### General Options

| Option | Description |
| :--- | :--- |
| `--help`, `-h` | Show the help message and exit. |
| `--pid <pid>`, `-p <pid>` | Target Process ID for the `show` command. |

---

## 💡 Practical Examples

### Basic Process Listing

1.  **List all processes** (default command):
    ```bash
    ./processAnalyzer
    ```

2.  **Filter by name**: Find all processes with "bash" in their name.
    ```bash
    ./processAnalyzer list --name bash
    ```

3.  **Filter by user**: List all processes owned by the user "root".
    ```bash
    ./processAnalyzer list --user root
    ```

### Advanced Filtering and Sorting

4.  **Find top 5 memory-consuming processes**:
    ```bash
    # Sort by RSS memory in descending order and show the top 5
    ./processAnalyzer list --sort-by rss --sort-order desc | head -n 6
    ```

5.  **Find all zombie processes**:
    ```bash
    ./processAnalyzer list --state Z
    ```

6.  **List all processes for a user, sorted by CPU usage**:
    ```bash
    ./processAnalyzer list --user myuser --sort-by cpu --sort-order desc
    ```

### Detailed Process Inspection

7.  **Get detailed information for a specific PID**:
    ```bash
    ./processAnalyzer show --pid 1234
    ```

8.  **Inspect a process with all details (requires sudo)**:
    This command shows children, threads, open files, and network connections.
    ```bash
    sudo ./processAnalyzer show --pid 1234 --children --threads --open-files --network
    ```

9.  **Investigate a web server process**:
    ```bash
    # Find the PID of nginx, then inspect its network connections
    NGINX_PID=$(./processAnalyzer list --name nginx --columns pid | awk 'NR==2 {print $1}')
    sudo ./processAnalyzer show --pid $NGINX_PID --network
    ```

### Custom Output Formats

10. **Export process list to JSON**:
    This is useful for programmatic analysis with tools like `jq`.
    ```bash
    ./processAnalyzer list --user myuser --output json > myuser_processes.json

    # Count the number of processes using jq
    jq '. | length' myuser_processes.json
    ```

11. **Export specific columns to CSV**:
    ```bash
    ./processAnalyzer list --columns pid,name,state,rss --output csv > process_report.csv
    ```

12. **View a process's details in vertical format**:
    This format is useful for readability with wide or numerous fields.
    ```bash
    ./processAnalyzer show --pid 1 --output vertical
    ```
## ⚡ Quick Start Examples

Here are some quick examples to get you started with `processAnalyzer`s command-line interface:

1.  **List all processes** in a table (the default view):
    ```bash
    ./build/processAnalyzer
    ```

2.  **Find processes by name** and sort by memory usage:
    ```bash
    ./build/processAnalyzer list --name nginx --sort-by rss --sort-order desc
    ```

3.  **Show detailed info for a PID**, including open files and network connections (may require `sudo`):
    ```bash
    sudo ./build/processAnalyzer show --pid 1 --open-files --network
    ```

4.  **Export process data to JSON** for scripting:
    ```bash
    ./build/processAnalyzer list --user www-data --output json > web-processes.json
    ```
