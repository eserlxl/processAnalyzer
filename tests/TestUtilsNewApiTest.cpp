// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "TestUtils.h" // Include the header with MockProc
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>
#include <ostream>

// Simple assertion macro for tests
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << (message) << " in " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; /* Indicate test failure */ \
    }

#define ASSERT_FALSE(condition, message) \
    if ((condition)) { \
        std::cerr << "Assertion failed: " << (message) << " in " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; /* Indicate test failure */ \
    }

#define ASSERT_EQ(val1, val2, message) \
    if ((val1) != (val2)) { \
        std::cerr << "Assertion failed: " << (message) << " (" << (val1) << " != " << (val2) << ") in " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; /* Indicate test failure */ \
    }

// Helper to print std::vector for debugging in assertions
template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        os << "\"" << vec[i] << "\"";
        if (i < vec.size() - 1) {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

// Helper to read file content for verification
std::string readFileContent(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ""; // Return empty string if file cannot be opened
    }
    return { (std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>() };
}

// Helper to read null-terminated strings from a file
std::vector<std::string> readNullSeparatedStrings(const std::filesystem::path& filePath) {
    std::vector<std::string> result;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return result;
    }

    std::string buffer;
    char c;
    while (file.get(c)) {
        if (c == '\0') {
            result.push_back(buffer);
            buffer.clear();
        } else {
            buffer += c;
        }
    }
    if (!buffer.empty()) { // Add last string if not null-terminated
        result.push_back(buffer);
    }
    return result;
}


