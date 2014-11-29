#include "../cutil.h"

#include <gtest/gtest.h>

struct item{
	int i;
	double j;
	long k;
};

TEST(Math, Basic)
{
    char carray[10];
    int i = countof(carray);

    static struct item items[] = {
        { 1, 1.0, 1 },
        { 2, 2.0, 2 },
        { 3, 3.0, 3 },
    };

    EXPECT_EQ(i, 10);
    EXPECT_EQ(countof(items), 3);

    EXPECT_EQ(min(1 + 1, -1), -1);
    EXPECT_EQ(max(0, 2), 2);
}

TEST(Math, Hex)
{
	EXPECT_EQ(ISDIGIT('0'), true);
    EXPECT_EQ(ISDIGIT('9'), true);
	EXPECT_EQ(ISDIGIT('a'), false);
	EXPECT_EQ(ISDIGIT('/'), false);

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

	EXPECT_EQ(XDIGIT_TO_NUM('0'), 0);
	EXPECT_EQ(XDIGIT_TO_NUM('9'), 9);
	EXPECT_EQ(XDIGIT_TO_NUM('a'), 10);
	EXPECT_EQ(XDIGIT_TO_NUM('f'), 15);
	EXPECT_EQ(XDIGIT_TO_NUM('A'), 10);
	EXPECT_EQ(XDIGIT_TO_NUM('F'), 15);

	EXPECT_EQ(X2DIGITS_TO_NUM('0', '0'), 0);
	EXPECT_EQ(X2DIGITS_TO_NUM('0', '9'), 9);
	EXPECT_EQ(X2DIGITS_TO_NUM('0', 'a'), 10);
	EXPECT_EQ(X2DIGITS_TO_NUM('0', 'f'), 15);
	EXPECT_EQ(X2DIGITS_TO_NUM('1', '0'), 16);
	EXPECT_EQ(X2DIGITS_TO_NUM('1', 'f'), 31);
	EXPECT_EQ(X2DIGITS_TO_NUM('9', 'a'), 154);
	EXPECT_EQ(X2DIGITS_TO_NUM('f', 'f'), 255);

	EXPECT_EQ(XNUM_TO_DIGIT(0), '0');
	EXPECT_EQ(XNUM_TO_DIGIT(9), '9');
	EXPECT_EQ(XNUM_TO_DIGIT(10), 'A');
	EXPECT_EQ(XNUM_TO_DIGIT(15), 'F');
	EXPECT_EQ(XNUM_TO_digit(10), 'a');
	EXPECT_EQ(XNUM_TO_digit(15), 'f');
}
