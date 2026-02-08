// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils.h"
#include <fstream>
#include <algorithm>
#include <charconv>
#include <cctype> // for std::isdigit, std::tolower, std::toupper
#include <vector> // for std::vector
#include <iterator> // for std::istreambuf_iterator
#include <cstdlib> // for getenv
#include <cstdarg> // for va_list, va_start, va_end
#include <cstdio>  // for vsnprintf
#include <memory>  // for std::unique_ptr

namespace Utils {

class UtilsErrorCategory : public std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override {
        return "UtilsError";
    }

    [[nodiscard]] std::string message(int ev) const override {
        switch (static_cast<UtilsError>(ev)) {
            case UtilsError::None: return "Success";
            case UtilsError::FileNotFound: return "File not found";
            case UtilsError::PermissionDenied: return "Permission denied";
            case UtilsError::IOError: return "I/O error";
            case UtilsError::InvalidArgument: return "Invalid argument";
            case UtilsError::UnsupportedOperation: return "Unsupported operation";
            case UtilsError::PathError: return "Path error";
            case UtilsError::CommandExecutionError: return "Command execution error";
            default: return "Unknown error";
        }
    }
};

const UtilsErrorCategory& utilsCategory() {
    static UtilsErrorCategory instance;
    return instance;
}

std::error_code make_error_code(UtilsError e) {
    return {static_cast<int>(e), utilsCategory()};
}

// --- Filesystem Operations ---

Result<std::string> readTextFile(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::FileNotFound));
    }
    // Removed is_regular_file check as it can be problematic for /proc pseudo-files
    
    std::ifstream file(path); 
    if (!file.is_open()) {
        // More specific error for permission issues
        return std::unexpected(make_error_code(UtilsError::PermissionDenied));
    }

    // Read file content using iterators for robustness with pseudo-files
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    if (file.bad()) { // Check for I/O errors during reading
        return std::unexpected(make_error_code(UtilsError::IOError));
    }
    
    return content;
}

Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::PermissionDenied));
    }
    if (file.write(content.data(), static_cast<std::streamsize>(content.size()))) {
        return {};
    }
    return std::unexpected(make_error_code(UtilsError::IOError));
}

Result<void> appendToFile(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::out | std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::PermissionDenied));
    }
    if (file.write(content.data(), static_cast<std::streamsize>(content.size()))) {
        return {};
    }
    return std::unexpected(make_error_code(UtilsError::IOError));
}

Result<std::vector<std::string>> readLines(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::FileNotFound));
    }
    if (!std::filesystem::is_regular_file(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::IOError));
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::PermissionDenied));
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(std::move(line));
    }

    if (file.bad()) {
        return std::unexpected(make_error_code(UtilsError::IOError));
    }
    return lines;
}

Result<void> createDirectories(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

Result<void> remove(const std::filesystem::path& path, bool recursive) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::FileNotFound));
    }

    if (recursive) {
        std::filesystem::remove_all(path, ec);
    } else {
        std::filesystem::remove(path, ec);
    }

    if (ec) {
        // Map specific std::error_code to UtilsError if applicable
        if (ec == std::make_error_code(std::errc::permission_denied)) {
            return std::unexpected(make_error_code(UtilsError::PermissionDenied));
        }
        return std::unexpected(ec); // Return the underlying filesystem error
    }
    return {};
}

Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::FileNotFound));
    }
    if (!std::filesystem::is_directory(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::IOError)); // Path is not a directory
    }

    std::vector<std::filesystem::path> entries;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (ec) {
            return std::unexpected(ec);
        }
        entries.push_back(entry.path());
    }
    if (ec) { // Check for errors after iteration
        return std::unexpected(ec);
    }
    return entries;
}

Result<void> copyFile(const std::filesystem::path& source, const std::filesystem::path& destination) {
    std::error_code ec;
    std::filesystem::copy(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

bool copyFile_deprecated(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& ec) {
    std::filesystem::copy(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination) {
    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

bool moveFile_deprecated(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& ec) {
    std::filesystem::rename(source, destination, ec);
    return !ec;
}

Result<uintmax_t> getFileSize(const std::filesystem::path& filePath) {
    std::error_code ec;
    auto size = std::filesystem::file_size(filePath, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return size;
}

std::optional<uintmax_t> getFileSize_deprecated(const std::filesystem::path& filePath, std::error_code& ec) {
    if (std::filesystem::exists(filePath, ec) && !ec && std::filesystem::is_regular_file(filePath, ec) && !ec) {
        return std::filesystem::file_size(filePath, ec);
    }
    if (!ec) {
        ec = make_error_code(UtilsError::FileNotFound);
    }
    return std::nullopt;
}

bool traverseDirectory_deprecated(const std::filesystem::path& dirPath, const std::function<void(const std::filesystem::path&)>& callback, bool recursive) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dirPath, ec)) {
        return false;
    }

    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath, ec)) {
            if (ec) return false;
            callback(entry.path());
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec)) {
            if (ec) return false;
            callback(entry.path());
        }
    }
    return !ec;
}

