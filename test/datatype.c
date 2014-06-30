#include "datatype.h"
#include "../cutil.h"

/* 常用数据类型的一致性 */
void datetype_limits()
{
	/* char */
	CU_ASSERT_EQUAL(CHAR_BIT, 8);

	/* short */
	CU_ASSERT_EQUAL(SHRT_MAX, 32767);
	CU_ASSERT_EQUAL(SHRT_MIN, -32768);
	CU_ASSERT_EQUAL(USHRT_MAX, 65535);

	/* int */
	CU_ASSERT_EQUAL(INT_MAX, 2147483647);
	CU_ASSERT_TRUE(INT_MIN == -2147483647-1);
	CU_ASSERT_TRUE(UINT_MAX == 0xffffffff);

	/* long */
#if __WORDSIZE == 32
	CU_ASSERT_EQUAL(LONG_MAX, 2147483647L);
	CU_ASSERT_EQUAL(LONG_MIN, -2147483647L-1);
	CU_ASSERT_TRUE(ULONG_MAX == 0xffffffffUL);

	/* size_t */
	CU_ASSERT_EQUAL(SIZE_T_MAX, UINT_MAX);
	CU_ASSERT_EQUAL(INFINITE, (size_t)-1);
#endif

	/* int32_t */

	/* int64_t */

	CU_ASSERT_EQUAL(TYPE_MAXIMUM(int), INT_MAX);
	CU_ASSERT_EQUAL(TYPE_MAXIMUM(short), SHRT_MAX);
	CU_ASSERT_EQUAL(TYPE_MAXIMUM(long), LONG_MAX);
	CU_ASSERT_EQUAL(TYPE_MAXIMUM(int64_t), INT64_MAX);
}

/* 自定义的无符号类型 */
void datetype_unsigned()
{
	CU_ASSERT_EQUAL(sizeof(byte), sizeof(char));
	CU_ASSERT_EQUAL(sizeof(ushort), sizeof(short));
	CU_ASSERT_EQUAL(sizeof(uint), sizeof(int));
	CU_ASSERT_EQUAL(sizeof(ulong), sizeof(long));
}

/* 测试格式化输入输出 */
void datetype_format()
{
	char buf[30];

	/* int64_t */
	snprintf(buf, sizeof(buf), "%"PRId64, (int64_t)INT64_MAX);
	CU_ASSERT_STRING_EQUAL(buf, "9223372036854775807");

	/* uint64_t */
	snprintf(buf, sizeof(buf), "%"PRIu64, (uint64_t)UINT64_MAX);
	CU_ASSERT_STRING_EQUAL(buf, "18446744073709551615");

	/* size_t */
	snprintf(buf, sizeof(buf), "%"PRIuS, SIZE_T_MAX);
	CU_ASSERT_STRING_EQUAL(buf, "4294967295");
}

/* 测试数据类型长度 */
void datetype_length()
{
#ifdef _WIN32
#if __WORDSIZE == 32
	CU_ASSERT_EQUAL(sizeof(char), 1);
	CU_ASSERT_EQUAL(sizeof(short), 2);
	CU_ASSERT_EQUAL(sizeof(int), 4);
	CU_ASSERT_EQUAL(sizeof(long), 4);
	CU_ASSERT_EQUAL(sizeof(int64_t), 8);
	CU_ASSERT_EQUAL(sizeof(float), 4);
	CU_ASSERT_EQUAL(sizeof(double), 8);
	CU_ASSERT_EQUAL(sizeof(void*), 4);
	CU_ASSERT_EQUAL(sizeof(size_t), 4);
	CU_ASSERT_EQUAL(sizeof(wchar_t), 2);
#if _MSC_VER > MSVC6
	CU_ASSERT_EQUAL(sizeof(long long), 8);
	CU_ASSERT_EQUAL(sizeof(long double), 8);
#endif
#else  // !__WORDSIZE == 32

#endif // __WORDSIZE == 32
#else  // !_WIN32
#if __WORDSIZE == 32

#else //Linux 64

#endif
#endif //_WIN32
}
