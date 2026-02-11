# Usage Guide for processAnalyzer

This document provides detailed instructions for using the `processAnalyzer` command-line interface (CLI). For C++ library usage, see the [Code Examples](code-examples.md).

---

## 📑 Table of Contents

- [Getting Started](#-getting-started)
- [Commands](#-commands)
  - [`list`](#list-command)
  - [`show`](#show-command)
- [Options Reference](#-options-reference)
  - [Filtering Options](#filtering-options)
  - [Sorting Options](#sorting-options)
  - [Output Control](#output-control)
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

**Example:**
```bash
# Show detailed info for PID 1, including children and open files
sudo ./processAnalyzer show --pid 1 --children --open-files
```

---

## ⚙️ Options Reference

### Filtering Options

Apply these options with the `list` command to narrow down results.

| Option | Description | Example |
| :--- | :--- | :--- |
| `--ppid <pid>` | Filter by Parent Process ID. | `--ppid 1` |
| `--name <name>` | Filter by process name (case-insensitive substring). | `--name nginx` |
| `--user <user>` | Filter by username or UID. | `--user root` |
| `--state <char>` | Filter by process state (e.g., 'R', 'S', 'Z'). | `--state Z` |

### Sorting Options

Apply these options with the `list` command to order the results.

| Option | Description |
| :--- | :--- |
| `--sort-by <field>` | Field to sort by. Valid fields: `pid`, `ppid`, `uid`, `user`, `name`, `state`, `rss`, `vm`, `threads`, `cpu`, `start-time`, `mem`. |
| `--sort-order <order>` | Sort order. Valid values: `asc` (ascending) or `desc` (descending). Default is `asc`. |

### Output Control

Customize the appearance of the output for the `list` command.

| Option | Description |
| :--- | :--- |
| `--output <format>` | Output format. Options: `table` (default), `csv`, `json`, `vertical`. |
| `--columns <c1,c2...>`| Comma-separated list of columns to display (e.g., `pid,name,cpu,rss`). |
| `--no-truncate-cmdline`| Prevents truncating long command line arguments in the output. |
| `--brief`, `-b` | Use a brief, single-line output format. |

### Inspection Options

Use these options with the `show` command to include additional details. May require `sudo`.

| Option | Description |
| :--- | :--- |
| `--children` | Show child processes recursively. |
| `--threads` | Show detailed information for each thread. |
| `--open-files` | List all files opened by the process. |
| `--network` | Display active network connections (TCP/UDP). |

### General Options

| Option | Description |
| :--- | :--- |
| `--help`, `-h` | Show the help message and exit. |
| `--pid <pid>`, `-p <pid>` | Target Process ID for the `show` command. |
| `--config-file <path>` | Specify a path to a custom configuration file. |

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
