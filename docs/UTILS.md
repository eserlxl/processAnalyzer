# Utils Library Documentation

The `utils` namespace, implemented in `src/utils.cpp` and declared in `include/utils.h`, provides a collection of general-purpose utility functions used throughout the `processAnalyzer` project. These utilities aim to simplify common tasks related to file system operations, string manipulation, numeric conversions, and system interactions.

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
- `UtilsError::fileAlreadyExists`: The file already exists.
- `UtilsError::directoryNotEmpty`: The directory is not empty.
- `UtilsError::notADirectory`: The path is not a directory.
- `UtilsError::notAFile`: The path is not a file.
- `UtilsError::isADirectory`: The path is a directory, but a file was expected.
- `UtilsError::diskFull`: The disk is full.
- `UtilsError::noSpaceOnDevice`: No space left on the device.
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
- `UtilsError::permissionDeniedCwd`: Permission denied when accessing the current working directory.
- `UtilsError::invalidTimeFormat`: The time format is invalid.
- `UtilsError::timeParseError`: An error occurred while parsing a time string.


## File System Operations

These functions interact with the file system using `std::filesystem`.

- `Result<std::string> readTextFile(const std::filesystem::path& path)`: Reads the entire content of a text file into a `std::string`.
- `Result<std::vector<std::byte>> readBinaryFile(const std::filesystem::path& path)`: Reads the entire content of a binary file into a `std::vector<std::byte>`.
- `Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content)`: Writes the given `content` to a text file, overwriting it if it exists.
- `Result<void> writeBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content)`: Writes the given `content` to a binary file, overwriting it if it exists.
- `Result<void> writeTextFileAtomic(const std::filesystem::path& path, std::string_view content)`: Atomically writes content to a text file.
- `Result<void> writeBinaryFileAtomic(const std::filesystem::path& path, std::span<const std::byte> content)`: Atomically writes content to a binary file.
- `Result<void> appendToFile(const std::filesystem::path& path, std::string_view content)`: Appends the given `content` to the end of a text file.
- `Result<void> appendToBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content)`: Appends the given `content` to the end of a binary file.
- `Result<std::filesystem::perms> getPermissions(const std::filesystem::path& path)`: Gets the permissions of a file or directory.
- `Result<void> setPermissions(const std::filesystem::path& path, std::filesystem::perms prms)`: Sets the permissions of a file or directory.
- `Result<void> addPermissions(const std::filesystem::path& path, std::filesystem::perms prms)`: Adds specified permissions to a file or directory.
- `Result<void> removePermissions(const std::filesystem::path& path, std::filesystem::perms prms)`: Removes specified permissions from a file or directory.
- `Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group)`: Changes the owner and group of a file or directory.
- `bool exists(const std::filesystem::path& path)`: Checks if a file or directory exists.
- `bool isFile(const std::filesystem::path& path)`: Checks if a path points to a regular file.
- `bool isDirectory(const std::filesystem::path& path)`: Checks if a path points to a directory.
- `bool isReadable(const std::filesystem::path& path)`: Checks if a file or directory is readable.
- `bool isWritable(const std::filesystem::path& path)`: Checks if a file or directory is writable.
- `bool isExecutable(const std::filesystem::path& path)`: Checks if a file is executable.
- `Result<std::vector<std::string>> readLines(const std::filesystem::path& path)`: Reads all lines from a text file into a vector of strings.
- `Result<void> createDirectories(const std::filesystem::path& path)`: Creates all directories in the specified path.
- `Result<void> remove(const std::filesystem::path& path, bool recursive = false)`: Removes a file or a directory. If `recursive` is `true`, it removes a directory and all its contents.
- `Result<std::filesystem::path> createTemporaryFile(std::string_view prefix = "", std::string_view suffix = "")`: Creates a temporary file.
- `Result<std::filesystem::path> createTemporaryDirectory(std::string_view prefix = "")`: Creates a temporary directory.
- `Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path)`: Lists all entries within a directory.
- `Result<void> copyFile(const std::filesystem::path& source, const std::filesystem::path& destination)`: Copies a file from source to destination.
- `Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination)`: Moves a file from source to destination.
- `Result<uintmax_t> getFileSize(const std::filesystem::path& filePath)`: Gets the size of a file in bytes.

### Directory Traversal

- `enum class TraversalControl { Continue, SkipDir, Stop }`: Controls the flow of directory traversal.
- `using TraversalCallback = std::function<TraversalControl(const std::filesystem::directory_entry& entry)>`: Callback function type for directory traversal.
- `struct TraversalOptions`: Options for directory traversal (`recursive`, `followSymlinks`, `includeDirectories`, `includeFiles`, `maxDepth`).
- `Result<void> traverseDirectory(const std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options = {})`: Traverses a directory and its subdirectories, invoking a callback for each entry.

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

