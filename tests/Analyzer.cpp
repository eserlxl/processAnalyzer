// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/Analyzer.h"
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
        mockProc->createProcFile(kInitPid, "maps", "00400000-00452000 r-xp 00000000 08:01 12345 /sbin/init\n");
        mockProc->createProcFile(kInitPid, "limits", "Limit                     Soft Limit           Hard Limit           Units     \nMax cpu time              unlimited            unlimited            seconds   \n");
        mockProc->createProcFile(kInitPid, "cgroup", "1:cpu,cpuacct:/\n");

        // PID 2: a child of PID 1
        mockProc->createProcFile(kKthreaddPid, "status", "Name:\tkthreadd\nState:\tS (sleeping)\nPPid:\t1\nUid:\t0\t0\t0\t0\nThreads:\t5\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(kKthreaddPid, "stat", "2 (kthreadd) S 1 2 2 0 -1 8388608 0 0 0 0 100 200 0 0 18 0 1 0 54321 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(kKthreaddPid, "cmdline", "");
        mockProc->createSymlink(kKthreaddPid, "exe", "/usr/bin/kthreadd");
        mockProc->createProcFile(kKthreaddPid, "io", "rchar: 1234\nwchar: 5678\n");
        mockProc->createProcFile(kKthreaddPid, "environ", "");


        // PID 3: another process, child of PID 1, running state
        mockProc->createProcFile(kMyAppPid, "status", "Name:\tmy-app\nState:\tR (running)\nPPid:\t1\nUid:\t1000\t1000\t1000\t1000\nThreads:\t10\nVmRSS:\t50000 kB\nVmSize:\t100000 kB\n");
        mockProc->createProcFile(kMyAppPid, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 15 25 0 0 20 15 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(kMyAppPid, "cmdline", std::string("/usr/bin/my-app\0--config\0/etc/my-app.conf", kMyAppCmdlineSize));
        mockProc->createProcFile(kMyAppPid, "environ", std::string("PATH=/usr/bin\0USER=testuser\0", kMyAppEnvSize));
        mockProc->createSymlink(kMyAppPid, "exe", "/usr/bin/my-app");
        mockProc->createSymlink(kMyAppPid, "cwd", "/home/testuser");

        // PID 4: a zombie process, child of PID 3
        mockProc->createProcFile(kZombiePid, "status", "Name:\tdefunct\nState:\tZ (zombie)\nPPid:\t3\nUid:\t1000\t1000\t1000\t1000\nThreads:\t0\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(kZombiePid, "stat", "4 (defunct) Z 3 4 4 0 -1 4202752 0 0 0 0 0 0 0 0 0 0 1 0 98765 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

        // Stat file for CPU usage tests
        mockProc->createFile("stat", "cpu  1000 200 800 5000 100 0 50 0\ncpu0 500 100 400 2500 50 0 25 0\nintr 12345\nctxt 6789\nprocesses 10000\n");
        // meminfo for system memory tests
        mockProc->createFile("meminfo", "MemTotal:       16384000 kB\nMemFree:         8192000 kB\nMemAvailable:   10240000 kB\nBuffers:          512000 kB\nCached:           2048000 kB\nSwapTotal:       8192000 kB\nSwapFree:        4096000 kB\n");
        // loadavg for system load tests
        mockProc->createFile("loadavg", "0.50 1.20 1.50 1/123 4567\n");
        // diskstats
        mockProc->createFile("diskstats", "   8       0 sda 100 10 1000 50 50 5 500 30 0 80 0\n");
        // net/dev
        mockProc->createDirectoryAt("net");
        mockProc->createFile("net/dev", "Inter-|   Receive                                                |  Transmit\n face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n  eth0: 100000      100    0    0    0     0          0         0   200000      200    0    0    0     0       0          0\n");
    }

    MockProc* mockProc;
};

TEST_F(ProcessAnalyzerTest, GetPidsWithMock) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto pidsResult = analyzer.getPids();
    ASSERT_TRUE(pidsResult.has_value());
    auto pids = *pidsResult;
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
    const auto& info = infoOpt.value(); 
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
    EXPECT_EQ(infoOpt.error(), utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsForZombieProcess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(kZombiePid);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = infoOpt.value();
    EXPECT_EQ(info.pid, kZombiePid);
    EXPECT_EQ(info.ppid, kMyAppPid);
    EXPECT_EQ(info.name, "defunct");
    EXPECT_EQ(info.state.substr(0,1), "Z");
    EXPECT_EQ(info.residentMemory, 0);
    EXPECT_EQ(info.executablePath, "[unreadable]");
    EXPECT_EQ(info.currentWorkingDirectory, "[unreadable]");
    EXPECT_EQ(info.cmdline, "[unreadable]");
    EXPECT_TRUE(info.environmentVariables.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesIOAndStatMissingGracefully) {
    mockProc->createPidDir(kNoStatPid);
    mockProc->createProcFile(kNoStatPid, "status", "Name:\tno-stat-io\nPPid:\t1\nUid:\t1000\n");
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(kNoStatPid);
    ASSERT_TRUE(infoOpt.has_value());
    EXPECT_EQ(infoOpt.value().ioReadBytes, 0); 
    EXPECT_EQ(infoOpt.value().cpuUserTimeTicks, 0); 
    EXPECT_EQ(infoOpt.value().name, "no-stat-io"); 
}

TEST_F(ProcessAnalyzerTest, GetSystemMemoryInfo) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto memInfoOpt = analyzer.getSystemMemoryInfo();
    ASSERT_TRUE(memInfoOpt.has_value());
    const auto& memInfo = memInfoOpt.value(); 
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
    EXPECT_EQ(memInfoFailOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(ProcessAnalyzerTest, GetSystemLoadAverage) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto loadAvgOpt = analyzer.getSystemLoadAverage();
    ASSERT_TRUE(loadAvgOpt.has_value());
    const auto& loadAvg = loadAvgOpt.value(); 
    EXPECT_DOUBLE_EQ(loadAvg.oneMin, 0.50);
    EXPECT_DOUBLE_EQ(loadAvg.fiveMin, 1.20);
    EXPECT_DOUBLE_EQ(loadAvg.fifteenMin, 1.50);

    // Test failure case
    ProcessAnalyzer analyzerNoFile("nonexistent_path");
    auto loadAvgFailOpt = analyzerNoFile.getSystemLoadAverage();
    ASSERT_FALSE(loadAvgFailOpt.has_value());
    EXPECT_EQ(loadAvgFailOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(ProcessAnalyzerTest, GetSystemCpuStats) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto cpuStatsOpt = analyzer.getSystemCpuStats();
    ASSERT_TRUE(cpuStatsOpt.has_value());
    const auto& cpuStats = cpuStatsOpt.value();
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
    EXPECT_EQ(cpuStatsFailOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(ProcessAnalyzerTest, QueryProcessesFiltering) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    ProcessFilter filter;

    // Filter by name
    filter = ProcessFilter{};
    filter.nameContains = "kthread";
    auto resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    auto results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kKthreaddPid);

    // Filter by parent PID (new in Iteration 7)
    filter = ProcessFilter{};
    filter.ppidFilter = kInitPid;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 2);
    EXPECT_TRUE((results[0].pid == kKthreaddPid && results[1].pid == kMyAppPid) || (results[0].pid == kMyAppPid && results[1].pid == kKthreaddPid));

    // Filter by state
    filter = ProcessFilter{};
    filter.stateFilter = 'Z';
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kZombiePid);

    // Filter by UID
    filter = ProcessFilter{};
    filter.uidFilter = kTestUserUid;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 2); // PIDs 3 and 4

    // Filter by resident memory
    filter = ProcessFilter{};
    filter.minResidentMemoryKB = kMinResidentMemoryKB;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kMyAppPid);

    // Filter by executable path
    filter = ProcessFilter{};
    filter.executablePathContains = "kthreadd";
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kKthreaddPid);

    // Filter by cmdline
    filter = ProcessFilter{};
    filter.cmdlineContains = "config";
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kMyAppPid);
}

