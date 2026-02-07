#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <system_error>
#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Utils {

enum class UtilsError {
    None = 0,
    FileNotFound,
    PermissionDenied,
    IOError,
    InvalidArgument,
    ParseError,
};

std::error_code make_error_code(UtilsError e);

template <typename T>
using Result = std::expected<T, std::error_code>;

// Existing APIs
std::string readFile(const std::string& path);
std::vector<std::string> split(const std::string& s, char delimiter);
bool isNumeric(const std::string& s);

// --- NEW APIs ---

// File I/O
Result<std::string> readTextFile(const std::filesystem::path& path);
Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content);
bool exists(const std::filesystem::path& path);
bool isFile(const std::filesystem::path& path);
bool isDirectory(const std::filesystem::path& path);

// String Manipulation
std::string trim(std::string_view s);
bool startsWith(std::string_view s, std::string_view prefix);
bool endsWith(std::string_view s, std::string_view suffix);
bool contains(std::string_view s, std::string_view substring);

std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false);
std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false);

// Numeric Parsing/Validation
bool isInteger(std::string_view s);
bool isFloatingPoint(std::string_view s);
std::optional<long> toLong(std::string_view s);
std::optional<double> toDouble(std::string_view s);

// Path Manipulation
std::filesystem::path parentPath(const std::filesystem::path& path);
std::filesystem::path fileName(const std::filesystem::path& path);
std::filesystem::path stem(const std::filesystem::path& path);
std::filesystem::path extension(const std::filesystem::path& path);
std::filesystem::path joinPaths(const std::filesystem::path& p1, const std::filesystem::path& p2);
std::filesystem::path joinPaths(const std::filesystem::path& p1, const std::filesystem::path& p2, const std::filesystem::path& p3);

} // namespace Utils

// Specialize std::is_error_code_enum
namespace std {
template <>
struct is_error_code_enum<Utils::UtilsError> : true_type {};
}

#endif // UTILS_H
