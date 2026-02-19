// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/string.h"
#include "utils/types.h" // For Result<T> and UtilsError
#include <string>
#include <vector>
#include <limits>

namespace { // Anonymous namespace to avoid name collisions for helper functions and tests

// Helper to check if Result contains a specific error
template<typename T>
bool hasError(const utils::Result<T>& res, utils::UtilsError expectedError) {
    return !res.has_value() && res.error().value() == static_cast<int>(expectedError);
}

// --- isInteger ---
TEST(StringConversionTest, IsIntegerValid) {
    EXPECT_TRUE(utils::isInteger("123"));
    EXPECT_TRUE(utils::isInteger("-456"));
    EXPECT_TRUE(utils::isInteger("0"));
    EXPECT_TRUE(utils::isInteger("+789"));
    EXPECT_TRUE(utils::isInteger("   123   ")); // With whitespace
}

TEST(StringConversionTest, IsIntegerInvalid) {
    EXPECT_FALSE(utils::isInteger("123.45"));
    EXPECT_FALSE(utils::isInteger("abc"));
    EXPECT_FALSE(utils::isInteger(""));
    EXPECT_FALSE(utils::isInteger("12a"));
    EXPECT_FALSE(utils::isInteger("--1"));
    EXPECT_FALSE(utils::isInteger("1-2"));
    EXPECT_FALSE(utils::isInteger("++1"));
    EXPECT_FALSE(utils::isInteger("   "));
    EXPECT_FALSE(utils::isInteger(".123"));
    EXPECT_FALSE(utils::isInteger("123."));
}

// --- isFloatingPoint ---
TEST(StringConversionTest, IsFloatingPointValid) {
    EXPECT_TRUE(utils::isFloatingPoint("123.45"));
    EXPECT_TRUE(utils::isFloatingPoint("-123.45"));
    EXPECT_TRUE(utils::isFloatingPoint("0.0"));
    EXPECT_TRUE(utils::isFloatingPoint("123")); // Integers are valid floating points
    EXPECT_TRUE(utils::isFloatingPoint(".123"));
    EXPECT_TRUE(utils::isFloatingPoint("123."));
    EXPECT_TRUE(utils::isFloatingPoint("1e5"));
    EXPECT_TRUE(utils::isFloatingPoint("1.2e-3"));
    EXPECT_TRUE(utils::isFloatingPoint("   123.45   ")); // With whitespace
}

TEST(StringConversionTest, IsFloatingPointInvalid) {
    EXPECT_FALSE(utils::isFloatingPoint("abc"));
    EXPECT_FALSE(utils::isFloatingPoint(""));
    EXPECT_FALSE(utils::isFloatingPoint("12a.45"));
    EXPECT_FALSE(utils::isFloatingPoint("--1.0"));
    EXPECT_FALSE(utils::isFloatingPoint("   "));
}

// --- toLong ---
TEST(StringConversionTest, ToLongValid) {
    EXPECT_EQ(utils::toLong("123").value(), 123L);
    EXPECT_EQ(utils::toLong("-456").value(), -456L);
    EXPECT_EQ(utils::toLong("0").value(), 0L);
    EXPECT_EQ(utils::toLong("+789").value(), 789L);
    EXPECT_EQ(utils::toLong("   1000   ").value(), 1000L); // With whitespace
    EXPECT_EQ(utils::toLong("FF", 16).value(), 255L);
    EXPECT_EQ(utils::toLong("101", 2).value(), 5L); // Binary
    EXPECT_EQ(utils::toLong("10", 8).value(), 8L); // Octal
}

TEST(StringConversionTest, ToLongInvalid) {
    EXPECT_TRUE(hasError(utils::toLong("123.45"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong("abc"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong("   "), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong("12a"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong("123a"), utils::UtilsError::invalidArgument)); // Partial parse
    
    // Invalid characters for base
    EXPECT_TRUE(hasError(utils::toLong("G", 16), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong("2", 2), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toLong("8", 8), utils::UtilsError::invalidArgument));

    // Prefixes not supported by std::from_chars (except sign)
    EXPECT_TRUE(hasError(utils::toLong("0xFF", 16), utils::UtilsError::invalidArgument));
}

TEST(StringConversionTest, ToLongOutOfRange) {
    // Max long + 1 (assuming 64-bit long for typical strict testing, or just a very large number)
    // "9223372036854775808" is 2^63, which is > LLONG_MAX (2^63 - 1)
    std::string overflowStr = "9223372036854775808"; 
    EXPECT_TRUE(hasError(utils::toLong(overflowStr), utils::UtilsError::outOfRange));

    // Min long - 1
    // "-9223372036854775809" is < LLONG_MIN (-2^63)
    std::string underflowStr = "-9223372036854775809";
    EXPECT_TRUE(hasError(utils::toLong(underflowStr), utils::UtilsError::outOfRange));
}

// --- toDouble ---
TEST(StringConversionTest, ToDoubleValid) {
    EXPECT_DOUBLE_EQ(utils::toDouble("123.45").value(), 123.45);
    EXPECT_DOUBLE_EQ(utils::toDouble("-123.45").value(), -123.45);
    EXPECT_DOUBLE_EQ(utils::toDouble("0.0").value(), 0.0);
    EXPECT_DOUBLE_EQ(utils::toDouble("123").value(), 123.0);
    EXPECT_DOUBLE_EQ(utils::toDouble(".5").value(), 0.5);
    EXPECT_DOUBLE_EQ(utils::toDouble("5.").value(), 5.0);
    EXPECT_DOUBLE_EQ(utils::toDouble("1e5").value(), 100000.0);
    EXPECT_DOUBLE_EQ(utils::toDouble("1.2e-3").value(), 0.0012);
    EXPECT_DOUBLE_EQ(utils::toDouble("   123.45   ").value(), 123.45); // With whitespace
}

TEST(StringConversionTest, ToDoubleInvalid) {
    EXPECT_TRUE(hasError(utils::toDouble("abc"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("   "), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("12a.45"), utils::UtilsError::invalidArgument)); // Partial parse
}

TEST(StringConversionTest, ToDoubleOutOfRange) {
    std::string overflowStr = "1e+1000";
    EXPECT_TRUE(hasError(utils::toDouble(overflowStr), utils::UtilsError::outOfRange));

    std::string underflowStr = "-1e+1000";
    EXPECT_TRUE(hasError(utils::toDouble(underflowStr), utils::UtilsError::outOfRange));
}

TEST(StringConversionTest, ToDoubleSpecialValues) {
    // Implementation explicitly rejects non-finite values after std::from_chars
    // So "inf", "nan", etc. should return invalidArgument.
    
    // Test for infinity
    EXPECT_TRUE(hasError(utils::toDouble("inf"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("+inf"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("-inf"), utils::UtilsError::invalidArgument));

    // Test for NaN
    EXPECT_TRUE(hasError(utils::toDouble("nan"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("NaN"), utils::UtilsError::invalidArgument));
}

// --- parseBool ---
TEST(StringConversionTest, ParseBoolValid) {
    EXPECT_TRUE(utils::parseBool("true").value());
    EXPECT_TRUE(utils::parseBool("TRUE").value());
    EXPECT_TRUE(utils::parseBool("True").value());
    EXPECT_TRUE(utils::parseBool("1").value());
    EXPECT_TRUE(utils::parseBool("  true  ").value()); // With whitespace
    
    EXPECT_FALSE(utils::parseBool("false").value());
    EXPECT_FALSE(utils::parseBool("FALSE").value());
    EXPECT_FALSE(utils::parseBool("False").value());
    EXPECT_FALSE(utils::parseBool("0").value());
    EXPECT_FALSE(utils::parseBool("  0  ").value()); // With whitespace
}

TEST(StringConversionTest, ParseBoolInvalid) {
    EXPECT_TRUE(hasError(utils::parseBool("yes"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool("no"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool("other"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool("   "), utils::UtilsError::invalidArgument));
}

// --- toInt ---
TEST(StringConversionTest, ToIntValid) {
    EXPECT_EQ(utils::toInt("123").value(), 123);
    EXPECT_EQ(utils::toInt("-456").value(), -456);
    EXPECT_EQ(utils::toInt("0").value(), 0);
    EXPECT_EQ(utils::toInt("+789").value(), 789);
    EXPECT_EQ(utils::toInt("   1000   ").value(), 1000); // With whitespace
    EXPECT_EQ(utils::toInt("FF", 16).value(), 255);
    EXPECT_EQ(utils::toInt("101", 2).value(), 5); // Binary
    EXPECT_EQ(utils::toInt("77", 8).value(), 63); // Octal
}

TEST(StringConversionTest, ToIntInvalid) {
    EXPECT_TRUE(hasError(utils::toInt("123.45"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toInt("abc"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toInt(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toInt("   "), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toInt("12a"), utils::UtilsError::invalidArgument));

    // Invalid characters for base
    EXPECT_TRUE(hasError(utils::toInt("G", 16), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toInt("2", 2), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toInt("8", 8), utils::UtilsError::invalidArgument));
    
    // Prefixes not supported
    EXPECT_TRUE(hasError(utils::toInt("0xFF", 16), utils::UtilsError::invalidArgument));
}

TEST(StringConversionTest, ToIntOutOfRange) {
    // Max int + 1 (2147483648 for 32-bit int)
    std::string overflowStr = "2147483648";
    EXPECT_TRUE(hasError(utils::toInt(overflowStr), utils::UtilsError::outOfRange));

    // Min int - 1 (-2147483649 for 32-bit int)
    std::string underflowStr = "-2147483649";
    EXPECT_TRUE(hasError(utils::toInt(underflowStr), utils::UtilsError::outOfRange));
}

// --- toFloat ---
TEST(StringConversionTest, ToFloatValid) {
    EXPECT_FLOAT_EQ(utils::toFloat("123.45").value(), 123.45F);
    EXPECT_FLOAT_EQ(utils::toFloat("-123.45").value(), -123.45F);
    EXPECT_FLOAT_EQ(utils::toFloat("0.0").value(), 0.0F);
    EXPECT_FLOAT_EQ(utils::toFloat("123").value(), 123.0F);
    EXPECT_FLOAT_EQ(utils::toFloat(".5").value(), 0.5F);
    EXPECT_FLOAT_EQ(utils::toFloat("5.").value(), 5.0F);
    EXPECT_FLOAT_EQ(utils::toFloat("1e5").value(), 100000.0F);
    EXPECT_FLOAT_EQ(utils::toFloat("1.2e-3").value(), 0.0012F);
    EXPECT_FLOAT_EQ(utils::toFloat("   123.45   ").value(), 123.45F); // With whitespace
}

TEST(StringConversionTest, ToFloatInvalid) {
    EXPECT_TRUE(hasError(utils::toFloat("abc"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("   "), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("12a.45"), utils::UtilsError::invalidArgument)); // Partial parse
}

TEST(StringConversionTest, ToFloatOutOfRange) {
    std.string overflowStr = "1e+100";
    EXPECT_TRUE(hasError(utils::toFloat(overflowStr), utils::UtilsError::outOfRange));

    std::string underflowStr = "-1e+100";
    EXPECT_TRUE(hasError(utils::toFloat(underflowStr), utils::UtilsError::outOfRange));
}

TEST(StringConversionTest, ToFloatSpecialValues) {
    // Test for infinity
    EXPECT_TRUE(hasError(utils::toFloat("inf"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("+inf"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("-inf"), utils::UtilsError::invalidArgument));

    // Test for NaN
    EXPECT_TRUE(hasError(utils::toFloat("nan"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("NaN"), utils::UtilsError::invalidArgument));
}

// --- tryParse ---
TEST(StringConversionTest, TryParseIntegerValid) {
    int val;
    EXPECT_TRUE(utils::tryParse("123", val));
    EXPECT_EQ(val, 123);

    EXPECT_TRUE(utils::tryParse("-456", val));
    EXPECT_EQ(val, -456);

    EXPECT_TRUE(utils::tryParse("0", val));
    EXPECT_EQ(val, 0);

    EXPECT_TRUE(utils::tryParse("+789", val));
    EXPECT_EQ(val, 789);

    EXPECT_TRUE(utils::tryParse("   1000   ", val)); // With whitespace
    EXPECT_EQ(val, 1000);
}

TEST(StringConversionTest, TryParseIntegerInvalid) {
    int val;
    EXPECT_FALSE(utils::tryParse("123.45", val));
    EXPECT_FALSE(utils::tryParse("abc", val));
    EXPECT_FALSE(utils::tryParse("", val));
    EXPECT_FALSE(utils::tryParse("   ", val));
    EXPECT_FALSE(utils::tryParse("12a", val));
    EXPECT_FALSE(utils::tryParse("123a", val)); // Partial parse
}

TEST(StringConversionTest, TryParseDoubleValid) {
    double val;
    EXPECT_TRUE(utils::tryParse("123.45", val));
    EXPECT_DOUBLE_EQ(val, 123.45);

    EXPECT_TRUE(utils::tryParse("-123.45", val));
    EXPECT_DOUBLE_EQ(val, -123.45);

    EXPECT_TRUE(utils::tryParse("0.0", val));
    EXPECT_DOUBLE_EQ(val, 0.0);

    EXPECT_TRUE(utils::tryParse("123", val)); // Integers are valid doubles
    EXPECT_DOUBLE_EQ(val, 123.0);

    EXPECT_TRUE(utils::tryParse("   123.45   ", val)); // With whitespace
    EXPECT_DOUBLE_EQ(val, 123.45);
}

TEST(StringConversionTest, TryParseDoubleInvalid) {
    double val;
    EXPECT_FALSE(utils::tryParse("abc", val));
    EXPECT_FALSE(utils::tryParse("", val));
    EXPECT_FALSE(utils::tryParse("   ", val));
    EXPECT_FALSE(utils::tryParse("12a.45", val)); // Partial parse
}

TEST(StringConversionTest, TryParseOutOfRange) {
    // Test with values that should cause out_of_range for std::from_chars
    int valInt;
    long valLong;
    double valDouble;

    std::string overflowIntStr = std::to_string(static_cast<long long>(std::numeric_limits<int>::max()) + 1);
    EXPECT_FALSE(utils::tryParse(overflowIntStr, valInt));

    std::string underflowIntStr = std::to_string(static_cast<long long>(std::numeric_limits<int>::min()) - 1);
    EXPECT_FALSE(utils::tryParse(underflowIntStr, valInt));

    std::string overflowLongStr = "9223372036854775808"; // > INT64_MAX
    EXPECT_FALSE(utils::tryParse(overflowLongStr, valLong));

    std::string largeFloatStr = "1e+309"; // Exceeds double range
    EXPECT_FALSE(utils::tryParse(largeFloatStr, valDouble));
}

TEST(StringConversionTest, TryParseSpecialValues) {
    double val;
    // tryParse implementation explicitly rejects non-finite values
    EXPECT_FALSE(utils::tryParse("inf", val));
    EXPECT_FALSE(utils::tryParse("-inf", val));
    EXPECT_FALSE(utils::tryParse("nan", val));
}

} // namespace
