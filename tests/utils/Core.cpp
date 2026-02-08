// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Core.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
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

TEST(UtilsTest, Trim) {
    EXPECT_EQ(utils::trim("  hello  "), "hello");
    EXPECT_EQ(utils::trim("hello"), "hello");
    EXPECT_EQ(utils::trim("  hello"), "hello");
    EXPECT_EQ(utils::trim("hello  "), "hello");
    EXPECT_EQ(utils::trim(""), "");
    EXPECT_EQ(utils::trim("   "), "");
    EXPECT_EQ(utils::trim("\t\n hello \r\v"), "hello");
}

TEST(UtilsTest, StartsWith) {
    EXPECT_TRUE(utils::startsWith("hello world", "hello"));
    EXPECT_FALSE(utils::startsWith("hello world", "world"));
    EXPECT_TRUE(utils::startsWith("hello", "hello"));
    EXPECT_FALSE(utils::startsWith("hello", "hellos"));
    EXPECT_TRUE(utils::startsWith("", ""));
    EXPECT_TRUE(utils::startsWith("hello", ""));
}

TEST(UtilsTest, EndsWith) {
    EXPECT_TRUE(utils::endsWith("hello world", "world"));
    EXPECT_FALSE(utils::endsWith("hello world", "hello"));
    EXPECT_TRUE(utils::endsWith("world", "world"));
    EXPECT_FALSE(utils::endsWith("world", "helloworld"));
    EXPECT_TRUE(utils::endsWith("", ""));
    EXPECT_TRUE(utils::endsWith("hello", ""));
}

TEST(UtilsTest, Contains) {
    EXPECT_TRUE(utils::contains("hello world", "lo wo"));
    EXPECT_FALSE(utils::contains("hello world", "foo"));
    EXPECT_TRUE(utils::contains("hello", "hello"));
    EXPECT_TRUE(utils::contains("hello", ""));
    EXPECT_FALSE(utils::contains("", "a"));
}

TEST(UtilsTest, StartsWithIgnoreCase) {
    EXPECT_TRUE(utils::startsWithIgnoreCase("Hello World", "hello"));
    EXPECT_FALSE(utils::startsWithIgnoreCase("Hello World", "world"));
    EXPECT_TRUE(utils::startsWithIgnoreCase("HELLO", "hello"));
}

TEST(UtilsTest, EndsWithIgnoreCase) {
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello World", "WORLD"));
    EXPECT_FALSE(utils::endsWithIgnoreCase("Hello World", "HELLO"));
    EXPECT_TRUE(utils::endsWithIgnoreCase("WORLD", "world"));
}

TEST(UtilsTest, ContainsIgnoreCase) {
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "lo wo"));
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "LO WO"));
    EXPECT_FALSE(utils::containsIgnoreCase("Hello World", "foo"));
}

TEST(UtilsTest, ToLower) {
    EXPECT_EQ(utils::toLower("HELLO World"), "hello world");
    EXPECT_EQ(utils::toLower("123!@#"), "123!@#");
    EXPECT_EQ(utils::toLower(""), "");
}

TEST(UtilsTest, ToUpper) {
    EXPECT_EQ(utils::toUpper("hello World"), "HELLO WORLD");
    EXPECT_EQ(utils::toUpper("123!@#"), "123!@#");
    EXPECT_EQ(utils::toUpper(""), "");
}

TEST(UtilsTest, Replace) {
    EXPECT_EQ(utils::replace("hello world", "world", "universe"), "hello universe");
    EXPECT_EQ(utils::replace("banana", "a", "o"), "bonono");
    EXPECT_EQ(utils::replace("hello", "l", ""), "heo");
    EXPECT_EQ(utils::replace("hello", "", "x"), "hello");
    EXPECT_EQ(utils::replace("", "a", "b"), "");
}

TEST(UtilsTest, ReplaceFirst) {
    EXPECT_EQ(utils::replaceFirst("banana", "a", "o"), "bonana");
    EXPECT_EQ(utils::replaceFirst("hello world world", "world", "galaxy"), "hello galaxy world");
    EXPECT_EQ(utils::replaceFirst("test", "not_found", "x"), "test");
}

