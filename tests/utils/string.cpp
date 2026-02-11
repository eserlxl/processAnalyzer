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

// --- trim ---
TEST(StringTest, TrimNoWhitespace) {
    EXPECT_EQ(utils::trim("hello"), "hello");
}

TEST(StringTest, TrimLeadingWhitespace) {
    EXPECT_EQ(utils::trim("  hello"), "hello");
}

TEST(StringTest, TrimTrailingWhitespace) {
    EXPECT_EQ(utils::trim("hello  "), "hello");
}

TEST(StringTest, TrimLeadingAndTrailingWhitespace) {
    EXPECT_EQ(utils::trim("  hello  "), "hello");
}

TEST(StringTest, TrimAllWhitespace) {
    EXPECT_EQ(utils::trim("   "), "");
}

TEST(StringTest, TrimEmptyString) {
    EXPECT_EQ(utils::trim(""), "");
}

TEST(StringTest, TrimMixedWhitespace) {
    EXPECT_EQ(utils::trim("\t\n hello \r\f "), "hello");
}

TEST(StringTest, TrimInternalWhitespace) {
    EXPECT_EQ(utils::trim("hello world"), "hello world");
}

// --- startsWith ---
TEST(StringTest, StartsWithPositive) {
    EXPECT_TRUE(utils::startsWith("hello world", "hello"));
}

TEST(StringTest, StartsWithNegative) {
    EXPECT_FALSE(utils::startsWith("hello world", "world"));
}

TEST(StringTest, StartsWithEmptyPrefix) {
    EXPECT_TRUE(utils::startsWith("hello world", ""));
}

TEST(StringTest, StartsWithEmptyString) {
    EXPECT_FALSE(utils::startsWith("", "hello"));
}

TEST(StringTest, StartsWithPrefixLongerThanString) {
    EXPECT_FALSE(utils::startsWith("hi", "hello"));
}

TEST(StringTest, StartsWithExactMatch) {
    EXPECT_TRUE(utils::startsWith("hello", "hello"));
}

// --- endsWith ---
TEST(StringTest, EndsWithPositive) {
    EXPECT_TRUE(utils::endsWith("hello world", "world"));
}

TEST(StringTest, EndsWithNegative) {
    EXPECT_FALSE(utils::endsWith("hello world", "hello"));
}

TEST(StringTest, EndsWithEmptySuffix) {
    EXPECT_TRUE(utils::endsWith("hello world", ""));
}

TEST(StringTest, EndsWithEmptyString) {
    EXPECT_FALSE(utils::endsWith("", "world"));
}

TEST(StringTest, EndsWithSuffixLongerThanString) {
    EXPECT_FALSE(utils::endsWith("hi", "world"));
}

TEST(StringTest, EndsWithExactMatch) {
    EXPECT_TRUE(utils::endsWith("world", "world"));
}

// --- contains ---
TEST(StringTest, ContainsPositive) {
    EXPECT_TRUE(utils::contains("hello world", "lo wo"));
}

TEST(StringTest, ContainsNegative) {
    EXPECT_FALSE(utils::contains("hello world", "foo"));
}

TEST(StringTest, ContainsEmptySubstring) {
    EXPECT_TRUE(utils::contains("hello world", ""));
}

TEST(StringTest, ContainsEmptyString) {
    EXPECT_FALSE(utils::contains("", "foo"));
}

TEST(StringTest, ContainsSubstringLongerThanString) {
    EXPECT_FALSE(utils::contains("hi", "hello"));
}

// --- startsWithIgnoreCase ---
TEST(StringTest, StartsWithIgnoreCasePositive) {
    EXPECT_TRUE(utils::startsWithIgnoreCase("Hello World", "hello"));
    EXPECT_TRUE(utils::startsWithIgnoreCase("hello World", "Hello"));
    EXPECT_TRUE(utils::startsWithIgnoreCase("HELLO World", "heLlO"));
}

TEST(StringTest, StartsWithIgnoreCaseNegative) {
    EXPECT_FALSE(utils::startsWithIgnoreCase("Hello World", "world"));
}

