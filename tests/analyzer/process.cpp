// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/Core.h"
#include "utils/Test.h"
#include <ranges>
#include <filesystem>

namespace {
    // PIDs
    constexpr int initPid = 1;
    constexpr int kthreaddPid = 2;
    constexpr int myAppPid = 3;
    constexpr int zombiePid = 4;
    constexpr int noStatPid = 5;
    constexpr int nonExistentPid = 9999;
    
    // UIDs
    constexpr int rootUid = 0;
    constexpr int testUserUid = 1000;
}

class ProcessAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_test");
        setupMockProcFiles();
    }

    void setupMockProcFiles() {
        // PID 1: init-like process
        mockProc->createProcFile(initPid, "status", "Name:\tinit\nState:\tS (sleeping)\nPPid:\t0\nUid:\t0\t0\t0\t0\nThreads:\t1\nVmRSS:\t1000 kB\nVmSize:\t4000 kB\n");
        mockProc->createProcFile(initPid, "stat", "1 (init) S 0 1 1 0 -1 4202752 239 0 0 0 10 20 0 0 20 0 1 0 12345 4096000 1000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(initPid, "cmdline", "systemd\0");
        mockProc->createProcFile(initPid, "io", "rchar: 100\nwchar: 200\n");
        mockProc->createSymlink(initPid, "exe", "/sbin/init");
        mockProc->createSymlink(initPid, "cwd", "/");
        mockProc->createProcFile(initPid, "environ", "PATH=/bin:/sbin\0HOME=/\0");
        mockProc->createProcFile(initPid, "maps", "00400000-00452000 r-xp 00000000 08:01 12345 /sbin/init\n");
        mockProc->createProcFile(initPid, "limits", "Limit                     Soft Limit           Hard Limit           Units     \nMax cpu time              unlimited            unlimited            seconds   \n");
        mockProc->createProcFile(initPid, "cgroup", "1:cpu,cpuacct:/\n");

        // PID 2: a child of PID 1
        mockProc->createProcFile(kthreaddPid, "status", "Name:\tkthreadd\nState:\tS (sleeping)\nPPid:\t1\nUid:\t0\t0\t0\t0\nThreads:\t5\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(kthreaddPid, "stat", "2 (kthreadd) S 1 2 2 0 -1 8388608 0 0 0 0 100 200 0 0 18 0 1 0 54321 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(kthreaddPid, "cmdline", "");
        mockProc->createSymlink(kthreaddPid, "exe", "/usr/bin/kthreadd");
        mockProc->createProcFile(kthreaddPid, "io", "rchar: 1234\nwchar: 5678\n");
        mockProc->createProcFile(kthreaddPid, "environ", "");


        // PID 3: another process, child of PID 1, running state
        mockProc->createProcFile(myAppPid, "status", "Name:\tmy-app\nState:\tR (running)\nPPid:\t1\nUid:\t1000\t1000\t1000\t1000\nThreads:\t10\nVmRSS:\t50000 kB\nVmSize:\t100000 kB\n");
        mockProc->createProcFile(myAppPid, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 15 25 0 0 20 15 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(myAppPid, "cmdline", "/usr/bin/my-app\0--config\0/etc/my-app.conf");
        mockProc->createProcFile(myAppPid, "environ", "PATH=/usr/bin\0USER=testuser\0");
        mockProc->createProcFile(myAppPid, "io", "rchar: 9876\nwchar: 5432\nread_bytes: 9999\nwrite_bytes: 4444\n");
        mockProc->createSymlink(myAppPid, "exe", "/usr/bin/my-app");
        mockProc->createSymlink(myAppPid, "cwd", "/home/testuser");

        // PID 4: a zombie process, child of PID 3
        mockProc->createProcFile(zombiePid, "status", "Name:\tdefunct\nState:\tZ (zombie)\nPPid:\t3\nUid:\t1000\t1000\t1000\t1000\nThreads:\t0\nVmRSS:\t0 kB\nVmSize:\t0 kB\n");
        mockProc->createProcFile(zombiePid, "stat", "4 (defunct) Z 3 4 4 0 -1 4202752 0 0 0 0 0 0 0 0 0 0 1 0 98765 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

        // Stat file for CPU usage tests
        mockProc->createFile("stat", "cpu  1000 200 800 5000 100 0 50 0\ncpu0 500 100 400 2500 50 0 25 0\nintr 12345\nctxt 6789\nprocesses 10000\n");
        // Uptime file for boot time calculation
        mockProc->createFile("uptime", "10000.0 40000.0\n");
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

    std::unique_ptr<MockProc> mockProc;
};

TEST_F(ProcessAnalyzerTest, GetPidsWithMock) {
    // Add a non-numeric directory to ensure it's ignored
    mockProc->createDirectoryAt("not-a-pid");
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto pidsResult = analyzer.getPids();
    ASSERT_TRUE(pidsResult.has_value());
    auto pids = *pidsResult;
    ASSERT_EQ(pids.size(), 4);
    ASSERT_NE(std::ranges::find(pids, initPid), pids.end());
    ASSERT_NE(std::ranges::find(pids, kthreaddPid), pids.end());
    ASSERT_NE(std::ranges::find(pids, myAppPid), pids.end());
    ASSERT_NE(std::ranges::find(pids, zombiePid), pids.end());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsWithMock) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoOpt = analyzer.getProcessDetails(myAppPid);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = infoOpt.value(); 
    EXPECT_EQ(info.pid, myAppPid);
    EXPECT_EQ(info.ppid, initPid);
    EXPECT_EQ(info.name, "my-app");
    EXPECT_EQ(info.state.substr(0,1), "R");
    EXPECT_EQ(info.uid, testUserUid);
    EXPECT_EQ(info.threadCount, 10);
    EXPECT_EQ(info.residentMemory, 50000);
    EXPECT_EQ(info.virtualMemory, 100000);
    EXPECT_EQ(info.cmdline, "/usr/bin/my-app --config /etc/my-app.conf");
    EXPECT_EQ(info.startTimeTicks, 23456);
    EXPECT_EQ(info.cpuUserTimeTicks, 15);
    EXPECT_EQ(info.cpuKernelTimeTicks, 25);
    EXPECT_EQ(info.priority, 15);
    EXPECT_EQ(info.ioReadBytes, 9999);
    EXPECT_EQ(info.ioWriteBytes, 4444);
    EXPECT_EQ(info.executablePath, "/usr/bin/my-app");
    EXPECT_EQ(info.currentWorkingDirectory, "/home/testuser");
    ASSERT_EQ(info.environmentVariables.size(), 2);
    EXPECT_EQ(info.environmentVariables[0], "PATH=/usr/bin");
    EXPECT_EQ(info.environmentVariables[1], "USER=testuser");
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsForNonExistentPid) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoOpt = analyzer.getProcessDetails(nonExistentPid);
    ASSERT_FALSE(infoOpt.has_value());
    EXPECT_EQ(infoOpt.error(), utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsForZombieProcess) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoOpt = analyzer.getProcessDetails(zombiePid);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = infoOpt.value();
    EXPECT_EQ(info.pid, zombiePid);
    EXPECT_EQ(info.ppid, myAppPid);
    EXPECT_EQ(info.name, "defunct");
    EXPECT_EQ(info.state.substr(0,1), "Z");
    EXPECT_EQ(info.residentMemory, 0);
    EXPECT_EQ(info.executablePath, "[unreadable]");
    EXPECT_EQ(info.currentWorkingDirectory, "[unreadable]");
    EXPECT_EQ(info.cmdline, "[unreadable]");
    EXPECT_TRUE(info.environmentVariables.empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesIOAndStatMissingGracefully) {
    mockProc->createPidDir(noStatPid);
    mockProc->createProcFile(noStatPid, "status", "Name:\tno-stat-io\nPPid:\t1\nUid:\t1000\n");
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoOpt = analyzer.getProcessDetails(noStatPid);
    ASSERT_TRUE(infoOpt.has_value());
    EXPECT_EQ(infoOpt.value().ioReadBytes, 0); 
    EXPECT_EQ(infoOpt.value().cpuUserTimeTicks, 0); 
    EXPECT_EQ(infoOpt.value().name, "no-stat-io"); 
}

TEST_F(ProcessAnalyzerTest, GetSystemMemoryInfo_Success) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
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
}

TEST_F(ProcessAnalyzerTest, GetSystemMemoryInfo_FileNotFound) {
    ProcessAnalyzer analyzerNoFile(std::filesystem::path("nonexistent_path"));
    auto memInfoFailOpt = analyzerNoFile.getSystemMemoryInfo();
    ASSERT_FALSE(memInfoFailOpt.has_value());
    EXPECT_EQ(memInfoFailOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(ProcessAnalyzerTest, GetSystemLoadAverage_Success) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto loadAvgOpt = analyzer.getSystemLoadAverage();
    ASSERT_TRUE(loadAvgOpt.has_value());
    const auto& loadAvg = loadAvgOpt.value(); 
    EXPECT_DOUBLE_EQ(loadAvg.oneMin, 0.50);
    EXPECT_DOUBLE_EQ(loadAvg.fiveMin, 1.20);
    EXPECT_DOUBLE_EQ(loadAvg.fifteenMin, 1.50);
}

TEST_F(ProcessAnalyzerTest, GetSystemLoadAverage_FileNotFound) {
    ProcessAnalyzer analyzerNoFile(std::filesystem::path("nonexistent_path"));
    auto loadAvgFailOpt = analyzerNoFile.getSystemLoadAverage();
    ASSERT_FALSE(loadAvgFailOpt.has_value());
    EXPECT_EQ(loadAvgFailOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(ProcessAnalyzerTest, GetSystemLoadAverageMalformedContent) {
    mockProc->createFile("loadavg", "malformed content\n");
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto loadAvgOpt = analyzer.getSystemLoadAverage();
    ASSERT_FALSE(loadAvgOpt.has_value());
    EXPECT_EQ(loadAvgOpt.error(), utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

TEST_F(ProcessAnalyzerTest, GetSystemCpuStats_Success) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
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
}

TEST_F(ProcessAnalyzerTest, GetSystemCpuStats_FileNotFound) {
    ProcessAnalyzer analyzerNoFile(std::filesystem::path("nonexistent_path"));
    auto cpuStatsFailOpt = analyzerNoFile.getSystemCpuStats();
    ASSERT_FALSE(cpuStatsFailOpt.has_value());
    EXPECT_EQ(cpuStatsFailOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

TEST_F(ProcessAnalyzerTest, GetSystemCpuStatsMalformedContent) {
    mockProc->createFile("stat", "cpu  1000 200\n");
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto cpuStatsOpt = analyzer.getSystemCpuStats();
    ASSERT_FALSE(cpuStatsOpt.has_value());
    EXPECT_EQ(cpuStatsOpt.error(), utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesMalformedStatus) {
    constexpr int malformedPid = 10;
    // Status file is missing the UID line entirely
    mockProc->createProcFile(malformedPid, "status", "Name:\tmalformed\nState:\tS (sleeping)\nPPid:\t1\nThreads:\t1\n");
    mockProc->createProcFile(malformedPid, "stat", "10 (malformed) S 1 10 10 0 -1 4202752 239 0 0 0 10 20 0 0 20 0 1 0 12345 4096000 1000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoResult = analyzer.getProcessDetails(malformedPid);
    ASSERT_TRUE(infoResult.has_value());
    // UID is value-initialized to 0 if not parsed.
    EXPECT_EQ(infoResult->uid, 0);
    EXPECT_EQ(infoResult->name, "malformed");
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesMalformedStat) {
    constexpr int malformedPid = 11;
    mockProc->createProcFile(malformedPid, "status", "Name:\tmalformed-stat\nPPid:\t1\nUid:\t1000\n");
    // Stat file has too few fields
    mockProc->createProcFile(malformedPid, "stat", "11 (malformed-stat) S 1");

    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoResult = analyzer.getProcessDetails(malformedPid);
    ASSERT_TRUE(infoResult.has_value());
    // Fields from stat should be value-initialized (0)
    EXPECT_EQ(infoResult->cpuUserTimeTicks, 0);
    EXPECT_EQ(infoResult->startTimeTicks, 0);
    EXPECT_EQ(infoResult->name, "malformed-stat");
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesMalformedIo) {
    constexpr int malformedPid = 12;
    mockProc->createProcFile(malformedPid, "status", "Name:\tmalformed-io\nPPid:\t1\nUid:\t1000\n");
    mockProc->createProcFile(malformedPid, "stat", "12 (malformed-io) S 1 12 12 0 -1 4202752 239 0 0 0 10 20 0 0 20 0 1 0 12345 4096000 1000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
    // IO file has non-numeric values
    mockProc->createProcFile(malformedPid, "io", "read_bytes: foo\nwrite_bytes: bar\n");

    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoResult = analyzer.getProcessDetails(malformedPid);
    ASSERT_TRUE(infoResult.has_value());
    // IO bytes should be value-initialized to 0
    EXPECT_EQ(infoResult->ioReadBytes, 0);
    EXPECT_EQ(infoResult->ioWriteBytes, 0);
}

TEST_F(ProcessAnalyzerTest, GetSystemMemoryInfoHandlesMalformed) {
    // meminfo file is missing values
    mockProc->createFile("meminfo", "MemTotal: 1000 kB\nMemFree:\nMemAvailable: 2000 kB\n");
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto memInfoResult = analyzer.getSystemMemoryInfo();
    ASSERT_TRUE(memInfoResult.has_value());
    // Parsed values should be correct, missing values should be 0.
    EXPECT_EQ(memInfoResult->memTotal, 1000);
    EXPECT_EQ(memInfoResult->memFree, 0);
    EXPECT_EQ(memInfoResult->memAvailable, 2000);
    EXPECT_EQ(memInfoResult->buffers, 0);
}


TEST_F(ProcessAnalyzerTest, QueryProcessesFilteringComprehensive) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    ProcessFilter filter;
    utils::Result<std::vector<ProcessInfo>> resultsResult;
    std::vector<ProcessInfo> results;

    // Filter by nameContains
    filter = {};
    filter.nameContains = "kthread";
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, kthreaddPid);

    // Filter by nameRegex
    filter = {};
    filter.nameRegex = std::regex("my-.*");
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, myAppPid);

    // Filter by uidFilter
    filter = {};
    filter.uidFilter = testUserUid;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 2); // my-app and defunct
    EXPECT_TRUE(std::ranges::any_of(results, [](const auto& p){ return p.pid == myAppPid; }));
    EXPECT_TRUE(std::ranges::any_of(results, [](const auto& p){ return p.pid == zombiePid; }));

    // Filter by stateFilter
    filter = {};
    filter.stateFilter = 'R';
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, myAppPid);

    // Filter by min threads
    filter = {};
    filter.minThreads = 6;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, myAppPid);

    // Filter by max threads
    filter = {};
    filter.maxThreads = 4;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 2); // init and zombie
    EXPECT_TRUE(std::ranges::any_of(results, [](const auto& p){ return p.pid == initPid; }));
    EXPECT_TRUE(std::ranges::any_of(results, [](const auto& p){ return p.pid == zombiePid; }));

    // Filter by memory ranges (rss)
    filter = {};
    filter.minResidentMemoryKB = 40000;
    filter.maxResidentMemoryKB = 60000;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, myAppPid);

    // Filter by memory ranges (vmsize)
    filter = {};
    filter.minVirtualMemoryKB = 90000;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, myAppPid);

    // Filter by ppidFilter
    filter = {};
    filter.ppidFilter = initPid;
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 2); // kthreadd and my-app
    EXPECT_TRUE(std::ranges::any_of(results, [](const auto& p){ return p.pid == kthreaddPid; }));
    EXPECT_TRUE(std::ranges::any_of(results, [](const auto& p){ return p.pid == myAppPid; }));

    // Filter by cmdlineContains
    filter = {};
    filter.cmdlineContains = "my-app.conf";
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, myAppPid);

    // Filter by executablePathContains
    filter = {};
    filter.executablePathContains = "/sbin/init";
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, initPid);
    
    // Filter by multiple criteria
    filter = {};
    filter.uidFilter = testUserUid;
    filter.stateFilter = 'Z';
    resultsResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pid, zombiePid);
}

