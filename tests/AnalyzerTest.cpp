// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "TestUtils.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <ranges>

namespace {
    // PIDs
    constexpr int kInitPid = 1;
    constexpr int kKthreaddPid = 2;
    constexpr int kMyAppPid = 3;
    constexpr int kZombiePid = 4;
    constexpr int kNoStatPid = 5;
    constexpr int kNonExistentPid = 9999;
    
    // UIDs
    constexpr int kRootUid = 0;
    constexpr int kTestUserUid = 1000;

    // String literal lengths
    constexpr int kSystemdCmdlineSize = 8;
    constexpr int kInitEnvSize = 22;
    constexpr int kMyAppCmdlineSize = 42;
    constexpr int kMyAppEnvSize = 28;

    // Durations
    constexpr int kUpdateFileDelayMs = 25;
    constexpr int kCpuUsageSampleTimeMs = 50;

    // Memory
    constexpr unsigned long kMinResidentMemoryKB = 40000;
}

class ProcessAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockProc = new MockProc("mock_proc_test");
        setupMockProcFiles();
    }

    void TearDown() override {
        delete mockProc;
    }

    void setupMockProcFiles() {
        // PID 1: init-like process
        mockProc->createProcFile(kInitPid, "status", "Name:\tinit\nState:\tS (sleeping)\nPPid:\t0\nUid:\t0\t0\t0\t0\nThreads:\t1\nVmRSS:\t1000 kB\nVmSize:\t4000 kB\n");
        mockProc->createProcFile(kInitPid, "stat", "1 (init) S 0 1 1 0 -1 4202752 239 0 0 0 10 20 0 0 20 0 1 0 12345 4096000 1000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(kInitPid, "cmdline", std::string("systemd\0", kSystemdCmdlineSize));
        mockProc->createProcFile(kInitPid, "io", "rchar: 100\nwchar: 200\n");
        mockProc->createSymlink(kInitPid, "exe", "/sbin/init");
        mockProc->createSymlink(kInitPid, "cwd", "/");
        mockProc->createProcFile(kInitPid, "environ", std::string("PATH=/bin:/sbin\0HOME=/\0", kInitEnvSize));

        // PID 2: a child of PID 1
        mockProc->createProcFile(kKthreaddPid, "status", "Name:\tkthreadd\nState:\tS (sleeping)\nPPid:\t1\nUid:\t0\t0\t0\t0\nThreads:\t5\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(kKthreaddPid, "stat", "2 (kthreadd) S 1 2 2 0 -1 8388608 0 0 0 0 100 200 0 0 18 0 1 0 54321 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(kKthreaddPid, "cmdline", "");
        mockProc->createSymlink(kKthreaddPid, "exe", "/usr/bin/kthreadd");
        mockProc->createProcFile(kKthreaddPid, "io", "rchar: 1234\nwchar: 5678\n");


        // PID 3: another process, child of PID 1, running state
        mockProc->createProcFile(kMyAppPid, "status", "Name:\tmy-app\nState:\tR (running)\nPPid:\t1\nUid:\t1000\t1000\t1000\t1000\nThreads:\t10\nVmRSS:\t50000 kB\nVmSize:\t100000 kB\n");
        mockProc->createProcFile(kMyAppPid, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 15 25 0 0 15 0 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(kMyAppPid, "cmdline", std::string("/usr/bin/my-app\0--config\0/etc/my-app.conf", kMyAppCmdlineSize));
        mockProc->createProcFile(kMyAppPid, "environ", std::string("PATH=/usr/bin\0USER=testuser\0", kMyAppEnvSize));
        mockProc->createSymlink(kMyAppPid, "exe", "/usr/bin/my-app");
        mockProc->createSymlink(kMyAppPid, "cwd", "/home/testuser");

        // PID 4: a zombie process, child of PID 3
        mockProc->createProcFile(kZombiePid, "status", "Name:\tdefunct\nState:\tZ (zombie)\nPPid:\t3\nUid:\t1000\t1000\t1000\t1000\nThreads:\t0\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(kZombiePid, "stat", "4 (defunct) Z 3 4 4 0 -1 4202752 0 0 0 0 0 0 0 0 0 0 1 0 98765 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

        // Stat file for CPU usage tests
        mockProc->createFile("stat", "cpu  1000 200 800 5000 100 0 50 0\ncpu0 500 100 400 2500 50 0 25 0\n");
        // meminfo for system memory tests
        mockProc->createFile("meminfo", "MemTotal:       16384000 kB\nMemFree:         8192000 kB\nMemAvailable:   10240000 kB\nBuffers:          512000 kB\nCached:           2048000 kB\nSwapTotal:       8192000 kB\nSwapFree:        4096000 kB\n");
        // loadavg for system load tests
        mockProc->createFile("loadavg", "0.50 1.20 1.50 1/123 4567\n");
    }

    MockProc* mockProc;
};

TEST_F(ProcessAnalyzerTest, GetPidsWithMock) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto pids = analyzer.getPids();
    ASSERT_EQ(pids.size(), 4);
    ASSERT_NE(std::ranges::find(pids, kInitPid), pids.end());
    ASSERT_NE(std::ranges::find(pids, kKthreaddPid), pids.end());
    ASSERT_NE(std::ranges::find(pids, kMyAppPid), pids.end());
    ASSERT_NE(std::ranges::find(pids, kZombiePid), pids.end());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsWithMock) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(kMyAppPid);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = infoOpt.value(); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_EQ(info.pid, kMyAppPid);
    EXPECT_EQ(info.ppid, kInitPid);
    EXPECT_EQ(info.name, "my-app");
    EXPECT_EQ(info.state.substr(0,1), "R");
    EXPECT_EQ(info.uid, kTestUserUid);
    EXPECT_EQ(info.threadCount, 10);
    EXPECT_EQ(info.residentMemory, 50000);
    EXPECT_EQ(info.virtualMemory, 100000);
    EXPECT_EQ(info.cmdline, "/usr/bin/my-app --config /etc/my-app.conf");
    EXPECT_EQ(info.startTimeTicks, 23456);
    EXPECT_EQ(info.cpuUserTimeTicks, 15);
    EXPECT_EQ(info.cpuKernelTimeTicks, 25);
    EXPECT_EQ(info.priority, 15);
    EXPECT_EQ(info.executablePath, "/usr/bin/my-app");
    EXPECT_EQ(info.currentWorkingDirectory, "/home/testuser");
    ASSERT_EQ(info.environmentVariables.size(), 2);
    EXPECT_EQ(info.environmentVariables[0], "PATH=/usr/bin");
    EXPECT_EQ(info.environmentVariables[1], "USER=testuser");
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsForNonExistentPid) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(kNonExistentPid);
    ASSERT_FALSE(infoOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsForZombieProcess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(kZombiePid);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = infoOpt.value(); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_EQ(info.pid, kZombiePid);
    EXPECT_EQ(info.ppid, kMyAppPid);
    EXPECT_EQ(info.name, "defunct");
    EXPECT_EQ(info.state.substr(0,1), "Z");
    EXPECT_EQ(info.residentMemory, 0);
    // For a zombie, many files are gone. Ensure we handle this gracefully.
    EXPECT_TRUE(info.executablePath.empty());
    EXPECT_TRUE(info.currentWorkingDirectory.empty());
    EXPECT_TRUE(info.cmdline.empty());
    EXPECT_TRUE(info.environmentVariables.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesIOAndStatMissingGracefully) {
    mockProc->createPidDir(kNoStatPid);
    mockProc->createProcFile(kNoStatPid, "status", "Name:\tno-stat-io\nPPid:\t1\nUid:\t1000\n");
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(kNoStatPid);
    ASSERT_TRUE(infoOpt.has_value());
    EXPECT_EQ(infoOpt.value().ioReadBytes, 0); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_EQ(infoOpt.value().cpuUserTimeTicks, 0); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_EQ(infoOpt.value().name, "no-stat-io"); // NOLINT(bugprone-unchecked-optional-access)
}

TEST_F(ProcessAnalyzerTest, GetSystemMemoryInfo) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto memInfoOpt = analyzer.getSystemMemoryInfo();
    ASSERT_TRUE(memInfoOpt.has_value());
    const auto& memInfo = memInfoOpt.value(); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_EQ(memInfo.memTotal, 16384000);
    EXPECT_EQ(memInfo.memFree, 8192000);
    EXPECT_EQ(memInfo.memAvailable, 10240000);
    EXPECT_EQ(memInfo.buffers, 512000);
    EXPECT_EQ(memInfo.cached, 2048000);
    EXPECT_EQ(memInfo.swapTotal, 8192000);
    EXPECT_EQ(memInfo.swapFree, 4096000);

    // Test failure case
    ProcessAnalyzer analyzerNoFile("nonexistent_path");
    auto memInfoFailOpt = analyzerNoFile.getSystemMemoryInfo();
    ASSERT_FALSE(memInfoFailOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, GetSystemLoadAverage) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto loadAvgOpt = analyzer.getSystemLoadAverage();
    ASSERT_TRUE(loadAvgOpt.has_value());
    const auto& loadAvg = loadAvgOpt.value(); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_DOUBLE_EQ(loadAvg.oneMin, 0.50);
    EXPECT_DOUBLE_EQ(loadAvg.fiveMin, 1.20);
    EXPECT_DOUBLE_EQ(loadAvg.fifteenMin, 1.50);

    // Test failure case
    ProcessAnalyzer analyzerNoFile("nonexistent_path");
    auto loadAvgFailOpt = analyzerNoFile.getSystemLoadAverage();
    ASSERT_FALSE(loadAvgFailOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, GetSystemCpuStats) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto cpuStatsOpt = analyzer.getSystemCpuStats();
    ASSERT_TRUE(cpuStatsOpt.has_value());
    const auto& cpuStats = cpuStatsOpt.value(); // NOLINT(bugprone-unchecked-optional-access)
    EXPECT_EQ(cpuStats.user, 1000);
    EXPECT_EQ(cpuStats.nice, 200);
    EXPECT_EQ(cpuStats.system, 800);
    EXPECT_EQ(cpuStats.idle, 5000);
    EXPECT_EQ(cpuStats.iowait, 100);
    EXPECT_EQ(cpuStats.irq, 0);
    EXPECT_EQ(cpuStats.softirq, 50);
    EXPECT_EQ(cpuStats.steal, 0);

    // Test failure case
    ProcessAnalyzer analyzerNoFile("nonexistent_path");
    auto cpuStatsFailOpt = analyzerNoFile.getSystemCpuStats();
    ASSERT_FALSE(cpuStatsFailOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, QueryProcessesFiltering) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    ProcessFilter filter;

    // Filter by name
    filter = {};
    filter.nameContains = "kthread";
    auto results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kKthreaddPid);

    // Filter by parent PID (new in Iteration 7)
    filter = {};
    filter.ppidFilter = kInitPid;
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 2);
    EXPECT_TRUE((results[0].pid == kKthreaddPid && results[1].pid == kMyAppPid) || (results[0].pid == kMyAppPid && results[1].pid == kKthreaddPid));

    // Filter by state
    filter = {};
    filter.stateFilter = 'Z';
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kZombiePid);

    // Filter by UID
    filter = {};
    filter.uidFilter = kTestUserUid;
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 2); // PIDs 3 and 4

    // Filter by resident memory
    filter = {};
    filter.minResidentMemoryKB = kMinResidentMemoryKB;
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kMyAppPid);

    // Filter by executable path
    filter = {};
    filter.executablePathContains = "kthreadd";
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kKthreaddPid);

    // Filter by cmdline
    filter = {};
    filter.cmdlineContains = "config";
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kMyAppPid);
}