bool exists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool isFile(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool isDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

// --- String Manipulation ---

std::string trim(std::string_view s) {
    auto first = s.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string_view::npos) {
        return "";
    }
    auto last = s.find_last_not_of(" \t\n\r\f\v");
    return std::string(s.substr(first, (last - first + 1)));
}

bool startsWith(std::string_view s, std::string_view prefix) {
    return s.starts_with(prefix);
}

bool endsWith(std::string_view s, std::string_view suffix) {
    return s.ends_with(suffix);
}

bool contains(std::string_view s, std::string_view substring) {
    return s.find(substring) != std::string_view::npos;
}

bool startsWithIgnoreCase(std::string_view str, std::string_view prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), str.begin(),
                      [](unsigned char c1, unsigned char c2) {
                          return std::tolower(c1) == std::tolower(c2);
                      });
}

bool endsWithIgnoreCase(std::string_view str, std::string_view suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
                      [](unsigned char c1, unsigned char c2) {
                          return std::tolower(c1) == std::tolower(c2);
                      });
}

bool containsIgnoreCase(std::string_view str, std::string_view subStr) {
    auto it = std::search(str.begin(), str.end(),
                          subStr.begin(), subStr.end(),
                          [](unsigned char ch1, unsigned char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
    return it != str.end();
}

std::string toLower(std::string_view s) {
    std::string result(s.length(), ' ');
    std::ranges::transform(s, result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string toUpper(std::string_view s) {
    std::string result(s.length(), ' ');
    std::ranges::transform(s, result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return result;
}

std::string replace(std::string_view s, std::string_view target, std::string_view replacement) {
    if (target.empty()) {
        return std::string(s); // Avoid infinite loop if target is empty
    }

    std::string result;
    result.reserve(s.length()); // Pre-allocate memory
    size_t currentPos = 0;
    size_t foundPos;

    while ((foundPos = s.find(target, currentPos)) != std::string_view::npos) {
        result.append(s.substr(currentPos, foundPos - currentPos));
        result.append(replacement);
        currentPos = foundPos + target.length();
    }
    result.append(s.substr(currentPos));
    return result;
}

std::string replaceFirst(std::string_view s, std::string_view from, std::string_view to) {
    auto pos = s.find(from);
    if (pos == std::string_view::npos) {
        return std::string(s);
    }
    std::string result;
    result.reserve(s.length() - from.length() + to.length());
    result.append(s.substr(0, pos));
    result.append(to);
    result.append(s.substr(pos + from.length()));
    return result;
}

std::string replaceN(std::string_view s, std::string_view from, std::string_view to, size_t count) {
    if (from.empty() || count == 0) {
        return std::string(s);
    }

    std::string result;
    result.reserve(s.length());
    size_t currentPos = 0;
    size_t replacements = 0;

    while (replacements < count) {
        auto foundPos = s.find(from, currentPos);
        if (foundPos == std::string_view::npos) {
            break;
        }
        result.append(s.substr(currentPos, foundPos - currentPos));
        result.append(to);
        currentPos = foundPos + from.length();
        replacements++;
    }

    result.append(s.substr(currentPos));
    return result;
}

std::string join(const std::vector<std::string>& parts, std::string_view delimiter) {
    if (parts.empty()) {
        return "";
    }

    std::string result;
    // Calculate approximate size to reserve memory
    size_t totalLength = 0;
    for (const auto& part : parts) {
        totalLength += part.length();
    }
    totalLength += (parts.size() - 1) * delimiter.length();
    result.reserve(totalLength);

    auto it = parts.begin();
    result.append(*it);
    for (++it; it != parts.end(); ++it) {
        result.append(delimiter);
        result.append(*it);
    }
    return result;
}

std::string formatString_deprecated(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    // First, determine the required buffer size
    va_list argsCopy;
    va_copy(argsCopy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, argsCopy);
    va_end(argsCopy);

    if (size < 0) {
        va_end(args);
        return ""; // Encoding error
    }

    auto buf = std::make_unique<char[]>(size + 1);
    std::vsnprintf(buf.get(), size + 1, fmt, args);
    
    va_end(args);
    return std::string(buf.get(), size);
}

std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty) {
    std::vector<std::string> tokens;
    if (s.empty()) {
        return tokens;
    }
    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string_view::npos) {
        auto token = s.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.emplace_back(token);
        }
        start = end + 1;
        end = s.find(delimiter, start);
    }
    auto token = s.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.emplace_back(token);
    }
    return tokens;
}

std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty) {
    std::vector<std::string> tokens;
    if (s.empty()) {
        return tokens;
    }
    if (delimiter.empty()) {
        if (!s.empty()) tokens.emplace_back(s);
        return tokens;
    }

    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string_view::npos) {
        auto token = s.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.emplace_back(token);
        }
        start = end + delimiter.size();
        end = s.find(delimiter, start);
    }
    auto token = s.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.emplace_back(token);
    }
    return tokens;
}

