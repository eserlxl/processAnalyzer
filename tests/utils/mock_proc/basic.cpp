// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/testing_framework.h" // Assuming MockProc is defined here
#include <sstream>
#include <memory>
#include <optional>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <ranges>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Constants for test values
constexpr int fd10 = 10;
constexpr int pid123 = 123;

// Helper to read file content for verification
std::optional<std::string> readFileContent(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary); // Open in binary to preserve line endings
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to split a string by null terminators
std::vector<std::string> splitNullSeparatedStrings(const std::string& content) {
    std::vector<std::string> result;
    if (content.empty()) {
        return result;
    }
    std::string::size_type start = 0;
    std::string::size_type end = content.find('\0');
    while (end != std::string::npos) {
        result.push_back(content.substr(start, end - start));
        start = end + 1;
        end = content.find('\0', start);
    }
    // This handles the case where the content does not end with a null
    if (start < content.length()) {
        result.push_back(content.substr(start));
    }
    return result;
}

// Helper to read null-terminated strings from a file
std::vector<std::string> readNullSeparatedStrings(const fs::path& filePath) {
    auto contentOpt = readFileContent(filePath);
    if (!contentOpt) {
        return {};
    }
    return splitNullSeparatedStrings(contentOpt.value());
}

// Helper to parse a key-value file (like /proc/status)
std::map<std::string, std::string> parseKeyValueFile(const fs::path& filePath) {
    std::map<std::string, std::string> data;
    auto contentOpt = readFileContent(filePath);
    if (!contentOpt) return data;

    std::stringstream ss(contentOpt.value());
    std::string line;
    while (std::getline(ss, line)) {
        auto colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // Trim leading whitespace from value
            auto firstChar = value.find_first_not_of(" \t");
            if (firstChar != std::string::npos) {
                value = value.substr(firstChar);
            }
            data[key] = value;
        }
    }
    return data;
}

} // namespace

// Test fixture for MockProc tests
class MockProcTest : public ::testing::Test {
protected:
    std::unique_ptr<MockProc> mockProc;
    fs::path mockRootPath;

    void SetUp() override {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        fs::path basePath = fs::temp_directory_path() / "ProcAnalyzerTest" /
                            (std::string(testInfo->test_case_name()) + "_" + std::string(testInfo->name()));
        
        if (fs::exists(basePath)) {
            fs::remove_all(basePath);
        }
        
        mockProc = std::make_unique<MockProc>(basePath.string());
        mockRootPath = mockProc->getPath();
    }
};

// --- Helper Function Tests ---
TEST(HelperTest, ReadFileContent_NonExistentFile) {
    auto content = readFileContent("a/file/that/does/not/exist.txt");
    ASSERT_FALSE(content.has_value());
}

TEST(HelperTest, SplitNullSeparatedStrings) {
    EXPECT_TRUE(splitNullSeparatedStrings("").empty());
    EXPECT_EQ(splitNullSeparatedStrings("abc"), std::vector<std::string>{"abc"});
    const std::array<char, 5> content1 = {'A', '\0', 'B', '\0', 'C'};
    EXPECT_EQ(splitNullSeparatedStrings(std::string(content1.data(), content1.size())), (std::vector<std::string>{"A", "B", "C"}));
    const std::array<char, 4> content2 = {'A', '\0', 'B', '\0'};
    EXPECT_EQ(splitNullSeparatedStrings(std::string(content2.data(), content2.size())), (std::vector<std::string>{"A", "B"}));
}


// --- Basic MockProc Functionality Tests ---

