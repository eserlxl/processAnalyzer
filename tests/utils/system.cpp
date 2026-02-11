// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/system.h"
#include "utils/string.h"
#include "utils/types.h"
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
    const size_t kLongStringLength = 10000;
    std::string longStr(kLongStringLength, 'a');
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

TEST(SystemExecuteCommandTest, commandWithEmbeddedNull) {
    const std::string commandWithNull("echo ok\0echo bad", 16);
    auto result = utils::executeCommand(commandWithNull);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST_F(SystemEnvTest, invalidEnvVariableName) {
    auto getResult = utils::getEnv("");
    ASSERT_FALSE(getResult.has_value());
    EXPECT_EQ(getResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    auto setResult = utils::setEnv("BAD=NAME", "value");
    ASSERT_FALSE(setResult.has_value());
    EXPECT_EQ(setResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    auto unsetResult = utils::unsetEnv("BAD=NAME");
    ASSERT_FALSE(unsetResult.has_value());
    EXPECT_EQ(unsetResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
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
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
    EXPECT_TRUE(std::filesystem::exists(*result));
}

TEST(SystemStubTest, setCurrentWorkingDirectory) {
    const auto original = std::filesystem::current_path();
    const auto target = std::filesystem::temp_directory_path();
    auto result = utils::setCurrentWorkingDirectory(target);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::filesystem::current_path(), target);
    std::filesystem::current_path(original);
}
