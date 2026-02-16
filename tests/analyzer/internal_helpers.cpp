// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/internal/internal_helpers.h"
#include "analyzer/core.h" // For ProcessInfo, ProcessFilter, ProcessSortField, ProcessAnalyzer::CpuStats
#include "utils/types.h"        // For utils::Result, utils::UtilsError

#include <vector>
#include <string>
#include <optional>
#include <regex>
#include <filesystem>
#include <fstream>
#include <cmath>

namespace fs = std::filesystem;

// Helper for comparing floats with tolerance
bool fuzzyCompare(float a, float b) {
    constexpr float epsilon = 0.001F;
    return std::abs(a - b) < epsilon;
}

class InternalHelpersTest : public ::testing::Test {
protected:
    // Magic numbers for ProcessInfo
    constexpr static pid_t testPid = 100;
    constexpr static pid_t testPpid = 1;
    constexpr static uid_t testUid = 1000;
    constexpr static unsigned long testResidentMemory = 50000; // KB
    constexpr static unsigned long testVirtualMemory = 100000; // KB
    constexpr static int testThreadCount = 5;
    constexpr static float testCpuUsage = 15.5F;
    constexpr static float testMemoryPercentage = 2.5F;

    // Magic numbers for filter
    constexpr static int testMinThreadsValid = 3;
    constexpr static int testMinThreadsInvalid = 6;
    constexpr static int testMaxThreadsValid = 7;
    constexpr static int testMaxThreadsInvalid = 4;
    constexpr static uid_t testFilterUidValid = 1000;
    constexpr static uid_t testFilterUidInvalid = 0;

    // Magic numbers for compareProcesses
    constexpr static pid_t pid1Compare = 100;
    constexpr static pid_t pid2Compare = 200;
    constexpr static pid_t ppid1Compare = 10;
    constexpr static pid_t ppid2Compare = 20;
    constexpr static float cpuUsage1Compare = 1.0F;
    constexpr static float cpuUsage2Compare = 2.0F;

    // Magic numbers for GetTotalSystemCpuTimeTicks
    constexpr static unsigned long long cpuUser = 100;
    constexpr static unsigned long long cpuNice = 10;
    constexpr static unsigned long long cpuSystem = 200;
    constexpr static unsigned long long cpuIdle = 500;
    constexpr static unsigned long long cpuIowait = 50;
    constexpr static unsigned long long cpuIrq = 20;
    constexpr static unsigned long long cpuSoftirq = 30;
    constexpr static unsigned long long cpuSteal = 40;
    constexpr static unsigned long long cpuGuest = 0;
    constexpr static unsigned long long cpuGuestNice = 0;

    // Magic numbers for CheckPidPathExistsAndPermissions
    constexpr static pid_t testPidExists = 12345;
    constexpr static pid_t testPidNonExistent = 54321;

    // Magic numbers for ReadProcessEnvironmentVars
    constexpr static pid_t testPidEnvVars = 777;
    // Removed ENV_VAR_CONTENT, defined locally in test


    void SetUp() override {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = nullptr;
    }

    void TearDown() override {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = nullptr;
    }
};

TEST_F(InternalHelpersTest, MatchesFilter) {
    ProcessInfo process{};
    process.pid = testPid;
    process.ppid = testPpid;
    process.uid = testUid;
    process.username = "testuser";
    process.name = "test_process";
    process.state = "R";
    process.residentMemory = testResidentMemory; // KB
    process.virtualMemory = testVirtualMemory; // KB
    process.threadCount = testThreadCount;
    process.cmdline = "/usr/bin/test_process --arg1 --arg2";
    process.executablePath = "/usr/bin/test_process";
    process.priority = 0;
    process.cpuUsage = testCpuUsage;
    process.memoryPercentage = testMemoryPercentage;

    // No filter set
    {
        ProcessFilter filter{};
        EXPECT_TRUE(Internal::matchesFilter(process, filter));
    }

    // nameContains filter
    {
        ProcessFilter filter{};
        filter.nameContains = "test";
        EXPECT_TRUE(Internal::matchesFilter(process, filter));

        filter.nameContains = "nonexistent";
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // nameRegex filter
    {
        ProcessFilter filter{};
        filter.nameRegex = std::regex("test_.*");
        EXPECT_TRUE(Internal::matchesFilter(process, filter));

        filter.nameRegex = std::regex("nonexistent_.*");
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // userFilter
    {
        ProcessFilter filter{};
        filter.userFilter = "testuser";
        EXPECT_TRUE(Internal::matchesFilter(process, filter));

        filter.userFilter = "root";
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // stateFilter
    {
        ProcessFilter filter{};
        filter.stateFilter = 'R';
        EXPECT_TRUE(Internal::matchesFilter(process, filter));

        filter.stateFilter = 'S';
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // minThreads and maxThreads filters
    {
        ProcessFilter filter{};
        filter.minThreads = 3;
        filter.minThreads = testMinThreadsInvalid;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));

        filter = {};
        filter.maxThreads = testMaxThreadsValid;
        EXPECT_TRUE(Internal::matchesFilter(process, filter));
        filter.maxThreads = 4;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // uidFilter
    {
        ProcessFilter filter{};
        filter.uidFilter = testFilterUidValid;
        EXPECT_TRUE(Internal::matchesFilter(process, filter));
        filter.uidFilter = testFilterUidInvalid;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // Combination of filters
    {
        ProcessFilter filter{};
        filter.nameContains = "test";
        filter.minThreads = testMinThreadsValid;
        filter.uidFilter = testFilterUidValid;
        EXPECT_TRUE(Internal::matchesFilter(process, filter));

        filter.minThreads = testMinThreadsInvalid;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }
}

TEST_F(InternalHelpersTest, CompareProcesses) {
    ProcessInfo p1;
    ProcessInfo p2;

    // Compare by PID
    {
        p1.pid = pid1Compare; p2.pid = pid2Compare;
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::pid), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::pid), 0);
        p2.pid = pid1Compare;
        EXPECT_EQ(Internal::compareProcesses(p1, p2, ProcessSortField::pid), 0);
    }

    // Compare by PPID
    {
        p1.ppid = ppid1Compare; p2.ppid = ppid2Compare;
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::ppid), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::ppid), 0);
        p2.ppid = ppid1Compare;
        EXPECT_EQ(Internal::compareProcesses(p1, p2, ProcessSortField::ppid), 0);
    }

    // Compare by Name
    {
        p1.name = "apple"; p2.name = "banana";
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::name), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::name), 0);
        p2.name = "apple";
        EXPECT_EQ(Internal::compareProcesses(p1, p2, ProcessSortField::name), 0);
    }

    // Compare by CPU Usage
    {
        p1.cpuUsage = cpuUsage1Compare; p2.cpuUsage = cpuUsage2Compare;
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::cpuUsage), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::cpuUsage), 0);
        p2.cpuUsage = cpuUsage1Compare;
        EXPECT_EQ(Internal::compareProcesses(p1, p2, ProcessSortField::cpuUsage), 0);
    }
}

