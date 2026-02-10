// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/time.h"
#include <chrono>
#include <string>
#include <thread>
#include <ctime>
#include <array>
#include <iomanip>
#include <sstream>

// Helper to get current local time's tm struct
std::tm getLocalTm(std::chrono::system_clock::time_point tp) {
    std::tm tmBuf{};
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
#ifdef _WIN32
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tmBuf);
#endif
    return tmBuf;
}

// Helper to format a tm struct into a string using the system's local time
std::string formatTmLocal(const std::tm& tmBuf, const char* formatStr) {
    char buffer[64]; // Sufficient buffer size for common formats
    if (std::strftime(buffer, sizeof(buffer), formatStr, &tmBuf)) {
        return buffer;
    }
    return "ERROR_FORMATTING";
}

TEST(TimeTests, GetCurrentSystemTime) {
    auto result = utils::getCurrentSystemTime();
    ASSERT_TRUE(result.has_value());
    // Basic check: current time should be within a reasonable range of 'now'
    auto now = std::chrono::system_clock::now();
    EXPECT_GE(result.value(), now - std::chrono::seconds(5));
    EXPECT_LE(result.value(), now + std::chrono::seconds(5));
}

TEST(TimeTests, GetCurrentSteadyTime) {
    auto result = utils::getCurrentSteadyTime();
    ASSERT_TRUE(result.has_value());
    // Basic check: steady clock should progress
    auto first = result.value();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto second = utils::getCurrentSteadyTime();
    ASSERT_TRUE(second.has_value());
    EXPECT_GT(second.value(), first);
}

