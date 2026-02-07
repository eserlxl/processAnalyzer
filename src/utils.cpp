// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils.h"
#include <fstream>
#include <algorithm>
#include <charconv>
#include <cctype> // for std::isdigit

namespace Utils {

class UtilsErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "UtilsError";
    }

    std::string message(int ev) const override {
        switch (static_cast<UtilsError>(ev)) {
            case UtilsError::None: return "Success";
            case UtilsError::FileNotFound: return "File not found";
            case UtilsError::PermissionDenied: return "Permission denied";
            case UtilsError::IOError: return "I/O error";
            case UtilsError::InvalidArgument: return "Invalid argument";
            case UtilsError::ParseError: return "Parse error";
            default: return "Unknown error";
        }
    }
};

const UtilsErrorCategory& utils_category() {
    static UtilsErrorCategory instance;
    return instance;
}

std::error_code make_error_code(UtilsError e) {
    return {static_cast<int>(e), utils_category()};
}

// --- NEW APIs ---

Result<std::string> readTextFile(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return std::unexpected(make_error_code(UtilsError::FileNotFound));
    }
    if (!std::filesystem::is_regular_file(path)) {
        return std::unexpected(make_error_code(UtilsError::IOError));
    }

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::PermissionDenied));
    }

    std::string content;
    try {
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size < 0) return std::unexpected(make_error_code(UtilsError::IOError));
        
        content.resize(static_cast<size_t>(size));
        if (file.read(content.data(), size)) {
             return content;
        } else {
             return std::unexpected(make_error_code(UtilsError::IOError));
        }
    } catch (...) {
        return std::unexpected(make_error_code(UtilsError::IOError));
    }
}

Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::PermissionDenied));
    }
    if (file.write(content.data(), content.size())) {
        return {};
    }
    return std::unexpected(make_error_code(UtilsError::IOError));
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

std::optional<long> toLong(std::string_view s) {
    long val;
    std::string_view sub_s = s;
    if (!s.empty() && s[0] == '+') {
        sub_s = s.substr(1);
    }
    
    if (sub_s.empty()) return std::nullopt;

    auto res = std::from_chars(sub_s.data(), sub_s.data() + sub_s.size(), val);
    if (res.ec == std::errc() && res.ptr == sub_s.data() + sub_s.size()) {
        return val;
    }
    return std::nullopt;
}

std::optional<double> toDouble(std::string_view s) {
    double val;
    std::string_view sub_s = s;
    if (!s.empty() && s[0] == '+') {
        sub_s = s.substr(1);
    }
    
    if (sub_s.empty()) return std::nullopt;

    auto res = std::from_chars(sub_s.data(), sub_s.data() + sub_s.size(), val);
    if (res.ec == std::errc() && res.ptr == sub_s.data() + sub_s.size()) {
        return val;
    }
    return std::nullopt;
}

} // namespace Utils