TEST_F(ProcessAnalyzerTest, QueryProcessesSorting) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    // Sort by PID descending
    auto resultsResult = analyzer.queryProcesses({}, ProcessSortField::pid, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    auto results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, zombiePid);
    EXPECT_EQ(results[3].pid, initPid);

    // Sort by RSS ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::rss, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].residentMemory, 0); // pid 2 or 4
    EXPECT_EQ(results[3].pid, myAppPid); // pid 3 has most RSS

    // Sort by total CPU Time ascending (new in Iteration 7)
    // PID 4: 0, PID 1: 30, PID 3: 40, PID 2: 300
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::cpuTime, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, zombiePid);
    EXPECT_EQ(results[1].pid, initPid);
    EXPECT_EQ(results[2].pid, myAppPid);
    EXPECT_EQ(results[3].pid, kthreaddPid);

    // Sort by start time descending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::startTime, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, zombiePid); // pid 4 has largest start time
    EXPECT_EQ(results[3].pid, initPid);

    // Sort by executable path ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::executablePath, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    EXPECT_EQ(results[0].pid, initPid);
    EXPECT_EQ(results[1].pid, kthreaddPid);
    EXPECT_EQ(results[2].pid, myAppPid);
    EXPECT_EQ(results[3].pid, zombiePid);

    // Sort by state descending and verify tie-break behavior.
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::state, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    // Expected state order: Z, S, S, R. Tie-breaker is PID ascending.
    // Expected PID order: 4 (Z), 1 (S), 2 (S), 3 (R).
    EXPECT_EQ(results[0].pid, zombiePid);
    EXPECT_EQ(results[1].pid, initPid);
    EXPECT_EQ(results[2].pid, kthreaddPid);
    EXPECT_EQ(results[3].pid, myAppPid);

    // Sort by ppid ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::ppid, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    // PPIDs: init(0), kthreadd(1), myApp(1), zombie(3)
    // PIDs: init(1), kthreadd(2), myApp(3), zombie(4)
    // Expected PID order: 1, 2, 3, 4
    EXPECT_EQ(results[0].pid, initPid);
    EXPECT_EQ(results[1].pid, kthreaddPid);
    EXPECT_EQ(results[2].pid, myAppPid);
    EXPECT_EQ(results[3].pid, zombiePid);

    // Sort by vmsize descending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::vmsize, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    // VMSizes: myApp(100000), init(4000), kthreadd(0), zombie(0)
    // PIDs: myApp(3), init(1), kthreadd(2), zombie(4)
    // Expected PID order: 3, 1, 2, 4
    EXPECT_EQ(results[0].pid, myAppPid);
    EXPECT_EQ(results[1].pid, initPid);
    EXPECT_EQ(results[2].pid, kthreaddPid);
    EXPECT_EQ(results[3].pid, zombiePid);

    // Sort by name ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::name, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    // Names: defunct, init, kthreadd, my-app
    // Expected PID order: 4, 1, 2, 3
    EXPECT_EQ(results[0].pid, zombiePid);
    EXPECT_EQ(results[1].pid, initPid);
    EXPECT_EQ(results[2].pid, kthreaddPid);
    EXPECT_EQ(results[3].pid, myAppPid);

    // Sort by cmdline ascending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::cmdline, SortOrder::asc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    // Cmdlines: "" (kthreadd), "[unreadable]" (zombie), "/usr/bin/my-app..." (myApp), "systemd" (init)
    // Expected PID order: 2, 4, 3, 1
    EXPECT_EQ(results[0].pid, kthreaddPid);
    EXPECT_EQ(results[1].pid, zombiePid);
    EXPECT_EQ(results[2].pid, myAppPid);
    EXPECT_EQ(results[3].pid, initPid);

    // Sort by uid descending
    resultsResult = analyzer.queryProcesses({}, ProcessSortField::uid, SortOrder::desc);
    ASSERT_TRUE(resultsResult.has_value());
    results = *resultsResult;
    ASSERT_EQ(results.size(), 4);
    // UIDs: myApp(1000), zombie(1000), init(0), kthreadd(0)
    // PIDs: myApp(3), zombie(4), init(1), kthreadd(2)
    // Expected PID order: 3, 4, 1, 2
    EXPECT_EQ(results[0].pid, myAppPid);
    EXPECT_EQ(results[1].pid, zombiePid);
    EXPECT_EQ(results[2].pid, initPid);
    EXPECT_EQ(results[3].pid, kthreaddPid);
}

