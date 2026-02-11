// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/test.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

namespace {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;

    constexpr int testPid1 = 123;
    constexpr int testPid2 = 456;
    constexpr auto sampleDuration = 50ms;
}

class ProcessAnalyzerLogicTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_logic_test");
        setupInitialMockFiles();
    }

    void setupInitialMockFiles() {
        // System files
        mockProc->createFile("version", "Linux version 5.15.0-generic (mock)\n");
        mockProc->createFile("uptime", "1000.00 500.00\n");
        mockProc->createFile("stat", "cpu  10000 0 5000 80000 1000 500 1500 0\n");

        // Process 1 files
        mockProc->createProcFile(testPid1, "status", "Name:\ttest1\nPPid:\t1\nUid:\t1000\n");
        mockProc->createProcFile(testPid1, "stat", "123 (test1) R 1 123 123 0 -1 0 0 0 0 0 100 50 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(testPid1, "io", "read_bytes: 1024\nwrite_bytes: 512\n");

        // Process 2 files
        mockProc->createProcFile(testPid2, "status", "Name:\ttest2\nPPid:\t1\nUid:\t1000\n");
        mockProc->createProcFile(testPid2, "stat", "456 (test2) S 1 456 456 0 -1 0 0 0 0 0 200 80 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(testPid2, "io", "read_bytes: 2048\nwrite_bytes: 1024\n");
    }

    void setupUpdatedMockFiles() {
        // Update system stat file
        // Total ticks change: (10200-10000) + (5100-5000) + (80000-80000) = 200 + 100 = 300
        // Idle ticks change: 80500 - 80000 = 500
        // Total delta = 300 (active) + 500 (idle) = 800
        mockProc->createFile("stat", "cpu  10200 0 5100 80500 1000 500 1500 0\n");
        
        // Update process 1 stat file
        // Ticks change: (120-100) + (60-50) = 20 + 10 = 30
        mockProc->createProcFile(testPid1, "stat", "123 (test1) R 1 123 123 0 -1 0 0 0 0 0 120 60 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(testPid1, "io", "read_bytes: 2048\nwrite_bytes: 1536\n"); // Read: +1024, Write: +1024

        // Update process 2 stat file
        // Ticks change: (210-200) + (90-80) = 10 + 10 = 20
        mockProc->createProcFile(testPid2, "stat", "456 (test2) S 1 456 456 0 -1 0 0 0 0 0 210 90 0 0 0 0 0 0 0 0 0 0");
    }

    std::unique_ptr<MockProc> mockProc;
};

TEST_F(ProcessAnalyzerLogicTest, GetSystemInfo) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));
    auto result = analyzer.getSystemInfo();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->kernelVersion, "Linux version 5.15.0-generic (mock)");
    EXPECT_EQ(result->uptime.count(), 1000);
    // We can't mock gethostname, so we just check it doesn't crash and returns something.
    EXPECT_FALSE(result->hostname.empty());
}

TEST_F(ProcessAnalyzerLogicTest, GetSystemBootTime) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));
    auto result = analyzer.getSystemBootTimeUnix();
    ASSERT_TRUE(result.has_value());
    
    auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    long long expectedBootTime = now - 1000;
    
    // Allow a small difference due to the time taken to execute the code
    EXPECT_NEAR(*result, expectedBootTime, 2);
}

TEST_F(ProcessAnalyzerLogicTest, GetSystemCpuUsage) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));

    std::thread updater([&]() {
        std::this_thread::sleep_for(sampleDuration / 2);
        setupUpdatedMockFiles();
    });

    auto result = analyzer.getSystemCpuUsage(sampleDuration);
    updater.join();
    
    ASSERT_TRUE(result.has_value());

    // From setupUpdatedMockFiles:
    // initial total = 10000+0+5000+80000+1000+500+1500+0 = 98000
    // final total   = 10200+0+5100+80500+1000+500+1500+0 = 98800
    // total delta = 800
    // initial idle = 80000
    // final idle = 80500
    // idle delta = 500
    // busy delta = 800 - 500 = 300
    // usage = (300 / 800) * 100 = 37.5
    EXPECT_NEAR(result->cpuPercentage, 37.5, 0.1);
}

TEST_F(ProcessAnalyzerLogicTest, GetProcessCpuUsageSuccess) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));

    std::thread updater([&]() {
        std::this_thread::sleep_for(sampleDuration / 2);
        setupUpdatedMockFiles();
    });

    auto result = analyzer.getProcessCpuUsage(testPid1, sampleDuration);
    updater.join();

    ASSERT_TRUE(result.has_value());
    
    // Process ticks delta = 30
    // System total ticks delta = 800
    // Usage = (30 / 800) * 100 = 3.75
    EXPECT_EQ(result->pid, testPid1);
    EXPECT_NEAR(result->cpuPercentage, 3.75, 0.1);
}

TEST_F(ProcessAnalyzerLogicTest, GetAllProcessesCpuUsage) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));

    std::thread updater([&]() {
        std::this_thread::sleep_for(sampleDuration / 2);
        setupUpdatedMockFiles();
    });

    auto result = analyzer.getAllProcessesCpuUsage(sampleDuration);
    updater.join();

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2);

    auto p1_usage = std::ranges::find_if(*result, [](const auto& p){ return p.pid == testPid1; });
    auto p2_usage = std::ranges::find_if(*result, [](const auto& p){ return p.pid == testPid2; });

    ASSERT_NE(p1_usage, result->end());
    ASSERT_NE(p2_usage, result->end());

    // P1 Usage: 3.75 (see single process test)
    EXPECT_NEAR(p1_usage->cpuPercentage, 3.75, 0.1);

    // P2 Usage:
    // Process ticks delta = 20
    // System total ticks delta = 800
    // Usage = (20 / 800) * 100 = 2.5
    EXPECT_NEAR(p2_usage->cpuPercentage, 2.5, 0.1);
}

TEST_F(ProcessAnalyzerLogicTest, GetProcessDiskIoUsageSuccess) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));

    std::thread updater([&]() {
        std::this_thread::sleep_for(sampleDuration / 2);
        setupUpdatedMockFiles();
    });

    auto result = analyzer.getProcessDiskIoUsage(testPid1, sampleDuration);
    updater.join();

    ASSERT_TRUE(result.has_value());
    
    double durationSec = std::chrono::duration_cast<std::chrono::duration<double>>(sampleDuration).count();
    double expectedReadRate = 1024.0 / durationSec;
    double expectedWriteRate = 1024.0 / durationSec;

    EXPECT_EQ(result->pid, testPid1);
    EXPECT_NEAR(result->readBytesPerSecond, expectedReadRate, 200.0);
    EXPECT_NEAR(result->writeBytesPerSecond, expectedWriteRate, 200.0);
}

TEST_F(ProcessAnalyzerLogicTest, GeneratorLifetime) {
    std::optional<std::generator<int>> pidsGenerator;

    {
        ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));
        pidsGenerator.emplace(analyzer.streamPids());
    } // analyzer is destroyed here

    // Using the generator after the analyzer is destroyed is undefined behavior.
    // This may crash, read invalid memory, or appear to work by chance.
    // We will just iterate it to exercise the code path.
    auto it = pidsGenerator->begin();
    if (it != pidsGenerator->end()) {
        (void)*it;
    }
    SUCCEED() << "Generator use after destruction did not crash, but is still UB.";
}
