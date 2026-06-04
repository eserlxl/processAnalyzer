// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h"
#include "utils/types.h"

#include <sys/resource.h>
#include <filesystem>
#include <memory>
#include <unistd.h>

class SetProcessPriorityTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;
    int originalNice = 0;

    SetProcessPriorityTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_perf_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath("/proc");
        errno = 0;
        originalNice = ::getpriority(PRIO_PROCESS, static_cast<id_t>(::getpid()));
    }

    void TearDown() override {
        ::setpriority(PRIO_PROCESS, static_cast<id_t>(::getpid()), originalNice);
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(SetProcessPriorityTest, SetsPriorityOnSelf) {
    // Raising nice value (lower priority) is always permitted.
    constexpr int kHighNice = 5;
    auto result = analyzer.setProcessPriority(static_cast<int>(::getpid()), kHighNice);
    if (!result.has_value() &&
        result.error() == utils::make_error_code(utils::UtilsError::analyzerPermissionDenied)) {
        GTEST_SKIP() << "setpriority not permitted in this environment";
    }
    EXPECT_TRUE(result.has_value());
}

TEST_F(SetProcessPriorityTest, NonexistentPidReturnsError) {
    analyzer.setProcPath(mockProc->getPath());
    constexpr int kAbsentPid = 99999;
    auto result = analyzer.setProcessPriority(kAbsentPid, 0);
    EXPECT_FALSE(result.has_value());
}
