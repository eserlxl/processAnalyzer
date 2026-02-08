// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/File.h"
#include <fstream>
#include <vector>
#include <iterator>

namespace utils {

Result<std::string> readTextFile(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::fileNotFound));
    }
    
    std::ifstream file(path); 
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::permissionDenied));
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    if (file.bad()) {
        return std::unexpected(make_error_code(UtilsError::ioError));
    }
    
    return content;
}

Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    if (file.write(content.data(), static_cast<std::streamsize>(content.size()))) {
        return {};
    }
    return std::unexpected(make_error_code(UtilsError::ioError));
}

Result<void> appendToFile(const std::filesystem::path& path, std::string_view content) {
    std::ofstream file(path, std::ios::out | std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    if (file.write(content.data(), static_cast<std::streamsize>(content.size()))) {
        return {};
    }
    return std::unexpected(make_error_code(UtilsError::ioError));
}

Result<std::vector<std::string>> readLines(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::fileNotFound));
    }
    if (!std::filesystem::is_regular_file(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::ioError));
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(make_error_code(UtilsError::permissionDenied));
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(std::move(line));
    }

    if (file.bad()) {
        return std::unexpected(make_error_code(UtilsError::ioError));
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
        return std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    if (recursive) {
        std::filesystem::remove_all(path, ec);
    } else {
        std::filesystem::remove(path, ec);
    }

    if (ec) {
        if (ec == std::make_error_code(std::errc::permission_denied)) {
            return std::unexpected(make_error_code(UtilsError::permissionDenied));
        }
        return std::unexpected(ec);
    }
    return {};
}

Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::fileNotFound));
    }
    if (!std::filesystem::is_directory(path, ec)) {
        if (ec) return std::unexpected(ec);
        return std::unexpected(make_error_code(UtilsError::ioError));
    }

    std::vector<std::filesystem::path> entries;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (ec) {
            return std::unexpected(ec);
        }
        entries.push_back(entry.path());
    }
    if (ec) {
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

Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination) {
    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

bool moveFileDeprecated(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& ec) {
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

std::optional<uintmax_t> getFileSizeDeprecated(const std::filesystem::path& filePath, std::error_code& ec) {
    if (std::filesystem::exists(filePath, ec) && !ec && std::filesystem::is_regular_file(filePath, ec) && !ec) {
        return std::filesystem::file_size(filePath, ec);
    }
    if (!ec) {
        ec = make_error_code(UtilsError::fileNotFound);
    }
    return std::nullopt;
}

bool traverseDirectoryDeprecated(const std::filesystem::path& dirPath, const std::function<void(const std::filesystem::path&)>& callback, bool recursive) {
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

Result<std::filesystem::perms> getPermissions(const std::filesystem::path& path) {
    std::error_code ec;
    auto status = std::filesystem::status(path, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return status.permissions();
}

Result<void> setPermissions(const std::filesystem::path& path, std::filesystem::perms prms) {
    std::error_code ec;
    std::filesystem::permissions(path, prms, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

Result<void> addPermissions(const std::filesystem::path& path, std::filesystem::perms prms) {
    std::error_code ec;
    std::filesystem::permissions(path, prms, std::filesystem::perm_options::add, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

Result<void> removePermissions(const std::filesystem::path& path, std::filesystem::perms prms) {
    std::error_code ec;
    std::filesystem::permissions(path, prms, std::filesystem::perm_options::remove, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group) {
    (void)path;
    (void)owner;
    (void)group;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

bool isReadable(const std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult.has_value()) return false;
    auto p = permsResult.value();
    return (p & (std::filesystem::perms::owner_read | std::filesystem::perms::group_read | std::filesystem::perms::others_read)) != std::filesystem::perms::none;
}

bool isWritable(const std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult.has_value()) return false;
    auto p = permsResult.value();
    return (p & (std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write)) != std::filesystem::perms::none;
}

bool isExecutable(const std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult.has_value()) return false;
    auto p = permsResult.value();
    return (p & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec)) != std::filesystem::perms::none;
}

} // namespace utils
