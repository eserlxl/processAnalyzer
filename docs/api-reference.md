# API Reference

The `processAnalyzer` project exposes a C++ API primarily through the headers in the `include/` directory:

*   [`include/Analyzer.h`](../include/Analyzer.h): Defines the core `Analyzer` class and related structures for process inspection and analysis.
*   [`include/utils.h`](../include/utils.h): Provides a collection of general-purpose utility functions used throughout the project, often within the `Utils` namespace.

While formal Doxygen-generated documentation is not currently provided, you can examine these header files directly for detailed information on available classes, methods, and functions.

## Key API Components

### `Analyzer` Class

The `Analyzer` class (defined in `include/Analyzer.h`) is the primary interface for interacting with system processes. It provides methods for:

*   Listing all processes.
*   Getting details for a specific process ID (PID).
*   Filtering and sorting processes based on various criteria.

### `Utils` Namespace

The `Utils` namespace (defined in `include/utils.h` and implemented in `src/utils.cpp`) offers a robust set of helper functions, including:

*   File system operations.
*   String manipulation.
*   Numeric parsing.
*   System interactions.

For a comprehensive overview and usage examples of the utility library, please refer to [docs/UTILS.md](UTILS.md).

## Future Enhancements

We plan to integrate Doxygen or a similar tool in the future to generate comprehensive, browsable API documentation automatically.