TEST_F(MockProcTest, CreateFileAt) {
    fs::path relativeFilePath = "etc/config.conf";
    std::string fileContent = "key = value\n";
    mockProc->createFileAt(relativeFilePath, fileContent);
    fs::path expectedFilePath = mockRootPath / relativeFilePath;
    ASSERT_TRUE(fs::exists(expectedFilePath));
    auto content = readFileContent(expectedFilePath);
    if (content) {
        EXPECT_EQ(content.value(), fileContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CreateFileAt_EmptyContent) {
    fs::path relativeFilePath = "etc/empty.conf";
    std::string fileContent;
    mockProc->createFileAt(relativeFilePath, fileContent);
    fs::path expectedFilePath = mockRootPath / relativeFilePath;
    ASSERT_TRUE(fs::exists(expectedFilePath));
    auto content = readFileContent(expectedFilePath);
    if (content) {
        EXPECT_EQ(content.value(), fileContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CreateDirectoryAt) {
    fs::path relativeDirPath = "data/logs";
    mockProc->createDirectoryAt(relativeDirPath);
    fs::path expectedDirPath = mockRootPath / relativeDirPath;
    ASSERT_TRUE(fs::is_directory(expectedDirPath));
}

TEST_F(MockProcTest, CreateSymlinkAt) {
    fs::path relativeSymlinkPath = "data/current_log";
    fs::path targetFilePath = "data/logs/app.log"; // Target relative to symlink's parent
    mockProc->createSymlinkAt(relativeSymlinkPath, targetFilePath);
    fs::path expectedSymlinkPath = mockRootPath / relativeSymlinkPath;
    ASSERT_TRUE(fs::is_symlink(expectedSymlinkPath));
    ASSERT_EQ(fs::read_symlink(expectedSymlinkPath).string(), targetFilePath.string());
}

TEST_F(MockProcTest, CreateCmdline) {
    const int testPid = pid123;
    std::vector<std::string> cmdArgs = {"/usr/bin/my_process", "--verbose", "-f", "config.txt"};
    mockProc->createCmdline(testPid, cmdArgs);
    fs::path cmdlinePath = mockRootPath / std::to_string(testPid) / "cmdline";
    ASSERT_TRUE(fs::exists(cmdlinePath));
    ASSERT_EQ(readNullSeparatedStrings(cmdlinePath), cmdArgs);
}

TEST_F(MockProcTest, CreateStatus) {
    const int testPid = pid123;
    std::map<std::string, std::string> statusData = {
        {"Name", "my_process"},
        {"State", "R (running)"},
        {"Pid", std::to_string(testPid)},
        {"PPid", "1"}
    };
    mockProc->createStatus(testPid, statusData);
    fs::path statusPath = mockRootPath / std::to_string(testPid) / "status";
    ASSERT_TRUE(fs::exists(statusPath));

    auto parsedData = parseKeyValueFile(statusPath);
    ASSERT_EQ(parsedData.size(), statusData.size());
    for(const auto& pair : statusData) {
        ASSERT_TRUE(parsedData.count(pair.first));
        EXPECT_EQ(parsedData[pair.first], pair.second);
    }
}

TEST_F(MockProcTest, CreateEnviron) {
    const int testPid = pid123;
    std::map<std::string, std::string> envVarsMap = {
        {"PATH", "/usr/local/bin:/usr/bin:/bin"},
        {"USER", "testuser"},
        {"HOME", "/home/testuser"}
    };
    mockProc->createEnviron(testPid, envVarsMap);
    fs::path environPath = mockRootPath / std::to_string(testPid) / "environ";
    ASSERT_TRUE(fs::exists(environPath));

    auto actualVars = readNullSeparatedStrings(environPath);
    std::vector<std::string> expectedVars;
    expectedVars.reserve(envVarsMap.size());
    for(const auto& pair : envVarsMap) {
        expectedVars.push_back(pair.first + "=" + pair.second);
    }

    std::ranges::sort(actualVars);
    std::ranges::sort(expectedVars);

    ASSERT_EQ(actualVars, expectedVars);
}

TEST_F(MockProcTest, CreateFdDir) {
    const int testPid = pid123;
    std::vector<std::pair<int, std::string>> fds = {
        {0, "/dev/stdin"},
        {1, "/dev/stdout"},
        {2, "/dev/stderr"},
        {fd10, "/path/to/some/file"}
    };
    mockProc->createFdDir(testPid, fds);
    fs::path fdDirPath = mockRootPath / std::to_string(testPid) / "fd";
    ASSERT_TRUE(fs::is_directory(fdDirPath));
    for (const auto& fdPair : fds) {
        fs::path symlinkPath = fdDirPath / std::to_string(fdPair.first);
        ASSERT_TRUE(fs::is_symlink(symlinkPath));
        ASSERT_EQ(fs::read_symlink(symlinkPath).string(), fdPair.second);
    }
}