TEST_F(ProcessAnalyzerTest, GetProcessOpenFileDetails) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    // Setup file descriptors for PID 3
    constexpr int logFd = 15;
    mockProc->createProcFdLink(myAppPid, 0, "/dev/stdin");
    mockProc->createProcFdLink(myAppPid, 1, "/dev/stdout");
    mockProc->createProcFdLink(myAppPid, 2, "/dev/stderr");
    mockProc->createProcFdLink(myAppPid, logFd, "/var/log/my-app.log");

    auto fdsResult = analyzer.getProcessOpenFileDetails(myAppPid);
    ASSERT_TRUE(fdsResult.has_value());
    auto fds = *fdsResult; // Get a mutable copy
    // Sort by FD as directory iteration order is not guaranteed.
    std::ranges::sort(fds, {}, &OpenFileDescriptorInfo::fd);

    ASSERT_EQ(fds.size(), 4);

    EXPECT_EQ(fds[0].fd, 0);
    EXPECT_EQ(fds[0].path, "/dev/stdin");

    EXPECT_EQ(fds[1].fd, 1);
    EXPECT_EQ(fds[1].path, "/dev/stdout");

    EXPECT_EQ(fds[2].fd, 2);
    EXPECT_EQ(fds[2].path, "/dev/stderr");

    EXPECT_EQ(fds[3].fd, logFd);
    EXPECT_EQ(fds[3].path, "/var/log/my-app.log");

    // Test for a process with no FDs (other than the directory itself)
    mockProc->createDirectoryAt(fs::path("2") / "fd");
    auto fds2Result = analyzer.getProcessOpenFileDetails(kthreaddPid);
    ASSERT_TRUE(fds2Result.has_value());
    EXPECT_TRUE(fds2Result->empty());
}

