// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "utils/file.h"

#include <vector>
#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif


namespace utils {

Result<void> traverseDirectory(const std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options) {
    std::error_code ec;
    if (!std::filesystem::exists(dirPath, ec) || !std::filesystem::is_directory(dirPath, ec)) {
         return std::unexpected(make_error_code(UtilsError::fileNotFound));
    }

    auto traverseImpl = [&](auto&& self, const std::filesystem::path& currentDir, int currentDepth) -> Result<void> {
        if (options.maxDepth != -1 && currentDepth > options.maxDepth) {
            return {};
        }

                std::error_code iterEc; // Error code for directory_iterator operations
                std::filesystem::directory_iterator dirIterator(currentDir, iterEc);
        
                if (iterEc) {
                    // Error during directory_iterator construction (e.g., directory not accessible)
                    return std::unexpected(iterEc);
                }
        
                // The default-constructed directory_iterator is the end iterator
                auto endIterator = std::filesystem::directory_iterator();
        
                while (dirIterator != endIterator) {
                    // Check for errors from the *previous* increment operation or constructor
                    if (iterEc) {
                        return std::unexpected(iterEc);
                    }
        
                    const auto& entry = *dirIterator; // Get the current directory entry
        
                    // Check for errors when stat-ing the entry (e.g., permissions, broken symlink)
                    std::error_code entryStatusEc;
                    bool isDir = false;
                    bool isSymlink = false;
        
                    // Determine if it's a directory. Only needed if includeDirectories or recursive is true.
                    if (options.includeDirectories || options.recursive) {
                        isDir = entry.is_directory(entryStatusEc);
                        if (entryStatusEc) {
                            // Error getting directory status. Propagate this error.
                            return std::unexpected(entryStatusEc);
                        }
                    }
        
                    // Determine if it's a symlink. Only needed if !options.followSymlinks.
                    if (!options.followSymlinks) {
                        isSymlink = entry.is_symlink(entryStatusEc);
                        if (entryStatusEc) {
                            // Error getting symlink status. Propagate this error.
                            return std::unexpected(entryStatusEc);
                        }
                    }
        
                    // Now that we have safely determined properties of the entry, call the callback if needed.
                    bool shouldCallback = (isDir && options.includeDirectories) || (!isDir && options.includeFiles);
                    
                    TraversalControl control = TraversalControl::Continue;
                    if (shouldCallback) {
                        control = callback(entry);
                    }
        
                    if (control == TraversalControl::stop) {
                         // The callback requested to stop the traversal.
                         // UtilsError::traversalStopped is used to signal this intentional stop.
                         // Callers should check for this specific error code to differentiate
                         // it from actual filesystem errors. This addresses the ambiguity noted in the audit.
                         return std::unexpected(make_error_code(UtilsError::traversalStopped)); // Represents a graceful stop.
                    }
        
                    // Handle recursion for directories
                    if (isDir && options.recursive && control != TraversalControl::skipDir) {
                        // Skip following symlinks if the option is disabled
                        if (isSymlink && !options.followSymlinks) {
                            // Increment iterator and continue to next entry
                            ++dirIterator;
                            continue;
                        }
                        
                        // Recurse into the subdirectory
                        auto recursiveResult = self(self, entry.path(), currentDepth + 1);
                        if (!recursiveResult) {
                            // Propagate any result (including traversalStopped if it occurred in a deeper call)
                            return recursiveResult;
                        }
                    }
                    
                    // Move to the next directory entry
                    ++dirIterator;
                }
                
                // Check for any errors that occurred during the last increment or after the loop finished
                if (iterEc) {
                    return std::unexpected(iterEc);
                }
                
                return {}; // Directory traversal successful for this level
            };
            
            // Initial call to the recursive lambda
            auto res = traverseImpl(traverseImpl, dirPath, 0);
            // Propagate the final result (could be success, traversalStopped, or another filesystem error)
            return res;
        }

Result<void> createDirectories(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);

    if (!ec) {
        return {}; // Success
    }

    // If create_directories fails, check the specific error.
    // If the path already exists and is a directory, consider it a success.
    // This check is important because `create_directories` might fail with file_exists
    // if an intermediate path component is a file, or if the target path itself is a file.
    std::error_code statusEc;
    if (std::filesystem::exists(path, statusEc)) {
        if (std::filesystem::is_directory(path, statusEc)) {
            // Target path exists and is a directory. Operation successful.
            return {};
        }             // Target path exists but is not a directory (it's a file).
            // Return fileAlreadyExists error.
            return std::unexpected(make_error_code(UtilsError::fileAlreadyExists));
       
    }

    // If the target path does not exist, but `create_directories` failed,
    // the error must be due to an intermediate path component.
    if (ec == std::make_error_code(std::errc::permission_denied)) {
        return std::unexpected(make_error_code(UtilsError::permissionDenied));
    }
    
    if (ec == std::make_error_code(std::errc::not_a_directory)) {
        // This error usually means an intermediate component is a file.
        return std::unexpected(make_error_code(UtilsError::fileAlreadyExists));
    }
    