- `Result<std::string> urlEncode(std::string_view s)`: URL-encodes a string.
- `Result<std::string> urlDecode(std::string_view s)`: URL-decodes a string.
- `Result<std::string> base64Encode(std::string_view s)`: Base64-encodes a string.
- `Result<std::string> base64Decode(std::string_view s)`: Base64-decodes a string.
- `Result<std::string> base64Encode(std::span<const std::byte> data)`: Base64-encodes binary data.
- `Result<std::vector<std::byte>> base64DecodeToBytes(std::string_view s)`: Base64-decodes a string to binary data.
- `Result<std::string> generateUuid()`: Generates a new UUID.
- `bool equalsIgnoreCase(std::string_view s1, std::string_view s2)`: Case-insensitive string comparison.
- `std::string trim(std::string_view s)`: Removes leading and trailing whitespace.
- `bool startsWith(std::string_view s, std::string_view prefix)`: Checks for a prefix.
- `bool endsWith(std::string_view s, std::string_view suffix)`: Checks for a suffix.
- `bool contains(std::string_view s, std::string_view substring)`: Checks for a substring.
- `bool startsWithIgnoreCase(std::string_view str, std::string_view prefix)`: Case-insensitive check for a prefix.
- `bool endsWithIgnoreCase(std::string_view str, std::string_view suffix)`: Case-insensitive check for a suffix.
- `bool containsIgnoreCase(std::string_view str, std::string_view subStr)`: Case-insensitive check for a substring.
- `std::string toLower(std::string_view s)`: Converts a string to lowercase.
- `std::string toUpper(std::string_view s)`: Converts a string to uppercase.
- `std::string replace(std::string_view s, std::string_view target, std::string_view replacement)`: Replaces all occurrences of a substring.
- `std::string replaceFirst(std::string_view s, std::string_view from, std::string_view to)`: Replaces the first occurrence of a substring.
- `std::string replaceN(std::string_view s, std::string_view from, std::string_view to, size_t count)`: Replaces the first `count` occurrences of a substring.
- `std::string join(const std::vector<std::string>& parts, std::string_view delimiter)`: Joins a collection of strings with a delimiter.
- `template<typename... Args> std::string format(std::string_view fmt, Args&&... args)`: Formats a string using `std::format`-like syntax.
- `std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false)`: Splits a string by a character delimiter.
- `std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false)`: Splits a string by a string delimiter.

## Numeric Parsing/Validation

- `bool isInteger(std::string_view s)`: Checks if a string represents an integer.
- `bool isFloatingPoint(std::string_view s)`: Checks if a string represents a floating-point number.
- `Result<long> toLong(std::string_view s, int base = 10)`: Converts a string to a `long`.
- `Result<double> toDouble(std::string_view s)`: Converts a string to a `double`.
- `Result<int> toInt(std::string_view s, int base = 10)`: Converts a string to an `int`.
- `Result<float> toFloat(std::string_view s)`: Converts a string to a `float`.
- `Result<bool> parseBool(std::string_view s)`: Parses a string into a boolean (`true`, `false`, `1`, `0`).

## System Interaction

- `struct CommandOutput { std::string stdoutStr; std::string stderrStr; int exitCode; }`: Holds the output of an executed command.
- `Result<CommandOutput> executeCommand(const std::string& command)`: Executes a shell command and captures its output and exit code.
- `Result<std::string> getEnv(const std::string& name)`: Retrieves the value of an environment variable.
- `Result<void> setEnv(std::string_view name, std::string_view value)`: Sets an environment variable.
- `Result<void> unsetEnv(std::string_view name)`: Unsets an environment variable.
- `Result<std::filesystem::path> getCurrentWorkingDirectory()`: Gets the current working directory.
- `Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path)`: Sets the current working directory.

## Time Utilities

- `Result<std::chrono::system_clock::time_point> getCurrentSystemTime()`: Gets the current system time.
- `Result<std::chrono::steady_clock::time_point> getCurrentSteadyTime()`: Gets the current steady time.
- `Result<std::string> formatTimestamp(std::chrono::system_clock::time_point tp, std::string_view formatStr)`: Formats a timestamp into a string.
- `Result<std::chrono::system_clock::time_point> parseTimestamp(std::string_view timestampStr, std::string_view formatStr)`: Parses a timestamp string.
- `std::string formatElapsedTime(long long seconds)`: Formats a duration in seconds into a human-readable string.
- `std::string formatTimestamp(long long unixTimestamp)`: Formats a Unix timestamp into a string.

## Unit Tests

The utility functions are tested in `tests/Utils.cpp`, `tests/TestUtils.cpp`, `tests/UtilsNewApiTest.cpp` and `tests/TestUtilsNewApiTest.cpp`. These tests cover various scenarios, including edge cases and error conditions.
