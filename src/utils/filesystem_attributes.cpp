// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/filesystem_attributes.h"

#include <system_error>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif

namespace utils {

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

Result<bool> isSymlink(const std::filesystem::path& path) {
    std::error_code ec;
    bool result = std::filesystem::is_symlink(path, ec);
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
