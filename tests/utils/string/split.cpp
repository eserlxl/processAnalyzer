// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/string.h"
#include <vector>

namespace {

// --- split(char) ---
TEST(StringSplitTest, SplitCharBasic) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one,two,three", ','), expected);
}

TEST(StringSplitTest, SplitCharMultipleDelimiters) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one,,two,,,three", ',', true), expected);
}

TEST(StringSplitTest, SplitCharLeadingDelimiter) {
    std::vector<std::string> expected = {"", "one", "two"};
    EXPECT_EQ(utils::split(",one,two", ',', false), expected);
}

TEST(StringSplitTest, SplitCharTrailingDelimiter) {
    std::vector<std::string> expected = {"one", "two", ""};
    EXPECT_EQ(utils::split("one,two,", ',', false), expected);
}

TEST(StringSplitTest, SplitCharOnlyDelimitersSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split(",,,", ',', true), expected);
}

TEST(StringSplitTest, SplitCharOnlyDelimitersSkipEmptyFalse) {
    std::vector<std::string> expected = {"", "", "", ""};
    EXPECT_EQ(utils::split(",,,", ',', false), expected);
}

TEST(StringSplitTest, SplitCharEmptyStringSkipEmptyFalse) {
    std::vector<std::string> expected = {""};
    EXPECT_EQ(utils::split("", ',', false), expected);
}

TEST(StringSplitTest, SplitCharEmptyStringSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split("", ',', true), expected);
}

// --- split(string_view) ---
TEST(StringSplitTest, SplitStringViewBasic) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one<->two<->three", "<->"), expected);
}

TEST(StringSplitTest, SplitStringViewMultipleDelimiters) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one<-><->two<-><-><->three", "<->", true), expected);
}

TEST(StringSplitTest, SplitStringViewLeadingDelimiter) {
    std::vector<std::string> expected = {"", "one", "two"};
    EXPECT_EQ(utils::split("<->one<->two", "<->", false), expected);
}

TEST(StringSplitTest, SplitStringViewTrailingDelimiter) {
    std::vector<std::string> expected = {"one", "two", ""};
    EXPECT_EQ(utils::split("one<->two<->", "<->", false), expected);
}

TEST(StringSplitTest, SplitStringViewOnlyDelimitersSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split("<-><-><->", "<->", true), expected);
}

TEST(StringSplitTest, SplitStringViewOnlyDelimitersSkipEmptyFalse) {
    std::vector<std::string> expected = {"", "", "", ""};
    EXPECT_EQ(utils::split("<-><-><->", "<->", false), expected);
}

TEST(StringSplitTest, SplitStringViewEmptyDelimiter) {
    std::vector<std::string> expected = {"h", "e", "l", "l", "o"};
    EXPECT_EQ(utils::split("hello", "", false), expected); // Empty delimiter now splits to chars
    EXPECT_EQ(utils::split("hello", "", true), expected); // skipEmpty shouldn't affect chars
}

TEST(StringSplitTest, SplitStringViewEmptyStringSkipEmptyFalse) {
    std::vector<std::string> expected = {""};
    EXPECT_EQ(utils::split("", "<->", false), expected);
}

TEST(StringSplitTest, SplitStringViewEmptyStringSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split("", "<->", true), expected);
}

TEST(StringSplitTest, SplitStringViewOverlapping) {
    // "ababab", split by "aba"
    // First "aba" matches at 0. Remaining: "bab".
    // "bab" does not contain "aba".
    // Result: {"", "bab"} (since first part before "aba" is empty)
    std::vector<std::string> expected = {"", "bab"};
    EXPECT_EQ(utils::split("ababab", "aba", false), expected);
}

} // namespace