TEST_F(ProcessAnalyzerTest, QueryProcessesSorting) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Sort by PID descending
    auto resultsResult = analyzer.queryProcesses({}, ProcessSortField::pid, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    auto results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid);
    EXPECT_EQ(results[3].pid, kInitPid);

    // Sort by RSS ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::rss, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].residentMemory, 0); // pid 2 or 4
    EXPECT_EQ(results[3].pid, kMyAppPid); // pid 3 has most RSS

    // Sort by total CPU Time ascending (new in Iteration 7)
    // PID 4: 0, PID 1: 30, PID 3: 40, PID 2: 300
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::cpuTime, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid);
    EXPECT_EQ(results[1].pid, kInitPid);
    EXPECT_EQ(results[2].pid, kMyAppPid);
    EXPECT_EQ(results[3].pid, kKthreaddPid);

    // Sort by start time descending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::startTime, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kZombiePid); // pid 4 has largest start time
    EXPECT_EQ(results[3].pid, kInitPid);

    // Sort by executable path ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::executablePath, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, kInitPid);
    EXPECT_EQ(results[1].pid, kKthreaddPid);
    EXPECT_EQ(results[2].pid, kMyAppPid);
    EXPECT_EQ(results[3].pid, kZombiePid);
}

TEST_F(ProcessAnalyzerTest, GetProcessOpenFileDetails) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Setup file descriptors for PID 3
    constexpr int kLogFd = 15;
    mockProc->createProcFdLink(kMyAppPid, 0, "/dev/stdin");
    mockProc->createProcFdLink(kMyAppPid, 1, "/dev/stdout");
    mockProc->createProcFdLink(kMyAppPid, 2, "/dev/stderr");
    mockProc->createProcFdLink(kMyAppPid, kLogFd, "/var/log/my-app.log");

    auto fdsResult = analyzer.getProcessOpenFileDetails(kMyAppPid);
    ASSERT_TRUE(fdsResult.has_value());
    const auto& fds = *fdsResult;
    ASSERT_EQ(fds.size(), 4);
    EXPECT_EQ(fds[0].path, "/dev/stdin");
    EXPECT_EQ(fds[1].path, "/dev/stdout");
    EXPECT_EQ(fds[2].path, "/dev/stderr");
    EXPECT_EQ(fds[3].path, "/var/log/my-app.log"); // Index 3 in vector, not FD 3

    // Test for a process with no FDs (other than the directory itself)
    mockProc->createDirectoryAt(fs::path("2") / "fd");
    auto fds2Result = analyzer.getProcessOpenFileDetails(kKthreaddPid);
    ASSERT_TRUE(fds2Result.has_value());
    EXPECT_TRUE(fds2Result->empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessOpenFileDetailsForProcessWithNoAccess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto fdsResult = analyzer.getProcessOpenFileDetails(kNonExistentPid);
    ASSERT_FALSE(fdsResult.has_value());
}