TEST(StringTest, StartsWithIgnoreCaseEmptyPrefix) {
    EXPECT_TRUE(utils::startsWithIgnoreCase("Hello World", ""));
}

TEST(StringTest, StartsWithIgnoreCaseEmptyString) {
    EXPECT_FALSE(utils::startsWithIgnoreCase("", "hello"));
}

// --- endsWithIgnoreCase ---
TEST(StringTest, EndsWithIgnoreCasePositive) {
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello World", "world"));
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello world", "World"));
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello WORLD", "wOrLd"));
}

TEST(StringTest, EndsWithIgnoreCaseNegative) {
    EXPECT_FALSE(utils::endsWithIgnoreCase("Hello World", "hello"));
}

TEST(StringTest, EndsWithIgnoreCaseEmptySuffix) {
    EXPECT_TRUE(utils::endsWithIgnoreCase("Hello World", ""));
}

TEST(StringTest, EndsWithIgnoreCaseEmptyString) {
    EXPECT_FALSE(utils::endsWithIgnoreCase("", "world"));
}

// --- containsIgnoreCase ---
TEST(StringTest, ContainsIgnoreCasePositive) {
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "lo wo"));
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "LO WO"));
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", "Lo wO"));
}

TEST(StringTest, ContainsIgnoreCaseNegative) {
    EXPECT_FALSE(utils::containsIgnoreCase("Hello World", "foo"));
}

TEST(StringTest, ContainsIgnoreCaseEmptySubstring) {
    EXPECT_TRUE(utils::containsIgnoreCase("Hello World", ""));
}

TEST(StringTest, ContainsIgnoreCaseEmptyString) {
    EXPECT_FALSE(utils::containsIgnoreCase("", "foo"));
}

