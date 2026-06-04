// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h"
#include "utils/types.h"

#include <csignal>
#include <filesystem>
#include <memory>
#include <unistd.h>

class SendSignalTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    SendSignalTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_control_test");
        originalProcPath = analyzer.getProcPath();
        // Use real /proc so the PID check against getpid() succeeds.
        // (checkPidPathExistsAndPermissions reads from procPath)
        analyzer.setProcPath("/proc");
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(SendSignalTest, SendsSIGCONTToSelf) {
    // SIGCONT is a safe no-op when the process is already running.
    auto result = analyzer.sendSignal(static_cast<int>(::getpid()), SIGCONT);
    EXPECT_TRUE(result.has_value());
}

TEST_F(SendSignalTest, InvalidSignalReturnsParsingError) {
    auto result = analyzer.sendSignal(static_cast<int>(::getpid()), -1);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

TEST_F(SendSignalTest, NonexistentPidReturnsError) {
    // PID 1 must exist (init/systemd); use a mock-backed nonexistent PID.
    // A PID of 0 is invalid for kill() but we need a PID path check first.
    // Point analyzer at the mock proc so checkPidPathExistsAndPermissions fails.
    analyzer.setProcPath(mockProc->getPath());
    constexpr int kAbsentPid = 99999;
    auto result = analyzer.sendSignal(kAbsentPid, SIGCONT);
    EXPECT_FALSE(result.has_value());
}