TEST_F(InternalHelpersTest, GetTotalSystemCpuTimeTicks) {
    // Successful CPU stats retrieval
    {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = []() {
            return SystemCpuStats{.user=cpuUser, .nice=cpuNice, .system=cpuSystem, .idle=cpuIdle, .iowait=cpuIowait, .irq=cpuIrq, .softirq=cpuSoftirq, .steal=cpuSteal, .guest=cpuGuest, .guest_nice=cpuGuestNice};
        };
        auto result = Internal::getTotalSystemCpuTimeTicks();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, (100 + 10 + 200 + 500 + 50 + 20 + 30 + 40));
    }

    // Error during CPU stats retrieval
    {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = []() -> utils::Result<SystemCpuStats> {
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
        };
        auto result = Internal::getTotalSystemCpuTimeTicks();
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
}

TEST_F(InternalHelpersTest, CheckPidPathExistsAndPermissions) {
    fs::path tempProcRoot = fs::temp_directory_path() / "test_proc";
    fs::create_directory(tempProcRoot);

    // Path exists and is accessible
    {
        pid_t testPidLocal = testPidExists;
        fs::path pidPath = tempProcRoot / std::to_string(testPidLocal);
        fs::create_directory(pidPath);
        auto result = Internal::checkPidPathExistsAndPermissions(tempProcRoot, testPidLocal);
        EXPECT_TRUE(result.has_value());
        fs::remove(pidPath);
    }

    // Path does not exist
    {
        pid_t testPidLocal = testPidNonExistent;
        auto result = Internal::checkPidPathExistsAndPermissions(tempProcRoot, testPidLocal);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
    }

    fs::remove(tempProcRoot);
}

TEST_F(InternalHelpersTest, ReadProcessEnvironmentVars) {
    fs::path tempProcRoot = fs::temp_directory_path() / "test_proc_env";
    fs::create_directory(tempProcRoot);
    pid_t testPidLocal = testPidEnvVars;
    fs::path pidPath = tempProcRoot / std::to_string(testPidLocal);
    fs::path environPath = pidPath / "environ";
    fs::create_directory(pidPath);

    // Standard environ file
    {
        std::ofstream ofs(environPath, std::ios::binary);
        // Construct string with embedded nulls: "VAR1=value1\0VAR2=value2\0"
        // VAR1=value1 is 11 chars. \0 is 1. VAR2=value2 is 11 chars. \0 is 1. Total 24.
        const std::string content("VAR1=value1\0VAR2=value2\0", 24);
        ofs.write(content.c_str(), static_cast<std::streamsize>(content.size()));
        ofs.close();

        auto result = Internal::readProcessEnvironmentVars(tempProcRoot, testPidLocal);
        ASSERT_TRUE(result.has_value());
        std::vector<std::string> expected = {"VAR1=value1", "VAR2=value2"};
        EXPECT_EQ(*result, expected);
    }

    // Empty environ file
    {
        std::ofstream ofs(environPath);
        ofs.close();

        auto result = Internal::readProcessEnvironmentVars(tempProcRoot, testPidLocal);
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->empty());
    }

    fs::remove_all(tempProcRoot);
}