TEST(UtilsTest, ReplaceN) {
    EXPECT_EQ(utils::replaceN("banana", "a", "o", 2), "bonona");
    EXPECT_EQ(utils::replaceN("ababab", "ab", "c", 2), "ccab");
    EXPECT_EQ(utils::replaceN("test", "t", "x", 10), "xesx"); // replace all
}

TEST(UtilsTest, Format) {
    EXPECT_EQ(utils::format("Hello, {}!", "world"), "Hello, world!");
    EXPECT_EQ(utils::format("Number: {}", 123), "Number: 123");
    EXPECT_EQ(utils::format("{} {} {}", -5, 3.14, "test"), "-5 3.14 test");
}

TEST(UtilsTest, Join) {
    std::vector<std::string> parts = {"one", "two", "three"};
    EXPECT_EQ(utils::join(parts, ","), "one,two,three");
    EXPECT_EQ(utils::join(parts, "---"), "one---two---three");
    EXPECT_EQ(utils::join({"single"}, ","), "single");
    EXPECT_EQ(utils::join({}, ","), "");
}

TEST(UtilsTest, SplitCharDelimiter) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one,two,three", ',', false), expected);

    expected = {"one", "two", "three", ""};
    EXPECT_EQ(utils::split("one,two,three,", ',', false), expected);

    expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one,two,three,", ',', true), expected);

    expected = {};
    EXPECT_EQ(utils::split("", ',', false), expected);

    expected = {"a", "b"};
    EXPECT_EQ(utils::split("a,,b", ',', true), expected);
    
    expected = {"a", "", "b"};
    EXPECT_EQ(utils::split("a,,b", ',', false), expected);
}

TEST(UtilsTest, SplitStringDelimiter) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one||two||three", "||", false), expected);

    expected = {"one", "two", "three", ""};
    EXPECT_EQ(utils::split("one||two||three||", "||", false), expected);

    expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one||two||three||", "||", true), expected);

    expected = {};
    EXPECT_EQ(utils::split("", "||", false), expected);

    expected = {"a", "b"};
    EXPECT_EQ(utils::split("a||||b", "||", true), expected);

    expected = {"a", "", "b"};
    EXPECT_EQ(utils::split("a||||b", "||", false), expected);
}

TEST(UtilsTest, IsInteger) {
    EXPECT_TRUE(utils::isInteger("123"));
    EXPECT_TRUE(utils::isInteger("-456"));
    EXPECT_TRUE(utils::isInteger("+789"));
    EXPECT_FALSE(utils::isInteger("123a"));
    EXPECT_FALSE(utils::isInteger("12.3"));
    EXPECT_FALSE(utils::isInteger(""));
    EXPECT_TRUE(utils::isInteger("0"));
    EXPECT_FALSE(utils::isInteger("-"));
    EXPECT_FALSE(utils::isInteger("+"));
}

TEST(UtilsTest, IsFloatingPoint) {
    EXPECT_TRUE(utils::isFloatingPoint("123.45"));
    EXPECT_TRUE(utils::isFloatingPoint("-12.3"));
    EXPECT_TRUE(utils::isFloatingPoint("+0.5"));
    EXPECT_TRUE(utils::isFloatingPoint(".5"));
    EXPECT_TRUE(utils::isFloatingPoint("123."));
    EXPECT_FALSE(utils::isFloatingPoint("12.3.4"));
    EXPECT_FALSE(utils::isFloatingPoint("abc"));
    EXPECT_FALSE(utils::isFloatingPoint(""));
    EXPECT_TRUE(utils::isFloatingPoint("0.0"));
    EXPECT_FALSE(utils::isFloatingPoint("."));
    EXPECT_FALSE(utils::isFloatingPoint("-"));
    EXPECT_FALSE(utils::isFloatingPoint("+"));
    EXPECT_TRUE(utils::isFloatingPoint("123"));
    
    // Scientific notation
    EXPECT_TRUE(utils::isFloatingPoint("1.23e4"));
    EXPECT_TRUE(utils::isFloatingPoint("1.23E4"));
    EXPECT_TRUE(utils::isFloatingPoint("1e-5"));
    EXPECT_TRUE(utils::isFloatingPoint("1.2E+2"));
    EXPECT_FALSE(utils::isFloatingPoint("1e"));
    EXPECT_FALSE(utils::isFloatingPoint("e5"));
}

