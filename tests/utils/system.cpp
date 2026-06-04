// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/system.h"
#include "utils/types.h"

#include <filesystem>
#include <string>

namespace {
constexpr const char* kTestVarName = "PROC_ANALYZER_TEST_VAR";
constexpr const char* kRoundTripVarName = "PROC_ANALYZER_ROUND_TRIP";
constexpr const char* kRoundTripValue = "hello_round_trip";
} // namespace

// ── getEnv ──────────────────────────────────────────────────────────────────

TEST(SystemUtilsTest, GetEnvRejectsEmptyName) {
    auto result = utils::getEnv("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(SystemUtilsTest, GetEnvRejectsEqualsInName) {
    auto result = utils::getEnv("A=B");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(SystemUtilsTest, GetEnvRejectsNullByteInName) {
    const std::string nameWithNull{"AB\0C", 4};
    auto result = utils::getEnv(nameWithNull);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(SystemUtilsTest, GetEnvReturnsErrorForMissingVar) {
    ::unsetenv(kTestVarName);
    auto result = utils::getEnv(kTestVarName);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::envVarNotFound));
}

TEST(SystemUtilsTest, GetEnvReturnsValueForExistingVar) {
    ::setenv(kTestVarName, "test_value_42", 1);
    auto result = utils::getEnv(kTestVarName);
    ::unsetenv(kTestVarName);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "test_value_42");
}

// ── setEnv / unsetEnv ────────────────────────────────────────────────────────

TEST(SystemUtilsTest, SetEnvAndGetEnvRoundTrip) {
    auto setResult = utils::setEnv(kRoundTripVarName, kRoundTripValue);
    ASSERT_TRUE(setResult.has_value());
    auto getResult = utils::getEnv(kRoundTripVarName);
    ::unsetenv(kRoundTripVarName);
    ASSERT_TRUE(getResult.has_value());
    EXPECT_EQ(getResult.value(), kRoundTripValue);
}

TEST(SystemUtilsTest, UnsetEnvRemovesVar) {
    ::setenv(kRoundTripVarName, kRoundTripValue, 1);
    auto unsetResult = utils::unsetEnv(kRoundTripVarName);
    ASSERT_TRUE(unsetResult.has_value());
    auto getResult = utils::getEnv(kRoundTripVarName);
    EXPECT_FALSE(getResult.has_value());
}

TEST(SystemUtilsTest, SetEnvRejectsEmptyName) {
    auto result = utils::setEnv("", "value");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(SystemUtilsTest, SetEnvRejectsEqualsInName) {
    auto result = utils::setEnv("A=B", "value");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

// ── getCurrentWorkingDirectory ───────────────────────────────────────────────

TEST(SystemUtilsTest, GetCurrentWorkingDirectoryReturnsValidPath) {
    auto result = utils::getCurrentWorkingDirectory();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().is_absolute());
    EXPECT_TRUE(std::filesystem::exists(result.value()));
}

// ── executeCommand ────────────────────────────────────────────────────────────

TEST(SystemUtilsTest, ExecuteCommandRejectsEmptyCommand) {
    auto result = utils::executeCommand("");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(SystemUtilsTest, ExecuteCommandCapturesStdout) {
    auto result = utils::executeCommand("echo hello");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().exitCode, 0);
    EXPECT_NE(result.value().stdoutStr.find("hello"), std::string::npos);
}