int main() {
    std::cout << "Running TestUtils.h new API tests..." << std::endl;

    // --- Test Constants ---
    const int testPid1 = 12345;
    const int testPid2 = 67890;
    const int testPid3 = 11111;
    const int fdNum = 5;

    // Test fixture: MockProc instance
    MockProc mockProc("test_mock_proc_root");
    std::filesystem::path mockRootPath = mockProc.getPath();

    // --- Test cases for General Filesystem Operations ---

    // Test: createFileAt
    std::cout << "Testing createFileAt..." << std::endl;
    std::filesystem::path relativeFilePath = "etc/config.conf";
    std::string fileContent = "key = value\n";
    mockProc.createFileAt(relativeFilePath, fileContent);
    std::filesystem::path expectedFilePath = mockRootPath / relativeFilePath;
    ASSERT_TRUE(std::filesystem::exists(expectedFilePath), "File should be created.");
    ASSERT_EQ(readFileContent(expectedFilePath), fileContent, "File content should match.");

    // Test: createDirectoryAt
    std::cout << "Testing createDirectoryAt..." << std::endl;
    std::filesystem::path relativeDirPath = "data/logs";
    mockProc.createDirectoryAt(relativeDirPath);
    std::filesystem::path expectedDirPath = mockRootPath / relativeDirPath;
    ASSERT_TRUE(std::filesystem::is_directory(expectedDirPath), "Directory should be created.");

    // Test: createSymlinkAt
    std::cout << "Testing createSymlinkAt..." << std::endl;
    std::filesystem::path relativeSymlinkPath = "data/current_log";
    std::filesystem::path targetFilePath = "data/logs/app.log"; // Target relative to symlink's parent
    mockProc.createSymlinkAt(relativeSymlinkPath, targetFilePath);
    std::filesystem::path expectedSymlinkPath = mockRootPath / relativeSymlinkPath;
    ASSERT_TRUE(std::filesystem::is_symlink(expectedSymlinkPath), "Symlink should be created.");
    ASSERT_EQ(fs::read_symlink(expectedSymlinkPath).string(), targetFilePath.string(), "Symlink target should be correct.");

    // --- Test cases for Process-Specific File Mocking ---

    // Test: createCmdline
    std::cout << "Testing createCmdline..." << std::endl;
    std::vector<std::string> cmdArgs = {"/usr/bin/my_process", "--verbose", "-f", "config.txt"};
    mockProc.createCmdline(testPid1, cmdArgs);
    std::filesystem::path cmdlinePath = mockRootPath / std::to_string(testPid1) / "cmdline";
    ASSERT_TRUE(std::filesystem::exists(cmdlinePath), "/proc/<pid>/cmdline should exist.");
    ASSERT_EQ(readNullSeparatedStrings(cmdlinePath), cmdArgs, "cmdline content should match.");

    // Test: createStatus
    std::cout << "Testing createStatus..." << std::endl;
    std::map<std::string, std::string> statusData = {
        {"Name", "my_process"},
        {"State", "R (running)"},
        {"Pid", std::to_string(testPid1)},
        {"PPid", "123"}
    };
    mockProc.createStatus(testPid1, statusData);
    std::filesystem::path statusPath = mockRootPath / std::to_string(testPid1) / "status";
    ASSERT_TRUE(std::filesystem::exists(statusPath), "/proc/<pid>/status should exist.");
    std::string statusContent = readFileContent(statusPath);
    // Verify content format (simple check)
    ASSERT_TRUE(statusContent.find("Name: my_process") != std::string::npos, "Status Name should be present.");
    ASSERT_TRUE(statusContent.find("State: R (running)") != std::string::npos, "Status State should be present.");
    ASSERT_TRUE(statusContent.find("Pid: " + std::to_string(testPid1)) != std::string::npos, "Status Pid should be present.");
    ASSERT_TRUE(statusContent.find("PPid: 123") != std::string::npos, "Status PPid should be present.");

    // Test: createEnviron
    std::cout << "Testing createEnviron..." << std::endl;
    std::map<std::string, std::string> envVars = {
        {"PATH", "/usr/local/bin:/usr/bin:/bin"},
        {"USER", "testuser"},
        {"HOME", "/home/testuser"}
    };
    mockProc.createEnviron(testPid1, envVars);
    std::filesystem::path environPath = mockRootPath / std::to_string(testPid1) / "environ";
    ASSERT_TRUE(std::filesystem::exists(environPath), "/proc/<pid>/environ should exist.");
    // readNullSeparatedStrings expects KEY=VALUE format in file, need to adjust helper or test logic
    // For now, let's verify by reading the raw content and checking for expected strings
    std::string environContent = readFileContent(environPath);
    ASSERT_TRUE(environContent.find("PATH=/usr/local/bin:/usr/bin:/bin") != std::string::npos, "Environ PATH should be present.");
    ASSERT_TRUE(environContent.find("USER=testuser") != std::string::npos, "Environ USER should be present.");
    ASSERT_TRUE(environContent.find("HOME=/home/testuser") != std::string::npos, "Environ HOME should be present.");
    // Check for null terminators specifically
    size_t nullCount = 0;
    for (char c : environContent) {
        if (c == '\0') nullCount++;
    }
    ASSERT_EQ(nullCount, envVars.size() - 1, "Number of null terminators in environ should be correct.");


    // Test: createFdDir
    std::cout << "Testing createFdDir..." << std::endl;
    std::vector<std::pair<int, std::string>> fds = {
        {0, "/dev/stdin"},
        {1, "/dev/stdout"},
        {2, "/dev/stderr"},
        {fdNum, "/path/to/some/file"}
    };
    mockProc.createFdDir(testPid1, fds);
    std::filesystem::path fdDirPath = mockRootPath / std::to_string(testPid1) / "fd";
    ASSERT_TRUE(std::filesystem::is_directory(fdDirPath), "/proc/<pid>/fd directory should exist.");
    for (const auto& fdPair : fds) {
        std::filesystem::path symlinkPath = fdDirPath / std::to_string(fdPair.first);
        ASSERT_TRUE(std::filesystem::is_symlink(symlinkPath), "FD symlink " + std::to_string(fdPair.first) + " should exist.");
        ASSERT_EQ(fs::read_symlink(symlinkPath).string(), fdPair.second, "FD symlink target should be correct.");
    }

    // --- Test cases for Higher-Level Process Management Helpers ---

    // Test: addProcess
    std::cout << "Testing addProcess..." << std::endl;
    std::string processName = "another_proc";
    std::vector<std::string> cmdlineArgs2 = {"/usr/bin/another", "--quiet"};
    mockProc.addProcess(testPid2, processName, cmdlineArgs2);

    std::filesystem::path pid2Path = mockRootPath / std::to_string(testPid2);
    ASSERT_TRUE(std::filesystem::is_directory(pid2Path), "/proc/<pid2> directory should be created by addProcess.");

    std::filesystem::path statusPath2 = pid2Path / "status";
    ASSERT_TRUE(std::filesystem::exists(statusPath2), "/proc/<pid2>/status should be created by addProcess.");
    std::string statusContent2 = readFileContent(statusPath2);
    ASSERT_TRUE(statusContent2.find("Name: " + processName) != std::string::npos, "addProcess should set process name in status.");

    std::filesystem::path cmdlinePath2 = pid2Path / "cmdline";
    ASSERT_TRUE(std::filesystem::exists(cmdlinePath2), "/proc/<pid2>/cmdline should be created by addProcess.");
    ASSERT_EQ(readNullSeparatedStrings(cmdlinePath2), cmdlineArgs2, "addProcess should set cmdline arguments correctly.");

    // Test addProcess without cmdline args
    std::cout << "Testing addProcess without cmdline args..." << std::endl;
    std::string processName3 = "no_args_proc";
    mockProc.addProcess(testPid3, processName3);
    std::filesystem::path pid3Path = mockRootPath / std::to_string(testPid3);
    ASSERT_TRUE(std::filesystem::is_directory(pid3Path), "/proc/<pid3> directory should be created by addProcess.");
    std::filesystem::path statusPath3 = pid3Path / "status";
    ASSERT_TRUE(std::filesystem::exists(statusPath3), "/proc/<pid3>/status should be created by addProcess.");
    std::string statusContent3 = readFileContent(statusPath3);
    ASSERT_TRUE(statusContent3.find("Name: " + processName3) != std::string::npos, "addProcess (no args) should set process name in status.");
    
    std::filesystem::path cmdlinePath3 = pid3Path / "cmdline";
    ASSERT_FALSE(std::filesystem::exists(cmdlinePath3), "/proc/<pid3>/cmdline should not exist when no args provided.");


    // Cleanup is handled by MockProc destructor.

    std::cout << "All TestUtils.h new API tests passed!" << std::endl;
    return 0; // Indicate test success
}