TEST(UtilsTest, ToLong) {
    EXPECT_EQ(utils::toLong("123", 10).value_or(0), 123L);
    EXPECT_EQ(utils::toLong("-456", 10).value_or(0), -456L);
    EXPECT_EQ(utils::toLong("+789", 10).value_or(0), 789L);
    EXPECT_FALSE(utils::toLong("123a", 10).has_value());
    EXPECT_FALSE(utils::toLong("12.3", 10).has_value());
    EXPECT_FALSE(utils::toLong("", 10).has_value());
    EXPECT_TRUE(utils::toLong("0", 10).has_value());
    EXPECT_FALSE(utils::toLong("-", 10).has_value());
    EXPECT_FALSE(utils::toLong("+", 10).has_value());

    // Overflow check
    EXPECT_FALSE(utils::toLong("9223372036854775808", 10).has_value());
    EXPECT_FALSE(utils::toLong("-9223372036854775809", 10).has_value());
}

TEST(UtilsTest, ToDouble) {
    EXPECT_DOUBLE_EQ(utils::toDouble("123.45").value_or(0.0), 123.45);
    EXPECT_DOUBLE_EQ(utils::toDouble("-12.3").value_or(0.0), -12.3);
    EXPECT_DOUBLE_EQ(utils::toDouble("+0.5").value_or(0.0), 0.5);
    EXPECT_DOUBLE_EQ(utils::toDouble(".5").value_or(0.0), 0.5);
    EXPECT_DOUBLE_EQ(utils::toDouble("123.").value_or(0.0), 123.0);
    EXPECT_FALSE(utils::toDouble("12.3.4").has_value());
    EXPECT_FALSE(utils::toDouble("abc").has_value());
    EXPECT_FALSE(utils::toDouble("").has_value());
    EXPECT_TRUE(utils::toDouble("0.0").has_value());
    EXPECT_FALSE(utils::toDouble(".").has_value());
    EXPECT_FALSE(utils::toDouble("-").has_value());
    EXPECT_FALSE(utils::toDouble("+").has_value());
    
    // Scientific notation
    EXPECT_DOUBLE_EQ(utils::toDouble("1.23e4").value_or(0.0), 12300.0);
    EXPECT_DOUBLE_EQ(utils::toDouble("1e-2").value_or(0.0), 0.01);
}

TEST(UtilsTest, ParseBool) {
    EXPECT_TRUE(utils::parseBool("true").value_or(false));
    EXPECT_TRUE(utils::parseBool("TRUE").value_or(false));
    EXPECT_TRUE(utils::parseBool("1").value_or(false));
    EXPECT_TRUE(utils::parseBool("yes").value_or(false));
    EXPECT_FALSE(utils::parseBool("false").value_or(true));
    EXPECT_FALSE(utils::parseBool("FALSE").value_or(true));
    EXPECT_FALSE(utils::parseBool("0").value_or(true));
    EXPECT_FALSE(utils::parseBool("no").value_or(true));
    EXPECT_FALSE(utils::parseBool("invalid").has_value());
    EXPECT_FALSE(utils::parseBool("").has_value());
}

TEST(UtilsTest, ToInt) {
    EXPECT_EQ(utils::toInt("123", 10).value_or(0), 123);
    EXPECT_EQ(utils::toInt("-456", 10).value_or(0), -456);
    EXPECT_FALSE(utils::toInt("2147483648", 10).has_value()); // Out of range
    EXPECT_FALSE(utils::toInt("abc", 10).has_value());
}

TEST(UtilsTest, ToFloat) {
    EXPECT_FLOAT_EQ(utils::toFloat("123.45").value_or(0.0F), 123.45F);
    EXPECT_FLOAT_EQ(utils::toFloat("-12.3").value_or(0.0F), -12.3F);
    EXPECT_FALSE(utils::toFloat("3.5e40").has_value()); // Out of range for float
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
    EXPECT_TRUE(resultSuccess.value().stderrStr.empty()); // Per audit, stderrStr should be empty for executeCommand

    // Test command that fails
    auto resultFail = utils::executeCommand("ls non_existent_dir_12345");
    ASSERT_TRUE(resultFail.has_value());
    EXPECT_NE(resultFail.value().exitCode, 0);
    EXPECT_TRUE(utils::contains(resultFail.value().stdoutStr, "No such file or directory"));
    EXPECT_TRUE(resultFail.value().stderrStr.empty()); // Per audit, stderrStr should be empty for executeCommand
}
