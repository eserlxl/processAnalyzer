// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h"
#include "utils/types.h"

#include <algorithm>
#include <filesystem>
#include <vector>

// Direct coverage for ProcessAnalyzer::getPids() — success path and fileNotFound error path.

namespace {
constexpr int kPidA = 101;
constexpr int kPidB = 202;
constexpr int kPidC = 303;
} // namespace

TEST(PidsTest, ReturnsPidListFromMockProc) {
    MockProc mockProc("mock_proc_pids_test");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.buildProcess(kPidA).withName("alpha").withParent(1).create();
    mockProc.buildProcess(kPidB).withName("beta").withParent(1).create();
    mockProc.buildProcess(kPidC).withName("gamma").withParent(1).create();

    auto result = analyzer.getPids();
    ASSERT_TRUE(result.has_value());

    auto pids = result.value();
    std::ranges::sort(pids);
    EXPECT_EQ(pids, (std::vector<int>{kPidA, kPidB, kPidC}));
}

TEST(PidsTest, ReturnsFileNotFoundWhenProcPathAbsent) {
    const std::filesystem::path absent = "/tmp/pa_pids_test_nonexistent_9f3a7c";
    ProcessAnalyzer analyzer(absent);

    auto result = analyzer.getPids();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}
