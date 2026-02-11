# Utils Library Documentation

The `utils` namespace provides a collection of general-purpose utility functions used throughout the `processAnalyzer` project. These utilities are organized into modules with headers in `include/utils/` and implementations in `src/utils/`. They aim to simplify common tasks related to file system operations, string manipulation, numeric conversions, and system interactions.

## Usage

To use the utility functions, include the specific header file that contains the functionality you need. All utility headers are located in the `include/utils/` directory.

For example, to use file-related functions, you would include `utils/file.h`:

```cpp
#include "utils/file.h"
#include <iostream>

int main() {
    auto result = utils::readTextFile("example.txt");
    if (result) {
        std::cout << "File content: " << *result << std::endl;
    } else {
        std::cerr << "Error reading file: " << result.error().message() << std::endl;
    }
    return 0;
}
```

This modular approach ensures that you only include what is necessary, which can help reduce compilation times.

You can then access the utilities via the `utils` namespace.

Many functions now return `Result<T>` which is a `std::expected<T, std::error_code>`, allowing for robust error handling. Older functions using `std::optional` or out-parameters for error codes are being deprecated.

## Error Handling

Functions in the `utils` namespace that can fail return a `Result<T>` type, which is an alias for `std::expected<T, std::error_code>`. This allows functions to return either a successful value `T` or an `std::error_code` on failure.

The custom error codes are defined in the `UtilsError` enum:
- `UtilsError::none`: No error.
- `UtilsError::fileNotFound`: The specified file was not found.
- `UtilsError::permissionDenied`: Access to the file or resource was denied.
- `UtilsError::ioError`: A general input/output error occurred.
- `UtilsError::invalidArgument`: An invalid argument was provided to the function.
- `UtilsError::unsupportedOperation`: The attempted operation is not supported.
- `UtilsError::pathError`: A path manipulation failure occurred.
- `UtilsError::commandExecutionError`: An error occurred during external command execution.
- `UtilsError::fileTooLarge`: The file is too large to be processed.
- `UtilsError::fileAlreadyExists`: The file already exists.
- `UtilsError::directoryNotEmpty`: The directory is not empty.
- `UtilsError::notADirectory`: The path is not a directory.
- `UtilsError::notAFile`: The path is not a file.
- `UtilsError::isADirectory`: The path is a directory, but a file was expected.
- `UtilsError::diskFull`: The disk is full.
- `UtilsError::noSpaceOnDevice`: No space left on the device.
- `UtilsError::tempDirectoryError`: Failed to find or create a temporary directory.
- `UtilsError::traversalStopped`: The directory traversal was stopped by the callback.
- `UtilsError::pathNotRelative`: The path is not relative.
- `UtilsError::pathNotAbsolute`: The path is not absolute.
- `UtilsError::basePathNotAncestor`: The base path is not an ancestor of the given path.
- `UtilsError::invalidPathFormat`: The path format is invalid.
- `UtilsError::invalidBase64Input`: The Base64 input is invalid.
- `UtilsError::invalidUrlEncoding`: The URL encoding is invalid.
- `UtilsError::invalidUuidFormat`: The UUID format is invalid.
- `UtilsError::envVarNotFound`: The environment variable was not found.
- `UtilsError::commandNotFound`: The command was not found.
- `UtilsError::commandFailed`: The command failed to execute.
- `UtilsError::processSpawnFailure`: Failed to spawn a new process.
- `UtilsError::permissionDeniedCwd`: Permission denied for changing CWD.
- `UtilsError::invalidTimeFormat`: The time format is invalid.
- `UtilsError::timeParseError`: An error occurred while parsing a time string.
- `UtilsError::outOfRange`: A value was out of range.

### Analyzer-Specific Errors
These errors are used within the `analyzer` component.
- `UtilsError::analyzerPermissionDenied`: Analyzer: Permission denied during process analysis.
- `UtilsError::analyzerParsingError`: Analyzer: A parsing error occurred.
- `UtilsError::analyzerProcessNotFound`: Analyzer: The specified process was not found.
- `UtilsError::analyzerSystemError`: Analyzer: A system-level error occurred.


## File System and Directory Operations

