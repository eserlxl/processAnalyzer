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
