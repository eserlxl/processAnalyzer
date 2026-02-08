// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/Path.h"

namespace utils {

Result<std::filesystem::path> canonicalPath(const std::filesystem::path& path) {
    std::error_code ec;
    auto canonical = std::filesystem::canonical(path, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return canonical;
}

Result<std::filesystem::path> makeRelative(const std::filesystem::path& path, const std::filesystem::path& base) {
    std::error_code ec;
    auto relativePath = std::filesystem::relative(path, base, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return relativePath;
}

bool pathsEquivalent(const std::filesystem::path& p1, const std::filesystem::path& p2) {
    std::error_code ec1;
    bool equivalent = std::filesystem::equivalent(p1, p2, ec1);
    if (ec1) {
        return false;
    }
    return equivalent;
}

Result<void> createSymlink(const std::filesystem::path& target, const std::filesystem::path& link) {
    std::error_code ec;
    std::filesystem::create_symlink(target, link, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return {};
}

Result<std::filesystem::path> readSymlink(const std::filesystem::path& link) {
    std::error_code ec;
    auto result = std::filesystem::read_symlink(link, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return result;
}

bool isSymlink(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_symlink(path, ec);
}

// Implementations that might have been missing or inline
std::filesystem::path getAbsolutePath(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::absolute(path, ec);
}

std::string getFileName(const std::filesystem::path& path) {
    return path.filename().string();
}

std::string getFileNameWithoutExtension(const std::filesystem::path& path) {
    return path.stem().string();
}

std::string getFileExtension(const std::filesystem::path& path) {
    return path.extension().string();
}

std::filesystem::path getParentPath(const std::filesystem::path& path) {
    return path.parent_path();
}

std::filesystem::path joinPaths(const std::vector<std::filesystem::path>& paths) {
    std::filesystem::path result;
    for (const auto& p : paths) {
        result /= p;
    }
    return result;
}

} // namespace utils