TEST(StringTest, CaseInsensitiveNonASCII) {
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

// --- toLower ---
TEST(StringTest, ToLowerAllUppercase) {
    EXPECT_EQ(utils::toLower("HELLO"), "hello");
}

TEST(StringTest, ToLowerAllLowercase) {
    EXPECT_EQ(utils::toLower("hello"), "hello");
}

TEST(StringTest, ToLowerMixedCase) {
    EXPECT_EQ(utils::toLower("HeLlO WoRlD"), "hello world");
}

TEST(StringTest, ToLowerNonAlphabetic) {
    EXPECT_EQ(utils::toLower("123!@#"), "123!@#");
}

TEST(StringTest, ToLowerEmptyString) {
    EXPECT_EQ(utils::toLower(""), "");
}

TEST(StringTest, ToLowerNonASCII) {
    // Should not change non-ASCII characters
    EXPECT_EQ(utils::toLower("Grüße"), "grüße"); // 'ü' is not in A-Z,a-z
}

// --- toUpper ---
TEST(StringTest, ToUpperAllLowercase) {
    EXPECT_EQ(utils::toUpper("hello"), "HELLO");
}

TEST(StringTest, ToUpperAllUppercase) {
    EXPECT_EQ(utils::toUpper("HELLO"), "HELLO");
}

TEST(StringTest, ToUpperMixedCase) {
    EXPECT_EQ(utils::toUpper("HeLlO WoRlD"), "HELLO WORLD");
}

TEST(StringTest, ToUpperNonAlphabetic) {
    EXPECT_EQ(utils::toUpper("123!@#"), "123!@#");
}

TEST(StringTest, ToUpperEmptyString) {
    EXPECT_EQ(utils::toUpper(""), "");
}

TEST(StringTest, ToUpperNonASCII) {
    EXPECT_EQ(utils::toUpper("Grüße"), "GRüßE"); // 'ü' and 'ß' are not converted as they are non-ASCII. 
}

// --- replaceAll ---
TEST(StringTest, ReplaceAllSingleOccurrence) {
    EXPECT_EQ(utils::replaceAll("one two three", "two", "2"), "one 2 three");
}

TEST(StringTest, ReplaceAllMultipleOccurrences) {
    EXPECT_EQ(utils::replaceAll("one two one three one", "one", "1"), "1 two 1 three 1");
}

TEST(StringTest, ReplaceAllNoOccurrence) {
    EXPECT_EQ(utils::replaceAll("one two three", "four", "4"), "one two three");
}

TEST(StringTest, ReplaceAllEmptyTarget) {
    EXPECT_EQ(utils::replaceAll("abc", "", "X"), "abc"); // Spec says target.empty() returns original
}

TEST(StringTest, ReplaceAllEmptyReplacement) {
    EXPECT_EQ(utils::replaceAll("hello world", "world", ""), "hello ");
}

TEST(StringTest, ReplaceAllTargetLongerThanReplacement) {
    EXPECT_EQ(utils::replaceAll("banana", "ana", "X"), "bXna");
}

TEST(StringTest, ReplaceAllReplacementLongerThanTarget) {
    EXPECT_EQ(utils::replaceAll("apple", "p", "pp"), "apppple");
}

TEST(StringTest, ReplaceAllEmptyString) {
    EXPECT_EQ(utils::replaceAll("", "a", "b"), "");
}

TEST(StringTest, ReplaceAllTargetAtBeginning) {
    EXPECT_EQ(utils::replaceAll("test string", "test", "new"), "new string");
}

TEST(StringTest, ReplaceAllTargetAtEnd) {
    EXPECT_EQ(utils::replaceAll("test string", "string", "text"), "test text");
}

TEST(StringTest, ReplaceAllOverlapping) {
    // Ensure non-overlapping replacement strategy
    EXPECT_EQ(utils::replaceAll("aaaa", "aa", "b"), "bb");
    EXPECT_EQ(utils::replaceAll("aaaaa", "aa", "b"), "bba");
}



// --- replaceFirst ---
TEST(StringTest, ReplaceFirstSingleOccurrence) {
    EXPECT_EQ(utils::replaceFirst("one two three", "two", "2"), "one 2 three");
}

TEST(StringTest, ReplaceFirstMultipleOccurrences) {
    EXPECT_EQ(utils::replaceFirst("one two one three one", "one", "1"), "1 two one three one");
}

TEST(StringTest, ReplaceFirstNoOccurrence) {
    EXPECT_EQ(utils::replaceFirst("one two three", "four", "4"), "one two three");
}

TEST(StringTest, ReplaceFirstEmptyTarget) {
    // Corrected expected value: replaceFirst on empty target should return original string
    EXPECT_EQ(utils::replaceFirst("abc", "", "X"), "abc");
}

TEST(StringTest, ReplaceFirstEmptyReplacement) {
    EXPECT_EQ(utils::replaceFirst("hello world", "world", ""), "hello ");
}

TEST(StringTest, ReplaceFirstEmptyString) {
    EXPECT_EQ(utils::replaceFirst("", "a", "b"), "");
}



// --- replaceN ---
TEST(StringTest, ReplaceNOneOccurrence) {
    EXPECT_EQ(utils::replaceN("one one one", "one", "X", 1), "X one one");
}

TEST(StringTest, ReplaceNTwoOccurrences) {
    EXPECT_EQ(utils::replaceN("one one one", "one", "X", 2), "X X one");
}

TEST(StringTest, ReplaceNMoreThanAvailable) {
    EXPECT_EQ(utils::replaceN("one one one", "one", "X", 5), "X X X");
}

TEST(StringTest, ReplaceNNoTarget) {
    EXPECT_EQ(utils::replaceN("one two three", "four", "X", 1), "one two three");
}

TEST(StringTest, ReplaceNEmptyTarget) {
    EXPECT_EQ(utils::replaceN("abc", "", "X", 1), "abc"); // Behaves like replaceAll if target empty
}

TEST(StringTest, ReplaceNEmptyString) {
    EXPECT_EQ(utils::replaceN("", "a", "b", 1), "");
}



// --- join ---
TEST(StringTest, JoinEmptyVector) {
    std::vector<std::string> parts{};
    EXPECT_EQ(utils::join(parts, ","), "");
}

TEST(StringTest, JoinSingleElement) {
    std::vector<std::string> parts{"one"};
    EXPECT_EQ(utils::join(parts, ","), "one");
}

TEST(StringTest, JoinMultipleElements) {
    std::vector<std::string> parts{"one", "two", "three"};
    EXPECT_EQ(utils::join(parts, ", "), "one, two, three");
}

TEST(StringTest, JoinEmptyDelimiter) {
    std::vector<std::string> parts{"one", "two", "three"};
    EXPECT_EQ(utils::join(parts, ""), "onetwothree");
}

TEST(StringTest, JoinEmptyStringsInVector) {
    std::vector<std::string> parts{"", "two", ""};
    EXPECT_EQ(utils::join(parts, "-"), "-two-");
}

// --- format ---
TEST(StringTest, FormatBasic) {
    EXPECT_EQ(utils::format("Hello, {}!", "World"), "Hello, World!");
}

TEST(StringTest, FormatBasicWithInt) {
    EXPECT_EQ(utils::format("The answer is {}.", 42), "The answer is 42.");
}

TEST(StringTest, FormatMultipleArgs) {
    EXPECT_EQ(utils::format("{}, {} and {}.", "one", 2, 3.0F), "one, 2 and 3."); // std::format default precision for float
}

TEST(StringTest, FormatOrderedArgs) {
    EXPECT_EQ(utils::format("{1}, {0}.", "world", "Hello"), "Hello, world.");
}

TEST(StringTest, FormatMixedTypes) {
    EXPECT_EQ(utils::format("Value: {:05d}, Ratio: {:.2f}", 123, 0.12345), "Value: 00123, Ratio: 0.12");
}

// TEST(StringTest, FormatCompileTimeError) {
//     // This test is to ensure compile-time check works, should fail to compile if format string is invalid
//     // EXPECT_EQ(format("Value: {:s}", 123), ""); // Should cause compile error
// }

// --- split(char) ---
TEST(StringTest, SplitCharBasic) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one,two,three", ','), expected);
}

