// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/internal_helpers.h"
#include "analyzer/analyzer_core.h" // For ProcessInfo, ProcessFilter, ProcessSortField, ProcessAnalyzer::CpuStats
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
    constexpr static pid_t TEST_PID = 100;
    constexpr static pid_t TEST_PPID = 1;
    constexpr static uid_t TEST_UID = 1000;
    constexpr static unsigned long TEST_RESIDENT_MEMORY = 50000; // KB
    constexpr static unsigned long TEST_VIRTUAL_MEMORY = 100000; // KB
    constexpr static int TEST_THREAD_COUNT = 5;
    constexpr static float TEST_CPU_USAGE = 15.5F;
    constexpr static float TEST_MEMORY_PERCENTAGE = 2.5F;

    // Magic numbers for filter
    constexpr static int TEST_MIN_THREADS_VALID = 3;
    constexpr static int TEST_MIN_THREADS_INVALID = 6;
    constexpr static int TEST_MAX_THREADS_VALID = 7;
    constexpr static int TEST_MAX_THREADS_INVALID = 4;
    constexpr static uid_t TEST_FILTER_UID_VALID = 1000;
    constexpr static uid_t TEST_FILTER_UID_INVALID = 0;

    // Magic numbers for compareProcesses
    constexpr static pid_t PID_1_COMPARE = 100;
    constexpr static pid_t PID_2_COMPARE = 200;
    constexpr static pid_t PPID_1_COMPARE = 10;
    constexpr static pid_t PPID_2_COMPARE = 20;
    constexpr static float CPU_USAGE_1_COMPARE = 1.0F;
    constexpr static float CPU_USAGE_2_COMPARE = 2.0F;

    // Magic numbers for GetTotalSystemCpuTimeTicks
    constexpr static unsigned long long CPU_USER = 100;
    constexpr static unsigned long long CPU_NICE = 10;
    constexpr static unsigned long long CPU_SYSTEM = 200;
    constexpr static unsigned long long CPU_IDLE = 500;
    constexpr static unsigned long long CPU_IOWAIT = 50;
    constexpr static unsigned long long CPU_IRQ = 20;
    constexpr static unsigned long long CPU_SOFTIRQ = 30;
    constexpr static unsigned long long CPU_STEAL = 40;
    constexpr static unsigned long long CPU_GUEST = 0;
    constexpr static unsigned long long CPU_GUEST_NICE = 0;

    // Magic numbers for CheckPidPathExistsAndPermissions
    constexpr static pid_t TEST_PID_EXISTS = 12345;
    constexpr static pid_t TEST_PID_NON_EXISTENT = 54321;

    // Magic numbers for ReadProcessEnvironmentVars
    constexpr static pid_t TEST_PID_ENV_VARS = 777;
    constexpr static char ENV_VAR_CONTENT[] = "VAR1=value1\\0VAR2=value2\\0";


    void SetUp() override {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = nullptr;
    }

    void TearDown() override {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = nullptr;
    }
};

TEST_F(InternalHelpersTest, MatchesFilter) {
    ProcessInfo process{};
    process.pid = TEST_PID;
    process.ppid = TEST_PPID;
    process.uid = TEST_UID;
    process.username = "testuser";
    process.name = "test_process";
    process.state = "R";
    process.residentMemory = TEST_RESIDENT_MEMORY; // KB
    process.virtualMemory = TEST_VIRTUAL_MEMORY; // KB
    process.threadCount = TEST_THREAD_COUNT;
    process.cmdline = "/usr/bin/test_process --arg1 --arg2";
    process.executablePath = "/usr/bin/test_process";
    process.priority = 0;
    process.cpuUsage = TEST_CPU_USAGE;
    process.memoryPercentage = TEST_MEMORY_PERCENTAGE;

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
        filter.minThreads = TEST_MIN_THREADS_INVALID;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));

        filter = {};
        filter.maxThreads = TEST_MAX_THREADS_VALID;
        EXPECT_TRUE(Internal::matchesFilter(process, filter));
        filter.maxThreads = 4;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // uidFilter
    {
        filter.uidFilter = TEST_FILTER_UID_VALID;
        EXPECT_TRUE(Internal::matchesFilter(process, filter));
        filter.uidFilter = TEST_FILTER_UID_INVALID;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }

    // Combination of filters
    {
        ProcessFilter filter{};
        filter.nameContains = "test";
        filter.minThreads = TEST_MIN_THREADS_VALID;
        filter.uidFilter = TEST_FILTER_UID_VALID;
        EXPECT_TRUE(Internal::matchesFilter(process, filter));

        filter.minThreads = TEST_MIN_THREADS_INVALID;
        EXPECT_FALSE(Internal::matchesFilter(process, filter));
    }
}

