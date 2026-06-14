// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h"
#include "utils/types.h"

#include <algorithm>
#include <filesystem>
#include <vector>

// Direct coverage for ProcessAnalyzer::getPids() — success path, fileNotFound error
// path, and the filesystem-error catch block mapping to analyzerSystemError.

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

TEST(PidsTest, ReturnsSystemErrorWhenProcPathIsNotDirectory) {
    MockProc mockProc("mock_proc_pids_not_a_dir");
    // procPath exists but is a regular file, so the fs::exists guard passes and
    // directory_iterator throws fs::filesystem_error with not_a_directory (not
    // permission_denied), exercising the catch block's analyzerSystemError branch.
    mockProc.createFileAt("not_a_dir", "x");
    const std::filesystem::path filePath =
        std::filesystem::path(mockProc.getPath()) / "not_a_dir";
    ProcessAnalyzer analyzer(filePath);

    auto result = analyzer.getPids();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::analyzerSystemError));
}
