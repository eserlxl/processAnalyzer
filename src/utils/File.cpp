// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/File.h"
#include <fstream>
#include <vector>
#include <iterator>
#include <random>
#include <sstream>
#include <iomanip>

namespace utils {

// Helper to generate random string for temp files
namespace {
    constexpr size_t kTempFileSuffixLen = 6;
    constexpr size_t kTempCreationRetries = 10;
    constexpr size_t kRandomNameLen = 16;
}

static ::std::string generateRandomString(size_t length) {
    static constexpr ::std::string_view charset =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    static ::std::mt19937 rg{::std::random_device{}()};
    static ::std::uniform_int_distribution<::std::string::size_type> pick(0, charset.size() - 1);

    ::std::string s;
    s.reserve(length);
    for (size_t i = 0; i < length; ++i)
        s += charset[pick(rg)];
    return s;
}

Result<::std::vector<::std::byte>> readBinaryFile(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    if (!::std::filesystem::exists(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    ::std::ifstream file(path, ::std::ios::binary | ::std::ios::ate);
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }

    auto fileSize = file.tellg();
    if (fileSize == -1) {
        return ::std::unexpected(make_error_code(UtilsError::ioError));
    }

    ::std::vector<::std::byte> buffer(static_cast<size_t>(fileSize));
    file.seekg(0, ::std::ios::beg);

    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return ::std::unexpected(make_error_code(UtilsError::ioError));
    }

    return buffer;
}

Result<void> writeBinaryFile(const ::std::filesystem::path& path, ::std::span<const ::std::byte> content) {
    ::std::ofstream file(path, ::std::ios::out | ::std::ios::trunc | ::std::ios::binary);
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    if (file.write(reinterpret_cast<const char*>(content.data()), static_cast<::std::streamsize>(content.size()))) {
        return {};
    }
    return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<void> writeTextFileAtomic(const ::std::filesystem::path& path, ::std::string_view content) {
    // Write to a temp file in the same directory then rename
    auto parent = path.parent_path();
    if (parent.empty()) parent = ".";
    
    ::std::error_code ec;
    if (!::std::filesystem::exists(parent, ec)) {
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    auto tempPath = parent / (path.filename().string() + "." + generateRandomString(kTempFileSuffixLen) + ".tmp");
    
    auto writeResult = writeTextFile(tempPath, content);
    if (!writeResult) {
        ::std::filesystem::remove(tempPath, ec); // Try cleanup
        return writeResult;
    }

    ::std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        ::std::filesystem::remove(tempPath, ec); // Try cleanup
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> writeBinaryFileAtomic(const ::std::filesystem::path& path, ::std::span<const ::std::byte> content) {
     auto parent = path.parent_path();
    if (parent.empty()) parent = ".";
    
    ::std::error_code ec;
    if (!::std::filesystem::exists(parent, ec)) {
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    auto tempPath = parent / (path.filename().string() + "." + generateRandomString(kTempFileSuffixLen) + ".tmp");
    
    auto writeResult = writeBinaryFile(tempPath, content);
    if (!writeResult) {
        ::std::filesystem::remove(tempPath, ec); 
        return writeResult;
    }

    ::std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        ::std::filesystem::remove(tempPath, ec);
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> appendToBinaryFile(const ::std::filesystem::path& path, ::std::span<const ::std::byte> content) {
    ::std::ofstream file(path, ::std::ios::out | ::std::ios::app | ::std::ios::binary);
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    if (file.write(reinterpret_cast<const char*>(content.data()), static_cast<::std::streamsize>(content.size()))) {
        return {};
    }
    return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<::std::filesystem::path> createTemporaryFile(::std::string_view prefix, ::std::string_view suffix) {
    auto tempDir = ::std::filesystem::temp_directory_path();
    ::std::filesystem::path tempPath;
    ::std::error_code ec;
    
    // Try a few times to generate a unique name
    for (size_t i = 0; i < kTempCreationRetries; ++i) {
        ::std::string name = ::std::string(prefix) + generateRandomString(kRandomNameLen) + ::std::string(suffix);
        tempPath = tempDir / name;
        if (!::std::filesystem::exists(tempPath, ec)) {
            // Create empty file
            ::std::ofstream file(tempPath);
            if (file.is_open()) {
                return tempPath;
            }
        }
    }
    return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<::std::filesystem::path> createTemporaryDirectory(::std::string_view prefix) {
    auto tempDir = ::std::filesystem::temp_directory_path();
    ::std::filesystem::path path;
    ::std::error_code ec;

    for (size_t i = 0; i < kTempCreationRetries; ++i) {
        ::std::string name = ::std::string(prefix) + generateRandomString(kRandomNameLen);
        path = tempDir / name;
        if (::std::filesystem::create_directory(path, ec)) {
            return path;
        }
    }
    return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<void> traverseDirectory(const ::std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options) {
    ::std::error_code ec;
    if (!::std::filesystem::exists(dirPath, ec) || !::std::filesystem::is_directory(dirPath, ec)) {
         return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    auto traverseImpl = [&](auto&& self, const ::std::filesystem::path& currentDir, int currentDepth) -> Result<void> {
        if (options.maxDepth != -1 && currentDepth > options.maxDepth) {
            return {};
        }

        ::std::error_code iterEc;
        auto iter = ::std::filesystem::directory_iterator(currentDir, iterEc);
        if (iterEc) return ::std::unexpected(iterEc);

        for (const auto& entry : iter) {
            bool isDir = entry.is_directory(iterEc);
            if (iterEc) continue; // Skip entries we can't stat

            bool shouldCallback = (isDir && options.includeDirectories) || (!isDir && options.includeFiles);
            
            TraversalControl control = TraversalControl::Continue;
            if (shouldCallback) {
                control = callback(entry);
            }

            if (control == TraversalControl::stop) {
                 return ::std::unexpected(make_error_code(UtilsError::none)); // Using 'none' as a signal "Stop/Success"
            }

            if (isDir && options.recursive && control != TraversalControl::skipDir) {
                if (entry.is_symlink() && !options.followSymlinks) {
                    continue;
                }
                
                auto res = self(self, entry.path(), currentDepth + 1);
                if (!res) {
                    if (res.error() == make_error_code(UtilsError::none)) {
                        return res; // Propagate stop
                    }
                    return res; // Propagate actual error
                }
            }
        }
        return {};
    };
    
    auto res = traverseImpl(traverseImpl, dirPath, 0);
    if (!res) {
        if (res.error() == make_error_code(UtilsError::none)) {
            return {}; // Stopped, treat as success
        }
        return res; // Real error
    }

    return {};
}

Result<::std::string> readTextFile(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    if (!::std::filesystem::exists(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }
    
    ::std::ifstream file(path); 
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }

    ::std::string content((::std::istreambuf_iterator<char>(file)),
                        ::std::istreambuf_iterator<char>());

    if (file.bad()) {
        return ::std::unexpected(make_error_code(UtilsError::ioError));
    }
    
    return content;
}

Result<void> writeTextFile(const ::std::filesystem::path& path, ::std::string_view content) {
    ::std::ofstream file(path, ::std::ios::out | ::std::ios::trunc | ::std::ios::binary);
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    if (file.write(content.data(), static_cast<::std::streamsize>(content.size()))) {
        return {};
    }
    return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<void> appendToFile(const ::std::filesystem::path& path, ::std::string_view content) {
    ::std::ofstream file(path, ::std::ios::out | ::std::ios::app | ::std::ios::binary);
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    if (file.write(content.data(), static_cast<::std::streamsize>(content.size()))) {
        return {};
    }
    return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<::std::vector<::std::string>> readLines(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    if (!::std::filesystem::exists(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }
    if (!::std::filesystem::is_regular_file(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::ioError));
    }

    ::std::ifstream file(path);
    if (!file.is_open()) {
        return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
    }

    ::std::vector<::std::string> lines;
    ::std::string line;
    while (::std::getline(file, line)) {
        lines.push_back(::std::move(line));
    }

    if (file.bad()) {
        return ::std::unexpected(make_error_code(UtilsError::ioError));
    }
    return lines;
}

Result<void> createDirectories(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    ::std::filesystem::create_directories(path, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> remove(const ::std::filesystem::path& path, bool recursive) {
    ::std::error_code ec;
    if (!::std::filesystem::exists(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    if (recursive) {
        ::std::filesystem::remove_all(path, ec);
    } else {
        ::std::filesystem::remove(path, ec);
    }

    if (ec) {
        if (ec == ::std::make_error_code(::std::errc::permission_denied)) {
            return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
        }
        return ::std::unexpected(ec);
    }
    return {};
}

Result<::std::vector<::std::filesystem::path>> listDirectory(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    if (!::std::filesystem::exists(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
    }
    if (!::std::filesystem::is_directory(path, ec)) {
        if (ec) return ::std::unexpected(ec);
        return ::std::unexpected(make_error_code(UtilsError::ioError));
    }

    ::std::vector<::std::filesystem::path> entries;
    for (const auto& entry : ::std::filesystem::directory_iterator(path, ec)) {
        if (ec) {
            return ::std::unexpected(ec);
        }
        entries.push_back(entry.path());
    }
    if (ec) {
        return ::std::unexpected(ec);
    }
    return entries;
}

Result<void> copyFile(const ::std::filesystem::path& source, const ::std::filesystem::path& destination) {
    ::std::error_code ec;
    ::std::filesystem::copy(source, destination, ::std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> moveFile(const ::std::filesystem::path& source, const ::std::filesystem::path& destination) {
    ::std::error_code ec;
    ::std::filesystem::rename(source, destination, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

bool moveFileDeprecated(const ::std::filesystem::path& source, const ::std::filesystem::path& destination, ::std::error_code& ec) {
    ::std::filesystem::rename(source, destination, ec);
    return !ec;
}

Result<uintmax_t> getFileSize(const ::std::filesystem::path& filePath) {
    ::std::error_code ec;
    auto size = ::std::filesystem::file_size(filePath, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return size;
}

::std::optional<uintmax_t> getFileSizeDeprecated(const ::std::filesystem::path& filePath, ::std::error_code& ec) {
    if (::std::filesystem::exists(filePath, ec) && !ec && ::std::filesystem::is_regular_file(filePath, ec) && !ec) {
        return ::std::filesystem::file_size(filePath, ec);
    }
    if (!ec) {
        ec = make_error_code(UtilsError::fileNotFound);
    }
    return ::std::nullopt;
}

bool traverseDirectoryDeprecated(const ::std::filesystem::path& dirPath, const ::std::function<void(const ::std::filesystem::path&)>& callback, bool recursive) {
    ::std::error_code ec;
    if (!::std::filesystem::is_directory(dirPath, ec)) {
        return false;
    }

    if (recursive) {
        for (const auto& entry : ::std::filesystem::recursive_directory_iterator(dirPath, ec)) {
            if (ec) return false;
            callback(entry.path());
        }
    } else {
        for (const auto& entry : ::std::filesystem::directory_iterator(dirPath, ec)) {
            if (ec) return false;
            callback(entry.path());
        }
    }
    return !ec;
}

bool exists(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    return ::std::filesystem::exists(path, ec);
}

bool isFile(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    return ::std::filesystem::is_regular_file(path, ec);
}

bool isDirectory(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    return ::std::filesystem::is_directory(path, ec);
}

Result<::std::filesystem::perms> getPermissions(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    auto status = ::std::filesystem::status(path, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return status.permissions();
}

Result<void> setPermissions(const ::std::filesystem::path& path, ::std::filesystem::perms prms) {
    ::std::error_code ec;
    ::std::filesystem::permissions(path, prms, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> addPermissions(const ::std::filesystem::path& path, ::std::filesystem::perms prms) {
    ::std::error_code ec;
    ::std::filesystem::permissions(path, prms, ::std::filesystem::perm_options::add, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> removePermissions(const ::std::filesystem::path& path, ::std::filesystem::perms prms) {
    ::std::error_code ec;
    ::std::filesystem::permissions(path, prms, ::std::filesystem::perm_options::remove, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> chown(const ::std::filesystem::path& path, const ::std::string& owner, const ::std::string& group) {
    (void)path;
    (void)owner;
    (void)group;
    return ::std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

bool isReadable(const ::std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult.has_value()) return false;
    auto p = permsResult.value();
    return (p & (::std::filesystem::perms::owner_read | ::std::filesystem::perms::group_read | ::std::filesystem::perms::others_read)) != ::std::filesystem::perms::none;
}

bool isWritable(const ::std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult.has_value()) return false;
    auto p = permsResult.value();
    return (p & (::std::filesystem::perms::owner_write | ::std::filesystem::perms::group_write | ::std::filesystem::perms::others_write)) != ::std::filesystem::perms::none;
}

bool isExecutable(const ::std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult.has_value()) return false;
    auto p = permsResult.value();
    return (p & (::std::filesystem::perms::owner_exec | ::std::filesystem::perms::group_exec | ::std::filesystem::perms::others_exec)) != ::std::filesystem::perms::none;
}

} // namespace utils