TEST_F(ProcessAnalyzerTest, QueryProcessesSorting) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Sort by PID descending
    auto results = analyzer.queryProcesses({}, ProcessSortField::PID, SortOrder::DESC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid);
    EXPECT_EQ(results[3].pid, kInitPid);

    // Sort by RSS ascending
    results = analyzer.queryProcesses({}, ProcessSortField::RSS, SortOrder::ASC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].residentMemory, 0); // pid 2 or 4
    EXPECT_EQ(results[3].pid, kMyAppPid); // pid 3 has most RSS

    // Sort by total CPU Time ascending (new in Iteration 7)
    // PID 4: 0, PID 1: 30, PID 3: 40, PID 2: 300
    results = analyzer.queryProcesses({}, ProcessSortField::CPU_TIME, SortOrder::ASC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid);
    EXPECT_EQ(results[1].pid, kInitPid);
    EXPECT_EQ(results[2].pid, kMyAppPid);
    EXPECT_EQ(results[3].pid, kKthreaddPid);

    // Sort by start time descending
    results = analyzer.queryProcesses({}, ProcessSortField::START_TIME, SortOrder::DESC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid); // pid 4 has largest start time
    EXPECT_EQ(results[3].pid, kInitPid);

    // Sort by executable path ascending
    results = analyzer.queryProcesses({}, ProcessSortField::EXECUTABLE_PATH, SortOrder::ASC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid); // Empty path comes first
    EXPECT_EQ(results[1].pid, kInitPid);
}