These functions provide a comprehensive interface for interacting with the file system using `std::filesystem`.

### File I/O

- `Result<std::string> readTextFile(const std::filesystem::path& path)`: Reads the entire content of a text file into a `std::string`.
- `Result<std::vector<std::byte>> readBinaryFile(const std::filesystem::path& path)`: Reads the entire content of a binary file into a `std::vector<std::byte>`.
- `Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content)`: Writes `content` to a text file, overwriting it if it exists.
- `Result<void> writeBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content)`: Writes `content` to a binary file, overwriting it if it exists.
- `Result<void> writeTextFileAtomic(const std::filesystem::path& path, std::string_view content)`: Atomically writes content to a text file by writing to a temporary file and then renaming.
- `Result<void> writeBinaryFileAtomic(const std::filesystem::path& path, std::span<const std::byte> content)`: Atomically writes content to a binary file.
- `Result<void> appendToFile(const std::filesystem::path& path, std::string_view content)`: Appends `content` to the end of a text file.
- `Result<void> appendToBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content)`: Appends `content` to the end of a binary file.
- `Result<std::vector<std::string>> readLines(const std::filesystem::path& path)`: Reads all lines from a text file into a vector of strings.

### Filesystem Manipulation

- `Result<void> createDirectories(const std::filesystem::path& path)`: Creates all directories in the specified path.
- `Result<void> remove(const std::filesystem::path& path, bool recursive = false)`: Removes a file or directory. If `recursive` is `true`, it removes a directory and all its contents.
- `Result<std::filesystem::path> createTemporaryFile(std::string_view prefix = "", std::string_view suffix = "")`: Creates a unique temporary file.
- `Result<std::filesystem::path> createTemporaryDirectory(std::string_view prefix = "")`: Creates a unique temporary directory.
- `Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path)`: Lists all entries (files and directories) within a directory.
- `Result<void> copyFile(const std::filesystem::path& source, const std::filesystem::path& destination)`: Copies a file from `source` to `destination`.
- `Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination)`: Moves/renames a file from `source` to `destination`. It will create any necessary parent directories for the destination path.
- `Result<uintmax_t> getFileSize(const std::filesystem::path& filePath)`: Gets the size of a file in bytes.

### Path Information and Status

- `Result<bool> exists(const std::filesystem::path& path)`: Checks if a file or directory exists. Returns an error if status cannot be determined.
- `Result<bool> isFile(const std::filesystem::path& path)`: Checks if a path points to a regular file. Returns `false` if the path does not exist, or an error for other failures.
- `Result<bool> isDirectory(const std::filesystem::path& path)`: Checks if a path points to a directory. Returns `false` if the path does not exist, or an error for other failures.
- `bool isReadable(const std::filesystem::path& path)`: Checks if a file or directory is readable based on its permissions. Returns `false` on error.
- `bool isWritable(const std::filesystem::path& path)`: Checks if a file or directory is writable based on its permissions. Returns `false` on error.
- `bool isExecutable(const std::filesystem::path& path)`: Checks if a file is executable based on its permissions. Returns `false` on error.

> **Note**: The `isReadable`, `isWritable`, and `isExecutable` functions are simple wrappers that return `false` if an underlying error occurs (e.g., file not found). For robust error handling, use `getPermissions` and check the result explicitly.

### Permissions

- `Result<std::filesystem::perms> getPermissions(const std::filesystem::path& path)`: Gets the permissions of a file or directory.
- `Result<void> setPermissions(const std::filesystem::path& path, std::filesystem::perms prms)`: Sets the permissions of a file or directory.
- `Result<void> addPermissions(const std::filesystem::path& path, std::filesystem::perms prms)`: Adds specified permissions to a file or directory.
- `Result<void> removePermissions(const std::filesystem::path& path, std::filesystem::perms prms)`: Removes specified permissions from a file or directory.
- `Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group)`: **Unsupported.** This function is a placeholder and will always return an `unsupportedOperation` error.

### Directory Traversal

This powerful API allows you to recursively scan directories and act on files and subdirectories.

