// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "TestUtils.h"
#include <gtest/gtest.h>

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
        mockProc->createProcFile(1, "status", "Name:\tinit\nState:\tS (sleeping)\nPPid:\t0\nUid:\t0\t0\t0\t0\nThreads:\t1\nVmRSS:\t1000 kB\nVmSize:\t4000 kB\n");
        mockProc->createProcFile(1, "stat", "1 (init) S 0 1 1 0 -1 4202752 239 0 0 0 10 20 0 0 20 0 1 0 12345 4096000 1000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
    mockProc->createProcFile(1, "cmdline", std::string("systemd\0", 8));
        mockProc->createProcFile(1, "io", "rchar: 100\nwchar: 200\n");
        mockProc->createSymlink(1, "exe", "/sbin/init");
        mockProc->createSymlink(1, "cwd", "/");
        mockProc->createProcFile(1, "environ", std::string("PATH=/bin:/sbin\0HOME=/\0", 22));

        // PID 2: a child of PID 1
        mockProc->createProcFile(2, "status", "Name:\tkthreadd\nState:\tS (sleeping)\nPPid:\t1\nUid:\t0\t0\t0\t0\nThreads:\t5\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(2, "stat", "2 (kthreadd) S 1 2 2 0 -1 8388608 0 0 0 0 100 200 0 0 18 0 1 0 54321 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(2, "cmdline", "");
        mockProc->createSymlink(2, "exe", "/usr/bin/kthreadd");
        mockProc->createProcFile(2, "io", "rchar: 1234\nwchar: 5678\n");


        // PID 3: another process, child of PID 1, running state
        mockProc->createProcFile(3, "status", "Name:\tmy-app\nState:\tR (running)\nPPid:\t1\nUid:\t1000\t1000\t1000\t1000\nThreads:\t10\nVmRSS:\t50000 kB\nVmSize:\t100000 kB\n");
        mockProc->createProcFile(3, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 15 25 0 0 15 0 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(3, "cmdline", std::string("/usr/bin/my-app\0--config\0/etc/my-app.conf", 42));
        mockProc->createProcFile(3, "environ", std::string("PATH=/usr/bin\0USER=testuser\0", 28));
        mockProc->createSymlink(3, "exe", "/usr/bin/my-app");
        mockProc->createSymlink(3, "cwd", "/home/testuser");

        // PID 4: a zombie process, child of PID 3
        mockProc->createProcFile(4, "status", "Name:\tdefunct\nState:\tZ (zombie)\nPPid:\t3\nUid:\t1000\t1000\t1000\t1000\nThreads:\t0\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(4, "stat", "4 (defunct) Z 3 4 4 0 -1 4202752 0 0 0 0 0 0 0 0 0 0 1 0 98765 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

        // Stat file for CPU usage tests
        mockProc->createFile("stat", "cpu  1000 200 800 5000 100 0 50 0 0 0\n");
    }

    MockProc* mockProc;
};

TEST_F(ProcessAnalyzerTest, GetPidsWithMock) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto pids = analyzer.getPids();
    ASSERT_EQ(pids.size(), 4);
    ASSERT_NE(std::find(pids.begin(), pids.end(), 1), pids.end());
    ASSERT_NE(std::find(pids.begin(), pids.end(), 2), pids.end());
    ASSERT_NE(std::find(pids.begin(), pids.end(), 3), pids.end());
    ASSERT_NE(std::find(pids.begin(), pids.end(), 4), pids.end());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsWithMock) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(3);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = *infoOpt;
    EXPECT_EQ(info.pid, 3);
    EXPECT_EQ(info.ppid, 1);
    EXPECT_EQ(info.name, "my-app");
    EXPECT_EQ(info.state.substr(0,1), "R");
    EXPECT_EQ(info.uid, 1000);
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

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesIOAndStatMissingGracefully) {
    mockProc->createPidDir(5);
    mockProc->createProcFile(5, "status", "Name:\tno-stat-io\nPPid:\t1\nUid:\t1000\n");
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto infoOpt = analyzer.getProcessDetails(5);
    ASSERT_TRUE(infoOpt.has_value());
    EXPECT_EQ(infoOpt->ioReadBytes, 0);
    EXPECT_EQ(infoOpt->cpuUserTimeTicks, 0);
    EXPECT_EQ(infoOpt->name, "no-stat-io");
}