TEST(StringTest, SplitCharMultipleDelimiters) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one,,two,,,three", ',', true), expected);
}

TEST(StringTest, SplitCharLeadingDelimiter) {
    std::vector<std::string> expected = {"", "one", "two"};
    EXPECT_EQ(utils::split(",one,two", ',', false), expected);
}

TEST(StringTest, SplitCharTrailingDelimiter) {
    std::vector<std::string> expected = {"one", "two", ""};
    EXPECT_EQ(utils::split("one,two,", ',', false), expected);
}

TEST(StringTest, SplitCharOnlyDelimitersSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split(",,,", ',', true), expected);
}

TEST(StringTest, SplitCharOnlyDelimitersSkipEmptyFalse) {
    std::vector<std::string> expected = {"", "", "", ""};
    EXPECT_EQ(utils::split(",,,", ',', false), expected);
}

TEST(StringTest, SplitCharEmptyStringSkipEmptyFalse) {
    std::vector<std::string> expected = {""};
    EXPECT_EQ(utils::split("", ',', false), expected);
}

TEST(StringTest, SplitCharEmptyStringSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split("", ',', true), expected);
}

// --- split(string_view) ---
TEST(StringTest, SplitStringViewBasic) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one<->two<->three", "<->"), expected);
}

TEST(StringTest, SplitStringViewMultipleDelimiters) {
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(utils::split("one<-><->two<-><-><->three", "<->", true), expected);
}

TEST(StringTest, SplitStringViewLeadingDelimiter) {
    std::vector<std::string> expected = {"", "one", "two"};
    EXPECT_EQ(utils::split("<->one<->two", "<->", false), expected);
}

TEST(StringTest, SplitStringViewTrailingDelimiter) {
    std::vector<std::string> expected = {"one", "two", ""};
    EXPECT_EQ(utils::split("one<->two<->", "<->", false), expected);
}

TEST(StringTest, SplitStringViewOnlyDelimitersSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split("<-><-><->", "<->", true), expected);
}

