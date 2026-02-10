// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/file.h"
#include <fstream>
#include <vector>
#include <iterator>
#include <random>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace utils {

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
                 return ::std::unexpected(make_error_code(UtilsError::traversalStopped)); // Using 'traversalStopped' as a signal "Stop/Success"
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
        return res; // Propagate all errors including traversalStopped
    }

    return {};
}

Result<void> createDirectories(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    ::std::filesystem::create_directories(path, ec);
    if (ec) {
        if (ec == ::std::make_error_code(::std::errc::permission_denied)) {
            return ::std::unexpected(make_error_code(UtilsError::permissionDenied));
        }
        if (ec == ::std::make_error_code(::std::errc::file_exists)) {
            // This can happen if an intermediate component is a non-directory file
            // Or if the target path already exists as a file.
            // Check if path exists and is a directory (it might have been created by another thread/process)
            ::std::error_code statusEc;
            if (::std::filesystem::exists(path, statusEc) && ::std::filesystem::is_directory(path, statusEc)) {
                return {}; // Directory already exists, consider it a success
            }
            if (!statusEc && ::std::filesystem::exists(path, statusEc) && !::std::filesystem::is_directory(path, statusEc)) {
                return ::std::unexpected(make_error_code(UtilsError::fileAlreadyExists)); // Path exists but is a file
            }
            return ::std::unexpected(make_error_code(UtilsError::invalidArgument)); // More generic for file_exists on intermediate paths
        }
        if (ec == ::std::make_error_code(::std::errc::not_a_directory)) {
            return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
        }
        return ::std::unexpected(ec); // Fallback to generic filesystem error
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
    const bool regularSource = ::std::filesystem::is_regular_file(source, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    if (!regularSource) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    auto parentPath = destination.parent_path();
    if (!parentPath.empty()) {
        auto createDirResult = createDirectories(parentPath);
        if (!createDirResult) {
            return createDirResult; // Propagate error from createDirectories
        }
    }
    ::std::filesystem::copy_file(source, destination, ::std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<void> moveFile(const ::std::filesystem::path& source, const ::std::filesystem::path& destination) {
    auto parentPath = destination.parent_path();
    if (!parentPath.empty()) {
        auto createDirResult = createDirectories(parentPath);
        if (!createDirResult) {
            return createDirResult;
        }
    }

    ::std::error_code ec;
    ::std::filesystem::rename(source, destination, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

Result<uintmax_t> getFileSize(const ::std::filesystem::path& filePath) {
    ::std::error_code ec;
    auto size = ::std::filesystem::file_size(filePath, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return size;
}

Result<bool> exists(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    bool result = ::std::filesystem::exists(path, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return result;
}

Result<bool> isFile(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    bool result = ::std::filesystem::is_regular_file(path, ec);
    if (ec) {
        if (ec == ::std::errc::no_such_file_or_directory) {
            return false;
        }
        return ::std::unexpected(ec);
    }
    return result;
}

Result<bool> isDirectory(const ::std::filesystem::path& path) {
    ::std::error_code ec;
    bool result = ::std::filesystem::is_directory(path, ec);
    if (ec) {
        if (ec == ::std::errc::no_such_file_or_directory) {
            return false;
        }
        return ::std::unexpected(ec);
    }
    return result;
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
    // Changing file ownership (chown) is a platform-specific and complex operation,
    // often requiring elevated privileges. It is not currently supported by this utility
    // to maintain cross-platform compatibility and simplicity.
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