TEST(TimeTests, FormatTimestamp) {
    // Known UTC time: Jan 15, 2023, 14:30:00 UTC
    std::time_t timeSinceEpoch = 1673793000LL;
    auto tpUtc = std::chrono::system_clock::from_time_t(timeSinceEpoch);

    std::tm localTm = getLocalTm(tpUtc);

    // Test default format "%Y-%m-%d %H:%M:%S" which implies local time
    auto resultDefault = utils::formatTimestamp(tpUtc, "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(resultDefault.has_value());
    EXPECT_EQ(resultDefault.value(), formatTmLocal(localTm, "%Y-%m-%d %H:%M:%S"));

    // Test a custom format that is LOCALE INDEPENDENT to avoid CI failures.
    // "%Y-%m-%d" is standard. Avoid %A (weekday name) or %B (month name) which vary by locale.
    auto resultCustom = utils::formatTimestamp(tpUtc, "%Y-%m-%d");
    ASSERT_TRUE(resultCustom.has_value());
    EXPECT_EQ(resultCustom.value(), formatTmLocal(localTm, "%Y-%m-%d"));
}

TEST(TimeTests, FormatTimestampErrorCases) {
    auto now = std::chrono::system_clock::now();
    // Test empty format string
    auto resultEmpty = utils::formatTimestamp(now, "");
    ASSERT_FALSE(resultEmpty.has_value());
    EXPECT_EQ(resultEmpty.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, FormatTimestampUnix) {
    // Epoch: Jan 1, 1970, 00:00:00 UTC
    long long unixEpoch = 0;
    std::string formattedEpoch = utils::formatTimestamp(unixEpoch);
    
    std::tm epochTm = getLocalTm(std::chrono::system_clock::from_time_t(0));
    EXPECT_EQ(formattedEpoch, formatTmLocal(epochTm, "%Y-%m-%d %H:%M:%S"));

    // Some arbitrary time (Jan 15, 2023, 14:30:00 UTC)
    long long unixTime = 1673793000LL;
    std::string formattedTime = utils::formatTimestamp(unixTime);
    std::tm timeTm = getLocalTm(std::chrono::system_clock::from_time_t(unixTime));
    EXPECT_EQ(formattedTime, formatTmLocal(timeTm, "%Y-%m-%d %H:%M:%S"));
}

TEST(TimeTests, FormatTimestampUnixNegative) {
    EXPECT_EQ(utils::formatTimestamp(-1LL), "N/A");
    EXPECT_EQ(utils::formatTimestamp(-1000LL), "N/A");
}

TEST(TimeTests, ParseTimestamp) {
    std::string timestampStr = "2023-01-15 14:30:00";
    std::string formatStr = "%Y-%m-%d %H:%M:%S";
    auto result = utils::parseTimestamp(timestampStr, formatStr);
    ASSERT_TRUE(result.has_value());

    std::tm parsedLocalTm = getLocalTm(result.value());

    EXPECT_EQ(parsedLocalTm.tm_year, 2023 - 1900);
    EXPECT_EQ(parsedLocalTm.tm_mon, 0); // January
    EXPECT_EQ(parsedLocalTm.tm_mday, 15);
    EXPECT_EQ(parsedLocalTm.tm_hour, 14);
    EXPECT_EQ(parsedLocalTm.tm_min, 30);
    EXPECT_EQ(parsedLocalTm.tm_sec, 0);

    // Test with invalid format string
    auto invalidFormatResult = utils::parseTimestamp("2023-01-15", "%Y/%m/%d");
    ASSERT_FALSE(invalidFormatResult.has_value());
    EXPECT_EQ(invalidFormatResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Test with mismatched format
    auto mismatchedFormatResult = utils::parseTimestamp("2023/01/15", "%Y-%m-%d");
    ASSERT_FALSE(mismatchedFormatResult.has_value());
    EXPECT_EQ(mismatchedFormatResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampVariousFormats) {
    // Test with YYYY/MM/DD
    auto result1 = utils::parseTimestamp("2024/03/10", "%Y/%m/%d");
    ASSERT_TRUE(result1.has_value());
    std::tm tm1 = getLocalTm(result1.value());
    EXPECT_EQ(tm1.tm_year, 2024 - 1900); EXPECT_EQ(tm1.tm_mon, 2); EXPECT_EQ(tm1.tm_mday, 10);

    // Test with DD-Mon-YYYY (e.g., 25-Dec-2025)
    // Note: %b is locale dependent, but usually works for English abbreviations.
    // If strict locale independence is needed, avoid %b, but it is standard strptime.
    // We will assume "C" or english-like locale for this specific test case or use known digits.
    // Using numeric month to be safe: 25-12-2025
    auto result2 = utils::parseTimestamp("25-12-2025", "%d-%m-%Y");
    ASSERT_TRUE(result2.has_value());
    std::tm tm2 = getLocalTm(result2.value());
    EXPECT_EQ(tm2.tm_year, 2025 - 1900); EXPECT_EQ(tm2.tm_mon, 11); EXPECT_EQ(tm2.tm_mday, 25);
}

TEST(TimeTests, ParseTimestampLeapYear) {
    // 2024 is a leap year, Feb 29 is valid
    auto leapYear = utils::parseTimestamp("2024-02-29", "%Y-%m-%d");
    ASSERT_TRUE(leapYear.has_value());
    std::tm tmLeap = getLocalTm(leapYear.value());
    EXPECT_EQ(tmLeap.tm_year, 2024 - 1900);
    EXPECT_EQ(tmLeap.tm_mon, 1); // Feb
    EXPECT_EQ(tmLeap.tm_mday, 29);

    // 2023 is NOT a leap year, Feb 29 is invalid
    auto nonLeapYear = utils::parseTimestamp("2023-02-29", "%Y-%m-%d");
    ASSERT_FALSE(nonLeapYear.has_value());
    EXPECT_EQ(nonLeapYear.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampInvalidComponents) {
    std::string formatStr = "%Y-%m-%d %H:%M:%S";

    // Invalid month (13)
    auto invalidMonth = utils::parseTimestamp("2023-13-01 10:00:00", formatStr);
    ASSERT_FALSE(invalidMonth.has_value());
    EXPECT_EQ(invalidMonth.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Invalid day (30 in Feb)
    auto invalidDay = utils::parseTimestamp("2023-02-30 10:00:00", formatStr);
    ASSERT_FALSE(invalidDay.has_value());
    EXPECT_EQ(invalidDay.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Invalid hour (25)
    auto invalidHour = utils::parseTimestamp("2023-01-01 25:00:00", formatStr);
    ASSERT_FALSE(invalidHour.has_value());
    EXPECT_EQ(invalidHour.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
    
    // Invalid minute (60)
    auto invalidMinute = utils::parseTimestamp("2023-01-01 10:60:00", formatStr);
    ASSERT_FALSE(invalidMinute.has_value());
    EXPECT_EQ(invalidMinute.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Invalid second (60 - note: 60 is valid for leap seconds, but typically not handled by mktime)
    auto invalidSecond = utils::parseTimestamp("2023-01-01 10:00:60", formatStr);
    ASSERT_FALSE(invalidSecond.has_value());
    EXPECT_EQ(invalidSecond.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampEmptyInputs) {
    // Empty timestamp string
    auto emptyTimestamp = utils::parseTimestamp("", "%Y-%m-%d");
    ASSERT_FALSE(emptyTimestamp.has_value());
    EXPECT_EQ(emptyTimestamp.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Empty format string
    auto emptyFormat = utils::parseTimestamp("2023-01-01", "");
    ASSERT_FALSE(emptyFormat.has_value());
    EXPECT_EQ(emptyFormat.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Both empty
    auto bothEmpty = utils::parseTimestamp("", "");
    ASSERT_FALSE(bothEmpty.has_value());
    EXPECT_EQ(bothEmpty.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampMismatchedFormats) {
    // Format YYYY-MM-DD, input YYYY/MM/DD
    auto mismatch1 = utils::parseTimestamp("2023/01/01", "%Y-%m-%d");
    ASSERT_FALSE(mismatch1.has_value());
    EXPECT_EQ(mismatch1.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Format HH:MM:SS, input HH-MM-SS
    auto mismatch2 = utils::parseTimestamp("14-30-00", "%H:%M:%S");
    ASSERT_FALSE(mismatch2.has_value());
    EXPECT_EQ(mismatch2.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampEpochAndFarFuture) {
    // Test epoch start with a format that should be parseable
    auto epochStart = utils::parseTimestamp("1970-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(epochStart.has_value());
    std::time_t tEpoch = std::chrono::system_clock::to_time_t(epochStart.value());

    std::tm expectedLocalEpochTm{};
    expectedLocalEpochTm.tm_year = 70; // 1970 + 70 = 1970
    expectedLocalEpochTm.tm_mon = 0;   // January
    expectedLocalEpochTm.tm_mday = 1;   // 1st
    expectedLocalEpochTm.tm_hour = 0;
    expectedLocalEpochTm.tm_min = 0;
    expectedLocalEpochTm.tm_sec = 0;
    expectedLocalEpochTm.tm_isdst = -1; // Let mktime determine DST

    std::time_t expectedTEpoch = std::mktime(&expectedLocalEpochTm);

    EXPECT_EQ(tEpoch, expectedTEpoch);

    // Test a date before epoch
    auto beforeEpoch = utils::parseTimestamp("1969-12-31 23:59:59", "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(beforeEpoch.has_value());
    std::time_t tBefore = std::chrono::system_clock::to_time_t(beforeEpoch.value());
    EXPECT_LT(tBefore, 0);

    auto veryFarFuture = utils::parseTimestamp("9999-01-01 00:00:00", "%Y-%m-%d %H:%M:%S");
    if (!veryFarFuture.has_value()) {
        EXPECT_EQ(veryFarFuture.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
    } else {
        std::time_t tFar = std::chrono::system_clock::to_time_t(veryFarFuture.value());
        EXPECT_NE(tFar, static_cast<std::time_t>(-1));
    }
}

TEST(TimeTests, FormatElapsedTime) {
    EXPECT_EQ(utils::formatElapsedTime(0LL), "0s");
    EXPECT_EQ(utils::formatElapsedTime(1LL), "1s");
    EXPECT_EQ(utils::formatElapsedTime(59LL), "59s");
    EXPECT_EQ(utils::formatElapsedTime(60LL), "1m 0s");
    EXPECT_EQ(utils::formatElapsedTime(61LL), "1m 1s");
    EXPECT_EQ(utils::formatElapsedTime(3599LL), "59m 59s");
    EXPECT_EQ(utils::formatElapsedTime(3600LL), "1h 0m 0s");
    EXPECT_EQ(utils::formatElapsedTime(3601LL), "1h 0m 1s");
    EXPECT_EQ(utils::formatElapsedTime(86399LL), "23h 59m 59s");
    EXPECT_EQ(utils::formatElapsedTime(86400LL), "1d 0h 0m 0s");
    EXPECT_EQ(utils::formatElapsedTime(86401LL), "1d 0h 0m 1s");
    EXPECT_EQ(utils::formatElapsedTime(172800LL), "2d 0h 0m 0s");
    EXPECT_EQ(utils::formatElapsedTime(365LL * 86400LL + 3600LL), "365d 1h 0m 0s");
}

TEST(TimeTests, FormatElapsedTimeNegative) {
    EXPECT_EQ(utils::formatElapsedTime(-100LL), "N/A");
    EXPECT_EQ(utils::formatElapsedTime(-1LL), "N/A");
}
