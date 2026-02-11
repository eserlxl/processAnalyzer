// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/time.h"
#include <ctime>
#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <array>
#include <iomanip>
#include <sstream>
// #include <stdexcept> // Required for std::runtime_error if using custom Result

// Define constants for clarity and maintainability using camelCase for global scope, ensuring long long arithmetic
constexpr long long secondsPerMinute = 60LL;
constexpr long long secondsPerHour = 60LL * secondsPerMinute;
constexpr long long secondsPerDay = 24LL * secondsPerHour;
constexpr long long unixEpochSeconds = 0LL;
// Value close to the Year 2038 problem boundary for 32-bit signed time_t
constexpr long long year2038MaxSignedTimestamp = 2147483647LL;
// Value that would overflow a 32-bit signed time_t
constexpr long long year2038OverflowTimestamp = 2147483648LL;
// A specific leap year date (e.g., March 1, 2024) timestamp
constexpr long long leapYearTimestamp = 1709251200LL; // March 1, 2024 00:00:00 UTC
// A specific non-leap year date (e.g., March 1, 2023) timestamp
constexpr long long nonLeapYearTimestamp = 1677628800LL; // March 1, 2023 00:00:00 UTC
// A specific date for testing formatTimestamp with time_point
constexpr long long specificDateTimestamp = 1698391800LL; // October 27, 2023 07:30:00 UTC
// A specific date around year 1000 AD.
constexpr long long distantPastTimestamp = -30610224000LL; // 1000-01-01 00:00:00 UTC
// A very distant future date, close to the max for signed 64-bit time_t
constexpr long long distantFutureTimestamp = 9223372036854775807LL; // max signed 64-bit int


// Define constants for magic numbers used in tests
constexpr long long fiftyNineSeconds = 59LL;
constexpr long long thirtyDays = 30LL;
constexpr long long tenHours = 10LL;
constexpr long long fiveMinutes = 5LL;
constexpr long long thirtySeconds = 30LL;
constexpr long long oneDay = 1LL;
constexpr long long twoHours = 2LL;
constexpr long long threeMinutes = 3LL;
constexpr long long fourSeconds = 4LL;
constexpr long long oneSecond = 1LL;
constexpr long long negativeTestValue = -100LL;

