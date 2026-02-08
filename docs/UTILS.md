# Utils Library Documentation

The `Utils` namespace, implemented in `src/utils.cpp` and declared in `include/utils.h`, provides a collection of general-purpose utility functions used throughout the `processAnalyzer` project. These utilities aim to simplify common tasks related to file system operations, string manipulation, numeric conversions, and system interactions.

All file system operations and some other functions return `Result<T>` which is a `std::expected<T, std::error_code>`, allowing for robust error handling.

## Error Handling

Functions in the `Utils` namespace that can fail return a `Result<T>` type, which is an alias for `std::expected<T, std::error_code>`. This allows functions to return either a successful value `T` or an `std::error_code` on failure.

The custom error codes are defined in the `UtilsError` enum:
- `UtilsError::None`: No error.
- `UtilsError::FileNotFound`: The specified file was not found.
- `UtilsError::PermissionDenied`: Access to the file or resource was denied.
- `UtilsError::IOError`: A general input/output error occurred.
- `UtilsError::InvalidArgument`: An invalid argument was provided to the function.
- `UtilsError::ParseError`: An error occurred during parsing (e.g., numeric conversion).

## File System Operations

These functions interact with the file system using `std::filesystem`.

- `Result<std::string> readTextFile(const std::filesystem::path& path)`
  Reads the entire content of a text file into a `std::string`. Returns `UtilsError::FileNotFound`, `UtilsError::PermissionDenied`, or `UtilsError::IOError` on failure.

- `Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content)`
  Writes the given `content` to a text file. If the file exists, its content is truncated. Returns `UtilsError::PermissionDenied` or `UtilsError::IOError` on failure.

- `bool exists(const std::filesystem::path& path)`
  Checks if a file or directory exists at the specified path.

- `bool isFile(const std::filesystem::path& path)`
  Checks if the specified path points to a regular file.

- `bool isDirectory(const std::filesystem::path& path)`
  Checks if the specified path points to a directory.

- `Result<void> appendToFile(const std::filesystem::path& path, std::string_view content)`
  Appends the given `content` to the end of a text file. Returns `UtilsError::PermissionDenied` or `UtilsError::IOError` on failure.

- `Result<std::vector<std::string>> readLines(const std::filesystem::path& path)`
  Reads all lines from a text file into a `std::vector<std::string>`. Returns `UtilsError::FileNotFound`, `UtilsError::PermissionDenied`, or `UtilsError::IOError` on failure.

- `Result<void> createDirectories(const std::filesystem::path& path)`
  Creates all directories in the specified path, including any necessary parent directories. Returns an `std::error_code` on failure.

- `Result<void> remove(const std::filesystem::path& path, bool recursive = false)`
  Removes a file or an empty directory. If `recursive` is `true`, it removes a directory and all its contents. Returns `UtilsError::FileNotFound`, `UtilsError::PermissionDenied`, or an `std::error_code` on failure.

- `Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path)`
  Lists all entries (files and directories) directly within the specified directory. Returns `UtilsError::FileNotFound`, `UtilsError::IOError`, or an `std::error_code` on failure.

## String Manipulation

These functions provide common string processing capabilities.

- `std::string trim(std::string_view s)`
  Removes leading and trailing whitespace characters (space, tab, newline, carriage return, form feed, vertical tab) from a string view.

- `bool startsWith(std::string_view s, std::string_view prefix)`
  Checks if a string view `s` starts with the given `prefix`.

- `bool endsWith(std::string_view s, std::string_view suffix)`
  Checks if a string view `s` ends with the given `suffix`.

- `bool contains(std::string_view s, std::string_view substring)`
  Checks if a string view `s` contains the given `substring`.

- `std::string toLower(std::string_view s)`
  Converts all characters in a string view to lowercase.

- `std::string toUpper(std::string_view s)`
  Converts all characters in a string view to uppercase.

- `std::string replace(std::string_view s, std::string_view target, std::string_view replacement)`
  Replaces all occurrences of `target` with `replacement` in a string view `s`.

- `std::string join(const std::vector<std::string>& parts, std::string_view delimiter)`
  Concatenates a vector of strings into a single string, separated by the `delimiter`.

- `std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false)`
  Splits a string view `s` into a vector of strings using a character `delimiter`. If `skipEmpty` is true, empty tokens are not included.

- `std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false)`
  Splits a string view `s` into a vector of strings using a string `delimiter`. If `skipEmpty` is true, empty tokens are not included.

## Numeric Parsing/Validation

Functions for checking and converting string representations of numbers.

- `bool isInteger(std::string_view s)`
  Checks if a string view `s` represents a valid integer (can include optional leading `+` or `-`).

- `bool isFloatingPoint(std::string_view s)`
  Checks if a string view `s` represents a valid floating-point number.

- `std::optional<long> toLong(std::string_view s)`
  Converts a string view `s` to a `long`. Returns `std::nullopt` if the conversion fails. Handles optional leading `+`.

- `std::optional<double> toDouble(std::string_view s)`
  Converts a string view `s` to a `double`. Returns `std::nullopt` if the conversion fails. Handles optional leading `+`.

## System Interaction

- `std::optional<std::string> getEnv(const std::string& name)`
  Retrieves the value of an environment variable specified by `name`. Returns `std::nullopt` if the variable is not set.

## Unit Tests

The utility functions provided by the `Utils` namespace are thoroughly tested to ensure their correctness and reliability. Unit tests for the `Utils` library can be found in the `tests/` directory, specifically in `tests/Utils.cpp` and `tests/TestUtils.cpp`. These tests cover various scenarios, including edge cases and error conditions, to validate the behavior of each utility function.