TEST_F(ProcessAnalyzerTest, GetProcessOpenFileDetailsEdgeCases) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    const auto fdPath = fs::path(std::to_string(myAppPid)) / "fd";

    // A regular file, should be ignored
    mockProc->createFileAt(fdPath / "not-a-symlink", "content");
    // A broken symlink, should be reported as [unreadable]
    mockProc->createSymlinkAt(fdPath / "10", "/path/to/nonexistent/target");
    // A valid symlink
    mockProc->createSymlinkAt(fdPath / "11", "/var/log/app.log");
    // A non-numeric entry, should be ignored
    mockProc->createSymlinkAt(fdPath / "not-a-number", "/dev/null");


    auto fdsResult = analyzer.getProcessOpenFileDetails(myAppPid);
    ASSERT_TRUE(fdsResult.has_value());
    auto fds = *fdsResult;
    std::ranges::sort(fds, {}, &OpenFileDescriptorInfo::fd);

    // Expect 2 entries: the broken link and the valid link.
    // The regular file and non-numeric symlink should be ignored.
    ASSERT_EQ(fds.size(), 2);

    // Broken symlink
    EXPECT_EQ(fds[0].fd, 10);
    EXPECT_EQ(fds[0].path, "[unreadable]");

    // Valid symlink
    EXPECT_EQ(fds[1].fd, 11);
    EXPECT_EQ(fds[1].path, "/var/log/app.log");
}


