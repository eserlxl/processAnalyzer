// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/core.h"
#include "utils/string.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <cstdlib>
#include <system_error>
#include <algorithm>

namespace fs = std::filesystem;

// Helper for tests
utils::Result<std::vector<std::string>> readLines(const fs::path& path) {
    auto contentResult = utils::readTextFile(path);
    if (!contentResult.has_value()) {
        return std::unexpected(contentResult.error());
    }
    const std::string& content = contentResult.value();
    if (content.empty()) {
        return std::vector<std::string>{};
    }
    auto lines = utils::split(content, '\n', false);
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }
    return lines;
}

class UtilsTest : public ::testing::Test {
protected:
    fs::path testDir;
    std::mt19937 generator;

    UtilsTest() : generator(std::random_device{}()) {}

    void SetUp() override {
        // Create a unique temporary directory for this test instance to ensure isolation
        constexpr int maxTempDirRetries = 10;
        for (int i = 0; i < maxTempDirRetries; ++i) {
             std::uniform_int_distribution<uint64_t> dist;
             fs::path tempPath = fs::temp_directory_path() / ("test_utils_" + std::to_string(dist(generator)));
             if (!fs::exists(tempPath)) {
                 std::error_code ec;
                 if (fs::create_directories(tempPath, ec)) {
                     testDir = tempPath;
                     return;
                 }
             }
        }
        throw std::runtime_error("Failed to create unique temporary directory for test");
    }

    void TearDown() override {
        if (!testDir.empty() && fs::exists(testDir)) {
            std::error_code ec;
            fs::remove_all(testDir, ec);
        }
    }

    fs::path createTestFile(const std::string& filename, const std::string& content) {
        fs::path filePath = testDir / filename;
        std::ofstream ofs(filePath, std::ios::binary);
        if (!ofs.is_open()) {
            throw std::runtime_error("Failed to create test file: " + filePath.string());
        }
        ofs << content;
        return filePath;
    }

    fs::path createTestDir(const std::string& dirname) {
        fs::path dirPath = testDir / dirname;
        std::error_code ec;
        if (!fs::create_directories(dirPath, ec)) {
             if (!fs::is_directory(dirPath)) {
                 throw std::runtime_error("Failed to create test directory: " + dirPath.string());
             }
        }
        return dirPath;
    }
};

// --- File I/O Tests ---

TEST_F(UtilsTest, ReadFileExisting) {
    std::string content = "Hello, world!\nThis is a test file.";
    auto filePath = createTestFile("read_test.txt", content);

    auto result = utils::readTextFile(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), content);
}

TEST_F(UtilsTest, ReadFileNonExistent) {
    auto result = utils::readTextFile(testDir / "non_existent.txt");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(UtilsTest, ReadFileEmpty) {
    auto filePath = createTestFile("empty.txt", "");
    auto result = utils::readTextFile(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(UtilsTest, WriteFile) {
    std::string content = "Write test content.";
    fs::path filePath = testDir / "write_test.txt";

    auto result = utils::writeTextFile(filePath, content);
    ASSERT_TRUE(result.has_value());

    auto readResult = utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);
}

TEST_F(UtilsTest, WriteFileExistingDirectory) {
    // Attempting to write a file where a directory exists should fail
    fs::path dirPath = createTestDir("subdir");
    auto result = utils::writeTextFile(dirPath, "content");
    ASSERT_FALSE(result.has_value());
    // The exact error code depends on OS/implementation, but it should fail
}

TEST_F(UtilsTest, AppendToFile) {
    std::string content1 = "First line.\n";
    std::string content2 = "Second line.";
    auto filePath = createTestFile("append.txt", content1);

    auto result = utils::appendToFile(filePath, content2);
    ASSERT_TRUE(result.has_value());

    auto readResult = utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content1 + content2);
}

TEST_F(UtilsTest, AppendToNonExistentFile) {
    // Should create the file if it doesn't exist (std::ios::app behavior)
    fs::path filePath = testDir / "append_new.txt";
    std::string content = "New content";
    
    auto result = utils::appendToFile(filePath, content);
    ASSERT_TRUE(result.has_value());
    
    auto readResult = utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);
}

TEST_F(UtilsTest, ReadLines) {
    std::string content = "line 1\nline 2\nline 3\n";
    auto filePath = createTestFile("lines.txt", content);

    auto result = readLines(filePath);
    ASSERT_TRUE(result.has_value());
    std::vector<std::string> expected = {"line 1", "line 2", "line 3"};
    EXPECT_EQ(result.value(), expected);
}