TEST_F(InternalHelpersTest, CompareProcesses) {
    ProcessInfo p1;
    ProcessInfo p2;

    // Compare by PID
    {
        p1.pid = PID_1_COMPARE; p2.pid = PID_2_COMPARE;
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::pid), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::pid), 0);
        p2.pid = PID_1_COMPARE;
        EXPECT_EQ(Internal::compareProcesses(p1, p2, ProcessSortField::pid), 0);
    }

    // Compare by PPID
    {
        p1.ppid = PPID_1_COMPARE; p2.ppid = PPID_2_COMPARE;
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::ppid), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::ppid), 0);
        p2.ppid = PPID_1_COMPARE;
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
        p1.cpuUsage = CPU_USAGE_1_COMPARE; p2.cpuUsage = CPU_USAGE_2_COMPARE;
        EXPECT_LT(Internal::compareProcesses(p1, p2, ProcessSortField::cpuUsage), 0);
        EXPECT_GT(Internal::compareProcesses(p2, p1, ProcessSortField::cpuUsage), 0);
        p2.cpuUsage = CPU_USAGE_1_COMPARE;
        EXPECT_EQ(Internal::compareProcesses(p1, p2, ProcessSortField::cpuUsage), 0);
    }
}

TEST_F(InternalHelpersTest, GetTotalSystemCpuTimeTicks) {
    // Successful CPU stats retrieval
    {
        ProcessAnalyzer::s_testMockGetSystemCpuStats = []() {
            return SystemCpuStats{.user=CPU_USER, .nice=CPU_NICE, .system=CPU_SYSTEM, .idle=CPU_IDLE, .iowait=CPU_IOWAIT, .irq=CPU_IRQ, .softirq=CPU_SOFTIRQ, .steal=CPU_STEAL, .guest=CPU_GUEST, .guest_nice=CPU_GUEST_NICE};
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
        pid_t testPid = TEST_PID_EXISTS;
        fs::path pidPath = tempProcRoot / std::to_string(testPid);
        fs::create_directory(pidPath);
        auto result = Internal::checkPidPathExistsAndPermissions(tempProcRoot, testPid);
        EXPECT_TRUE(result.has_value());
        fs::remove(pidPath);
    }

    // Path does not exist
    {
        pid_t testPid = TEST_PID_NON_EXISTENT;
        auto result = Internal::checkPidPathExistsAndPermissions(tempProcRoot, testPid);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
    }

    fs::remove(tempProcRoot);
}

TEST_F(InternalHelpersTest, ReadProcessEnvironmentVars) {
    fs::path tempProcRoot = fs::temp_directory_path() / "test_proc_env";
    fs::create_directory(tempProcRoot);
    pid_t testPid = TEST_PID_ENV_VARS;
    fs::path pidPath = tempProcRoot / std::to_string(testPid);
    fs::path environPath = pidPath / "environ";
    fs::create_directory(pidPath);

    // Standard environ file
    {
        std::ofstream ofs(environPath, std::ios::binary);
        const char* content = ENV_VAR_CONTENT;
        ofs.write(content, sizeof(ENV_VAR_CONTENT) - 1);
        ofs.close();

        auto result = Internal::readProcessEnvironmentVars(tempProcRoot, testPid);
        ASSERT_TRUE(result.has_value());
        std::vector<std::string> expected = {"VAR1=value1", "VAR2=value2"};
        EXPECT_EQ(*result, expected);
    }

    // Empty environ file
    {
        std::ofstream ofs(environPath);
        ofs.close();

        auto result = Internal::readProcessEnvironmentVars(tempProcRoot, testPid);
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result->empty());
    }

    fs::remove_all(tempProcRoot);
}