    // For any other filesystem error during directory creation.
    return std::unexpected(ec);
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
        if (std::filesystem::is_directory(path, ec) && !std::filesystem::is_empty(path, ec)) {
            return std::unexpected(std::make_error_code(std::errc::directory_not_empty));
        }
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
    const bool regularSource = std::filesystem::is_regular_file(source, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    if (!regularSource) {
        return std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    auto parentPath = destination.parent_path();
    if (!parentPath.empty()) {
        auto createDirResult = createDirectories(parentPath);
        if (!createDirResult) {
            return createDirResult; // Propagate error from createDirectories
        }
    }
    std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination) {
    auto parentPath = destination.parent_path();
    if (!parentPath.empty()) {
        auto createDirResult = createDirectories(parentPath);
        if (!createDirResult) {
            return createDirResult;
        }
    }

    std::error_code ec;
    std::filesystem::rename(source, destination, ec);
    
    if (!ec) {
        // rename succeeded
        return {};
    }

    if (ec == std::make_error_code(std::errc::cross_device_link)) {
        // rename failed because of cross-device link, fallback to copy-delete
        auto copyResult = copyFile(source, destination);
        if (!copyResult) {
            return copyResult;
        }

        auto removeResult = remove(source, false);
        if (!removeResult) {
            // This is tricky. The file is copied, but not removed.
            return std::unexpected(make_error_code(UtilsError::ioError));
        }
        return {};
    }

    // rename failed for another reason
    return std::unexpected(ec);
}

Result<uintmax_t> getFileSize(const std::filesystem::path& filePath) {
    std::error_code ec;
    auto size = std::filesystem::file_size(filePath, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return size;
}

Result<bool> exists(const std::filesystem::path& path) {
    std::error_code ec;
    bool result = std::filesystem::exists(path, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return result;
}

Result<bool> isFile(const std::filesystem::path& path) {
    std::error_code ec;
    bool result = std::filesystem::is_regular_file(path, ec);
    if (ec) {
        if (ec == std::errc::no_such_file_or_directory) {
            return false;
        }
        return std::unexpected(ec);
    }
    return result;
}

Result<bool> isDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    bool result = std::filesystem::is_directory(path, ec);
    if (ec) {
        if (ec == std::errc::no_such_file_or_directory) {
            return false;
        }
        return std::unexpected(ec);
    }
    return result;
}

// Note: This function returns POSIX-style permissions. On non-POSIX systems like Windows,
// this may not fully represent the file's permissions, as they use ACLs.
Result<std::filesystem::perms> getPermissions(const std::filesystem::path& path) {
    std::error_code ec;
    auto status = std::filesystem::status(path, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return status.permissions();
}

// Note: This function sets POSIX-style permissions. On non-POSIX systems like Windows,
// this may not fully map to the platform's native permission model (e.g., ACLs).
Result<void> setPermissions(const std::filesystem::path& path, std::filesystem::perms prms) {
    std::error_code ec;
    std::filesystem::permissions(path, prms, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

// Note: This function adds POSIX-style permissions. On non-POSIX systems like Windows,
// this may not fully map to the platform's native permission model (e.g., ACLs).
Result<void> addPermissions(const std::filesystem::path& path, std::filesystem::perms prms) {
    std::error_code ec;
    std::filesystem::permissions(path, prms, std::filesystem::perm_options::add, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

// Note: This function removes POSIX-style permissions. On non-POSIX systems like Windows,
// this may not fully map to the platform's native permission model (e.g., ACLs).
Result<void> removePermissions(const std::filesystem::path& path, std::filesystem::perms prms) {
    std::error_code ec;
    std::filesystem::permissions(path, prms, std::filesystem::perm_options::remove, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

#ifdef __linux__
Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group) {
    uid_t uid = -1;
    gid_t gid = -1;

    if (!owner.empty()) {
        struct passwd *pwd = getpwnam(owner.c_str());
        if (pwd == nullptr) {
            return std::unexpected(make_error_code(UtilsError::invalidArgument)); // "Owner not found"
        }
        uid = pwd->pw_uid;
    }

    if (!group.empty()) {
        struct group *grp = getgrnam(group.c_str());
        if (grp == nullptr) {
            return std::unexpected(make_error_code(UtilsError::invalidArgument)); // "Group not found"
        }
        gid = grp->gr_gid;
    }

    if (::chown(path.c_str(), uid, gid) != 0) {
        return std::unexpected(std::error_code(errno, std::generic_category()));
    }

    return {};
}
#else
Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group) {
    (void)path;
    (void)owner;
    (void)group;
    // Changing file ownership (chown) is a platform-specific and complex operation,
    // often requiring elevated privileges. It is not currently supported by this utility
    // to maintain cross-platform compatibility and simplicity.
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}
#endif


Result<bool> isReadable(const std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult) {
        return std::unexpected(permsResult.error());
    }
    auto p = permsResult.value();
    return (p & (std::filesystem::perms::owner_read | std::filesystem::perms::group_read | std::filesystem::perms::others_read)) != std::filesystem::perms::none;
}

Result<bool> isWritable(const std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult) {
        return std::unexpected(permsResult.error());
    }
    auto p = permsResult.value();
    return (p & (std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write)) != std::filesystem::perms::none;
}

Result<bool> isExecutable(const std::filesystem::path& path) {
    auto permsResult = getPermissions(path);
    if (!permsResult) {
        return std::unexpected(permsResult.error());
    }
    auto p = permsResult.value();
    return (p & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec)) != std::filesystem::perms::none;
}

} // namespace utils
