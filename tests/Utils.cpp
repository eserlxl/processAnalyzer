// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

// Helper to create a temporary file for testing
std::string createTempFile(const std::string& content) {
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "test_file.txt";
    std::ofstream ofs(tempPath);
    ofs << content;
    ofs.close();
    return tempPath.string();
}

// Helper to create a temporary directory for testing
std::string createTempDir() {
    std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "test_dir";
    std::filesystem::create_directory(tempPath);
    return tempPath.string();
}

TEST(UtilsTest, ReadFileExisting) {
    std::string content = "Hello, world!\nThis is a test file.";
    std::string filePath = createTempFile(content);

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

TEST(UtilsTest, Exists) {
    std::string filePath = createTempFile("temp");
    EXPECT_TRUE(Utils::exists(filePath));
    std::filesystem::remove(filePath);
    EXPECT_FALSE(Utils::exists(filePath));

    std::string dirPath = createTempDir();
    EXPECT_TRUE(Utils::exists(dirPath));
    std::filesystem::remove(dirPath); // Remove directory
    EXPECT_FALSE(Utils::exists(dirPath));
}

TEST(UtilsTest, IsFile) {
    std::string filePath = createTempFile("temp");
    EXPECT_TRUE(Utils::isFile(filePath));
    std::filesystem::remove(filePath);
    EXPECT_FALSE(Utils::isFile(filePath));

    std::string dirPath = createTempDir();
    EXPECT_FALSE(Utils::isFile(dirPath));
    std::filesystem::remove(dirPath);
}

TEST(UtilsTest, IsDirectory) {
    std::string dirPath = createTempDir();
    EXPECT_TRUE(Utils::isDirectory(dirPath));
    std::filesystem::remove(dirPath);
    EXPECT_FALSE(Utils::isDirectory(dirPath));

    std::string filePath = createTempFile("temp");
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
    EXPECT_TRUE(Utils::startsWith("hello", "")); // Fix: empty string is a prefix
}

TEST(UtilsTest, EndsWith) {
    EXPECT_TRUE(Utils::endsWith("hello world", "world"));
    EXPECT_FALSE(Utils::endsWith("hello world", "hello"));
    EXPECT_TRUE(Utils::endsWith("world", "world"));
    EXPECT_FALSE(Utils::endsWith("world", "helloworld"));
    EXPECT_TRUE(Utils::endsWith("", ""));
    EXPECT_TRUE(Utils::endsWith("hello", "")); // Fix: empty string is a suffix
}

TEST(UtilsTest, Contains) {
    EXPECT_TRUE(Utils::contains("hello world", "lo wo"));
    EXPECT_FALSE(Utils::contains("hello world", "foo"));
    EXPECT_TRUE(Utils::contains("hello", "hello"));
    EXPECT_TRUE(Utils::contains("hello", "")); // Empty string is always contained
    EXPECT_FALSE(Utils::contains("", "a"));
}

TEST(UtilsTest, SplitCharDelimiter) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(Utils::split("one,two,three", ',', false), expected);

    expected = {"one", "two", "three", ""};
    EXPECT_EQ(Utils::split("one,two,three,", ',', false), expected);

    expected = {"one", "two", "three"};
    EXPECT_EQ(Utils::split("one,two,three,", ',', true), expected);

    expected = {};
    EXPECT_EQ(Utils::split("", ',', false), expected); // Fix: Empty string splits to empty vector

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
    EXPECT_EQ(Utils::split("", "||", false), expected); // Fix: Empty string splits to empty vector

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
    EXPECT_TRUE(Utils::isFloatingPoint("123")); // Integers are also floating points
    
    // Scientific notation
    EXPECT_TRUE(Utils::isFloatingPoint("1.23e4"));
    EXPECT_TRUE(Utils::isFloatingPoint("1.23E4"));
    EXPECT_TRUE(Utils::isFloatingPoint("1e-5"));
    EXPECT_TRUE(Utils::isFloatingPoint("1.2E+2"));
    EXPECT_FALSE(Utils::isFloatingPoint("1e")); // Incomplete
    EXPECT_FALSE(Utils::isFloatingPoint("e5")); // Missing significand
}

TEST(UtilsTest, ToLong) {
    EXPECT_EQ(Utils::toLong("123").value_or(0), 123L);
    EXPECT_EQ(Utils::toLong("-456").value_or(0), -456L);
    EXPECT_EQ(Utils::toLong("+789").value_or(0), 789L);
    EXPECT_FALSE(Utils::toLong("123a").has_value());
    EXPECT_FALSE(Utils::toLong("12.3").has_value());
    EXPECT_FALSE(Utils::toLong("").has_value());
    EXPECT_TRUE(Utils::toLong("0").has_value());
    EXPECT_FALSE(Utils::toLong("-").has_value()); // Just '-' is not a valid number
    EXPECT_FALSE(Utils::toLong("+").has_value()); // Just '+' is not a valid number

    // Overflow check (assuming 64-bit long or checking strictly for overflow behavior)
    // A very large number that definitely overflows 64-bit signed integer
    EXPECT_FALSE(Utils::toLong("9223372036854775808").has_value()); // MAX_LONG + 1
    EXPECT_FALSE(Utils::toLong("-9223372036854775809").has_value()); // MIN_LONG - 1
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
    EXPECT_FALSE(Utils::toDouble(".").has_value()); // Just '.' is not a valid number
    EXPECT_FALSE(Utils::toDouble("-").has_value()); // Just '-' is not a valid number
    EXPECT_FALSE(Utils::toDouble("+").has_value()); // Just '+' is not a valid number
    
    // Scientific notation
    EXPECT_DOUBLE_EQ(Utils::toDouble("1.23e4").value_or(0.0), 12300.0);
    EXPECT_DOUBLE_EQ(Utils::toDouble("1e-2").value_or(0.0), 0.01);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
