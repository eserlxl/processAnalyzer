# Usage Guide for processAnalyzer

This document provides detailed instructions on how to use the `processAnalyzer` command-line tool, including available commands, arguments, and practical examples.

## Basic Commands

The `processAnalyzer` executable can be found in the `build/` directory after compilation.

### Listing All Processes

To list all currently running processes on your system, use the `list` command:

```bash
./processAnalyzer list
```

This command will output a table or list of processes, typically including:
*   Process ID (PID)
*   Process Name
*   User
*   CPU Usage
*   Memory Usage

*(Note: The exact output format may vary based on implementation.)*

### Getting Details for a Specific Process

To retrieve detailed information for a particular process, use the `pid` command followed by the Process ID:

```bash
./processAnalyzer pid <PID>
```

Replace `<PID>` with the actual Process ID you wish to inspect. For example:

```bash
./processAnalyzer pid 1234
```

This command will provide a more granular view of the specified process, which might include:
*   Full process name/path
*   Parent Process ID (PPID)
*   Executable path
*   Command-line arguments
*   Current status
*   Resource limits
*   Open file descriptors
*   Network connections

*(Note: The level of detail provided depends on the `processAnalyzer` implementation and operating system capabilities.)*

## Advanced Usage (Placeholder)

*(This section can be expanded later with more advanced features, filtering options, output formats, or configuration if the tool supports them.)*

*   Filtering processes by name: `./processAnalyzer list --name firefox`
*   Sorting output: `./processAnalyzer list --sort memory`
*   Outputting in JSON format: `./processAnalyzer list --format json`

## Configuration (Placeholder)

*(If `processAnalyzer` supports any configuration files or environment variables to alter its behavior, details would go here.)*
