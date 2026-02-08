// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Time.h"
#include <chrono>
#include <string>
#include <thread> // For std::this_thread::sleep_for
#include <ctime>  // For std::mktime, std::gmtime, struct tm, std::localtime_r, std::localtime_s
#include <array> // For std::array
#include <iomanip> // For std::put_time
#include <sstream> // For std::stringstream

// Helper to get current local time's tm struct
std::tm get_local_tm(std::chrono::system_clock::time_point tp) {
    std::tm tm_buf{};
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
#if defined(_WIN32)
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif
    return tm_buf;
}

// Helper to format a tm struct into a string using the system's local time
std::string format_tm_local(const std::tm& tm_buf, const char* format_str) {
    char buffer[64]; // Sufficient buffer size for common formats
    if (std::strftime(buffer, sizeof(buffer), format_str, &tm_buf)) {
        return buffer;
    }
    return "ERROR_FORMATTING";
}

TEST(TimeTests, GetCurrentSystemTime) {
    auto result = utils::getCurrentSystemTime();
    ASSERT_TRUE(result.has_value());
    // Basic check: current time should be within a reasonable range of 'now'
    auto now = std::chrono::system_clock::now();
    EXPECT_GE(result.value(), now - std::chrono::seconds(5)); // Allow a few seconds slack
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
    std::time_t time_since_epoch = 1673793000LL; // Equivalent to Jan 15, 2023, 14:30:00 UTC
    auto tp_utc = std::chrono::system_clock::from_time_t(time_since_epoch);

    // formatTimestamp uses local time functions (localtime_r/s)
    std::tm local_tm = get_local_tm(tp_utc);

    // Test default format "%Y-%m-%d %H:%M:%S" which implies local time
    auto resultDefault = utils::formatTimestamp(tp_utc, std::string("%Y-%m-%d %H:%M:%S"));
    ASSERT_TRUE(resultDefault.has_value());
    EXPECT_EQ(resultDefault.value(), format_tm_local(local_tm, "%Y-%m-%d %H:%M:%S"));

    // Test different format "%A, %B %d, %Y" (Day of week, Month Day, Year)
    auto resultCustom = utils::formatTimestamp(tp_utc, std::string("%A, %B %d, %Y"));
    ASSERT_TRUE(resultCustom.has_value());
    // Note: %A and %B are locale dependent. This test might fail if the system locale is not English.
    // For robust testing, locale should be set or a locale-independent format used.
    // Assuming English locale for demonstration.
    EXPECT_EQ(resultCustom.value(), format_tm_local(local_tm, "%A, %B %d, %Y"));
}

TEST(TimeTests, FormatTimestampUnix) {
    // Epoch: Jan 1, 1970, 00:00:00 UTC
    long long unixEpoch = 0;
    std::string formattedEpoch = utils::formatTimestamp(unixEpoch);
    // The output depends on the local timezone. We check if it's a valid date around epoch.
    // E.g., if local time is UTC-5, it might show 1969-12-31.
    // A more specific test could be done by checking components against get_local_tm(from_time_t(0)).
    std::tm epoch_tm = get_local_tm(std::chrono::system_clock::from_time_t(0));
    EXPECT_EQ(formattedEpoch, format_tm_local(epoch_tm, "%Y-%m-%d %H:%M:%S"));

    // Some arbitrary time (Jan 15, 2023, 14:30:00 UTC)
    long long unixTime = 1673793000LL;
    std::string formattedTime = utils::formatTimestamp(unixTime);
    std::tm time_tm = get_local_tm(std::chrono::system_clock::from_time_t(unixTime));
    EXPECT_EQ(formattedTime, format_tm_local(time_tm, "%Y-%m-%d %H:%M:%S"));
}

TEST(TimeTests, FormatTimestampUnixNegative) {
    // Test negative timestamps, which should result in "N/A"
    EXPECT_EQ(utils::formatTimestamp(-1LL), "N/A");
    EXPECT_EQ(utils::formatTimestamp(-1000LL), "N/A");
}

