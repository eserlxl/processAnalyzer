// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/test.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>

namespace {
    namespace fs = std::filesystem;
    using namespace std::chrono_literals;

    constexpr int numThreads = 8;
    constexpr int callsPerThread = 10;
    constexpr int testPid = 789;
    constexpr auto sampleDuration = 10ms;
}

class ProcessAnalyzerThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_thread_safety_test");
        setupMockFiles();
    }

    void setupMockFiles() {
        mockProc->createFile("stat", "cpu  10000 0 5000 80000 1000 500 1500 0\n");
        mockProc->createProcFile(testPid, "status", "Name:\ttest_thread\nPPid:\t1\nUid:\t1000\n");
        mockProc->createProcFile(testPid, "stat", "789 (test_thread) R 1 789 789 0 -1 0 0 0 0 0 100 50 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(testPid, "io", "read_bytes: 1024\nwrite_bytes: 512\n");
    }

    std::unique_ptr<MockProc> mockProc;
};

// This test is designed to be run with the ThreadSanitizer (-fsanitize=thread).
// It calls various const methods that perform calculations from multiple threads
// concurrently on the same ProcessAnalyzer instance.
// The refactored design removed mutable state, so this test should pass without
// any data race warnings from TSAN.
TEST_F(ProcessAnalyzerThreadSafetyTest, ConcurrentConstMethodCalls) {
    ProcessAnalyzer analyzer(fs::path(mockProc->getPath()));
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&analyzer]() {
            for (int j = 0; j < callsPerThread; ++j) {
                // These calls read system/process stats, sleep, and then read again.
                // In the old design, they modified mutable members, causing data races.
                // In the new design, all state is local to the function call,
                // making them thread-safe.
                (void)analyzer.getProcessCpuUsage(testPid, sampleDuration);
                (void)analyzer.getProcessDiskIoUsage(testPid, sampleDuration);
                (void)analyzer.getSystemCpuUsage(sampleDuration);
                (void)analyzer.getPerCpuUsage(sampleDuration);

                // Simple read-only methods for good measure
                (void)analyzer.getProcessDetails(testPid);
                (void)analyzer.getSystemMemoryInfo();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // The test passes if it completes without crashing and if TSAN reports no data races.
    SUCCEED();
}
