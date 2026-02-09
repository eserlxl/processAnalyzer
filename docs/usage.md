# Usage Guide for processAnalyzer

This document provides detailed instructions on how to use the `processAnalyzer` command-line tool, including available commands, arguments, and practical examples.

## Basic Structure

The command-line syntax is:

```bash
processAnalyzer [command] [options]
```

Where `[command]` is one of:
*   `list` (default): List and filter processes.
*   `show`: Show detailed information for a specific process.
*   `pid <pid>`: Show detailed information for a specific process ID (alias of `show --pid <pid>`).
*   `name <name>`: List processes filtered by name (alias of `list --name <name>`).
*   `user <user>`: List processes filtered by user (alias of `list --user <user>`).
*   `help`: Show help information.

## Commands

### `list` Command

The `list` command allows you to enumerate running processes, filter them, and sort the output.

**Examples:**

```bash
# List all processes
./processAnalyzer list

# Filter by name "chrome"
./processAnalyzer list --name chrome

# Filter by user "root"
./processAnalyzer list --user root

# List processes in state 'R' (Running)
./processAnalyzer list --state R
```

### `show` Command

The `show` command provides in-depth details about a single process. It requires the `--pid` option.

**Examples:**

```bash
# Show details for PID 1234
./processAnalyzer show --pid 1234

# Show details including children and open files
./processAnalyzer show --pid 1234 --children --open-files

# Show details including threads and network connections
./processAnalyzer show --pid 1234 --threads --network
```

### `pid` Command

The `pid` command is a positional alias for inspecting a single PID.

**Examples:**

```bash
# Equivalent to: ./processAnalyzer show --pid 1234
./processAnalyzer pid 1234

# PID-specific inspection flags are also supported
./processAnalyzer pid 1234 --children --open-files --threads --network
```

## Options Reference

### Filtering Options (for `list`)

*   `--ppid <pid>`: Filter by Parent Process ID.
*   `--name <name>`: Filter by process name (substring match).
*   `--user <username>`: Filter by username.
*   `--state <char>`: Filter by state (e.g., 'R' for Running, 'S' for Sleeping, 'Z' for Zombie).

### Sorting Options (for `list`)

*   `--sort-by <field>`: Sort by field. Valid fields: `pid`, `ppid`, `uid`, `user`, `name`, `state`, `rss`, `vm`, `threads`, `cpu`, `start-time`, `mem`.
*   `--sort-order <order>`: Sort order. Valid values: `asc`, `desc`.

### Output Control (for `list`)

*   `--brief`, `-b`: Use brief output mode.
*   `--columns <c1,c2...>`: Select specific columns to display (comma-separated).
*   `--no-truncate-cmdline`: Do not truncate the command line string.
*   `--output <format>`: Output format. Valid values: `table` (default), `csv`, `json`, `vertical`.

### Inspection Options (for `show`/`pid`)

These options are only valid with the `show` and `pid` commands.

*   `--children`: Show child processes.
*   `--threads`: Show thread information.
*   `--open-files`: Show open files.
*   `--network`: Show network connections.

### Other Options

*   `--help`, `-h`: Show help message.
*   `--pid <pid>`, `-p <pid>`: Target process ID for `show`.
*   `--config-file <path>`: Specify a configuration file path.

## Examples

### JSON Output

Generate a JSON report of all processes consuming significant memory:

```bash
./processAnalyzer list --sort-by rss --sort-order desc --output json
```

### Investigating a Process Tree

Find a process by name, then inspect its parent or children:

```bash
# Find the PID
./processAnalyzer list --name nginx

# Inspect specific PID (e.g., 567)
./processAnalyzer show --pid 567 --children
```

### Custom Column View

View only specific columns for a cleaner output:

```bash
./processAnalyzer list --columns pid,user,state,rss,name
```
