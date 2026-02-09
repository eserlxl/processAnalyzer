// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/System.h"
#include "utils/String.h"
#include "utils/Types.h"
#include <cstdlib>

// --- getEnv Tests ---

class SystemEnvTest : public ::testing::Test {
protected:
    void SetUp() override {
        setenv("PROCESS_ANALYZER_TEST_VAR", "test_value", 1);
        setenv("PROCESS_ANALYZER_TEST_EMPTY_VAR", "", 1);
    }

    void TearDown() override {
        unsetenv("PROCESS_ANALYZER_TEST_VAR");
        unsetenv("PROCESS_ANALYZER_TEST_EMPTY_VAR");
    }
};

TEST_F(SystemEnvTest, getEnvDefined) {
    auto result = utils::getEnv("PROCESS_ANALYZER_TEST_VAR");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "test_value");
}

TEST_F(SystemEnvTest, getEnvUndefined) {
    auto result = utils::getEnv("PROCESS_ANALYZER_NON_EXISTENT_VAR");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::envVarNotFound));
}

TEST_F(SystemEnvTest, getEnvEmpty) {
    auto result = utils::getEnv("PROCESS_ANALYZER_TEST_EMPTY_VAR");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "");
}

// --- executeCommand Tests ---

TEST(SystemExecuteCommandTest, simpleStdout) {
    auto result = utils::executeCommand("echo 'hello world'");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stdoutStr, "hello world\n");
    EXPECT_EQ(result->stderrStr, "");
    EXPECT_EQ(result->exitCode, 0);
}

TEST(SystemExecuteCommandTest, simpleStderr) {
    auto result = utils::executeCommand("echo 'hello error' >&2");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stdoutStr, "");
    EXPECT_EQ(result->stderrStr, "hello error\n");
    EXPECT_EQ(result->exitCode, 0);
}

TEST(SystemExecuteCommandTest, bothStdoutAndStderr) {
    auto result = utils::executeCommand("echo 'hello world'; echo 'hello error' >&2");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stdoutStr, "hello world\n");
    EXPECT_EQ(result->stderrStr, "hello error\n");
    EXPECT_EQ(result->exitCode, 0);
}

TEST(SystemExecuteCommandTest, nonZeroExitCode) {
    auto result = utils::executeCommand("false");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->exitCode, 1);
}

TEST(SystemExecuteCommandTest, commandNotFound) {
    auto result = utils::executeCommand("a_very_non_existent_command_xyz");
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->exitCode, 0);
    EXPECT_FALSE(result->stderrStr.empty());
    EXPECT_TRUE(utils::contains(result->stderrStr, "not found"));
}

TEST(SystemExecuteCommandTest, longOutput) {
    // Creates a 10000 character string
    std::string longStr(10000, 'a');
    auto result = utils::executeCommand("echo '" + longStr + "'");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stdoutStr, longStr + "\n");
    EXPECT_EQ(result->exitCode, 0);
}

TEST(SystemExecuteCommandTest, noOutput) {
    auto result = utils::executeCommand("true");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stdoutStr, "");
    EXPECT_EQ(result->stderrStr, "");
    EXPECT_EQ(result->exitCode, 0);
}

TEST(SystemExecuteCommandTest, commandWithQuotesAndSpaces) {
    auto result = utils::executeCommand("echo \"'hello world' with spaces\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stdoutStr, "'hello world' with spaces\n");
    EXPECT_EQ(result->exitCode, 0);
}

TEST(SystemExecuteCommandTest, emptyCommand) {
    auto result = utils::executeCommand("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}


// --- Stub Function Tests ---

TEST(SystemStubTest, setEnv) {
    unsetenv("PROCESS_ANALYZER_SETENV_VAR");
    auto result = utils::setEnv("PROCESS_ANALYZER_SETENV_VAR", "VALUE");
    ASSERT_TRUE(result.has_value());
    auto readBack = utils::getEnv("PROCESS_ANALYZER_SETENV_VAR");
    ASSERT_TRUE(readBack.has_value());
    EXPECT_EQ(*readBack, "VALUE");
    unsetenv("PROCESS_ANALYZER_SETENV_VAR");
}

TEST(SystemStubTest, unsetEnv) {
    setenv("PROCESS_ANALYZER_UNSETENV_VAR", "VALUE", 1);
    auto result = utils::unsetEnv("PROCESS_ANALYZER_UNSETENV_VAR");
    ASSERT_TRUE(result.has_value());
    auto readBack = utils::getEnv("PROCESS_ANALYZER_UNSETENV_VAR");
    ASSERT_FALSE(readBack.has_value());
    EXPECT_EQ(readBack.error(), utils::make_error_code(utils::UtilsError::envVarNotFound));
}

TEST(SystemStubTest, getCurrentWorkingDirectory) {
    auto result = utils::getCurrentWorkingDirectory();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}

TEST(SystemStubTest, setCurrentWorkingDirectory) {
    auto result = utils::setCurrentWorkingDirectory("/tmp");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}