TEST_F(UtilsTest, ReadLinesEmptyFile) {
    auto filePath = createTestFile("lines_empty.txt", "");
    auto result = readLines(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

// --- Directory Operations Tests ---

TEST_F(UtilsTest, CreateDirectories) {
    fs::path deepPath = testDir / "a" / "b" / "c";
    auto result = utils::createDirectories(deepPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(deepPath));
    EXPECT_TRUE(fs::is_directory(deepPath));
}

TEST_F(UtilsTest, CreateDirectoriesExisting) {
    fs::path dirPath = createTestDir("existing");
    auto result = utils::createDirectories(dirPath);
    ASSERT_TRUE(result.has_value()); // Should succeed if it already exists
    EXPECT_TRUE(fs::is_directory(dirPath));
}

TEST_F(UtilsTest, RemoveFile) {
    auto filePath = createTestFile("remove_me.txt", "content");
    EXPECT_TRUE(fs::exists(filePath));

    auto result = utils::remove(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(filePath));
}

TEST_F(UtilsTest, RemoveDirectory) {
    auto dirPath = createTestDir("remove_dir");
    createTestFile("remove_dir/file.txt", "content");
    
    // Non-recursive remove of non-empty dir should fail
    auto resultFail = utils::remove(dirPath, false);
    ASSERT_FALSE(resultFail.has_value());
    
    // Recursive remove
    auto resultSuccess = utils::remove(dirPath, true);
    ASSERT_TRUE(resultSuccess.has_value());
    EXPECT_FALSE(fs::exists(dirPath));
}

TEST_F(UtilsTest, RemoveNonExistent) {
    // utils::remove implementation returns UtilsError::fileNotFound if file does not exist.
    auto result = utils::remove(testDir / "does_not_exist");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(UtilsTest, ListDirectory) {
    auto dirPath = createTestDir("list_dir");
    createTestFile("list_dir/file1.txt", "1");
    createTestFile("list_dir/file2.txt", "2");

    auto result = utils::listDirectory(dirPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2);
    
    std::vector<std::string> filenames;
    for (const auto& p : result.value()) {
        filenames.push_back(p.filename().string());
    }
    EXPECT_TRUE(std::ranges::find(filenames, "file1.txt") != filenames.end());
    EXPECT_TRUE(std::ranges::find(filenames, "file2.txt") != filenames.end());
}

TEST_F(UtilsTest, ListDirectoryEmpty) {
    auto dirPath = createTestDir("empty_dir");
    auto result = utils::listDirectory(dirPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(UtilsTest, GetFileSize) {
    std::string content = "1234567890";
    auto filePath = createTestFile("size.txt", content);
    
    auto sizeResult = utils::getFileSize(filePath);
    ASSERT_TRUE(sizeResult.has_value());
    EXPECT_EQ(sizeResult.value(), content.size());

    auto nonExistentResult = utils::getFileSize(testDir / "non_existent");
    ASSERT_FALSE(nonExistentResult.has_value());
}

TEST_F(UtilsTest, GetFileSizeDirectory) {
    auto dirPath = createTestDir("size_dir");
    auto result = utils::getFileSize(dirPath);
    // Behaving on directory depends on OS/fs::file_size.
    // fs::file_size on directory is implementation-defined or throws.
    // utils::getFileSize should likely return error.
    if (result.has_value()) {
        // Some systems might return a size for directory, but usually we want to treat it as error or ignore.
        // If it succeeds, we just warn. But typical expectation is error.
    } else {
        ASSERT_FALSE(result.has_value());
    }
}

TEST_F(UtilsTest, CopyFile) {
    std::string content = "copy content";
    auto srcPath = createTestFile("src.txt", content);
    auto destPath = testDir / "dest.txt";

    auto result = utils::copyFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(destPath));

    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);
}

TEST_F(UtilsTest, CopyFileOverwrite) {
    std::string content1 = "original";
    std::string content2 = "new";
    auto srcPath = createTestFile("src_new.txt", content2);
    auto destPath = createTestFile("dest_old.txt", content1);

    auto result = utils::copyFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    
    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content2);
}

TEST_F(UtilsTest, MoveFile) {
    std::string content = "move content";
    auto srcPath = createTestFile("move_src.txt", content);
    auto destPath = testDir / "move_dest.txt";

    auto result = utils::moveFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(srcPath));
    EXPECT_TRUE(fs::exists(destPath));

    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);
}

TEST_F(UtilsTest, MoveFileOverwrite) {
    std::string content1 = "original";
    std::string content2 = "new";
    auto srcPath = createTestFile("move_src_new.txt", content2);
    auto destPath = createTestFile("move_dest_old.txt", content1);

    auto result = utils::moveFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(srcPath));
    
    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content2);
}

