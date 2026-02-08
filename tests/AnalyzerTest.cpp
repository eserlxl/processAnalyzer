// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include <gtest/gtest.h>
#include <unistd.h> // For getpid(), getuid()
#include <pwd.h>    // For getpwuid()
#include <ranges>

// Helper to get current username
std::string getCurrentUsername() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    if (pw != nullptr) {
        return pw->pw_name;
    }
    return "unknown";
}

namespace { // Anonymous namespace for local constants
    const int kInvalidPid = -999;
    const int kNonExistentPid = 9999999;
}

TEST(ProcessAnalyzerTest, ConstructorWithDefaultPath) {
    ProcessAnalyzer analyzer;
    // No direct way to check the internal procPath, but subsequent tests will implicitly verify it.
    // This test primarily checks if it can be constructed without throwing.
    SUCCEED(); 
}

TEST(ProcessAnalyzerTest, ConstructorWithCustomPath) {
    // This could be used for mocking /proc in future, for now just ensures it takes a path
    ProcessAnalyzer analyzer("/tmp"); 
    SUCCEED();
}

TEST(ProcessAnalyzerTest, GetPidsReturnsNonEmptyList) {
    ProcessAnalyzer analyzer;
    std::vector<int> pids = analyzer.getPids();
    ASSERT_FALSE(pids.empty());
    // Check for some common PIDs
    ASSERT_TRUE(std::ranges::find(pids, 1) != pids.end()); // init/systemd
    ASSERT_TRUE(std::ranges::find(pids, getpid()) != pids.end()); // self
}

TEST(ProcessAnalyzerTest, GetProcessDetailsForSelf) {
    ProcessAnalyzer analyzer;
    int selfPid = getpid();
    std::optional<ProcessInfo> infoOpt = analyzer.getProcessDetails(selfPid);

    ASSERT_TRUE(infoOpt.has_value());
    if (infoOpt) {
        const ProcessInfo& info = *infoOpt;

        EXPECT_EQ(info.pid, selfPid);
        EXPECT_FALSE(info.name.empty());
        EXPECT_FALSE(info.state.empty());
        EXPECT_GT(info.residentMemory, 0); // Test process should have some memory
        EXPECT_GT(info.virtualMemory, 0);
        EXPECT_EQ(info.uid, getuid());
        EXPECT_EQ(info.username, getCurrentUsername());
        EXPECT_GT(info.threadCount, 0);
        EXPECT_FALSE(info.cmdline.empty());
    } // Added closing brace
}

TEST(ProcessAnalyzerTest, GetProcessDetailsForNonExistentPidReturnsNullOpt) {
    ProcessAnalyzer analyzer;
    std::optional<ProcessInfo> infoOpt = analyzer.getProcessDetails(kInvalidPid); // Invalid PID
    ASSERT_FALSE(infoOpt.has_value());

    infoOpt = analyzer.getProcessDetails(kNonExistentPid); // Hopefully non-existent PID
    ASSERT_FALSE(infoOpt.has_value());
}

TEST(ProcessAnalyzerTest, SnapshotReturnsNonEmptyList) {
    ProcessAnalyzer analyzer;
    std::vector<ProcessInfo> processes = analyzer.snapshot();
    ASSERT_FALSE(processes.empty());

    // Check if at least PID 1 (init/systemd) is present
    bool foundInit = false;
    for (const auto& p : processes) {
        if (p.pid == 1) {
            foundInit = true;
            break;
        }
    }
    EXPECT_TRUE(foundInit);
}

TEST(ProcessAnalyzerTest, FindProcessesWithPredicate) {
    ProcessAnalyzer analyzer;
    std::string selfUsername = getCurrentUsername();
    
    // Find processes belonging to the current user
    auto userProcesses = analyzer.findProcesses([&selfUsername](const ProcessInfo& info) {
        return info.username == selfUsername;
    });

    ASSERT_FALSE(userProcesses.empty());

    // Verify all found processes indeed belong to the current user
    for (const auto& p : userProcesses) {
        EXPECT_EQ(p.username, selfUsername);
    }
    
    // Find processes with "bash" in their name
    auto bashProcesses = analyzer.findProcesses([](const ProcessInfo& info) {
        return info.name.find("bash") != std::string::npos;
    });
    // This might be empty if bash is not running, so not a hard assert.
    // We mainly test the predicate mechanism.
    SUCCEED();
}

TEST(ProcessAnalyzerTest, GetProcessesByName) {
    ProcessAnalyzer analyzer;
    // Test for a common process name like "systemd" or "init"
    std::vector<ProcessInfo> systemdProcesses = analyzer.getProcessesByName("systemd");
    if (systemdProcesses.empty()) {
        systemdProcesses = analyzer.getProcessesByName("init"); // Fallback for older systems
    }
    
    ASSERT_FALSE(systemdProcesses.empty());
    for (const auto& p : systemdProcesses) {
        EXPECT_TRUE(p.name == "systemd" || p.name == "init");
    }

    // Test for the test executable name (if it's simple enough)
    // The actual name might be truncated or slightly different, this is a best effort.
    std::string testName = "AnalyzerTest"; 
    auto testProcesses = analyzer.getProcessesByName(testName);
    bool foundTestProcess = false;
    for (const auto& p : testProcesses) {
        if (p.pid == getpid()) {
            foundTestProcess = true;
            break;
        }
    }
    // Only check if it's found if we're sure the name matches.
    // If the name is truncated (Linux often truncates to 15 chars), we might not find it by full name.
    // But since we are testing the mechanism, at least check it doesn't crash.
    // Let's add an assertion to make the variable used.
    (void)foundTestProcess; 
    
    // More robust would be to check against process name from getProcessDetails for self.
    std::optional<ProcessInfo> selfInfo = analyzer.getProcessDetails(getpid());
    if (selfInfo) {
        auto selfNamedProcesses = analyzer.getProcessesByName(selfInfo->name);
        ASSERT_FALSE(selfNamedProcesses.empty());
        bool foundSelfByName = false;
        for(const auto& p : selfNamedProcesses) {
            if (p.pid == getpid()) {
                foundSelfByName = true;
                break;
            }
        }
        EXPECT_TRUE(foundSelfByName);
    }
}

TEST(ProcessAnalyzerTest, GetProcessesByUser) {
    ProcessAnalyzer analyzer;
    std::string selfUsername = getCurrentUsername();
    std::vector<ProcessInfo> userProcesses = analyzer.getProcessesByUser(selfUsername);
    
    ASSERT_FALSE(userProcesses.empty());
    for (const auto& p : userProcesses) {
        EXPECT_EQ(p.username, selfUsername);
    }

    // Test for a non-existent user
    auto nonExistentUserProcesses = analyzer.getProcessesByUser("nonexistentuser12345");
    ASSERT_TRUE(nonExistentUserProcesses.empty());
}