- `enum class TraversalControl { Continue, skipDir, stop }`: Controls the flow of directory traversal from within the callback.
  - `Continue`: Continue traversal normally.
  - `skipDir`: If the current entry is a directory, do not traverse into it. Continue with its siblings.
  - `stop`: Stop the entire traversal immediately.
- `using TraversalCallback = std::function<TraversalControl(const std::filesystem::directory_entry& entry)>`: The callback function signature. It receives a `directory_entry` and returns a `TraversalControl` value.
- `struct TraversalOptions`: A struct to configure traversal behavior:
  - `bool recursive = true`: Traverse subdirectories.
  - `bool followSymlinks = false`: Follow symbolic links to directories.
  - `bool includeDirectories = true`: Invoke the callback for directory entries.
  - `bool includeFiles = true`: Invoke the callback for file entries.
  - `int maxDepth = -1`: Maximum recursion depth (`-1` for unlimited).
- `Result<void> traverseDirectory(const std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options = {})`: Traverses a directory, invoking the callback for each entry that matches the options.

**Example: Find all `.cpp` files in a directory**
```cpp
#include "utils/File.h"
#include <iostream>

int main() {
    utils::TraversalOptions options;
    options.recursive = true;
    options.includeFiles = true;
    options.includeDirectories = false;

    auto callback = [](const std::filesystem::directory_entry& entry) {
        if (entry.path().extension() == ".cpp") {
            std::cout << "Found C++ file: " << entry.path() << std::endl;
        }
        return utils::TraversalControl::Continue;
    };

    auto result = utils::traverseDirectory("/path/to/source", callback, options);

    if (!result && result.error() != utils::make_error_code(utils::UtilsError::traversalStopped)) {
        std::cerr << "Error during traversal: " << result.error().message() << std::endl;
    }

    return 0;
}
```

## Path Manipulation

- `Result<std::filesystem::path> canonicalPath(const std::filesystem::path& path)`: Returns the canonical path.
- `Result<std::filesystem::path> makeRelative(const std::filesystem::path& path, const std::filesystem::path& base)`: Makes a path relative to a base path.
- `bool pathsEquivalent(const std::filesystem::path& p1, const std::filesystem::path& p2)`: Checks if two paths are equivalent.
- `std::filesystem::path getAbsolutePath(const std::filesystem::path& path)`: Returns the absolute path for a given path.
- `std::string getFileName(const std::filesystem::path& path)`: Extracts the file name from a path.
- `std::string getFileNameWithoutExtension(const std::filesystem::path& path)`: Extracts the file name without its extension.
- `std::string getFileExtension(const std::filesystem::path& path)`: Extracts the file extension from a path.
- `std::filesystem::path getParentPath(const std::filesystem::path& path)`: Gets the parent path of a given path.
- `std::filesystem::path joinPaths(const std::vector<std::filesystem::path>& paths)`: Joins multiple path components into a single path.
- `Result<void> createSymlink(const std::filesystem::path& target, const std::filesystem::path& link)`: Creates a symbolic link.
- `Result<std::filesystem::path> readSymlink(const std::filesystem::path& link)`: Reads the target of a symbolic link.
- `bool isSymlink(const std::filesystem::path& path)`: Checks if a path is a symbolic link.

## File Hashing

- `enum class HashAlgorithm { shA256, mD5, crC32, shA512 }`: Specifies the hashing algorithm to use.
- `Result<std::string> calculateFileHash(const std::filesystem::path& path, HashAlgorithm algo)`: Calculates the hash of a file using the specified algorithm.

## String Manipulation

