// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "utils/path.h"

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

Result<bool> pathsEquivalent(const std::filesystem::path& p1, const std::filesystem::path& p2) {
    std::error_code ec;
    bool equivalent = std::filesystem::equivalent(p1, p2, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return equivalent;
}



// Simple utility wrappers for common path operations
Result<std::filesystem::path> getAbsolutePath(const std::filesystem::path& path) {
    std::error_code ec;
    auto absolutePath = std::filesystem::absolute(path, ec);
    if (ec) {
        return std::unexpected(ec);
    }
    return absolutePath;
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
        // If `p` is an absolute path, operator/= replaces the existing `result`.
        result /= p;
    }
    return result;
}

} // namespace utils