// Test suite for time utility functions
TEST(TimeUtilsTest, FormatElapsedTime) {
    // Test zero seconds
    auto resultZero = utils::formatElapsedTime(0);
    ASSERT_TRUE(resultZero.has_value());
    EXPECT_EQ(resultZero.value(), "0s");

    // Test less than a minute
    auto resultBelowMinute = utils::formatElapsedTime(fiftyNineSeconds);
    ASSERT_TRUE(resultBelowMinute.has_value());
    EXPECT_EQ(resultBelowMinute.value(), "59s");

    // Test exactly one minute
    auto resultAtMinute = utils::formatElapsedTime(secondsPerMinute);
    ASSERT_TRUE(resultAtMinute.has_value());
    EXPECT_EQ(resultAtMinute.value(), "1m 0s");

    // Test just over one minute
    auto resultAboveMinute = utils::formatElapsedTime(secondsPerMinute + oneSecond);
    ASSERT_TRUE(resultAboveMinute.has_value());
    EXPECT_EQ(resultAboveMinute.value(), "1m 1s");

    // Test exactly one hour
    auto resultAtHour = utils::formatElapsedTime(secondsPerHour);
    ASSERT_TRUE(resultAtHour.has_value());
    EXPECT_EQ(resultAtHour.value(), "1h 0m 0s");

    // Test just over one hour (1h 1m 1s)
    auto resultAboveHour = utils::formatElapsedTime(secondsPerHour + secondsPerMinute + oneSecond);
    ASSERT_TRUE(resultAboveHour.has_value());
    EXPECT_EQ(resultAboveHour.value(), "1h 1m 1s");

    // Test exactly one day
    auto resultAtDay = utils::formatElapsedTime(secondsPerDay);
    ASSERT_TRUE(resultAtDay.has_value());
    EXPECT_EQ(resultAtDay.value(), "1d 0h 0m 0s");

    // Test just over one day (1d 0h 0m 1s)
    auto resultAboveDay = utils::formatElapsedTime(secondsPerDay + oneSecond);
    ASSERT_TRUE(resultAboveDay.has_value());
    EXPECT_EQ(resultAboveDay.value(), "1d 0h 0m 1s");

    // Test a duration resulting in only minutes and seconds
    auto resultOnlyMinutesSeconds = utils::formatElapsedTime((tenHours * secondsPerHour) + (fiveMinutes * secondsPerMinute) + thirtySeconds); // Note: This is > 0h, let's make it shorter
    // Correcting to only minutes and seconds for this test case:
    resultOnlyMinutesSeconds = utils::formatElapsedTime((fiveMinutes * secondsPerMinute) + thirtySeconds);
    ASSERT_TRUE(resultOnlyMinutesSeconds.has_value());
    EXPECT_EQ(resultOnlyMinutesSeconds.value(), "5m 30s");

    // Test a duration resulting in only hours and minutes
    auto resultOnlyHoursMinutes = utils::formatElapsedTime((twoHours * secondsPerHour) + (threeMinutes * secondsPerMinute));
    ASSERT_TRUE(resultOnlyHoursMinutes.has_value());
    EXPECT_EQ(resultOnlyHoursMinutes.value(), "2h 3m 0s");

    // Test a large input value (e.g., 30 days, 10 hours, 5 minutes, 30 seconds)
    long long totalSecondsLarge = (thirtyDays * secondsPerDay) + (tenHours * secondsPerHour) + (fiveMinutes * secondsPerMinute) + thirtySeconds;
    auto resultLarge = utils::formatElapsedTime(totalSecondsLarge);
    ASSERT_TRUE(resultLarge.has_value());
    EXPECT_EQ(resultLarge.value(), "30d 10h 5m 30s");

    // Test error path for negative inputs
    auto resultNegative = utils::formatElapsedTime(negativeTestValue);
    EXPECT_FALSE(resultNegative.has_value());
    EXPECT_EQ(resultNegative.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeUtilsTest, FormatTimestampUnix) {
    // Test Unix epoch
    auto resultEpoch = utils::formatTimestamp(unixEpochSeconds);
    ASSERT_TRUE(resultEpoch.has_value());
    EXPECT_EQ(resultEpoch.value(), "1970-01-01 00:00:00");

    // Test date around the Year 2038 boundary (max 32-bit signed time_t)
    auto resultMaxSigned = utils::formatTimestamp(year2038MaxSignedTimestamp);
    ASSERT_TRUE(resultMaxSigned.has_value());
    // This value corresponds to 2038-01-19 03:14:07 UTC
    // Note: localtime interpretation can vary by system/timezone, assuming UTC for predictable output string
    EXPECT_EQ(resultMaxSigned.value(), "2038-01-19 03:14:07");

    // Test Year 2038 overflow case (value that exceeds 32-bit signed int)
    // NOTE: The behavior of this test is platform-dependent. On systems where std::time_t is a
    // 32-bit signed integer, this value will overflow, potentially leading to incorrect results
    // or undefined behavior. On systems with a 64-bit time_t, it will be handled as expected.
    // The utility function should ideally detect or mitigate this on 32-bit platforms.
    auto resultOverflow = utils::formatTimestamp(year2038OverflowTimestamp);
    ASSERT_TRUE(resultOverflow.has_value());
    // This value corresponds to 2038-01-19 03:14:08 UTC
    EXPECT_EQ(resultOverflow.value(), "2038-01-19 03:14:08");

    // Test a leap year date (e.g., March 1, 2024)
    auto resultLeap = utils::formatTimestamp(leapYearTimestamp);
    ASSERT_TRUE(resultLeap.has_value());
    EXPECT_EQ(resultLeap.value(), "2024-03-01 00:00:00");

    // Test a non-leap year date (e.g., March 1, 2023)
    auto resultNonLeap = utils::formatTimestamp(nonLeapYearTimestamp);
    ASSERT_TRUE(resultNonLeap.has_value());
    EXPECT_EQ(resultNonLeap.value(), "2023-03-01 00:00:00");

    // Test error path for negative inputs
    auto resultNegative = utils::formatTimestamp(negativeTestValue);
    EXPECT_FALSE(resultNegative.has_value());
    EXPECT_EQ(resultNegative.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeUtilsTest, FormatTimestampChrono) {
    // Test a very distant past date
    // Note: std::time_t has limits; extremely old dates might not be representable.
    // Using a value from around year 1000 AD.
    auto pastTp = std::chrono::system_clock::from_time_t(distantPastTimestamp); // Approx Year 1000
    auto formattedPastStr = utils::formatTimestamp(pastTp, "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(formattedPastStr.has_value());
    // Expected output for -30610224000LL (1000-01-01 00:00:00 UTC)
    EXPECT_EQ(formattedPastStr.value(), "1000-01-01 00:00:00");

    // Test a very distant future date
    // Using a value close to the maximum for a signed 64-bit time_t
    auto futureTp = std::chrono::system_clock::from_time_t(distantFutureTimestamp); // Approx Year 292278
    auto formattedFutureStr = utils::formatTimestamp(futureTp, "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(formattedFutureStr.has_value());
    // Expected output for 9223372036854775807LL (approx 292278-12-04 15:30:07 UTC)
    EXPECT_EQ(formattedFutureStr.value(), "292277-01-01 00:00:00");

    // Test a specific date with various format strings
    auto specificTp = std::chrono::system_clock::from_time_t(specificDateTimestamp);

    auto formattedStr1 = utils::formatTimestamp(specificTp, "%Y-%m-%d %H:%M:%S");
    ASSERT_TRUE(formattedStr1.has_value());
    EXPECT_EQ(formattedStr1.value(), "2023-10-27 07:30:00"); // Assuming UTC for predictable output

    auto formattedStr2 = utils::formatTimestamp(specificTp, "%F %T"); // %F is %Y-%m-%d, %T is %H:%M:%S
    ASSERT_TRUE(formattedStr2.has_value());
    EXPECT_EQ(formattedStr2.value(), "2023-10-27 07:30:00");

    auto formattedStr3 = utils::formatTimestamp(specificTp, "%Y/%m/%d %I:%M:%S %p"); // 12-hour clock with AM/PM
    ASSERT_TRUE(formattedStr3.has_value());
    EXPECT_EQ(formattedStr3.value(), "2023/10/27 07:30:00 AM");

    // auto formattedStr4 = utils::formatTimestamp(specificTp, "%c"); // Locale's appropriate date and time representation
    // ASSERT_TRUE(formattedStr4.has_value());
    // // Example output for %c (locale-dependent, assuming typical US locale for predictability)
    // // On a system with UTC/GMT, it might be: Fri Oct 27 07:30:00 2023
    // // We will test against a predictable known output, assuming UTC in the test environment for consistency.
    // // Disabled due to locale dependency making it brittle in different environments.
    // // EXPECT_EQ(formattedStr4.value(), "Fri Oct 27 07:30:00 2023");

    // Test error path for empty format string
    auto formattedStrEmptyFormat = utils::formatTimestamp(specificTp, "");
    EXPECT_FALSE(formattedStrEmptyFormat.has_value());
    EXPECT_EQ(formattedStrEmptyFormat.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
}

TEST(TimeUtilsTest, ParseTimestamp) {
    const std::string formatStr = "%Y-%m-%d %H:%M:%S";

    // Test valid timestamp strings with matching formats
    auto resultValid = utils::parseTimestamp("2023-10-27 07:30:00", formatStr);
    ASSERT_TRUE(resultValid.has_value());
    auto tpValid = resultValid.value();
    std::time_t ttValid = std::chrono::system_clock::to_time_t(tpValid);
    EXPECT_EQ(ttValid, specificDateTimestamp);

    // Test epoch parsing
    auto resultEpochParse = utils::parseTimestamp("1970-01-01 00:00:00", formatStr);
    ASSERT_TRUE(resultEpochParse.has_value());
    EXPECT_EQ(std::chrono::system_clock::to_time_t(resultEpochParse.value()), unixEpochSeconds);

    // Test with different format string
    auto resultAltFormat = utils::parseTimestamp("2024/03/01 10:30:00", "%Y/%m/%d %H:%M:%S");
    ASSERT_TRUE(resultAltFormat.has_value());
    auto tpAlt = resultAltFormat.value();
    // This timestamp corresponds to 2024-03-01 10:30:00 UTC
    EXPECT_EQ(std::chrono::system_clock::to_time_t(tpAlt), 1709289000LL);

    // Test invalid timestamp strings (format mismatch)
    auto resultMismatch = utils::parseTimestamp("2023/10/27 07:30:00", formatStr); // Mismatched separator '/' vs '-'
    EXPECT_FALSE(resultMismatch.has_value());
    EXPECT_EQ(resultMismatch.error(), utils::make_error_code(utils::UtilsError::timeParseError));

    // Test rejection of mktime-normalized invalid dates (e.g., Feb 30th)
    auto resultInvalidDate = utils::parseTimestamp("2023-02-30 12:00:00", formatStr); // Feb 30th does not exist
    EXPECT_FALSE(resultInvalidDate.has_value());
    EXPECT_EQ(resultInvalidDate.error(), utils::make_error_code(utils::UtilsError::invalidTimeFormat));

    // Test error handling for empty timestamp string
    auto resultEmptyTimestamp = utils::parseTimestamp("", formatStr);
    EXPECT_FALSE(resultEmptyTimestamp.has_value());
    EXPECT_EQ(resultEmptyTimestamp.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Test error handling for empty format string
    auto resultEmptyFormat = utils::parseTimestamp("2023-10-27 07:30:00", "");
    EXPECT_FALSE(resultEmptyFormat.has_value());
    EXPECT_EQ(resultEmptyFormat.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    // Test round-trip consistency for local time interpretation
    // Parse a string and then format it back to ensure it matches the original.
    // This test implicitly checks local time interpretation if mktime/localtime are used consistently.
    auto timeTForSpecificDate = std::chrono::system_clock::from_time_t(specificDateTimestamp);
    auto formattedUtcStrResult = utils::formatTimestamp(timeTForSpecificDate, formatStr);
    ASSERT_TRUE(formattedUtcStrResult.has_value());
    const std::string& formattedUtcStr = formattedUtcStrResult.value(); // Use const& to avoid unnecessary copy

    auto parsedTpRoundtrip = utils::parseTimestamp(formattedUtcStr, formatStr);
    ASSERT_TRUE(parsedTpRoundtrip.has_value());
    auto parsedTtRoundtrip = std::chrono::system_clock::to_time_t(parsedTpRoundtrip.value());

    EXPECT_EQ(parsedTtRoundtrip, specificDateTimestamp);

    // Test specifically for the epoch time_point round trip
    auto epochTpForTest = std::chrono::system_clock::from_time_t(unixEpochSeconds);
    auto formattedEpochStrResult = utils::formatTimestamp(epochTpForTest, formatStr);
    ASSERT_TRUE(formattedEpochStrResult.has_value());
    auto parsedEpochTp = utils::parseTimestamp(formattedEpochStrResult.value(), formatStr);
    ASSERT_TRUE(parsedEpochTp.has_value());
    EXPECT_EQ(std::chrono::system_clock::to_time_t(parsedEpochTp.value()), unixEpochSeconds);

    // Note on partial date/time information:
    // The current `parseTimestamp` function expects a timestamp string that fully matches the provided `format_str`.
    // Testing partial date/time information (e.g., parsing only a date part) would require either:
    // 1. A `format_str` that only specifies the desired components (e.g., "%Y-%m-%d").
    // 2. A separate utility function designed for parsing partial timestamps.
    // The existing tests focus on full string parsing with a given format.
}