TEST_F(ProcessAnalyzerTest, QueryProcessesFiltering) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    ProcessFilter filter;

    // Filter by name
    filter = {};
    filter.nameContains = "kthread";
    auto results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, 2);

    // Filter by state
    filter = {};
    filter.stateFilter = 'Z';
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, 4);

    // Filter by UID
    filter = {};
    filter.uidFilter = 1000;
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 2); // PIDs 3 and 4

    // Filter by resident memory
    filter = {};
    filter.minResidentMemoryKB = 40000;
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, 3);
    
    // Filter by executable path
    filter = {};
    filter.executablePathContains = "kthreadd";
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, 2);

    // Filter by cmdline
    filter = {};
    filter.cmdlineContains = "config";
    results = analyzer.queryProcesses(filter);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, 3);
}

TEST_F(ProcessAnalyzerTest, QueryProcessesSorting) {
    ProcessAnalyzer analyzer(mockProc->getPath());

    // Sort by PID descending
    auto results = analyzer.queryProcesses({}, ProcessSortField::PID, SortOrder::DESC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, 4);
    EXPECT_EQ(results[3].pid, 1);
    
    // Sort by RSS ascending
    results = analyzer.queryProcesses({}, ProcessSortField::RSS, SortOrder::ASC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].residentMemory, 0); // pid 2 or 4
    EXPECT_EQ(results[3].pid, 3); // pid 3 has most RSS

    // Sort by start time descending
    results = analyzer.queryProcesses({}, ProcessSortField::START_TIME, SortOrder::DESC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, 4); // pid 4 has largest start time
    EXPECT_EQ(results[3].pid, 1);

    // Sort by executable path ascending
    results = analyzer.queryProcesses({}, ProcessSortField::EXECUTABLE_PATH, SortOrder::ASC);
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, 4); // Empty path comes first
    EXPECT_EQ(results[1].pid, 1);
}

TEST_F(ProcessAnalyzerTest, GetSystemClockTicks) {
    // This test uses the real sysconf, not a mock
    long ticks = ProcessAnalyzer::getSystemClockTicksPerSecond();
    ASSERT_GT(ticks, 0);
}

TEST_F(ProcessAnalyzerTest, GetParentProcess) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto parentOpt = analyzer.getParentProcess(3);
    ASSERT_TRUE(parentOpt.has_value());
    EXPECT_EQ(parentOpt->pid, 1);
    
    // Test for pid 1 (parent is 0)
    parentOpt = analyzer.getParentProcess(1);
    ASSERT_FALSE(parentOpt.has_value());
}

TEST_F(ProcessAnalyzerTest, GetAllDescendantProcesses) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto descendants = analyzer.getAllDescendantProcesses(1);
    ASSERT_EQ(descendants.size(), 3); // pids 2, 3, 4

    // Find a leaf node
    descendants = analyzer.getAllDescendantProcesses(4);
    ASSERT_TRUE(descendants.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessEnvironment) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    auto env = analyzer.getProcessEnvironment(3);
    ASSERT_EQ(env.size(), 2);
    EXPECT_EQ(env[0], "PATH=/usr/bin");
    EXPECT_EQ(env[1], "USER=testuser");

    // Process with no environment
    auto env2 = analyzer.getProcessEnvironment(2);
    ASSERT_TRUE(env2.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessCpuUsage) {
    ProcessAnalyzer analyzer(mockProc->getPath());
    
    // Update files to simulate work
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mockProc->createProcFile(3, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 35 45 0 0 15 0 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createFile("stat", "cpu  1050 200 850 5000 100 0 100 0 0 0\n");
    });

    auto usageOpt = analyzer.getProcessCpuUsage(3, std::chrono::milliseconds(50));
    t.join();

    ASSERT_TRUE(usageOpt.has_value());
    // Initial process ticks: 15+25=40. Final: 35+45=80. Delta: 40
    // Initial system ticks: 1000+200+800+5000+100+50=7150. Final: 1050+200+850+5000+100+100=7300. Delta: 150
    // Usage: 100.0 * 40 / 150 = 26.66%
    EXPECT_NEAR(usageOpt->cpuPercentage, 100.0 * 40.0 / 150.0, 1.0);
}