TEST_F(ProcessAnalyzerTest, GetOpenFileDescriptors) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Setup file descriptors for PID 3
    constexpr int kLogFd = 15;
    mockProc->createProcFdLink(kMyAppPid, 0, "/dev/stdin");
    mockProc->createProcFdLink(kMyAppPid, 1, "/dev/stdout");
    mockProc->createProcFdLink(kMyAppPid, 2, "/dev/stderr");
    mockProc->createProcFdLink(kMyAppPid, kLogFd, "/var/log/my-app.log");

    auto fds = analyzer.getOpenFileDescriptors(kMyAppPid);
    ASSERT_EQ(fds.size(), 4);
    EXPECT_EQ(fds[0], "/dev/stdin");
    EXPECT_EQ(fds[1], "/dev/stdout");
    EXPECT_EQ(fds[2], "/dev/stderr");
    EXPECT_EQ(fds[kLogFd], "/var/log/my-app.log");

    // Test for a process with no FDs (other than the directory itself)
    mockProc->createDirectoryAt(fs::path("2") / "fd");
    auto fds2 = analyzer.getOpenFileDescriptors(kKthreaddPid);
    EXPECT_TRUE(fds2.empty());
}

TEST_F(ProcessAnalyzerTest, GetOpenFileDescriptorsForProcessWithNoAccess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    // Process 999 doesn't exist, so its /proc/999/fd dir won't be found.
    // This simulates a case where we can't access the fd directory.
    auto fds = analyzer.getOpenFileDescriptors(kNonExistentPid);
    EXPECT_TRUE(fds.empty());
}