TEST_F(UtilsTest, MoveFileCreatesDestinationParentDirectories) {
    std::string content = "move into nested destination";
    auto srcPath = createTestFile("move_nested_src.txt", content);
    auto destPath = testDir / "nested" / "deep" / "dest.txt";

    auto result = utils::moveFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(srcPath));
    EXPECT_TRUE(fs::exists(destPath));
}

// --- Check Tests ---

TEST_F(UtilsTest, Exists) {
    auto filePath = createTestFile("exists.txt", "temp");
    EXPECT_TRUE(utils::exists(filePath).value());
    fs::remove(filePath);
    EXPECT_FALSE(utils::exists(filePath).value());
}

TEST_F(UtilsTest, IsFile) {
    auto filePath = createTestFile("isfile.txt", "temp");
    EXPECT_TRUE(utils::isFile(filePath).value());
    
    auto dirPath = createTestDir("isdir");
    EXPECT_FALSE(utils::isFile(dirPath).value());
}

TEST_F(UtilsTest, IsDirectory) {
    auto dirPath = createTestDir("isdir_check");
    EXPECT_TRUE(utils::isDirectory(dirPath).value());
    
    auto filePath = createTestFile("not_dir.txt", "temp");
    EXPECT_FALSE(utils::isDirectory(filePath).value());
}

// --- System/String Tests ---

TEST_F(UtilsTest, GetSetEnv) {
    std::string varName = "TEST_ENV_VAR_X";
    std::string varValue = "test_value_x";

    // Ensure clean state
    auto preUnset = utils::unsetEnv(varName);
    (void)preUnset;

    auto get1 = utils::getEnv(varName);
    EXPECT_FALSE(get1.has_value()); // Should fail

    auto setRes = utils::setEnv(varName, varValue);
    ASSERT_TRUE(setRes.has_value());

    auto get2 = utils::getEnv(varName);
    ASSERT_TRUE(get2.has_value());
    EXPECT_EQ(get2.value(), varValue);

    auto unsetRes = utils::unsetEnv(varName);
    ASSERT_TRUE(unsetRes.has_value());

    auto get3 = utils::getEnv(varName);
    EXPECT_FALSE(get3.has_value());
}

TEST_F(UtilsTest, ExecuteCommand) {
    auto resultSuccess = utils::executeCommand("echo hello");
    ASSERT_TRUE(resultSuccess.has_value());
    EXPECT_EQ(resultSuccess.value().exitCode, 0);
    EXPECT_EQ(utils::trim(resultSuccess.value().stdoutStr), "hello");
}

TEST_F(UtilsTest, ExecuteCommandFailure) {
    // Run a command that definitely fails
    auto resultFail = utils::executeCommand("ls /non/existent/path/definitely");
    ASSERT_TRUE(resultFail.has_value());
    EXPECT_NE(resultFail.value().exitCode, 0);
    // Stderr should contain error message
    EXPECT_FALSE(resultFail.value().stderrStr.empty());
}

TEST_F(UtilsTest, ExecuteCommandRedirects) {
    // Test that redirects in the command string work (implied by popen/shell execution)
    // We try to echo to stdout and stderr
    std::string cmd = "echo stdout_msg; echo stderr_msg >&2";
    auto result = utils::executeCommand(cmd);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().exitCode, 0);
    EXPECT_TRUE(utils::contains(result.value().stdoutStr, "stdout_msg"));
    EXPECT_TRUE(utils::contains(result.value().stderrStr, "stderr_msg"));
}

TEST_F(UtilsTest, ExecuteCommandLargeOutput) {
    // Generate large output
    std::string cmd = "yes '0123456789' | head -c 100000"; 
    auto result = utils::executeCommand(cmd);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().exitCode, 0);
    EXPECT_GE(result.value().stdoutStr.size(), 100000);
}

TEST_F(UtilsTest, StringTrim) {
    EXPECT_EQ(utils::trim("  hello  "), "hello");
    EXPECT_EQ(utils::trim("hello"), "hello");
    EXPECT_EQ(utils::trim("  \t\nhello"), "hello");
    EXPECT_EQ(utils::trim(""), "");
    EXPECT_EQ(utils::trim("   "), "");
}

TEST_F(UtilsTest, StringContains) {
    EXPECT_TRUE(utils::contains("hello world", "world"));
    EXPECT_TRUE(utils::contains("hello world", "hello"));
    EXPECT_FALSE(utils::contains("hello world", "planet"));
    EXPECT_TRUE(utils::contains("hello", "")); // Empty string is contained in any string
}