- `std::string trim(std::string_view s)`: Removes leading and trailing whitespace from a string view.
- `bool startsWith(std::string_view s, std::string_view prefix)`: Checks if a string view starts with a specified prefix.
- `bool endsWith(std::string_view s, std::string_view suffix)`: Checks if a string view ends with a specified suffix.
- `bool contains(std::string_view s, std::string_view substring)`: Checks if a string view contains a specified substring.
- `bool startsWithIgnoreCase(std::string_view str, std::string_view prefix)`: Checks if a string view starts with a specified prefix, ignoring case.
- `bool endsWithIgnoreCase(std::string_view str, std::string_view suffix)`: Checks if a string view ends with a specified suffix, ignoring case.
- `bool containsIgnoreCase(std::string_view str, std::string_view subStr)`: Checks if a string view contains a specified substring, ignoring case.
- `std::string toLower(std::string_view s)`: Converts a string view to its lowercase equivalent.
- `std::string toUpper(std::string_view s)`: Converts a string view to its uppercase equivalent.
- `std::string replaceAll(std::string_view s, std::string_view target, std::string_view replacement)`: Replaces all occurrences of a target substring with a replacement substring in a string view.
- `std::string replaceFirst(std::string_view s, std::string_view from, std::string_view to)`: Replaces the first occurrence of a target substring with a replacement substring in a string view.
- `std::string replaceN(std::string_view s, std::string_view from, std::string_view to, size_t count)`: Replaces up to 'count' occurrences of a target substring with a replacement substring in a string view.
- `std::string join(const std::vector<std::string>& parts, std::string_view delimiter)`: Joins a vector of strings into a single string using a specified delimiter.
- `template<typename... Args> std::string format(std::format_string<Args...> fmt, Args&&... args)`: Formats a string using a format string and arguments.
- `std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false)`: Splits a string view into a vector of strings based on a character delimiter.
- `std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false)`: Splits a string view into a vector of strings based on a string view delimiter.

## Numeric Parsing/Validation

- `template<typename T> bool tryParse(std::string_view s, T& out)`: Safely parses a string view and updates the output variable on success.
- `bool isInteger(std::string_view s)`: Checks if a string represents an integer.
- `bool isFloatingPoint(std::string_view s)`: Checks if a string represents a floating-point number.
- `Result<long> toLong(std::string_view s, int base = 10)`: Converts a string to a `long`.
- `Result<double> toDouble(std::string_view s)`: Converts a string to a `double`.
- `Result<int> toInt(std::string_view s, int base = 10)`: Converts a string to an `int`.
- `Result<float> toFloat(std::string_view s)`: Converts a string to a `float`.
- `Result<bool> parseBool(std::string_view s)`: Parses a string into a boolean (`true`, `false`, `1`, `0`).

## System Interaction

- `struct CommandOutput { std::string stdoutStr; std::string stderrStr; int exitCode; }`: Holds command standard output, standard error, and exit code.
- `Result<CommandOutput> executeCommand(std::string_view command)`: Executes a shell command and captures `stdout` and `stderr` separately. Returns `UtilsError::invalidArgument` for empty commands or commands containing embedded NUL bytes.
- `Result<std::string> getEnv(std::string_view name)`: Retrieves the value of an environment variable. Returns `UtilsError::envVarNotFound` when the variable does not exist, and `UtilsError::invalidArgument` for invalid names.
- `Result<void> setEnv(std::string_view name, std::string_view value)`: Sets an environment variable. Returns `UtilsError::invalidArgument` for invalid names/values.
- `Result<void> unsetEnv(std::string_view name)`: Unsets an environment variable. Returns `UtilsError::invalidArgument` for invalid names.
- `Result<std::filesystem::path> getCurrentWorkingDirectory()`: Gets the current working directory.
- `Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path)`: Sets the current working directory and validates that the path argument is non-empty.

## Time Utilities

- `Result<std::chrono::system_clock::time_point> getCurrentSystemTime()`: Gets the current system time.
- `Result<std::chrono::steady_clock::time_point> getCurrentSteadyTime()`: Gets the current steady time.
- `Result<std::string> formatTimestamp(std::chrono::system_clock::time_point tp, std::string_view formatStr)`: Formats a timestamp into a string.
- `Result<std::chrono::system_clock::time_point> parseTimestamp(std::string_view timestampStr, std::string_view formatStr)`: Parses a timestamp string.
- `std::string formatElapsedTime(long long seconds)`: Formats a duration in seconds into a human-readable string.
- `std::string formatTimestamp(long long unixTimestamp)`: Formats a Unix timestamp into a string.

## Unit Tests

The utility functions are tested in `tests/utils/`, with specific tests such as `core.cpp`, `FileSystemTest.cpp`, and `TypesTest.cpp`. These tests cover various scenarios, including edge cases and error conditions.
