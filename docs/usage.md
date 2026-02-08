# Usage Guide for processAnalyzer

This document provides detailed instructions on how to use the `processAnalyzer` command-line tool, including available commands, arguments, and practical examples.

## Basic Commands

The `processAnalyzer` executable can be found in the `build/` directory after compilation.

### Listing All Processes

To list all currently running processes on your system, use the `list` command. This is the default command if no specific command is provided.

```bash
./processAnalyzer list
# Or simply
./processAnalyzer
```

By default, this command will output a table including: Process ID (PID), User, Name, State, Resident Set Size (RSS), and Virtual Memory (VM).

### Getting Details for a Specific Process

To retrieve detailed information for a particular process, use the `pid` command followed by the Process ID:

```bash
./processAnalyzer pid <PID>
```

Replace `<PID>` with the actual Process ID you wish to inspect. For example:

```bash
./processAnalyzer pid 1234
```

This command, when used without output formatting options (`--columns` or `--format`), provides a granular, vertical view of the specified process, including: PID, PPID, UID, User, Name, State, RSS Memory, Virtual Memory, Threads, and Command.

### Filtering Processes by Name

To filter processes by their name (or a substring of their name), use the `name` command:

```bash
./processAnalyzer name <process_name_substring>
```

For example, to find all processes containing "chrome" in their name:

```bash
./processAnalyzer name chrome
```

### Filtering Processes by User

To filter processes by the username that owns them, use the `user` command:

```bash
./processAnalyzer user <username>
```

For example, to see all processes owned by the user "root":

```bash
./processAnalyzer user root
```

## Options

`processAnalyzer` provides several options to customize filtering, sorting, and output. These options can generally be combined with `list`, `name`, and `user` commands, and some are specific to the `pid` command.

### General Options

*   `-h`, `--help`: Display the help message and exit.

### Filtering Options

*   `--state <char>`: Filter processes by their current state. The state is a single character (e.g., `R` for Running, `S` for Sleeping, `Z` for Zombie, `T` for Stopped, `D` for Disk Sleep).
    ```bash
    ./processAnalyzer list --state R
    ./processAnalyzer name bash --state S
    ```

### Sorting Options

*   `--sort-by <field>`: Sort the output by a specific process field. Valid fields are `pid`, `ppid`, `uid`, `user`, `name`, `state`, `rss`, `vm`, `threads`.
    ```bash
    ./processAnalyzer list --sort-by rss
    ./processAnalyzer user root --sort-by name
    ```
*   `--desc`: Sort the output in descending order. This option must be used in conjunction with `--sort-by`.
    ```bash
    ./processAnalyzer list --sort-by rss --desc
    ```

### Output Options

*   `--brief`: Show a condensed table view when listing processes. This typically displays fewer columns.
    ```bash
    ./processAnalyzer list --brief
    ```
*   `--columns <c1,c2,...>`: Select specific columns to display in the table output. Available columns include: `pid`, `ppid`, `uid`, `user`, `name`, `state`, `rss`, `vm`, `threads`, `cmdline`. Column names are case-insensitive.
    ```bash
    ./processAnalyzer list --columns pid,name,rss,cmdline
    ./processAnalyzer pid 1234 --columns pid,name,cmdline
    ```
*   `--no-truncate-cmdline`: Prevent the command line (`cmdline`) column from being truncated in table view. This option only affects table output.
    ```bash
    ./processAnalyzer list --columns pid,cmdline --no-truncate-cmdline
    ```
*   `--format <csv|json>`: Specify the output format. `csv` provides Comma Separated Values, and `json` provides JSON array output. When this option is used with the `pid` command, a table/CSV/JSON output is produced instead of the default vertical details, and `--columns` can be used to select fields.
    ```bash
    ./processAnalyzer list --format csv --columns pid,name,rss
    ./processAnalyzer name systemd --format json
    ./processAnalyzer pid 1234 --format json --columns pid,name,cmdline
    ```

### PID Specific Options

These options are only valid when used with the `pid` command. When used, the output will first show the vertical details of the process (unless `--format` is also used), followed by the requested additional information.

*   `--children`: Display a list of child processes for the specified PID. Child processes are shown in a standard table format.
    ```bash
    ./processAnalyzer pid 1 --children
    ```
*   `--open-files`: Display a list of files opened by the specified PID.
    ```bash
    ./processAnalyzer pid 1234 --open-files
    ```
*   Combining PID specific options:
    ```bash
    ./processAnalyzer pid 1 --children --open-files
    ```

## Configuration

`processAnalyzer` does not currently support external configuration files or environment variables to alter its behavior. All configurations are done via command-line arguments.