TEST_F(ProcessAnalyzerTest, GetSystemClockTicks) {
    // This test uses the real sysconf, not a mock
    long ticks = ProcessAnalyzer::getSystemClockTicksPerSecond();
    ASSERT_GT(ticks, 0);
}

TEST_F(ProcessAnalyzerTest, GetParentProcess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto parentOpt = analyzer.getParentProcess(kMyAppPid);
    ASSERT_TRUE(parentOpt.has_value());
    EXPECT_EQ(parentOpt.value().pid, kInitPid); // NOLINT(bugprone-unchecked-optional-access)

    // Test for pid 1 (parent is 0)
    parentOpt = analyzer.getParentProcess(kInitPid);
    ASSERT_FALSE(parentOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, GetAllDescendantProcesses) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto descendants = analyzer.getAllDescendantProcesses(kInitPid);
    ASSERT_EQ(descendants.size(), 3); // pids 2, 3, 4

    // Find a leaf node
    descendants = analyzer.getAllDescendantProcesses(kZombiePid);
    ASSERT_TRUE(descendants.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessEnvironment) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto env = analyzer.getProcessEnvironment(kMyAppPid);
    ASSERT_EQ(env.size(), 2);
    EXPECT_EQ(env[0], "PATH=/usr/bin");
    EXPECT_EQ(env[1], "USER=testuser");

    // Process with no environment
    auto env2 = analyzer.getProcessEnvironment(kKthreaddPid);
    ASSERT_TRUE(env2.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessCpuUsage) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Update files to simulate work
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(kUpdateFileDelayMs));
        mockProc->createProcFile(kMyAppPid, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 35 45 0 0 15 0 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createFile("stat", "cpu  1050 200 850 5000 100 0 100 0 0 0\n");
    });

    auto usageOpt = analyzer.getProcessCpuUsage(kMyAppPid, std::chrono::milliseconds(kCpuUsageSampleTimeMs));
    t.join();

    ASSERT_TRUE(usageOpt.has_value());
    // Initial process ticks: 15+25=40. Final: 35+45=80. Delta: 40
    // Initial system ticks: 1000+200+800+5000+100+50=7150. Final: 1050+200+850+5000+100+100=7300. Delta: 150
    // Usage: 100.0 * 40 / 150 = 26.66%
    constexpr double expectedUsage = 100.0 * 40.0 / 150.0;
    ASSERT_TRUE(usageOpt.has_value());
    EXPECT_NEAR(usageOpt.value().cpuPercentage, expectedUsage, 1.0); // NOLINT(bugprone-unchecked-optional-access)
}
