// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/string.h"
#include <vector>

namespace {

// --- format ---
TEST(StringFormatTest, FormatBasic) {
    EXPECT_EQ(utils::format("Hello, {}!", "World"), "Hello, World!");
}

TEST(StringFormatTest, FormatBasicWithInt) {
    EXPECT_EQ(utils::format("The answer is {}.", 42), "The answer is 42.");
}

TEST(StringFormatTest, FormatMultipleArgs) {
    EXPECT_EQ(utils::format("{}, {} and {}.", "one", 2, 3.0F), "one, 2 and 3."); // std::format default precision for float
}

TEST(StringFormatTest, FormatOrderedArgs) {
    EXPECT_EQ(utils::format("{1}, {0}.", "world", "Hello"), "Hello, world.");
}

TEST(StringFormatTest, FormatMixedTypes) {
    EXPECT_EQ(utils::format("Value: {:05d}, Ratio: {:.2f}", 123, 0.12345), "Value: 00123, Ratio: 0.12");
}

} // namespace
