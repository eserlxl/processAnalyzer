# Configuration

`processAnalyzer` allows for flexible configuration through a dedicated configuration file, specified using the `--config` command-line option. This enables users to define default behaviors, output formats, and other settings without repeatedly typing command-line arguments.

## Specifying a Configuration File

You can provide a configuration file at runtime:

```bash
./processAnalyzer --config /path/to/your/config.yaml list
```

## Configuration File Format

(Add details here about the expected format, e.g., YAML, JSON, or a custom format, and example content.)

### Example Configuration (YAML)

```yaml
# Example configuration for processAnalyzer
defaults:
  format: json
  sort_by: cpu_usage
  descending: true
filters:
  name_contains: ["chrome", "firefox"]
  min_memory_mb: 100
output:
  include_children: true
  include_open_files: false
```

## Available Configuration Options

(List and describe the configuration options that the `processAnalyzer` supports. Refer to `src/main.cpp` and `include/` for actual options.)

*   **`defaults`**:
    *   `format`: Default output format (e.g., `text`, `json`).
    *   `sort_by`: Default sorting key (e.g., `pid`, `name`, `cpu_usage`, `memory_rss`).
    *   `descending`: `true` for descending order, `false` for ascending.
*   **`filters`**:
    *   `name_contains`: A list of strings; processes whose names contain any of these strings will be filtered.
    *   `min_memory_mb`: Minimum Resident Set Size (RSS) memory in MB for a process to be included.
*   **`output`**:
    *   `include_children`: `true` to include child processes in detailed views.
    *   `include_open_files`: `true` to include open files for a process in detailed views.

This section should be expanded based on the actual configuration parsing logic in `src/main.cpp`.
