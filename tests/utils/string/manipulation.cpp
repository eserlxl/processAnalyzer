// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/string.h"

namespace {

// --- trim ---
TEST(StringManipulationTest, TrimNoWhitespace) {
    EXPECT_EQ(utils::trim("hello"), "hello");
}

TEST(StringManipulationTest, TrimLeadingWhitespace) {
    EXPECT_EQ(utils::trim("  hello"), "hello");
}

TEST(StringManipulationTest, TrimTrailingWhitespace) {
    EXPECT_EQ(utils::trim("hello  "), "hello");
}

TEST(StringManipulationTest, TrimLeadingAndTrailingWhitespace) {
    EXPECT_EQ(utils::trim("  hello  "), "hello");
}

TEST(StringManipulationTest, TrimAllWhitespace) {
    EXPECT_EQ(utils::trim("   "), "");
}

TEST(StringManipulationTest, TrimEmptyString) {
    EXPECT_EQ(utils::trim(""), "");
}

TEST(StringManipulationTest, TrimMixedWhitespace) {
    EXPECT_EQ(utils::trim("\t\n hello \r\f "), "hello");
}

TEST(StringManipulationTest, TrimInternalWhitespace) {
    EXPECT_EQ(utils::trim("hello world"), "hello world");
}

// --- toLower ---
TEST(StringManipulationTest, ToLowerAllUppercase) {
    EXPECT_EQ(utils::toLower("HELLO"), "hello");
}

TEST(StringManipulationTest, ToLowerAllLowercase) {
    EXPECT_EQ(utils::toLower("hello"), "hello");
}

TEST(StringManipulationTest, ToLowerMixedCase) {
    EXPECT_EQ(utils::toLower("HeLlO WoRlD"), "hello world");
}

TEST(StringManipulationTest, ToLowerNonAlphabetic) {
    EXPECT_EQ(utils::toLower("123!@#"), "123!@#");
}

TEST(StringManipulationTest, ToLowerEmptyString) {
    EXPECT_EQ(utils::toLower(""), "");
}

TEST(StringManipulationTest, ToLowerNonASCII) {
    // Should not change non-ASCII characters
    EXPECT_EQ(utils::toLower("Grüße"), "grüße"); // 'ü' is not in A-Z,a-z
}

// --- toUpper ---
TEST(StringManipulationTest, ToUpperAllLowercase) {
    EXPECT_EQ(utils::toUpper("hello"), "HELLO");
}

TEST(StringManipulationTest, ToUpperAllUppercase) {
    EXPECT_EQ(utils::toUpper("HELLO"), "HELLO");
}

TEST(StringManipulationTest, ToUpperMixedCase) {
    EXPECT_EQ(utils::toUpper("HeLlO WoRlD"), "HELLO WORLD");
}

TEST(StringManipulationTest, ToUpperNonAlphabetic) {
    EXPECT_EQ(utils::toUpper("123!@#"), "123!@#");
}

TEST(StringManipulationTest, ToUpperEmptyString) {
    EXPECT_EQ(utils::toUpper(""), "");
}

TEST(StringManipulationTest, ToUpperNonASCII) {
    EXPECT_EQ(utils::toUpper("Grüße"), "GRüßE"); // 'ü' and 'ß' are not converted as they are non-ASCII. 
}

// --- replaceAll ---
TEST(StringManipulationTest, ReplaceAllSingleOccurrence) {
    EXPECT_EQ(utils::replaceAll("one two three", "two", "2"), "one 2 three");
}

TEST(StringManipulationTest, ReplaceAllMultipleOccurrences) {
    EXPECT_EQ(utils::replaceAll("one two one three one", "one", "1"), "1 two 1 three 1");
}

TEST(StringManipulationTest, ReplaceAllNoOccurrence) {
    EXPECT_EQ(utils::replaceAll("one two three", "four", "4"), "one two three");
}

TEST(StringManipulationTest, ReplaceAllEmptyTarget) {
    EXPECT_EQ(utils::replaceAll("abc", "", "X"), "abc"); // Spec says target.empty() returns original
}

TEST(StringManipulationTest, ReplaceAllEmptyReplacement) {
    EXPECT_EQ(utils::replaceAll("hello world", "world", ""), "hello ");
}

TEST(StringManipulationTest, ReplaceAllTargetLongerThanReplacement) {
    EXPECT_EQ(utils::replaceAll("banana", "ana", "X"), "bXna");
}

TEST(StringManipulationTest, ReplaceAllReplacementLongerThanTarget) {
    EXPECT_EQ(utils::replaceAll("apple", "p", "pp"), "apppple");
}

TEST(StringManipulationTest, ReplaceAllEmptyString) {
    EXPECT_EQ(utils::replaceAll("", "a", "b"), "");
}

TEST(StringManipulationTest, ReplaceAllTargetAtBeginning) {
    EXPECT_EQ(utils::replaceAll("test string", "test", "new"), "new string");
}

TEST(StringManipulationTest, ReplaceAllTargetAtEnd) {
    EXPECT_EQ(utils::replaceAll("test string", "string", "text"), "test text");
}

TEST(StringManipulationTest, ReplaceAllOverlapping) {
    // Ensure non-overlapping replacement strategy
    EXPECT_EQ(utils::replaceAll("aaaa", "aa", "b"), "bb");
    EXPECT_EQ(utils::replaceAll("aaaaa", "aa", "b"), "bba");
}

// --- replaceFirst ---
TEST(StringManipulationTest, ReplaceFirstSingleOccurrence) {
    EXPECT_EQ(utils::replaceFirst("one two three", "two", "2"), "one 2 three");
}

TEST(StringManipulationTest, ReplaceFirstMultipleOccurrences) {
    EXPECT_EQ(utils::replaceFirst("one two one three one", "one", "1"), "1 two one three one");
}

TEST(StringManipulationTest, ReplaceFirstNoOccurrence) {
    EXPECT_EQ(utils::replaceFirst("one two three", "four", "4"), "one two three");
}

TEST(StringManipulationTest, ReplaceFirstEmptyTarget) {
    // Corrected expected value: replaceFirst on empty target should return original string
    EXPECT_EQ(utils::replaceFirst("abc", "", "X"), "abc");
}

TEST(StringManipulationTest, ReplaceFirstEmptyReplacement) {
    EXPECT_EQ(utils::replaceFirst("hello world", "world", ""), "hello ");
}

TEST(StringManipulationTest, ReplaceFirstEmptyString) {
    EXPECT_EQ(utils::replaceFirst("", "a", "b"), "");
}

// --- replaceN ---
TEST(StringManipulationTest, ReplaceNOneOccurrence) {
    EXPECT_EQ(utils::replaceN("one one one", "one", "X", 1), "X one one");
}

TEST(StringManipulationTest, ReplaceNTwoOccurrences) {
    EXPECT_EQ(utils::replaceN("one one one", "one", "X", 2), "X X one");
}

TEST(StringManipulationTest, ReplaceNMoreThanAvailable) {
    EXPECT_EQ(utils::replaceN("one one one", "one", "X", 5), "X X X");
}

TEST(StringManipulationTest, ReplaceNNoTarget) {
    EXPECT_EQ(utils::replaceN("one two three", "four", "X", 1), "one two three");
}

TEST(StringManipulationTest, ReplaceNEmptyTarget) {
    EXPECT_EQ(utils::replaceN("abc", "", "X", 1), "abc"); // Behaves like replaceAll if target empty
}

TEST(StringManipulationTest, ReplaceNEmptyString) {
    EXPECT_EQ(utils::replaceN("", "a", "b", 1), "");
}

// --- join ---
TEST(StringManipulationTest, JoinEmptyVector) {
    std::vector<std::string> parts{};
    EXPECT_EQ(utils::join(parts, ","), "");
}

TEST(StringManipulationTest, JoinSingleElement) {
    std::vector<std::string> parts{"one"};
    EXPECT_EQ(utils::join(parts, ","), "one");
}

TEST(StringManipulationTest, JoinMultipleElements) {
    std::vector<std::string> parts{"one", "two", "three"};
    EXPECT_EQ(utils::join(parts, ", "), "one, two, three");
}

TEST(StringManipulationTest, JoinEmptyDelimiter) {
    std::vector<std::string> parts{"one", "two", "three"};
    EXPECT_EQ(utils::join(parts, ""), "onetwothree");
}

TEST(StringManipulationTest, JoinEmptyStringsInVector) {
    std::vector<std::string> parts{"", "two", ""};
    EXPECT_EQ(utils::join(parts, "-"), "-two-");
}

} // namespace
