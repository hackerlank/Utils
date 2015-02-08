#include "../cutil.h"

#include <gtest/gtest.h>

/* ×Ö·ûÀàÐÍÅÐ¶Ï */
TEST(Char, Type)
{
    EXPECT_TRUE(ISXDIGIT('0'));
    EXPECT_TRUE(ISXDIGIT('9'));
    EXPECT_TRUE(ISXDIGIT('a'));
    EXPECT_TRUE(ISXDIGIT('f'));
    EXPECT_FALSE(ISXDIGIT('g'));
    EXPECT_FALSE(ISXDIGIT('z'));
    EXPECT_TRUE(ISXDIGIT('A'));
    EXPECT_TRUE(ISXDIGIT('F'));
    EXPECT_FALSE(ISXDIGIT('G'));
    EXPECT_FALSE(ISXDIGIT('Z'));
    EXPECT_FALSE(ISXDIGIT(0));
    EXPECT_FALSE(ISXDIGIT('\n'));
    EXPECT_FALSE(ISXDIGIT(INT_MAX));

    EXPECT_TRUE(xisdigit('0'));
    EXPECT_TRUE(xisdigit('9'));
    EXPECT_FALSE(xisdigit('a'));
    EXPECT_FALSE(xisdigit('/'));
    EXPECT_FALSE(xisdigit('\0'));
    EXPECT_FALSE(xisdigit('\xFF'));
    EXPECT_FALSE(xisdigit(INT_MAX));

    EXPECT_TRUE(xisascii('a'));
    EXPECT_TRUE(xisascii('6'));
    EXPECT_TRUE(xisascii('*'));
    EXPECT_TRUE(xisascii('\n'));
    EXPECT_TRUE(xisascii(' '));
    EXPECT_FALSE(xisascii(-1));
    EXPECT_FALSE(xisascii(INT_MAX));

    EXPECT_TRUE(xisalpha('a'));
    EXPECT_TRUE(xisalpha('A'));
    EXPECT_FALSE(xisalpha('0'));
    EXPECT_FALSE(xisalpha(' '));
    EXPECT_FALSE(xisalpha(INT_MAX));

    EXPECT_TRUE(xisalnum('a'));
    EXPECT_TRUE(xisalnum('A'));
    EXPECT_TRUE(xisalnum('0'));
    EXPECT_FALSE(xisalnum(' '));
    EXPECT_FALSE(xisalnum(INT_MAX));

    EXPECT_TRUE(xisupper('A'));
    EXPECT_TRUE(xisupper('Z'));
    EXPECT_FALSE(xisupper('a'));
    EXPECT_FALSE(xisupper('z'));
    EXPECT_FALSE(xisupper(' '));
    EXPECT_FALSE(xisupper(INT_MAX));

    EXPECT_TRUE(xislower('a'));
    EXPECT_TRUE(xislower('z'));
    EXPECT_FALSE(xislower('A'));
    EXPECT_FALSE(xislower('Z'));
    EXPECT_FALSE(xislower(' '));
    EXPECT_FALSE(xislower(INT_MAX));
}