TEST(StringTest, SplitStringViewOnlyDelimitersSkipEmptyFalse) {
    std::vector<std::string> expected = {"", "", "", ""};
    EXPECT_EQ(utils::split("<-><-><->", "<->", false), expected);
}

TEST(StringTest, SplitStringViewEmptyDelimiter) {
    std::vector<std::string> expected = {"h", "e", "l", "l", "o"};
    EXPECT_EQ(utils::split("hello", "", false), expected); // Empty delimiter now splits to chars
    EXPECT_EQ(utils::split("hello", "", true), expected); // skipEmpty shouldn't affect chars
}

TEST(StringTest, SplitStringViewEmptyStringSkipEmptyFalse) {
    std::vector<std::string> expected = {""};
    EXPECT_EQ(utils::split("", "<->", false), expected);
}

TEST(StringTest, SplitStringViewEmptyStringSkipEmptyTrue) {
    std::vector<std::string> expected = {};
    EXPECT_EQ(utils::split("", "<->", true), expected);
}

TEST(StringTest, SplitStringViewOverlapping) {
    // "ababab", split by "aba"
    // First "aba" matches at 0. Remaining: "bab".
    // "bab" does not contain "aba".
    // Result: {"", "bab"} (since first part before "aba" is empty)
    std::vector<std::string> expected = {"", "bab"};
    EXPECT_EQ(utils::split("ababab", "aba", false), expected);
}

// --- isInteger ---
TEST(StringTest, IsIntegerValid) {
    EXPECT_TRUE(utils::isInteger("123"));
    EXPECT_TRUE(utils::isInteger("-456"));
    EXPECT_TRUE(utils::isInteger("0"));
    EXPECT_TRUE(utils::isInteger("+789"));
    EXPECT_TRUE(utils::isInteger("   123   ")); // With whitespace
}

