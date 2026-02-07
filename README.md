# Process Analyzer CLI

A C++ command-line tool to analyze running processes.

## Build

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
# List all processes
./processAnalyzer list

# Get details for a specific PID
./processAnalyzer pid 1234
```

## Structure

- `src/`: Source files
- `include/`: Header files
- `CMakeLists.txt`: CMake build configuration
