// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <algorithm>
#include <filesystem>
#include <memory>
#include <vector>

class StreamTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kPidA = 10;
    static constexpr int kPidB = 20;

    StreamTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_stream_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPidA).withName("alpha").withParent(1).create();
        mockProc->buildProcess(kPidB).withName("bravo").withParent(1).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(StreamTest, StreamProcessesYieldsBuiltProcesses) {
    std::vector<pid_t> pids;
    for (const auto& info : analyzer.streamProcesses()) {
        pids.push_back(info.pid);
    }
    std::ranges::sort(pids);
    EXPECT_EQ(pids, (std::vector<pid_t>{kPidA, kPidB}));
}

// A filesystem error (procPath is a regular file, not a directory) must end
// the stream gracefully rather than throwing out of the coroutine.
TEST_F(StreamTest, StreamProcessesOnNonDirectoryYieldsNothingWithoutThrowing) {
    mockProc->createFileAt("not_a_directory", "x");
    const std::filesystem::path filePath = std::filesystem::path(mockProc->getPath()) / "not_a_directory";
    analyzer.setProcPath(filePath);

    std::vector<pid_t> pids;
    EXPECT_NO_THROW({
        for (const auto& info : analyzer.streamProcesses()) {
            pids.push_back(info.pid);
        }
    });
    EXPECT_TRUE(pids.empty());
}

// streamPids() documents that it yields PIDs in ascending order regardless of
// directory enumeration order. Collect the stream WITHOUT re-sorting so the
// assertion fails if streamPids()'s internal std::ranges::sort is removed.
TEST_F(StreamTest, StreamPidsYieldsAscendingOrder) {
    // SetUp() already built kPidA(10) and kPidB(20); add more in non-ascending
    // creation order so directory enumeration is unlikely to be ascending by luck.
    constexpr int kPidHigh = 35;
    constexpr int kPidLow = 5;
    constexpr int kPidMidHigh = 25;
    constexpr int kPidMidLow = 15;
    mockProc->buildProcess(kPidHigh).withName("p35").withParent(1).create();
    mockProc->buildProcess(kPidLow).withName("p05").withParent(1).create();
    mockProc->buildProcess(kPidMidHigh).withName("p25").withParent(1).create();
    mockProc->buildProcess(kPidMidLow).withName("p15").withParent(1).create();

    std::vector<int> pids;
    for (int pid : analyzer.streamPids()) {
        pids.push_back(pid);
    }

    EXPECT_EQ(pids, (std::vector<int>{kPidLow, kPidA, kPidMidLow, kPidB, kPidMidHigh, kPidHigh}));
    EXPECT_TRUE(std::ranges::is_sorted(pids));
}

// A non-directory procPath must end streamPids() gracefully, mirroring the
// streamProcesses() guarantee, rather than throwing out of the coroutine.
TEST_F(StreamTest, StreamPidsOnNonDirectoryYieldsNothingWithoutThrowing) {
    mockProc->createFileAt("not_a_directory_pids", "x");
    const std::filesystem::path filePath = std::filesystem::path(mockProc->getPath()) / "not_a_directory_pids";
    analyzer.setProcPath(filePath);

    std::vector<int> pids;
    EXPECT_NO_THROW({
        for (int pid : analyzer.streamPids()) {
            pids.push_back(pid);
        }
    });
    EXPECT_TRUE(pids.empty());
}
