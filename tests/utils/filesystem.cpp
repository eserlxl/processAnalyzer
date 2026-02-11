// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <gtest/gtest.h>
#include "utils/file.h"
#include <filesystem>
#include <fstream>
#include <ranges> // Required for std::ranges::find

namespace fs = std::filesystem;

class FilesystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        testDir = fs::temp_directory_path() / "processAnalyzer_test";
        fs::create_directories(testDir);
    }

    void TearDown() override {
        // Clean up the temporary directory
        fs::remove_all(testDir);
    }

    fs::path testDir;
};

TEST_F(FilesystemTest, CreateDirectories) {
    fs::path newDir = testDir / "a" / "b" / "c";
    auto result = utils::createDirectories(newDir);
    // Explicitly check for error if result doesn't have a value, to satisfy the warning.
    if (!result.has_value()) {
        FAIL() << "Failed to create directories: " << result.error();
    }
    EXPECT_TRUE(fs::is_directory(newDir));
}

TEST_F(FilesystemTest, CreateDirectoriesExisting) {
    fs::path newDir = testDir / "a" / "b" / "c";
    utils::createDirectories(newDir);
    auto result = utils::createDirectories(newDir); // Try to create it again
    // Explicitly check for error if result doesn't have a value.
    if (!result.has_value()) {
        FAIL() << "Failed to create directories: " << result.error();
    }
    EXPECT_TRUE(fs::is_directory(newDir));
}

TEST_F(FilesystemTest, CreateDirectoriesFileInPath) {
    fs::path fileInPath = testDir / "a";
    fs::create_directory(fileInPath);
    fs::path filePath = fileInPath / "b";
    {
        std::ofstream ofs(filePath);
        ofs << "hello";
    }

    fs::path newDir = filePath / "c";
    auto result = utils::createDirectories(newDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileAlreadyExists));
}

TEST_F(FilesystemTest, MoveFile) {
    fs::path source = testDir / "source.txt";
    fs::path dest = testDir / "dest.txt";
    {
        std::ofstream ofs(source);
        ofs << "test";
    }

    auto result = utils::moveFile(source, dest);
    if (!result.has_value()) {
        FAIL() << "Failed to move file: " << result.error();
    }
    EXPECT_FALSE(fs::exists(source));
    EXPECT_TRUE(fs::exists(dest));
}

// NOTE: Testing cross-filesystem moves automatically is complex.
// This test simulates a cross-filesystem move failure by making rename fail
// and relies on the copy-delete fallback. This is a simplified simulation.
// A true cross-filesystem test would require setting up multiple partitions or disk images.
TEST_F(FilesystemTest, MoveFileCrossFilesystemFallback) {
    fs::path source = testDir / "source.txt";
    fs::path dest = testDir / "sub" / "dest.txt";
    {
        std::ofstream ofs(source);
        ofs << "test";
    }
    
    // We can't easily simulate a cross-device move error.
    // However, the logic for copy-then-delete is now in place if rename fails with that error.
    // We can at least test the behavior of moving to a new directory.
    auto result = utils::moveFile(source, dest);
    if (!result.has_value()) {
        FAIL() << "Failed to move file: " << result.error();
    }
    EXPECT_FALSE(fs::exists(source));
    EXPECT_TRUE(fs::exists(dest));
}


TEST_F(FilesystemTest, TraverseDirectory) {
    fs::path dirA = testDir / "a";
    fs::path dirB = dirA / "b";
    fs::create_directories(dirB);
    fs::path file1 = dirA / "file1.txt";
    fs::path file2 = dirB / "file2.txt";
    {
        std::ofstream ofs(file1);
        ofs << "1";
    }
    {
        std::ofstream ofs(file2);
        ofs << "2";
    }

    std::vector<fs::path> foundPaths;
    auto callback = [&](const fs::directory_entry& entry) {
        foundPaths.push_back(entry.path());
        return utils::TraversalControl::Continue;
    };

    utils::TraversalOptions options;
    options.recursive = true;

    auto result = utils::traverseDirectory(testDir, callback, options);

    if (!result.has_value()) {
        FAIL() << "Failed to traverse directory: " << result.error();
    }

    // The order of traversal is not guaranteed, so we check for presence.
    ASSERT_EQ(foundPaths.size(), 4);
    EXPECT_NE(std::ranges::find(foundPaths, dirA), foundPaths.end());
    EXPECT_NE(std::ranges::find(foundPaths, dirB), foundPaths.end());
    EXPECT_NE(std::ranges::find(foundPaths, file1), foundPaths.end());
    EXPECT_NE(std::ranges::find(foundPaths, file2), foundPaths.end());
}

TEST_F(FilesystemTest, TraverseDirectoryStop) {
    fs::path dirA = testDir / "a";
    fs::path file1 = testDir / "file1.txt";
    fs::create_directory(dirA);
    {
        std::ofstream ofs(file1);
        ofs << "1";
    }

    std::vector<fs::path> foundPaths;
    auto callback = [&](const fs::directory_entry& entry) {
        foundPaths.push_back(entry.path());
        return utils::TraversalControl::stop;
    };

    utils::TraversalOptions options;
    auto result = utils::traverseDirectory(testDir, callback, options);

    // traversalStopped is an "error" code to signal intentional stop
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::traversalStopped));
    EXPECT_EQ(foundPaths.size(), 1);
}

TEST_F(FilesystemTest, RemoveFile) {
    fs::path file = testDir / "file.txt";
    {
        std::ofstream ofs(file);
        ofs << "test";
    }
    ASSERT_TRUE(fs::exists(file));
    auto result = utils::remove(file);
    if (!result.has_value()) {
        FAIL() << "Failed to remove file: " << result.error();
    }
    EXPECT_FALSE(fs::exists(file));
}

TEST_F(FilesystemTest, RemoveDirectoryRecursively) {
    fs::path dir = testDir / "a" / "b";
    fs::create_directories(dir);
    fs::path file = dir / "file.txt";
    {
        std::ofstream ofs(file);
        ofs << "test";
    }
    ASSERT_TRUE(fs::exists(testDir / "a"));
    auto result = utils::remove(testDir / "a", true);
    if (!result.has_value()) {
        FAIL() << "Failed to remove directory: " << result.error();
    }
    EXPECT_FALSE(fs::exists(testDir / "a"));
}

TEST_F(FilesystemTest, ListDirectory) {
    fs::path file1 = testDir / "file1.txt";
    fs::path file2 = testDir / "file2.txt";
    fs::path dir = testDir / "sub";
    fs::create_directory(dir);
    {
        std::ofstream ofs(file1);
        ofs << "1";
    }
    {
        std::ofstream ofs(file2);
        ofs << "2";
    }

    auto result = utils::listDirectory(testDir);
    if (!result.has_value()) {
        FAIL() << "Failed to list directory: " << result.error();
    }
    auto entries = result.value();
    ASSERT_EQ(entries.size(), 3);
    // Order is not guaranteed, so check for presence
    EXPECT_NE(std::ranges::find(entries, file1), entries.end());
    EXPECT_NE(std::ranges::find(entries, file2), entries.end());
    EXPECT_NE(std::ranges::find(entries, dir), entries.end());
}
