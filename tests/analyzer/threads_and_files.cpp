// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {
constexpr int kPid = 5000;
constexpr int kWorkerTid = 5001;
constexpr int kAbsentPid = 9999;
} // namespace

class GetProcessThreadsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    GetProcessThreadsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_threads_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPid).withName("main").withParent(1).create();
        // addProcess does not populate /proc/<pid>/task; each thread (including
        // the main thread, whose tid equals the pid) is added explicitly.
        MockProc::AddThreadOptions mainThread;
        mainThread.name = "main";
        mockProc->addThread(kPid, kPid, mainThread);
        MockProc::AddThreadOptions worker;
        worker.name = "worker";
        mockProc->addThread(kPid, kWorkerTid, worker);
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetProcessThreadsTest, EnumeratesMainAndAddedThread) {
    auto result = analyzer.getProcessThreads(kPid);
    ASSERT_TRUE(result.has_value());
    const auto& threads = result.value();
    ASSERT_EQ(threads.size(), 2U);

    std::vector<pid_t> tids;
    std::string workerName;
    for (const auto& t : threads) {
        tids.push_back(t.tid);
        if (t.tid == kWorkerTid) {
            workerName = t.name;
        }
    }
    std::ranges::sort(tids);
    EXPECT_EQ(tids, (std::vector<pid_t>{kPid, kWorkerTid}));
    EXPECT_EQ(workerName, "worker");
}

class GetOpenFilesTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    GetOpenFilesTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_files_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        // A real regular file referenced by an absolute path so the fd target
        // begins with '/' and classifies as a file (the mock root is relative).
        mockProc->createFileAt("realfile", "x");
        const std::string realFile =
            std::filesystem::absolute(std::filesystem::path(mockProc->getPath()) / "realfile").string();

        mockProc->buildProcess(kPid).withName("fileproc").withParent(1)
            .withFd(0, "socket:[12345]")
            .withFd(1, "pipe:[67890]")
            .withFd(2, "anon_inode:[eventfd]")
            .withFd(3, realFile)
            .create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetOpenFilesTest, ClassifiesFdTypes) {
    auto result = analyzer.getProcessOpenFileDetails(kPid);
    ASSERT_TRUE(result.has_value());

    std::map<int, OpenFileType> typeByFd;
    for (const auto& fdInfo : result.value()) {
        typeByFd[fdInfo.fd] = fdInfo.type;
    }
    ASSERT_EQ(typeByFd.size(), 4U);
    EXPECT_EQ(typeByFd[0], OpenFileType::socket);
    EXPECT_EQ(typeByFd[1], OpenFileType::pipe);
    EXPECT_EQ(typeByFd[2], OpenFileType::anonInode);
    EXPECT_EQ(typeByFd[3], OpenFileType::file);
}

TEST_F(GetOpenFilesTest, AbsentFdDirReturnsError) {
    auto result = analyzer.getProcessOpenFileDetails(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}