TEST_F(ProcessAnalyzerTest, GetProcessOpenFileDetailsForProcessWithNoAccess) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto fdsResult = analyzer.getProcessOpenFileDetails(nonExistentPid);
    ASSERT_FALSE(fdsResult.has_value());
}

TEST_F(ProcessAnalyzerTest, GetProcessOpenFileDetailsHandlesUnreadableDir) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    // Setup an fd directory for a process
    const auto fdPath = fs::path(std::to_string(myAppPid)) / "fd";
    mockProc->createDirectoryAt(fdPath);
    
    // Make the fd directory unreadable
    mockProc->setPermissions(fdPath, fs::perms::none);

    auto fdsResult = analyzer.getProcessOpenFileDetails(myAppPid);
    // The function should return an error because it cannot open the directory.
    ASSERT_FALSE(fdsResult.has_value());
    EXPECT_EQ(fdsResult.error(), utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
}

TEST_F(ProcessAnalyzerTest, GetProcessDetailsHandlesUnreadableStatus) {
    // Make the status file unreadable
    const auto statusPath = fs::path(std::to_string(myAppPid)) / "status";
    mockProc->setPermissions(statusPath, fs::perms::none);
    
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoOpt = analyzer.getProcessDetails(myAppPid);
    
    // Expect a failure because the essential status file is unreadable.
    // The underlying readTextFile will fail, and getProcessDetails interprets this as fileNotFound.
    ASSERT_FALSE(infoOpt.has_value());
    EXPECT_EQ(infoOpt.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

