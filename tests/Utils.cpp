// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

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

    auto result = Utils::readTextFile(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), content);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, ReadFileNonExistent) {
    auto result = Utils::readTextFile("non_existent_file.txt");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), Utils::make_error_code(Utils::UtilsError::FileNotFound));
}

TEST(UtilsTest, WriteFile) {
    std::string content = "Write test content.";
    std::filesystem::path filePath = std::filesystem::temp_directory_path() / "write_test.txt";

    auto result = Utils::writeTextFile(filePath, content);
    ASSERT_TRUE(result.has_value());

    auto readResult = Utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, AppendToFile) {
    std::string content1 = "First line.\n";
    std::string content2 = "Second line.";
    auto filePath = createTempFile(content1);

    auto result = Utils::appendToFile(filePath, content2);
    ASSERT_TRUE(result.has_value());

    auto readResult = Utils::readTextFile(filePath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), content1 + content2);

    std::filesystem::remove(filePath);
}

TEST(UtilsTest, ReadLines) {
    std::string content = "line 1\nline 2\nline 3\n";
    auto filePath = createTempFile(content);

    auto result = Utils::readLines(filePath);
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

    auto result = Utils::createDirectories(tempPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(tempPath));
    EXPECT_TRUE(std::filesystem::is_directory(tempPath));

    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "a");
}

TEST(UtilsTest, Remove) {
    auto filePath = createTempFile("to be removed");
    EXPECT_TRUE(std::filesystem::exists(filePath));

    auto result = Utils::remove(filePath);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(std::filesystem::exists(filePath));

    auto dirPath = createTempDir();
    auto subFilePath = dirPath / "file.txt";
    std::ofstream(subFilePath) << "content";
    
    // Remove non-empty dir without recursive should fail (standard behavior)
    auto result2 = Utils::remove(dirPath, false);
    ASSERT_FALSE(result2.has_value());
    
    // Remove with recursive
    auto result3 = Utils::remove(dirPath, true);
    ASSERT_TRUE(result3.has_value());
    EXPECT_FALSE(std::filesystem::exists(dirPath));
}

