// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Core.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include "utils/String.h" // Added for utils::trim and utils::contains


// Helper to create a temporary file for testing
std::filesystem::path createTempFile(const std::string& content) {
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / ("test_file_" + std::to_string(rand()) + ".txt");
    std::ofstream ofs(tempPath, std::ios::binary);
    ofs << content;
    ofs.close();
    return tempPath;
}

// Helper to create a temporary directory for testing
std::filesystem::path createTempDir() {
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / ("test_dir_" + std::to_string(rand()));
    std::filesystem::create_directories(tempPath);
    return tempPath;
}

TEST(UtilsTest, ReadFileExisting) {
    std::string content = "Hello, world!\nThis is a test file.";
    auto filePath = createTempFile(content);

    auto result = utils::readTextFile(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), content);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, ReadFileNonExistent) {
    auto result = utils::readTextFile("non_existent_file.txt");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST(UtilsTest, WriteFile) {
    std::string content = "Write test content.";
    std::filesystem::path filePath = std::filesystem::temp_directory_path() / "write_test.txt";

    auto result = utils::writeTextFile(filePath, content);
    ASSERT_TRUE(result.has_value());

    auto readResult = utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, AppendToFile) {
    std::string content1 = "First line.\n";
    std::string content2 = "Second line.";
    auto filePath = createTempFile(content1);

    auto result = utils::appendToFile(filePath, content2);
    ASSERT_TRUE(result.has_value());

    auto readResult = utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content1 + content2);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, ReadLines) {
    std::string content = "line 1\nline 2\nline 3\n";
    auto filePath = createTempFile(content);

    auto result = utils::readLines(filePath);
    ASSERT_TRUE(result.has_value());
    std::vector<std::string> expected = {"line 1", "line 2", "line 3"};
    EXPECT_EQ(result.value(), expected);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, CreateDirectories) {
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "a" / "b" / "c";
    if (std::filesystem::exists(tempPath)) {
        std::filesystem::remove_all(std::filesystem::temp_directory_path() / "a");
    }

    auto result = utils::createDirectories(tempPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(tempPath));
    EXPECT_TRUE(std::filesystem::is_directory(tempPath));

    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "a");
}

TEST(UtilsTest, Remove) {
    auto filePath = createTempFile("to be removed");
    EXPECT_TRUE(std::filesystem::exists(filePath));

    auto result = utils::remove(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(std::filesystem::exists(filePath));

    auto dirPath = createTempDir();
    auto subFilePath = dirPath / "file.txt";
    std::ofstream(subFilePath) << "content";
    
    // Remove non-empty dir without recursive should fail (standard behavior)
    auto result2 = utils::remove(dirPath, false);
    ASSERT_FALSE(result2.has_value());
    
    // Remove with recursive
    auto result3 = utils::remove(dirPath, true);
    ASSERT_TRUE(result3.has_value());
    EXPECT_FALSE(std::filesystem::exists(dirPath));
}

TEST(UtilsTest, ListDirectory) {
    auto dirPath = createTempDir();
    auto file1 = dirPath / "file1.txt";
    auto file2 = dirPath / "file2.txt";
    std::ofstream(file1) << "1";
    std::ofstream(file2) << "2";

    auto result = utils::listDirectory(dirPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2);
    
    bool found1 = false;
    bool found2 = false;
    for (const auto& p : result.value()) {
        if (p.filename() == "file1.txt") found1 = true;
        if (p.filename() == "file2.txt") found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);

    std::filesystem::remove_all(dirPath);
}

TEST(UtilsTest, GetFileSize) {
    std::string content = "1234567890";
    auto filePath = createTempFile(content);
    
    auto sizeResult = utils::getFileSize(filePath);
    ASSERT_TRUE(sizeResult.has_value());
    EXPECT_EQ(sizeResult.value(), content.size());
    std::filesystem::remove(filePath);

    auto nonExistentResult = utils::getFileSize("non_existent_file.txt");
    ASSERT_FALSE(nonExistentResult.has_value());
    EXPECT_EQ(nonExistentResult.error(), std::make_error_code(std::errc::no_such_file_or_directory));
}

TEST(UtilsTest, CopyFile) {
    std::string content = "copy content";
    auto srcPath = createTempFile(content);
    auto destPath = std::filesystem::temp_directory_path() / "dest_file.txt";

    auto result = utils::copyFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(destPath));

    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);

    std::filesystem::remove(srcPath);
    std::filesystem::remove(destPath);
}