// --- Numeric Parsing/Validation ---

bool isInteger(std::string_view s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-' || s[0] == '+') {
        start = 1;
    }
    if (start == s.size()) return false;
    return std::all_of(s.begin() + start, s.end(), [](unsigned char c){ return std::isdigit(c); });
}

bool isFloatingPoint(std::string_view s) {
    return toDouble(s).has_value();
}

Result<double> toDouble(std::string_view s) {
    double val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::unexpected(make_error_code(UtilsError::InvalidArgument));

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::unexpected(make_error_code(UtilsError::InvalidArgument));
}

Result<long> toLong(std::string_view s, int base) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::unexpected(make_error_code(UtilsError::InvalidArgument));

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val, base);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::unexpected(make_error_code(UtilsError::InvalidArgument));
}

std::optional<long> toLong_deprecated(std::string_view s) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::nullopt;

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::nullopt;
}

std::optional<double> toDouble_deprecated(std::string_view s) {
    double val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::nullopt;

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::nullopt;
}

Result<bool> parseBool(std::string_view s) {
    std::string lowerS = toLower(s);
    if (lowerS == "true" || lowerS == "1" || lowerS == "yes") {
        return true;
    }
    if (lowerS == "false" || lowerS == "0" || lowerS == "no") {
        return false;
    }
    return std::unexpected(make_error_code(UtilsError::InvalidArgument));
}

std::optional<bool> parseBool_deprecated(std::string_view s) {
    std::string lowerS = toLower(s);
    if (lowerS == "true" || lowerS == "1" || lowerS == "yes") {
        return true;
    }
    if (lowerS == "false" || lowerS == "0" || lowerS == "no") {
        return false;
    }
    return std::nullopt;
}

Result<int> toInt(std::string_view s, int base) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::unexpected(make_error_code(UtilsError::InvalidArgument));

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val, base);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        if (val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max()) {
            return static_cast<int>(val);
        }
    }
    return std::unexpected(make_error_code(UtilsError::InvalidArgument));
}

std::optional<int> toInt_deprecated(std::string_view s) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::nullopt;

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        if (val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max()) {
            return static_cast<int>(val);
        }
    }
    return std::nullopt;
}

Result<float> toFloat(std::string_view s) {
    double val;
    auto res = toDouble(s);
    if (res) {
        val = *res;
        if (val >= -std::numeric_limits<float>::max() && val <= std::numeric_limits<float>::max()) {
            return static_cast<float>(val);
        }
    }
    return std::unexpected(make_error_code(UtilsError::InvalidArgument));
}

std::optional<float> toFloat_deprecated(std::string_view s) {
    double val;
    auto opt = toDouble(s);
    if (opt) {
        val = *opt;
        if (val >= -std::numeric_limits<float>::max() && val <= std::numeric_limits<float>::max()) {
            return static_cast<float>(val);
        }
    }
    return std::nullopt;
}

// --- System Interaction ---

Result<std::string> getEnv(const std::string& name) {
    char* value = std::getenv(name.c_str());
    if (value) {
        return std::string(value);
    }
    return std::unexpected(make_error_code(UtilsError::InvalidArgument));
}

Result<CommandOutput> executeCommand(const std::string& command) {
    std::string commandRedirect = command + " 2>&1";
    FILE* pipe = popen(commandRedirect.c_str(), "r");
    if (!pipe) {
        return std::unexpected(make_error_code(UtilsError::CommandExecutionError));
    }

    std::array<char, 128> buffer;
    std::string stdoutStr;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        stdoutStr += buffer.data();
    }

    int pcloseResult = pclose(pipe);
    int exitCode = WEXITSTATUS(pcloseResult);

    // With 2>&1, stderr is mixed with stdout. We can't separate them with popen.
    // For more complex scenarios, a different approach (e.g., custom fork/exec) would be needed.
    return CommandOutput{stdoutStr, "", exitCode};
}

bool executeCommand_deprecated(const std::string& command, std::string& stdoutStr, std::string& stderrStr, int& exitCode) {
    std::string commandRedirect = command + " 2>&1";
    FILE* pipe = popen(commandRedirect.c_str(), "r");
    if (!pipe) {
        stderrStr = "popen() failed!";
        return false;
    }

    std::array<char, 128> buffer;
    stdoutStr.clear();
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        stdoutStr += buffer.data();
    }

    int pcloseResult = pclose(pipe);
    exitCode = WEXITSTATUS(pcloseResult);

    return true;
}

} // namespace Utils
