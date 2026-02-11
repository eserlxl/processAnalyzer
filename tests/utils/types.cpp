// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/types.h"
#include "gtest/gtest.h"
#include <limits>
#include <cstdint>
#include <string>
#include <string_view>

// --- Test Cases ---

TEST(ParseIntegerNoThrowTest, BasicFunctionality) {
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("123", 10), 123);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("-456", 10), -456);
    EXPECT_EQ(utils::parseIntegerNoThrow<long>("-456", 10), -456L);
    EXPECT_EQ(utils::parseIntegerNoThrow<short>("123", 10), 123);
    EXPECT_EQ(utils::parseIntegerNoThrow<unsigned char>("255", 10), 255);
    EXPECT_EQ(utils::parseIntegerNoThrow<uint32_t>("4294967295", 10), 4294967295U);
}

TEST(ParseIntegerNoThrowTest, BaseVariations) {
    // Hex
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("FF", 16), 255);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("ff", 16), 255);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("0xFF", 0), 255);

    // Octal
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("77", 8), 63);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("077", 0), 63);

    // Binary
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("10110", 2), 22);
    // std::from_chars with base 0 doesn't support "0b" prefix, but we can test explicit base 2.
    // Let's add an explicit test for "0b" prefix if the function is ever changed to support it.
    // For now, testing what from_chars supports.
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("0b10110", 2), std::nullopt); // 'b' is not a valid binary digit
}


TEST(ParseIntegerNoThrowTest, EdgeCases) {
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("", 10), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("   ", 10), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("123a", 10), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("a123", 10), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("0", 10), 0);

    // Test with string_view
    const std::string largeString = "Some prefix 12345 some suffix";
    constexpr size_t prefixLength = 12;
    constexpr size_t numberLength = 5;
    std::string_view numberSv(largeString.data() + prefixLength, numberLength); // "12345"
    EXPECT_EQ(utils::parseIntegerNoThrow<int>(numberSv, 10), 12345);
}

TEST(ParseIntegerNoThrowTest, TypeLimits) {
    // Int
    EXPECT_EQ(utils::parseIntegerNoThrow<int>(std::to_string(std::numeric_limits<int>::max()), 10), std::numeric_limits<int>::max());
    EXPECT_EQ(utils::parseIntegerNoThrow<int>(std::to_string(std::numeric_limits<int>::min()), 10), std::numeric_limits<int>::min());

    // Unsigned Char
    EXPECT_EQ(utils::parseIntegerNoThrow<unsigned char>("255", 10), 255);
    EXPECT_EQ(utils::parseIntegerNoThrow<unsigned char>("0", 10), 0);
}

TEST(ParseIntegerNoThrowTest, OverflowAndUnderflow) {
    // Int overflow
    std::string maxIntStr = std::to_string(std::numeric_limits<int>::max());
    std::string posOverflowStr = maxIntStr + "0";
    EXPECT_EQ(utils::parseIntegerNoThrow<int>(posOverflowStr, 10), std::nullopt);

    // Int underflow
    std::string minIntStr = std::to_string(std::numeric_limits<int>::min());
    // Remove minus sign and append a digit to make it larger magnitude negative
    std::string negOverflowStr = minIntStr + "0";
    EXPECT_EQ(utils::parseIntegerNoThrow<int>(negOverflowStr, 10), std::nullopt);

    // Unsigned Char overflow
    EXPECT_EQ(utils::parseIntegerNoThrow<unsigned char>("256", 10), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<unsigned char>("-1", 10), std::nullopt);

    // uint32_t overflow
    std::string maxUint32Str = std::to_string(std::numeric_limits<uint32_t>::max());
    std::string uint32OverflowStr = maxUint32Str + "0";
    EXPECT_EQ(utils::parseIntegerNoThrow<uint32_t>(uint32OverflowStr, 10), std::nullopt);
}

TEST(ParseIntegerNoThrowTest, InvalidBases) {
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("100", 1), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("100", 37), std::nullopt);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("100", -1), std::nullopt);

    // Valid bases should still work
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("100", 0), 100);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("100", 2), 4);
    EXPECT_EQ(utils::parseIntegerNoThrow<int>("100", 36), 1296);
}
