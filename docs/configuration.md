# Configuration

> **Note:** Configuration file support is currently on the **roadmap** and is **experimental**. While the `--config-file` argument is present in the command-line interface, the logic to fully load and apply these settings is under development.

Future versions of `processAnalyzer` will allow for flexible configuration through a dedicated configuration file. This will enable users to define default behaviors, output formats, and other settings without repeatedly typing command-line arguments.

## Planned Configuration Features

The following features are planned for the configuration system:

### Specifying a Configuration File

You will be able to provide a configuration file at runtime:

```bash
./processAnalyzer list --config-file /path/to/your/config.yaml
```

### Proposed Configuration File Format (YAML)

```yaml
# Example configuration for processAnalyzer
defaults:
  output: json
  sort_by: cpu
  sort_order: desc
filters:
  name_contains: ["chrome", "firefox"]
  min_memory_mb: 100
output:
  include_children: true
  include_open_files: false
```

## Current Configuration

Currently, `processAnalyzer` relies entirely on **command-line arguments** for all configuration. Please refer to the [Usage Guide](usage.md) for a complete list of available options.

*   **Sorting**: Use `--sort-by` and `--sort-order`.
*   **Filtering**: Use `--state`, `--ppid`, `--name`, or `--user`.
*   **Output**: Use `--output`, `--columns`, `--brief`, etc.
