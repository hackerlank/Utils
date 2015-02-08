#include "../cutil.h"
#include <gtest/gtest.h>

/* 常用数据类型的一致性 */
TEST(Datetype, Basic)
{
	/* char */
	EXPECT_EQ(CHAR_BIT, 8);

	/* short */
	EXPECT_EQ(SHRT_MAX, 32767);
	EXPECT_EQ(SHRT_MIN, -32768);
	EXPECT_EQ(USHRT_MAX, 65535);

	/* int */
	EXPECT_EQ(INT_MAX, 2147483647);
	EXPECT_TRUE(INT_MIN == -2147483647-1);
	EXPECT_TRUE(UINT_MAX == 0xffffffff);

	/* long */
#if __WORDSIZE == 32
	EXPECT_EQ(LONG_MAX, 2147483647L);
	EXPECT_EQ(LONG_MIN, -2147483647L-1);
	EXPECT_TRUE(ULONG_MAX == 0xffffffffUL);

	/* size_t */
	EXPECT_EQ(SIZE_T_MAX, UINT_MAX);
	EXPECT_EQ(INFINITE, (size_t)-1);
#endif

	/* int32_t */

	/* int64_t */

	EXPECT_EQ(TYPE_MAXIMUM(int), INT_MAX);
	EXPECT_EQ(TYPE_MAXIMUM(short), SHRT_MAX);
	EXPECT_EQ(TYPE_MAXIMUM(long), LONG_MAX);
	EXPECT_EQ(TYPE_MAXIMUM(int64_t), INT64_MAX);
}

/* 自定义的无符号类型 */
TEST(Datetype, Unsigned)
{
	EXPECT_EQ(sizeof(byte), sizeof(char));
	EXPECT_EQ(sizeof(ushort), sizeof(short));
	EXPECT_EQ(sizeof(uint), sizeof(int));
	EXPECT_EQ(sizeof(ulong), sizeof(long));
}

/* 测试格式化输入输出 */
TEST(Datetype, Format)
{
	char buf[30];

	/* int64_t */
	snprintf(buf, sizeof(buf), "%" PRId64, (int64_t)INT64_MAX);
	EXPECT_STREQ(buf, "9223372036854775807");

	/* uint64_t */
	snprintf(buf, sizeof(buf), "%" PRIu64, (uint64_t)UINT64_MAX);
	EXPECT_STREQ(buf, "18446744073709551615");

	/* size_t */
	snprintf(buf, sizeof(buf), "%" PRIuS, SIZE_T_MAX);
#if __WORDSIZE == 64
    EXPECT_STREQ(buf, "18446744073709551615");
#else
	EXPECT_STREQ(buf, "4294967295");
#endif
}

/* 测试数据类型长度 */
TEST(Datetype, Length)
{
#ifdef _WIN32
#if __WORDSIZE == 32
	EXPECT_EQ(sizeof(char), 1);
	EXPECT_EQ(sizeof(short), 2);
	EXPECT_EQ(sizeof(int), 4);
	EXPECT_EQ(sizeof(long), 4);
	EXPECT_EQ(sizeof(int64_t), 8);
	EXPECT_EQ(sizeof(float), 4);
	EXPECT_EQ(sizeof(double), 8);
	EXPECT_EQ(sizeof(void*), 4);
	EXPECT_EQ(sizeof(size_t), 4);
	EXPECT_EQ(sizeof(wchar_t), 2);
#if _MSC_VER > MSVC6
	EXPECT_EQ(sizeof(long long), 8);
	EXPECT_EQ(sizeof(long double), 8);
#endif
#else  // !__WORDSIZE == 32

#endif // __WORDSIZE == 32
#else  // !_WIN32
#if __WORDSIZE == 32

#else //Linux 64

#endif
#endif //_WIN32
}