TEST(TimeTests, ParseTimestamp) {
    // Test parsing a valid timestamp in local time
    std::string timestampStr = "2023-01-15 14:30:00";
    std::string formatStr = "%Y-%m-%d %H:%M:%S";
    auto result = utils::parseTimestamp(timestampStr, formatStr);
    ASSERT_TRUE(result.has_value());

    // Convert parsed time back to local tm struct for verification
    std::tm parsed_local_tm = get_local_tm(result.value());

    // Verify components. Note: mktime and get_time interpret input as local time.
    EXPECT_EQ(parsed_local_tm.tm_year, 2023 - 1900);
    EXPECT_EQ(parsed_local_tm.tm_mon, 0); // January
    EXPECT_EQ(parsed_local_tm.tm_mday, 15);
    EXPECT_EQ(parsed_local_tm.tm_hour, 14);
    EXPECT_EQ(parsed_local_tm.tm_min, 30);
    EXPECT_EQ(parsed_local_tm.tm_sec, 0);

    // Test with invalid format string
    auto invalidFormatResult = utils::parseTimestamp(std::string("2023-01-15"), std::string("%Y/%m/%d"));
    ASSERT_FALSE(invalidFormatResult.has_value());
    EXPECT_EQ(invalidFormatResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Test with mismatched format
    auto mismatchedFormatResult = utils::parseTimestamp(std::string("2023/01/15"), std::string("%Y-%m-%d"));
    ASSERT_FALSE(mismatchedFormatResult.has_value());
    EXPECT_EQ(mismatchedFormatResult.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampVariousFormats) {
    // Test with YYYY/MM/DD
    auto result1 = utils::parseTimestamp(std::string("2024/03/10"), std::string("%Y/%m/%d"));
    ASSERT_TRUE(result1.has_value());
    std::tm tm1 = get_local_tm(result1.value());
    EXPECT_EQ(tm1.tm_year, 2024 - 1900); EXPECT_EQ(tm1.tm_mon, 2); EXPECT_EQ(tm1.tm_mday, 10);

    // Test with DD-Mon-YYYY (e.g., 25-Dec-2025)
    auto result2 = utils::parseTimestamp(std::string("25-Dec-2025"), std::string("%d-%b-%Y"));
    ASSERT_TRUE(result2.has_value());
    std::tm tm2 = get_local_tm(result2.value());
    EXPECT_EQ(tm2.tm_year, 2025 - 1900); EXPECT_EQ(tm2.tm_mon, 11); EXPECT_EQ(tm2.tm_mday, 25);
}

TEST(TimeTests, ParseTimestampInvalidComponents) {
    std::string formatStr = "%Y-%m-%d %H:%M:%S";

    // Invalid month (13)
    auto invalidMonth = utils::parseTimestamp(std::string("2023-13-01 10:00:00"), formatStr);
    ASSERT_FALSE(invalidMonth.has_value());
    EXPECT_EQ(invalidMonth.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Invalid day (30 in Feb)
    auto invalidDay = utils::parseTimestamp(std::string("2023-02-30 10:00:00"), formatStr);
    ASSERT_FALSE(invalidDay.has_value());
    EXPECT_EQ(invalidDay.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Invalid hour (25)
    auto invalidHour = utils::parseTimestamp(std::string("2023-01-01 25:00:00"), formatStr);
    ASSERT_FALSE(invalidHour.has_value());
    EXPECT_EQ(invalidHour.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
    
    // Invalid minute (60)
    auto invalidMinute = utils::parseTimestamp(std::string("2023-01-01 10:60:00"), formatStr);
    ASSERT_FALSE(invalidMinute.has_value());
    EXPECT_EQ(invalidMinute.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Invalid second (60 - note: 60 is valid for leap seconds, but typically not handled by mktime)
    auto invalidSecond = utils::parseTimestamp(std::string("2023-01-01 10:00:60"), formatStr);
    ASSERT_FALSE(invalidSecond.has_value());
    EXPECT_EQ(invalidSecond.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampEmptyInputs) {
    // Empty timestamp string
    auto emptyTimestamp = utils::parseTimestamp(std::string(""), std::string("%Y-%m-%d"));
    ASSERT_FALSE(emptyTimestamp.has_value());
    EXPECT_EQ(emptyTimestamp.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Empty format string
    auto emptyFormat = utils::parseTimestamp(std::string("2023-01-01"), std::string(""));
    ASSERT_FALSE(emptyFormat.has_value());
    EXPECT_EQ(emptyFormat.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Both empty
    auto bothEmpty = utils::parseTimestamp(std::string(""), std::string(""));
    ASSERT_FALSE(bothEmpty.has_value());
    EXPECT_EQ(bothEmpty.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampMismatchedFormats) {
    // Format YYYY-MM-DD, input YYYY/MM/DD
    auto mismatch1 = utils::parseTimestamp(std::string("2023/01/01"), std::string("%Y-%m-%d"));
    ASSERT_FALSE(mismatch1.has_value());
    EXPECT_EQ(mismatch1.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Format HH:MM:SS, input HH-MM-SS
    auto mismatch2 = utils::parseTimestamp(std::string("14-30-00"), std::string("%H:%M:%S"));
    ASSERT_FALSE(mismatch2.has_value());
    EXPECT_EQ(mismatch2.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeTests, ParseTimestampEpochAndFarFuture) {
    // Test epoch start with a format that should be parseable
    auto epochStart = utils::parseTimestamp(std::string("1970-01-01 00:00:00"), std::string("%Y-%m-%d %H:%M:%S"));
    ASSERT_TRUE(epochStart.has_value());
    std::time_t t_epoch = std::chrono::system_clock::to_time_t(epochStart.value());

    // Calculate the expected time_t for "1970-01-01 00:00:00" in local time.
    // This is what std::mktime inside parseTimestamp should produce.
    std::tm expected_local_epoch_tm{};
    expected_local_epoch_tm.tm_year = 70; // 1970 + 70 = 1970
    expected_local_epoch_tm.tm_mon = 0;   // January
    expected_local_epoch_tm.tm_mday = 1;   // 1st
    expected_local_epoch_tm.tm_hour = 0;
    expected_local_epoch_tm.tm_min = 0;
    expected_local_epoch_tm.tm_sec = 0;
    expected_local_epoch_tm.tm_isdst = -1; // Let mktime determine DST

    std::time_t expected_t_epoch = std::mktime(&expected_local_epoch_tm);

    EXPECT_EQ(t_epoch, expected_t_epoch); // Expect the value based on local timezone

    // Test a date before epoch
    auto beforeEpoch = utils::parseTimestamp(std::string("1969-12-31 23:59:59"), std::string("%Y-%m-%d %H:%M:%S"));
    ASSERT_TRUE(beforeEpoch.has_value());
    std::time_t t_before = std::chrono::system_clock::to_time_t(beforeEpoch.value());
    EXPECT_LT(t_before, 0); // Should be before epoch

    // Test a date far in the future. std::mktime's limits are implementation-defined.
    // On 64-bit systems, time_t can represent dates up to ~292 billion years.
    // However, some mktime implementations might have stricter limits.
    // The goal is to ensure it returns invalidArgument if it's unrepresentable.
    auto veryFarFuture = utils::parseTimestamp(std::string("9999-01-01 00:00:00"), std::string("%Y-%m-%d %H:%M:%S"));
    if (!veryFarFuture.has_value()) {
        // If mktime fails to represent it, it should return invalidArgument.
        EXPECT_EQ(veryFarFuture.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
    } else {
        // If mktime succeeds, we can assert that the time_t is not -1 (which indicates error).
        // The exact value for 9999-01-01 is complex to predict across all systems,
        // but if it parsed, we expect a valid time_t.
        std::time_t t_far = std::chrono::system_clock::to_time_t(veryFarFuture.value());
        EXPECT_NE(t_far, static_cast<std::time_t>(-1));
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
    EXPECT_EQ(utils::formatElapsedTime(365LL * 86400LL + 3600LL), "365d 1h 0m 0s"); // Over a year
}

TEST(TimeTests, FormatElapsedTimeNegative) {
    // Test negative elapsed time, which should result in "N/A"
    EXPECT_EQ(utils::formatElapsedTime(-100LL), "N/A");
    EXPECT_EQ(utils::formatElapsedTime(-1LL), "N/A");
}