TEST(UtilsTest, ListDirectory) {
    auto dirPath = createTempDir();
    auto file1 = dirPath / "file1.txt";
    auto file2 = dirPath / "file2.txt";
    std::ofstream(file1) << "1";
    std::ofstream(file2) << "2";

    auto result = Utils::listDirectory(dirPath);
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

TEST(UtilsTest, Exists) {
    auto filePath = createTempFile("temp");
    EXPECT_TRUE(Utils::exists(filePath));
    std::filesystem::remove(filePath);
    EXPECT_FALSE(Utils::exists(filePath));

    auto dirPath = createTempDir();
    EXPECT_TRUE(Utils::exists(dirPath));
    std::filesystem::remove(dirPath); // Remove directory
    EXPECT_FALSE(Utils::exists(dirPath));
}

TEST(UtilsTest, IsFile) {
    auto filePath = createTempFile("temp");
    EXPECT_TRUE(Utils::isFile(filePath));
    std::filesystem::remove(filePath);
    EXPECT_FALSE(Utils::isFile(filePath));

    auto dirPath = createTempDir();
    EXPECT_FALSE(Utils::isFile(dirPath));
    std::filesystem::remove(dirPath);
}

TEST(UtilsTest, IsDirectory) {
    auto dirPath = createTempDir();
    EXPECT_TRUE(Utils::isDirectory(dirPath));
    std::filesystem::remove(dirPath);
    EXPECT_FALSE(Utils::isDirectory(dirPath));

    auto filePath = createTempFile("temp");
    EXPECT_FALSE(Utils::isDirectory(filePath));
    std::filesystem::remove(filePath);
}

TEST(UtilsTest, Trim) {
    EXPECT_EQ(Utils::trim("  hello  "), "hello");
    EXPECT_EQ(Utils::trim("hello"), "hello");
    EXPECT_EQ(Utils::trim("  hello"), "hello");
    EXPECT_EQ(Utils::trim("hello  "), "hello");
    EXPECT_EQ(Utils::trim(""), "");
    EXPECT_EQ(Utils::trim("   "), "");
    EXPECT_EQ(Utils::trim("\t\n hello \r\v"), "hello");
}

TEST(UtilsTest, StartsWith) {
    EXPECT_TRUE(Utils::startsWith("hello world", "hello"));
    EXPECT_FALSE(Utils::startsWith("hello world", "world"));
    EXPECT_TRUE(Utils::startsWith("hello", "hello"));
    EXPECT_FALSE(Utils::startsWith("hello", "hellos"));
    EXPECT_TRUE(Utils::startsWith("", ""));
    EXPECT_TRUE(Utils::startsWith("hello", ""));
}

TEST(UtilsTest, EndsWith) {
    EXPECT_TRUE(Utils::endsWith("hello world", "world"));
    EXPECT_FALSE(Utils::endsWith("hello world", "hello"));
    EXPECT_TRUE(Utils::endsWith("world", "world"));
    EXPECT_FALSE(Utils::endsWith("world", "helloworld"));
    EXPECT_TRUE(Utils::endsWith("", ""));
    EXPECT_TRUE(Utils::endsWith("hello", ""));
}

TEST(UtilsTest, Contains) {
    EXPECT_TRUE(Utils::contains("hello world", "lo wo"));
    EXPECT_FALSE(Utils::contains("hello world", "foo"));
    EXPECT_TRUE(Utils::contains("hello", "hello"));
    EXPECT_TRUE(Utils::contains("hello", ""));
    EXPECT_FALSE(Utils::contains("", "a"));
}

TEST(UtilsTest, ToLower) {
    EXPECT_EQ(Utils::toLower("HELLO World"), "hello world");
    EXPECT_EQ(Utils::toLower("123!@#"), "123!@#");
    EXPECT_EQ(Utils::toLower(""), "");
}

TEST(UtilsTest, ToUpper) {
    EXPECT_EQ(Utils::toUpper("hello World"), "HELLO WORLD");
    EXPECT_EQ(Utils::toUpper("123!@#"), "123!@#");
    EXPECT_EQ(Utils::toUpper(""), "");
}

TEST(UtilsTest, Replace) {
    EXPECT_EQ(Utils::replace("hello world", "world", "universe"), "hello universe");
    EXPECT_EQ(Utils::replace("banana", "a", "o"), "bonono");
    EXPECT_EQ(Utils::replace("hello", "l", ""), "heo");
    EXPECT_EQ(Utils::replace("hello", "", "x"), "hello");
    EXPECT_EQ(Utils::replace("", "a", "b"), "");
}

TEST(UtilsTest, Join) {
    std::vector<std::string> parts = {"one", "two", "three"};
    EXPECT_EQ(Utils::join(parts, ","), "one,two,three");
    EXPECT_EQ(Utils::join(parts, "---"), "one---two---three");
    EXPECT_EQ(Utils::join({"single"}, ","), "single");
    EXPECT_EQ(Utils::join({}, ","), "");
}

TEST(UtilsTest, SplitCharDelimiter) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(Utils::split("one,two,three", ',', false), expected);

    expected = {"one", "two", "three", ""};
    EXPECT_EQ(Utils::split("one,two,three,", ',', false), expected);

    expected = {"one", "two", "three"};
    EXPECT_EQ(Utils::split("one,two,three,", ',', true), expected);

    expected = {};
    EXPECT_EQ(Utils::split("", ',', false), expected);

    expected = {"a", "b"};
    EXPECT_EQ(Utils::split("a,,b", ',', true), expected);
    
    expected = {"a", "", "b"};
    EXPECT_EQ(Utils::split("a,,b", ',', false), expected);
}

TEST(UtilsTest, SplitStringDelimiter) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(Utils::split("one||two||three", "||", false), expected);

    expected = {"one", "two", "three", ""};
    EXPECT_EQ(Utils::split("one||two||three||", "||", false), expected);

    expected = {"one", "two", "three"};
    EXPECT_EQ(Utils::split("one||two||three||", "||", true), expected);

    expected = {};
    EXPECT_EQ(Utils::split("", "||", false), expected);

    expected = {"a", "b"};
    EXPECT_EQ(Utils::split("a||||b", "||", true), expected);

    expected = {"a", "", "b"};
    EXPECT_EQ(Utils::split("a||||b", "||", false), expected);
}

TEST(UtilsTest, IsInteger) {
    EXPECT_TRUE(Utils::isInteger("123"));
    EXPECT_TRUE(Utils::isInteger("-456"));
    EXPECT_TRUE(Utils::isInteger("+789"));
    EXPECT_FALSE(Utils::isInteger("123a"));
    EXPECT_FALSE(Utils::isInteger("12.3"));
    EXPECT_FALSE(Utils::isInteger(""));
    EXPECT_TRUE(Utils::isInteger("0"));
    EXPECT_FALSE(Utils::isInteger("-"));
    EXPECT_FALSE(Utils::isInteger("+"));
}