TEST_F(ProcessAnalyzerTest, GetSystemClockTicks) {
    // This test uses the real sysconf, not a mock
    auto ticksResult = ProcessAnalyzer::getSystemClockTicksPerSecond();
    ASSERT_TRUE(ticksResult.has_value());
    ASSERT_GT(*ticksResult, 0);
}

TEST_F(ProcessAnalyzerTest, GetParentProcess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto parentOpt = analyzer.getParentProcess(kMyAppPid);
    ASSERT_TRUE(parentOpt.has_value());
    EXPECT_EQ(parentOpt.value().pid, kInitPid); 

    // Test for pid 1 (parent is 0)
    parentOpt = analyzer.getParentProcess(kInitPid);
    ASSERT_FALSE(parentOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, GetAllDescendantProcesses) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto descendantsResult = analyzer.getAllDescendantProcesses(kInitPid);
    ASSERT_TRUE(descendantsResult.has_value());
    auto descendants = *descendantsResult;
    ASSERT_EQ(descendants.size(), 3); // pids 2, 3, 4

    // Find a leaf node
    descendantsResult = analyzer.getAllDescendantProcesses(kZombiePid);
    ASSERT_TRUE(descendantsResult.has_value());
    descendants = *descendantsResult;
    ASSERT_TRUE(descendants.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessEnvironment) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto envResult = analyzer.getProcessEnvironment(kMyAppPid);
    ASSERT_TRUE(envResult.has_value());
    const auto& env = *envResult;
    ASSERT_EQ(env.size(), 2);
    EXPECT_EQ(env[0], "PATH=/usr/bin");
    EXPECT_EQ(env[1], "USER=testuser");

    // Process with no environment
    auto env2Result = analyzer.getProcessEnvironment(kKthreaddPid);
    ASSERT_TRUE(env2Result.has_value());
    ASSERT_TRUE(env2Result->empty());
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
    constexpr double expectedUsage = 100.0 * 40.0 / 150.0;
    EXPECT_NEAR(usageOpt.value().cpuPercentage, expectedUsage, 1.0); 
}

// --- New Tests for Iteration 14 ---

TEST_F(ProcessAnalyzerTest, GetProcessMemoryMaps) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto mapsResult = analyzer.getProcessMemoryMaps(kInitPid);
    ASSERT_TRUE(mapsResult.has_value());
    const auto& maps = *mapsResult;
    ASSERT_EQ(maps.size(), 1);
    EXPECT_EQ(maps[0].startAddress, 0x00400000);
    EXPECT_EQ(maps[0].endAddress, 0x00452000);
    EXPECT_EQ(maps[0].permissions, "r-xp");
    EXPECT_EQ(maps[0].pathname, "/sbin/init");
}

TEST_F(ProcessAnalyzerTest, GetProcessResourceLimits) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto limitsResult = analyzer.getProcessResourceLimits(kInitPid);
    ASSERT_TRUE(limitsResult.has_value());
    const auto& info = *limitsResult;
    ASSERT_EQ(info.limits.size(), 1);
    EXPECT_EQ(info.limits[0].resource, "Max cpu time");
    EXPECT_EQ(info.limits[0].softLimit, "unlimited");
}