TEST(StringTest, IsIntegerInvalid) {
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
TEST(StringTest, IsFloatingPointValid) {
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

TEST(StringTest, IsFloatingPointInvalid) {
    EXPECT_FALSE(utils::isFloatingPoint("abc"));
    EXPECT_FALSE(utils::isFloatingPoint(""));
    EXPECT_FALSE(utils::isFloatingPoint("12a.45"));
    EXPECT_FALSE(utils::isFloatingPoint("--1.0"));
    EXPECT_FALSE(utils::isFloatingPoint("   "));
}

// --- toLong ---
TEST(StringTest, ToLongValid) {
    EXPECT_EQ(utils::toLong("123").value(), 123L);
    EXPECT_EQ(utils::toLong("-456").value(), -456L);
    EXPECT_EQ(utils::toLong("0").value(), 0L);
    EXPECT_EQ(utils::toLong("+789").value(), 789L);
    EXPECT_EQ(utils::toLong("   1000   ").value(), 1000L); // With whitespace
    EXPECT_EQ(utils::toLong("FF", 16).value(), 255L);
    EXPECT_EQ(utils::toLong("101", 2).value(), 5L); // Binary
    EXPECT_EQ(utils::toLong("10", 8).value(), 8L); // Octal
}

TEST(StringTest, ToLongInvalid) {
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

TEST(StringTest, ToLongOutOfRange) {
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
TEST(StringTest, ToDoubleValid) {
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

TEST(StringTest, ToDoubleInvalid) {
    EXPECT_TRUE(hasError(utils::toDouble("abc"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("   "), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toDouble("12a.45"), utils::UtilsError::invalidArgument)); // Partial parse
}

TEST(StringTest, ToDoubleOutOfRange) {
    std::string overflowStr = "1e+1000";
    EXPECT_TRUE(hasError(utils::toDouble(overflowStr), utils::UtilsError::outOfRange));

    std::string underflowStr = "-1e+1000";
    EXPECT_TRUE(hasError(utils::toDouble(underflowStr), utils::UtilsError::outOfRange));
}

TEST(StringTest, ToDoubleSpecialValues) {
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
TEST(StringTest, ParseBoolValid) {
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

TEST(StringTest, ParseBoolInvalid) {
    EXPECT_TRUE(hasError(utils::parseBool("yes"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool("no"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool("other"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::parseBool("   "), utils::UtilsError::invalidArgument));
}

// --- toInt ---
TEST(StringTest, ToIntValid) {
    EXPECT_EQ(utils::toInt("123").value(), 123);
    EXPECT_EQ(utils::toInt("-456").value(), -456);
    EXPECT_EQ(utils::toInt("0").value(), 0);
    EXPECT_EQ(utils::toInt("+789").value(), 789);
    EXPECT_EQ(utils::toInt("   1000   ").value(), 1000); // With whitespace
    EXPECT_EQ(utils::toInt("FF", 16).value(), 255);
    EXPECT_EQ(utils::toInt("101", 2).value(), 5); // Binary
    EXPECT_EQ(utils::toInt("77", 8).value(), 63); // Octal
}

TEST(StringTest, ToIntInvalid) {
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

TEST(StringTest, ToIntOutOfRange) {
    // Max int + 1 (2147483648 for 32-bit int)
    std::string overflowStr = "2147483648";
    EXPECT_TRUE(hasError(utils::toInt(overflowStr), utils::UtilsError::outOfRange));

    // Min int - 1 (-2147483649 for 32-bit int)
    std::string underflowStr = "-2147483649";
    EXPECT_TRUE(hasError(utils::toInt(underflowStr), utils::UtilsError::outOfRange));
}

// --- toFloat ---
TEST(StringTest, ToFloatValid) {
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

TEST(StringTest, ToFloatInvalid) {
    EXPECT_TRUE(hasError(utils::toFloat("abc"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat(""), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("   "), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("12a.45"), utils::UtilsError::invalidArgument)); // Partial parse
}

TEST(StringTest, ToFloatOutOfRange) {
    std::string overflowStr = "1e+100";
    EXPECT_TRUE(hasError(utils::toFloat(overflowStr), utils::UtilsError::outOfRange));

    std::string underflowStr = "-1e+100";
    EXPECT_TRUE(hasError(utils::toFloat(underflowStr), utils::UtilsError::outOfRange));
}

TEST(StringTest, ToFloatSpecialValues) {
    // Test for infinity
    EXPECT_TRUE(hasError(utils::toFloat("inf"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("+inf"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("-inf"), utils::UtilsError::invalidArgument));

    // Test for NaN
    EXPECT_TRUE(hasError(utils::toFloat("nan"), utils::UtilsError::invalidArgument));
    EXPECT_TRUE(hasError(utils::toFloat("NaN"), utils::UtilsError::invalidArgument));
}

// --- tryParse ---
TEST(StringTest, TryParseIntegerValid) {
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

TEST(StringTest, TryParseIntegerInvalid) {
    int val;
    EXPECT_FALSE(utils::tryParse("123.45", val));
    EXPECT_FALSE(utils::tryParse("abc", val));
    EXPECT_FALSE(utils::tryParse("", val));
    EXPECT_FALSE(utils::tryParse("   ", val));
    EXPECT_FALSE(utils::tryParse("12a", val));
    EXPECT_FALSE(utils::tryParse("123a", val)); // Partial parse
}

TEST(StringTest, TryParseDoubleValid) {
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

TEST(StringTest, TryParseDoubleInvalid) {
    double val;
    EXPECT_FALSE(utils::tryParse("abc", val));
    EXPECT_FALSE(utils::tryParse("", val));
    EXPECT_FALSE(utils::tryParse("   ", val));
    EXPECT_FALSE(utils::tryParse("12a.45", val)); // Partial parse
}

TEST(StringTest, TryParseOutOfRange) {
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

TEST(StringTest, TryParseSpecialValues) {
    double val;
    // tryParse implementation explicitly rejects non-finite values
    EXPECT_FALSE(utils::tryParse("inf", val));
    EXPECT_FALSE(utils::tryParse("-inf", val));
    EXPECT_FALSE(utils::tryParse("nan", val));
}

} // namespace