TEST(UtilsTest, IsFloatingPoint) {
    EXPECT_TRUE(Utils::isFloatingPoint("123.45"));
    EXPECT_TRUE(Utils::isFloatingPoint("-12.3"));
    EXPECT_TRUE(Utils::isFloatingPoint("+0.5"));
    EXPECT_TRUE(Utils::isFloatingPoint(".5"));
    EXPECT_TRUE(Utils::isFloatingPoint("123."));
    EXPECT_FALSE(Utils::isFloatingPoint("12.3.4"));
    EXPECT_FALSE(Utils::isFloatingPoint("abc"));
    EXPECT_FALSE(Utils::isFloatingPoint(""));
    EXPECT_TRUE(Utils::isFloatingPoint("0.0"));
    EXPECT_FALSE(Utils::isFloatingPoint("."));
    EXPECT_FALSE(Utils::isFloatingPoint("-"));
    EXPECT_FALSE(Utils::isFloatingPoint("+"));
    EXPECT_TRUE(Utils::isFloatingPoint("123"));
    
    // Scientific notation
    EXPECT_TRUE(Utils::isFloatingPoint("1.23e4"));
    EXPECT_TRUE(Utils::isFloatingPoint("1.23E4"));
    EXPECT_TRUE(Utils::isFloatingPoint("1e-5"));
    EXPECT_TRUE(Utils::isFloatingPoint("1.2E+2"));
    EXPECT_FALSE(Utils::isFloatingPoint("1e"));
    EXPECT_FALSE(Utils::isFloatingPoint("e5"));
}

TEST(UtilsTest, ToLong) {
    EXPECT_EQ(Utils::toLong("123").value_or(0), 123L);
    EXPECT_EQ(Utils::toLong("-456").value_or(0), -456L);
    EXPECT_EQ(Utils::toLong("+789").value_or(0), 789L);
    EXPECT_FALSE(Utils::toLong("123a").has_value());
    EXPECT_FALSE(Utils::toLong("12.3").has_value());
    EXPECT_FALSE(Utils::toLong("").has_value());
    EXPECT_TRUE(Utils::toLong("0").has_value());
    EXPECT_FALSE(Utils::toLong("-").has_value());
    EXPECT_FALSE(Utils::toLong("+").has_value());

    // Overflow check
    EXPECT_FALSE(Utils::toLong("9223372036854775808").has_value());
    EXPECT_FALSE(Utils::toLong("-9223372036854775809").has_value());
}

TEST(UtilsTest, ToDouble) {
    EXPECT_DOUBLE_EQ(Utils::toDouble("123.45").value_or(0.0), 123.45);
    EXPECT_DOUBLE_EQ(Utils::toDouble("-12.3").value_or(0.0), -12.3);
    EXPECT_DOUBLE_EQ(Utils::toDouble("+0.5").value_or(0.0), 0.5);
    EXPECT_DOUBLE_EQ(Utils::toDouble(".5").value_or(0.0), 0.5);
    EXPECT_DOUBLE_EQ(Utils::toDouble("123.").value_or(0.0), 123.0);
    EXPECT_FALSE(Utils::toDouble("12.3.4").has_value());
    EXPECT_FALSE(Utils::toDouble("abc").has_value());
    EXPECT_FALSE(Utils::toDouble("").has_value());
    EXPECT_TRUE(Utils::toDouble("0.0").has_value());
    EXPECT_FALSE(Utils::toDouble(".").has_value());
    EXPECT_FALSE(Utils::toDouble("-").has_value());
    EXPECT_FALSE(Utils::toDouble("+").has_value());
    
    // Scientific notation
    EXPECT_DOUBLE_EQ(Utils::toDouble("1.23e4").value_or(0.0), 12300.0);
    EXPECT_DOUBLE_EQ(Utils::toDouble("1e-2").value_or(0.0), 0.01);
}

TEST(UtilsTest, GetEnv) {
    // Set an environment variable
    #ifdef _WIN32
    _putenv("TEST_VAR=test_value");
    #else
    setenv("TEST_VAR", "test_value", 1);
    #endif

    auto value = Utils::getEnv("TEST_VAR");
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "test_value");

    auto nonExistent = Utils::getEnv("NON_EXISTENT_VAR_XYZ_123");
    EXPECT_FALSE(nonExistent.has_value());
}