TEST(UtilsTest, MoveFile) {
    std::string content = "move content";
    auto srcPath = createTempFile(content);
    auto destPath = std::filesystem::temp_directory_path() / "dest_file_moved.txt";

    auto result = utils::moveFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(std::filesystem::exists(srcPath));
    EXPECT_TRUE(std::filesystem::exists(destPath));

    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);

    std::filesystem::remove(destPath);
}

TEST(UtilsTest, MoveFileCreatesDestinationParentDirectories) {
    std::string content = "move into nested destination";
    auto srcPath = createTempFile(content);
    auto destPath = std::filesystem::temp_directory_path() / "pa_move_parent_test" / "nested" / "dest.txt";

    std::error_code ec;
    std::filesystem::remove_all(destPath.parent_path().parent_path(), ec);

    auto result = utils::moveFile(srcPath, destPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(std::filesystem::exists(srcPath));
    EXPECT_TRUE(std::filesystem::exists(destPath));

    auto readResult = utils::readTextFile(destPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);

    std::filesystem::remove_all(destPath.parent_path().parent_path(), ec);
}



TEST(UtilsTest, Exists) {
    auto filePath = createTempFile("temp");
    EXPECT_TRUE(utils::exists(filePath).value());
    std::filesystem::remove(filePath);
    EXPECT_FALSE(utils::exists(filePath).value());

    auto dirPath = createTempDir();
    EXPECT_TRUE(utils::exists(dirPath).value());
    std::filesystem::remove(dirPath); // Remove directory
    EXPECT_FALSE(utils::exists(dirPath).value());
}

TEST(UtilsTest, IsFile) {
    auto filePath = createTempFile("temp");
    auto res1 = utils::isFile(filePath);
    ASSERT_TRUE(res1.has_value()) << "IsFile failed: " << res1.error().message();
    EXPECT_TRUE(res1.value());
    
    std::filesystem::remove(filePath);
    auto res2 = utils::isFile(filePath);
    ASSERT_TRUE(res2.has_value()) << "IsFile failed: " << res2.error().message();
    EXPECT_FALSE(res2.value());

    auto dirPath = createTempDir();
    auto res3 = utils::isFile(dirPath);
    ASSERT_TRUE(res3.has_value()) << "IsFile failed: " << res3.error().message();
    EXPECT_FALSE(res3.value());
    std::filesystem::remove(dirPath);
}

TEST(UtilsTest, IsDirectory) {
    auto dirPath = createTempDir();
    auto res1 = utils::isDirectory(dirPath);
    ASSERT_TRUE(res1.has_value()) << "IsDirectory failed: " << res1.error().message();
    EXPECT_TRUE(res1.value());
    
    std::filesystem::remove(dirPath);
    auto res2 = utils::isDirectory(dirPath);
    ASSERT_TRUE(res2.has_value()) << "IsDirectory failed: " << res2.error().message();
    EXPECT_FALSE(res2.value());

    auto filePath = createTempFile("temp");
    auto res3 = utils::isDirectory(filePath);
    ASSERT_TRUE(res3.has_value()) << "IsDirectory failed: " << res3.error().message();
    EXPECT_FALSE(res3.value());
    std::filesystem::remove(filePath);
}















































TEST(UtilsTest, GetEnv) {
    // Set an environment variable
    #ifdef _WIN32
    _putenv("TEST_VAR=test_value");
    #else
    setenv("TEST_VAR", "test_value", 1);
    #endif

    auto value = utils::getEnv("TEST_VAR");
    ASSERT_TRUE(value.has_value());
    if (value) { // Redundant check for static analyzer
        EXPECT_EQ(*value, "test_value");
    }

    auto nonExistent = utils::getEnv("NON_EXISTENT_VAR_XYZ_123");
    EXPECT_FALSE(nonExistent.has_value());
}

TEST(UtilsTest, ExecuteCommand) {
    auto resultSuccess = utils::executeCommand("echo 'hello world'");
    ASSERT_TRUE(resultSuccess.has_value());
    EXPECT_EQ(resultSuccess.value().exitCode, 0);
    EXPECT_EQ(utils::trim(resultSuccess.value().stdoutStr), "hello world");
    EXPECT_TRUE(resultSuccess.value().stderrStr.empty());

    // Test command that fails
    auto resultFail = utils::executeCommand("ls non_existent_dir_12345");
    ASSERT_TRUE(resultFail.has_value());
    EXPECT_NE(resultFail.value().exitCode, 0);
    EXPECT_TRUE(resultFail.value().stdoutStr.empty());
    EXPECT_TRUE(utils::contains(resultFail.value().stderrStr, "No such file or directory"));
}