TEST_F(ProcessAnalyzerTest, GetProcessCgroupInfo) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto cgroupResult = analyzer.getProcessCgroupInfo(kInitPid);
    ASSERT_TRUE(cgroupResult.has_value());
    const auto& info = *cgroupResult;
    ASSERT_EQ(info.entries.size(), 1);
    EXPECT_EQ(info.entries[0].id, 1);
    EXPECT_EQ(info.entries[0].controllers, "cpu,cpuacct");
    EXPECT_EQ(info.entries[0].path, "/");
}

TEST_F(ProcessAnalyzerTest, GetSystemDiskIoStats) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto statsResult = analyzer.getSystemDiskIoStats();
    ASSERT_TRUE(statsResult.has_value());
    const auto& stats = *statsResult;
    ASSERT_EQ(stats.size(), 1);
    EXPECT_EQ(stats[0].deviceName, "sda");
    EXPECT_EQ(stats[0].readsCompleted, 100);
}

TEST_F(ProcessAnalyzerTest, GetNetworkInterfaceStats) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_TRUE(statsResult.has_value());
    const auto& stats = *statsResult;
    ASSERT_EQ(stats.size(), 1);
    EXPECT_EQ(stats[0].interfaceName, "eth0");
    EXPECT_EQ(stats[0].rxBytes, 100000);
    EXPECT_EQ(stats[0].txBytes, 200000);
}

TEST_F(ProcessAnalyzerTest, GetSystemActivityStats) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto statsResult = analyzer.getSystemActivityStats();
    ASSERT_TRUE(statsResult.has_value());
    const auto& stats = *statsResult;
    EXPECT_EQ(stats.interruptsTotal, 12345);
    EXPECT_EQ(stats.contextSwitches, 6789);
    EXPECT_EQ(stats.processesForked, 10000);
}

TEST_F(ProcessAnalyzerTest, GetPerCpuUsage) {
     ProcessAnalyzer analyzer(mockProc->getPath());
     // NOTE: This test depends on sleep and file modification which is tricky with mock.
     // We just test basic parsing if possible, or skip if too complex to mock dynamic /proc/stat nicely.
     // Given getPerCpuUsage sleeps, we'd need a separate thread to update the file during the sleep.
     
     std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(kUpdateFileDelayMs));
        // Update to show some usage
        // Initial: cpu  1000 200 800 5000 100 0 50 0 (Total: 7150, Idle: 5000)
        //          cpu0 500 100 400 2500 50 0 25 0
        // New:
        // cpu  1050 200 850 5000 100 0 100 0 (Total: 7300, Idle: 5000 -> Delta Total: 150, Delta Idle: 0 -> 100% usage)
        // cpu0 525 100 425 2500 50 0 50 0 (Half of total changes)
        mockProc->createFile("stat", "cpu  1050 200 850 5000 100 0 100 0\ncpu0 525 100 425 2500 50 0 50 0\n");
    });

    auto usageResult = analyzer.getPerCpuUsage(std::chrono::milliseconds(kCpuUsageSampleTimeMs));
    t.join();

    ASSERT_TRUE(usageResult.has_value());
    // We expect at least one CPU (cpu0, mapped to index 0)
    ASSERT_FALSE(usageResult->cpuUsages.empty());
    // Index 0 corresponds to cpu0 in our mock data for this specific implementation which skips "cpu" line in the helper?
    // Wait, the implementation returns vector of usages.
    // The implementation iterates lines. First is "cpu" (total), then "cpu0".
    // "cpu" -> cpuId 0?
    // My implementation: if label == "cpu", cpuId = 0. if label == "cpu0", cpuId = 1.
    // So index 0 in vector is total (cpuId 0), index 1 is cpu0 (cpuId 1).
    
    // Total usage: 100%
    EXPECT_NEAR(usageResult->cpuUsages[0].cpuPercentage, 100.0, 1.0);
}

TEST_F(ProcessAnalyzerTest, GetNetworkConnectionsIPv6) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Setup mock /proc/net/tcp6 file with space-separated hex address parts.
    // The address is ::ffff:172.16.1.1, which in hex is 00000000 00000000 0000ffff ac100101.
    // The inode is 54321.
    mockProc->createFile("net/tcp6",
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000FFFFAC100101:0050 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 54321 2 0000000000000000 100 0 0 10 0\n"
    );

    // Setup file descriptors for a mock process to link to the socket inode.
    mockProc->createProcFdLink(kMyAppPid, 10, "socket:[54321]");

    auto connectionsResult = analyzer.getNetworkConnections(kMyAppPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = *connectionsResult;

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "TCP6");
    EXPECT_EQ(connections[0].localAddress, "::ffff:172.16.1.1:80");
    EXPECT_EQ(connections[0].remoteAddress, "*");
    EXPECT_EQ(connections[0].state, "LISTEN");
}
