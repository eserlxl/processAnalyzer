// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/string.h"

namespace {

// --- startsWith ---
TEST(StringSearchTest, StartsWithPositive) {
    EXPECT_TRUE(utils::startsWith("hello world", "hello"));
}

TEST(StringSearchTest, StartsWithNegative) {
    EXPECT_FALSE(utils::startsWith("hello world", "world"));
}

TEST(StringSearchTest, StartsWithEmptyPrefix) {
    EXPECT_TRUE(utils::startsWith("hello world", ""));
}

TEST(StringSearchTest, StartsWithEmptyString) {
    EXPECT_FALSE(utils::startsWith("", "hello"));
}

TEST(StringSearchTest, StartsWithPrefixLongerThanString) {
    EXPECT_FALSE(utils::startsWith("hi", "hello"));
}

TEST(StringSearchTest, StartsWithExactMatch) {
    EXPECT_TRUE(utils::startsWith("hello", "hello"));
}

// --- endsWith ---
TEST(StringSearchTest, EndsWithPositive) {
    EXPECT_TRUE(utils::endsWith("hello world", "world"));
}

TEST(StringSearchTest, EndsWithNegative) {
    EXPECT_FALSE(utils::endsWith("hello world", "hello"));
}

TEST(StringSearchTest, EndsWithEmptySuffix) {
    EXPECT_TRUE(utils::endsWith("hello world", ""));
}

TEST(StringSearchTest, EndsWithEmptyString) {
    EXPECT_FALSE(utils::endsWith("", "world"));
}

TEST(StringSearchTest, EndsWithSuffixLongerThanString) {
    EXPECT_FALSE(utils::endsWith("hi", "world"));
}

TEST(StringSearchTest, EndsWithExactMatch) {
    EXPECT_TRUE(utils::endsWith("world", "world"));
}

// --- contains ---
TEST(StringSearchTest, ContainsPositive) {
    EXPECT_TRUE(utils::contains("hello world", "lo wo"));
}

TEST(StringSearchTest, ContainsNegative) {
    EXPECT_FALSE(utils::contains("hello world", "foo"));
}

TEST(StringSearchTest, ContainsEmptySubstring) {
    EXPECT_TRUE(utils::contains("hello world", ""));
}

TEST(StringSearchTest, ContainsEmptyString) {
    EXPECT_FALSE(utils::contains("", "foo"));
}

TEST(StringSearchTest, ContainsSubstringLongerThanString) {
    EXPECT_FALSE(utils::contains("hi", "hello"));
}

// --- startsWithIgnoreCase ---
TEST(StringSearchTest, StartsWithIgnoreCasePositive) {
    EXPECT_TRUE(utils::startsWithIgnoreCase("Hello World", "hello"));
    EXPECT_TRUE(utils::startsWithIgnoreCase("hello World", "Hello"));
    EXPECT_TRUE(utils::startsWithIgnoreCase("HELLO World", "heLlO"));
}

TEST(StringSearchTest, StartsWithIgnoreCaseNegative) {
    EXPECT_FALSE(utils::startsWithIgnoreCase("Hello World", "world"));
}

TEST(StringSearchTest, StartsWithIgnoreCaseEmptyPrefix) {
    EXPECT_TRUE(utils::startsWithIgnoreCase("Hello World", ""));
}

TEST(StringSearchTest, StartsWithIgnoreCaseEmptyString) {
    EXPECT_FALSE(utils::startsWithIgnoreCase("", "hello"));
}

// --- endsWithIgnoreCase ---
TEST(StringSearchTest, EndsWithIgnoreCasePositive) {
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello World", "world"));
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello world", "World"));
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello WORLD", "wOrLd"));
}

TEST(StringSearchTest, EndsWithIgnoreCaseNegative) {
    EXPECT_FALSE(utils::endsWithIgnoreCase("Hello World", "hello"));
}

TEST(StringSearchTest, EndsWithIgnoreCaseEmptySuffix) {
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello World", ""));
}

TEST(StringSearchTest, EndsWithIgnoreCaseEmptyString) {
    EXPECT_FALSE(utils::endsWithIgnoreCase("", "world"));
}

// --- containsIgnoreCase ---
TEST(StringSearchTest, ContainsIgnoreCasePositive) {
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "lo wo"));
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "LO WO"));
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "Lo wO"));
}

TEST(StringSearchTest, ContainsIgnoreCaseNegative) {
    EXPECT_FALSE(utils::containsIgnoreCase("Hello World", "foo"));
}

TEST(StringSearchTest, ContainsIgnoreCaseEmptySubstring) {
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", ""));
}

TEST(StringSearchTest, ContainsIgnoreCaseEmptyString) {
    EXPECT_FALSE(utils::containsIgnoreCase("", "foo"));
}

TEST(StringSearchTest, CaseInsensitiveNonASCII) {
    // These functions are ASCII-only. They should match non-ASCII bytes exactly.
    // They should not lowercase non-ASCII chars.
    EXPECT_TRUE(utils::startsWithIgnoreCase("Grüße", "Gr"));
    EXPECT_TRUE(utils::startsWithIgnoreCase("Grüße", "gr")); // G -> g works
    
    EXPECT_TRUE(utils::endsWithIgnoreCase("Grüße", "ße"));
    // 'ß' (0xDF) does not equal 'S' or 's' in ASCII compare, nor does it have a simple ASCII upper/lower pair.
    // So "SS" or "ss" won't match "ß" in this implementation.
    EXPECT_FALSE(utils::endsWithIgnoreCase("Grüße", "SS")); 
    
    EXPECT_TRUE(utils::containsIgnoreCase("Grüße", "üß"));
}

} // namespace
