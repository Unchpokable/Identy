#include <gtest/gtest.h>
#include <string_view>

#include <Identy_strings.hxx>

namespace identy::test
{

// ============================================================================
// trim_whitespace() Tests
// ============================================================================

TEST(StringsTest, TrimWhitespace_LeadingSpaces)
{
    auto result = strings::trim_whitespace("  abc");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_TrailingSpaces)
{
    auto result = strings::trim_whitespace("abc  ");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_BothSides)
{
    auto result = strings::trim_whitespace("  abc  ");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_EmptyString)
{
    auto result = strings::trim_whitespace("");
    EXPECT_EQ(result, "");
    EXPECT_TRUE(result.empty());
}

TEST(StringsTest, TrimWhitespace_OnlySpaces)
{
    auto result = strings::trim_whitespace("   ");
    EXPECT_EQ(result, "");
    EXPECT_TRUE(result.empty());
}

TEST(StringsTest, TrimWhitespace_NoSpaces)
{
    auto result = strings::trim_whitespace("abc");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_SingleCharacter)
{
    auto result = strings::trim_whitespace("a");
    EXPECT_EQ(result, "a");
}

TEST(StringsTest, TrimWhitespace_SingleSpace)
{
    auto result = strings::trim_whitespace(" ");
    EXPECT_EQ(result, "");
    EXPECT_TRUE(result.empty());
}

TEST(StringsTest, TrimWhitespace_Tabs)
{
    auto result = strings::trim_whitespace("\tabc\t");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_Newlines)
{
    auto result = strings::trim_whitespace("\nabc\n");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_CarriageReturn)
{
    auto result = strings::trim_whitespace("\rabc\r");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_MixedWhitespace)
{
    auto result = strings::trim_whitespace(" \t\n\rabc \t\n\r");
    EXPECT_EQ(result, "abc");
}

TEST(StringsTest, TrimWhitespace_InternalSpacesPreserved)
{
    auto result = strings::trim_whitespace("  abc def  ");
    EXPECT_EQ(result, "abc def");
}

TEST(StringsTest, TrimWhitespace_InternalTabsPreserved)
{
    auto result = strings::trim_whitespace("  abc\tdef  ");
    EXPECT_EQ(result, "abc\tdef");
}

TEST(StringsTest, TrimWhitespace_MultipleInternalSpaces)
{
    auto result = strings::trim_whitespace("abc   def");
    EXPECT_EQ(result, "abc   def");
}

TEST(StringsTest, TrimWhitespace_ReturnsStringView)
{
    // Verify return type is string_view
    std::string_view input = "  test  ";
    auto result = strings::trim_whitespace(input);

    static_assert(std::is_same_v<decltype(result), std::string_view>,
        "trim_whitespace should return string_view");
}

TEST(StringsTest, TrimWhitespace_ViewPointsToOriginal)
{
    std::string original = "  hello  ";
    std::string_view input = original;
    auto result = strings::trim_whitespace(input);

    // Result should point into original string
    EXPECT_GE(result.data(), original.data());
    EXPECT_LT(result.data(), original.data() + original.size());
}

TEST(StringsTest, TrimWhitespace_LongString)
{
    std::string long_string(1000, ' ');
    long_string[500] = 'x';

    auto result = strings::trim_whitespace(long_string);
    EXPECT_EQ(result, "x");
}

TEST(StringsTest, TrimWhitespace_UnicodeNotAffected)
{
    // Non-ASCII characters should not be treated as whitespace
    auto result = strings::trim_whitespace("  \xC3\xA9  ");  // 'e' with accent in UTF-8
    EXPECT_EQ(result, "\xC3\xA9");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(StringsTest, TrimWhitespace_AllWhitespaceTypes)
{
    // Test common whitespace characters (space, tab, newline, carriage return)
    // Note: implementation may not trim all C whitespace types (\v, \f)
    auto result = strings::trim_whitespace(" \t\n\rtest \t\n\r");
    EXPECT_EQ(result, "test");
}

TEST(StringsTest, TrimWhitespace_NullCharactersNotWhitespace)
{
    // Null characters are not whitespace
    std::string with_null = "  a";
    with_null.push_back('\0');
    with_null += "b  ";

    auto result = strings::trim_whitespace(with_null);
    // Should contain the null character in the middle
    EXPECT_GT(result.size(), 2u);
}

} // namespace identy::test
