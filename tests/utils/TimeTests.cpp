// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Time.h"
#include <chrono>
#include <string>
#include <thread> // For std::this_thread::sleep_for
#include <ctime>  // For std::mktime, std::gmtime, struct tm

TEST(TimeTests, GetCurrentSystemTime) {
    auto result = utils::getCurrentSystemTime();
    ASSERT_TRUE(result.has_value());
    // Basic check: current time should be somewhat recent
    auto now = std::chrono::system_clock::now();
    EXPECT_GE(result.value(), now - std::chrono::seconds(1));
    EXPECT_LE(result.value(), now + std::chrono::seconds(1));
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
    std::chrono::system_clock::time_point tp;
    // Set a known time: Jan 15, 2023, 14:30:00 UTC
    std::tm t{};
    t.tm_year = 2023 - 1900; // Year since 1900
    t.tm_mon = 0;           // Month (0-11)
    t.tm_mday = 15;         // Day of the month (1-31)
    t.tm_hour = 14;         // Hour (0-23)
    t.tm_min = 30;          // Minute (0-59)
    t.tm_sec = 0;           // Second (0-59)

    // Using mktime for local time, then converting to system_clock::time_point
    std::time_t tt = std::mktime(&t);
    tp = std::chrono::system_clock::from_time_t(tt);

    // Test default format "%Y-%m-%d %H:%M:%S"
    auto resultDefault = utils::formatTimestamp(tp, "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(resultDefault.has_value());
    // Note: This test can be flaky if run in a timezone where 14:30 UTC is different from local time.
    // For a robust test, it's better to convert tp to UTC time before comparison or use a fixed UTC string.
    // For simplicity, assuming local time is UTC-like for this specific format.
    // A more robust test would explicitly specify a timezone or use only UTC formats.
    EXPECT_TRUE(resultDefault.value().find("2023-01-15 14:30:00") != std::string::npos ||
                resultDefault.value().find("2023-01-15 15:30:00") != std::string::npos || // e.g. UTC+1
                resultDefault.value().find("2023-01-15 13:30:00") != std::string::npos); // e.g. UTC-1
    

    // Test different format
    auto resultCustom = utils::formatTimestamp(tp, "%A, %B %d, %Y");
    ASSERT_TRUE(resultCustom.has_value());
    EXPECT_EQ(resultCustom.value(), "Sunday, January 15, 2023"); // Day of week might vary by locale
}

TEST(TimeTests, FormatTimestampUnix) {
    // Epoch: Jan 1, 1970, 00:00:00 UTC
    long long unixEpoch = 0;
    std::string formattedEpoch = utils::formatTimestamp(unixEpoch);
    // This will depend on the local timezone
    EXPECT_TRUE(formattedEpoch.find("1970-01-01") != std::string::npos ||
                formattedEpoch.find("1969-12-31") != std::string::npos);

    // Some arbitrary time
    long long unixTime = 1673793000; // Jan 15, 2023, 14:30:00 UTC
    std::string formattedTime = utils::formatTimestamp(unixTime);
    EXPECT_TRUE(formattedTime.find("2023-01-15") != std::string::npos);
}

TEST(TimeTests, ParseTimestamp) {
    // Test parsing a valid timestamp
    std::string timestampStr = "2023-01-15 14:30:00";
    std::string formatStr = "%Y-%m-%d %H:%M:%S";
    auto result = utils::parseTimestamp(timestampStr, formatStr);
    ASSERT_TRUE(result.has_value());

    // Convert back to check correctness (considering local timezone conversion)
    std::time_t t = std::chrono::system_clock::to_time_t(result.value());
    std::tm tm_buf;
    #ifdef _WIN32
        localtime_s(&tm_buf, &t);
    #else
        localtime_r(&t, &tm_buf); // Use reentrant version for POSIX
    #endif
    
    EXPECT_EQ(tm_buf.tm_year, 2023 - 1900);
    EXPECT_EQ(tm_buf.tm_mon, 0);
    EXPECT_EQ(tm_buf.tm_mday, 15);
    EXPECT_EQ(tm_buf.tm_hour, 14); // Matches input because parseTimestamp assumes input is local time
    EXPECT_EQ(tm_buf.tm_min, 30);
    EXPECT_EQ(tm_buf.tm_sec, 0);

    // Test with invalid format string
    auto invalidFormatResult = utils::parseTimestamp("2023-01-15", "%Y/%m/%d");
    ASSERT_FALSE(invalidFormatResult.has_value());

    // Test with mismatched format
    auto mismatchedFormatResult = utils::parseTimestamp("2023/01/15", "%Y-%m-%d");
    ASSERT_FALSE(mismatchedFormatResult.has_value());

    // Test with invalid date/time values
    auto invalidDateResult = utils::parseTimestamp("2023-13-01 00:00:00", formatStr); // Invalid month
    ASSERT_FALSE(invalidDateResult.has_value());

    auto invalidTimeResult = utils::parseTimestamp("2023-01-01 25:00:00", formatStr); // Invalid hour
    ASSERT_FALSE(invalidTimeResult.has_value());
}

TEST(TimeTests, FormatElapsedTime) {
    EXPECT_EQ(utils::formatElapsedTime(0), "0s");
    EXPECT_EQ(utils::formatElapsedTime(1), "1s");
    EXPECT_EQ(utils::formatElapsedTime(59), "59s");
    EXPECT_EQ(utils::formatElapsedTime(60), "1m 0s");
    EXPECT_EQ(utils::formatElapsedTime(61), "1m 1s");
    EXPECT_EQ(utils::formatElapsedTime(3599), "59m 59s");
    EXPECT_EQ(utils::formatElapsedTime(3600), "1h 0m 0s");
    EXPECT_EQ(utils::formatElapsedTime(3601), "1h 0m 1s");
    EXPECT_EQ(utils::formatElapsedTime(86399), "23h 59m 59s");
    EXPECT_EQ(utils::formatElapsedTime(86400), "1d 0h 0m 0s");
    EXPECT_EQ(utils::formatElapsedTime(86401), "1d 0h 0m 1s");
    EXPECT_EQ(utils::formatElapsedTime(172800), "2d 0h 0m 0s");
    EXPECT_EQ(utils::formatElapsedTime(365 * 86400 + 3600), "365d 1h 0m 0s"); // Over a year
}
