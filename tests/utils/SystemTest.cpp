// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/System.h"
#include "utils/String.h"
#include "utils/Types.h"

// Test `getEnv` with an empty string
TEST(SystemTest, GetEnvEmpty) {
    auto result = utils::getEnv("");
    EXPECT_FALSE(result.has_value());
}

// Test `executeCommand` with a command that prints to stderr but exits successfully
TEST(SystemTest, ExecuteCommandStderrSuccess) {
    auto result = utils::executeCommand("echo 'Error message' >&2");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().exitCode, 0);
    EXPECT_TRUE(utils::contains(result.value().stdoutStr, "Error message"));
    EXPECT_TRUE(result.value().stderrStr.empty());
}

// Test `executeCommand` with a command that prints to both stdout and stderr and fails
TEST(SystemTest, ExecuteCommandStderrFail) {
    auto result = utils::executeCommand("echo 'Output'; echo 'Error' >&2; exit 1");
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value().exitCode, 0);
    EXPECT_TRUE(utils::contains(result.value().stdoutStr, "Output"));
    EXPECT_TRUE(utils::contains(result.value().stdoutStr, "Error"));
    EXPECT_TRUE(result.value().stderrStr.empty());
}

// Test `executeCommand` with a command that generates a large amount of output
TEST(SystemTest, ExecuteCommandLargeOutput) {
    constexpr size_t kLargeOutputSize = 2048;
    std::string longStr(kLargeOutputSize, 'a');
    auto result = utils::executeCommand("echo '" + longStr + "'");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().exitCode, 0);
    EXPECT_TRUE(utils::contains(result.value().stdoutStr, longStr));
}

// Test `executeCommand` with a non-existent command
TEST(SystemTest, ExecuteCommandNonExistent) {
    auto result = utils::executeCommand("non_existent_command_xyz_123");
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result.value().exitCode, 0);
    // The shell's error message should be captured in stdoutStr
    EXPECT_FALSE(result.value().stdoutStr.empty());
}

// Test `executeCommand` with an empty command string
TEST(SystemTest, ExecuteCommandEmpty) {
    auto result = utils::executeCommand("");
    ASSERT_TRUE(result.has_value());
    // Executing an empty command now results in a shell syntax error due to the "{ ; }" wrapping,
    // so we expect a non-zero exit code.
    EXPECT_NE(result.value().exitCode, 0);
}

// Test the `setEnv` stub function
TEST(SystemTest, StubSetEnv) {
    auto result = utils::setEnv("VAR", "VALUE");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}

// Test the `unsetEnv` stub function
TEST(SystemTest, StubUnsetEnv) {
    auto result = utils::unsetEnv("VAR");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}

// Test the `getCurrentWorkingDirectory` stub function
TEST(SystemTest, StubGetCurrentWorkingDirectory) {
    auto result = utils::getCurrentWorkingDirectory();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}

// Test the `setCurrentWorkingDirectory` stub function
TEST(SystemTest, StubSetCurrentWorkingDirectory) {
    auto result = utils::setCurrentWorkingDirectory("/tmp");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}
