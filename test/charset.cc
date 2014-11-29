#include "../cutil.h"

#include <gtest/gtest.h>
#include <locale.h>

#define SEP PATH_SEP_STR

class Charset : public testing::Test {
protected:
	static void SetUpTestCase() {

	}

	static void TearDownTestCase() {

	}

	// Some expensive resource shared by all tests.
	//static T* shared_resource_;
};

/* 系统本地字符集 */
TEST_F(Charset, Locale)
{
	printf("system locale: %s\n", get_locale());
}

/* 字符编码探测 */
TEST_F(Charset, Detect)
{
	const char *ascii = "abcdedfghnqwernozuv~!@$%^&*()%18231892+_\\\t\n\r|][?><./";
	const char *gb2312 = "这是GB2312编码！";
	const char *gbk = "這是GBK編碼";
	char charset[MAX_CHARSET];
	char* utf8 = NULL;
	size_t ulen = 0;

	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), &utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	EXPECT_TRUE(is_ascii(ascii, strlen(ascii)));			/* 纯ASCII字符 */

	EXPECT_FALSE(is_ascii(gb2312, strlen(gb2312)));
	EXPECT_TRUE(is_gb2312(gb2312, strlen(gb2312)));

	EXPECT_FALSE(is_gb2312(gbk, strlen(gbk)));
	EXPECT_TRUE(is_gbk(gbk, strlen(gbk)));

	EXPECT_FALSE(is_gb2312(utf8, strlen(utf8)));
	EXPECT_TRUE(is_gbk(utf8, strlen(utf8)));	/* UTF-8编码有时会被误认为是GBK编码 */
	EXPECT_TRUE(is_utf8(utf8, strlen(utf8)));

	/* 对于ASCII字符串，如果探测结果可以是ASCII字符串，则返回ASCII; */
	/* 如果不可以返回ASCII，则返回其超集UTF-8 */
	EXPECT_TRUE(get_charset(ascii, strlen(ascii), charset, MAX_CHARSET, 1));
	EXPECT_STREQ(charset, "ASCII");
	EXPECT_TRUE(get_charset(ascii, strlen(ascii), charset, MAX_CHARSET, 0));
	EXPECT_STREQ(charset, "UTF-8");

	/* 对于非ASCII字符串，则返回值总不可能是ASCII */
	EXPECT_TRUE(get_charset(gb2312, strlen(gb2312), charset, MAX_CHARSET, 1));
	EXPECT_STREQ(charset, "GB2312");
	EXPECT_TRUE(get_charset(gb2312, strlen(gb2312), charset, MAX_CHARSET, 0));
	EXPECT_STREQ(charset, "GB2312");

	/* GBK */
	EXPECT_TRUE(get_charset(gbk, strlen(gbk), charset, MAX_CHARSET, 1));
	EXPECT_STREQ(charset, "GBK");

	/* UTF-8 */
	EXPECT_TRUE(get_charset(utf8, strlen(utf8), charset, MAX_CHARSET, 1));
	EXPECT_STREQ(charset, "UTF-8");	/* 首先检测是否是UTF-8编码，立即返回 */

	xfree(utf8);
}

/* UTF-8相关测试 */
TEST_F(Charset, Utf8)
{
	const char *gbk = "這是GBK編碼";
	char trimbuf[128];
	char* utf8 = NULL;
	size_t ulen = 0, i;

	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), &utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	/* UTF-8 字符数 */
	EXPECT_EQ(utf8_len(utf8), 7);

	/* UTF-8 截取 */
	for (i = 0; i <= strlen(utf8); i++)
	{
		size_t leave = utf8_trim(utf8, trimbuf, i);

		switch(i)
		{
		case 0:
		case 1:
		case 2:
			EXPECT_EQ(leave, 0);
			EXPECT_EQ(strlen(trimbuf), 0);
			break;
		case 3:
		case 4:
		case 5:
			EXPECT_EQ(leave, 3);
			EXPECT_EQ(strlen(trimbuf), 3);
			break;
		case 6:
			EXPECT_EQ(leave, 6);
			EXPECT_EQ(strlen(trimbuf), 6);
			break;
		case 7:
			EXPECT_EQ(leave, 7);
			EXPECT_EQ(strlen(trimbuf), 7);
			break;
		case 8:
			EXPECT_EQ(leave, 8);
			EXPECT_EQ(strlen(trimbuf), 8);
			break;
		case 9:
		case 10:
		case 11:
			EXPECT_EQ(leave, 9);
			EXPECT_EQ(strlen(trimbuf), 9);
			break;
		case 12:
		case 13:
		case 14:
			EXPECT_EQ(leave, 12);
			EXPECT_EQ(strlen(trimbuf), 12);
			break;
		case 15:
		case 16:
			EXPECT_EQ(leave, 15);
			EXPECT_EQ(strlen(trimbuf), 15);
			break;
		default:
			EXPECT_FALSE(1);
			break;
		}
	}

	xfree(utf8);
}

/* 文件编码相关 */
TEST_F(Charset, File)
{
	const char *gbk = "這是GBK編碼";
	const char *file = "temp" SEP "charset.tmp";
	char charset[MAX_CHARSET];
	char* utf8 = NULL;
	double prob;
	size_t ulen = 0;
	FILE *fp;

	/* 写入GBK文件 */
	fp = fopen(file, "w");
	ASSERT_NE((FILE*)NULL, fp);
	fwrite(gbk, strlen(gbk), 1, fp);
	fclose(fp);

	/* 读取BOM(失败) */
	fp = fopen(file, "rb");
	ASSERT_NE((FILE*)NULL, fp);
	EXPECT_FALSE(read_file_bom(fp, charset, MAX_CHARSET));
	EXPECT_TRUE(ftell(fp) == 0);	/* 文件流应被复原 */
	fclose(fp);

	EXPECT_TRUE(get_file_charset(file, charset, MAX_CHARSET, &prob, 0));
	EXPECT_STREQ(charset, "GBK");
	EXPECT_EQ(prob, 1);

	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), &utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	/* 写入UTF-8文件 */
	fp = fopen(file, "w");
	ASSERT_NE((FILE*)NULL, fp);
	EXPECT_TRUE(write_file_bom(fp, "UTF-8"));
	fwrite(utf8, strlen(utf8), 1, fp);
	fclose(fp);

	/* 读取BOM(成功) */
	fp = fopen(file, "rb");
	ASSERT_NE((FILE*)NULL, fp);
	EXPECT_TRUE(read_file_bom(fp, charset, MAX_CHARSET));
	EXPECT_TRUE(ftell(fp) == 3);	/* UTF-8 BOM 占3个字节 */
	fclose(fp);

	/* 探测文件字符集 */
	EXPECT_TRUE(get_file_charset(file, charset, MAX_CHARSET, &prob, 0));
	EXPECT_STREQ(charset, "UTF-8");
	EXPECT_EQ(prob, 1);

	xfree(utf8);
}

/* UNICODE字符转换 */
TEST_F(Charset, Unicode)
{
	const char *gbk = "這是GBK編碼";
	UTF8* utf8 = NULL;
	UTF16* utf16 = NULL;
	UTF32* utf32 = NULL;
	char *hex;
	size_t ulen = 0;

#define UTF8_HEX "E9 80 99 E6 98 AF 47 42 4B E7 B7 A8 E7 A2 BC "
#define UTF16_HEX "19 90 2F 66 47 00 42 00 4B 00 E8 7D BC 78 "
#define UTF32_HEX "19 90 00 00 2F 66 00 00 47 00 00 00 42 00 00 00 4B 00 00 00 E8 7D 00 00 BC 78 00 00 "

#define CHECK_UTF8() \
	hex = hexdump(utf8, strlen((const char*)utf8)); \
	EXPECT_STREQ(hex, UTF8_HEX); \
	xfree(hex);

#define CHECK_UTF16() \
	hex = hexdump(utf16, utf16_len(utf16) * sizeof(UTF16));\
	EXPECT_STREQ(hex, UTF16_HEX);\
	xfree(hex);

#define CHECK_UTF32() \
	hex = hexdump(utf32, utf32_len(utf32) * sizeof(UTF32));\
	EXPECT_STREQ(hex, UTF32_HEX);\
	xfree(hex);

	//UTF-8
	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), (char**)&utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	CHECK_UTF8();

	//UTF8 -> UTF-16LE
	utf16 = utf8_to_utf16(utf8, 0);
	CHECK_UTF16();

	//UTF8 -> UTF-32LE
	utf32 = utf8_to_utf32(utf8, 0);
	CHECK_UTF32();

	xfree(utf8);
	xfree(utf16);

	//UTF-32 -> UTF16
	utf16 = utf32_to_utf16(utf32, 0);
	CHECK_UTF16();

	//UTF-32 -> UTF8
	utf8 = utf32_to_utf8(utf32, 0);
	CHECK_UTF8();

	xfree(utf8);
	xfree(utf32);

	//UTF16 -> UTF32
	utf32 = utf16_to_utf32(utf16, 0);
	CHECK_UTF32();

	//UTF16 -> UTF8
	utf8 = utf16_to_utf8(utf16, 0);
	CHECK_UTF8();

	xfree(utf8);
	xfree(utf16);
	xfree(utf32);
}

/* 多字节宽字符转换 */
TEST_F(Charset, MBCS)
{
	const char *gbk = "编码测试";
	const char *file = "temp" SEP "wcs.tmp";
	char *mbcs = NULL;
	wchar_t *wcs = NULL;
	FILE* fp;
	struct file_mem *fm;
	size_t llen;

	/* MBCS */
	ASSERT_TRUE(convert_to_charset("GBK", get_locale(), gbk, strlen(gbk), &mbcs, &llen, 1));
	printf("MBCS: %s\n", mbcs);

	/* MBCS -> WCS */
	/* 因为标准输出流和标准错误流已经被定向字节流，所以不能用于输出测试
	   创建一个新的文件流用于测试，写入文件后的字符集为系统多字节 */
	wcs = mbcs_to_wcs(mbcs);
	fp = fopen(file,"wb");
    ASSERT_NE((FILE*)NULL, fp);
	fwprintf(fp, L"%ls", wcs);
	fclose(fp);
	fm = read_file_mem(file, 0);
#ifdef OS_POSIX
	/* linux平台下wprintf写入的是多字节字符串 */
	EXPECT_STREQ(fm->content, mbcs);
#else
	{
	/* windows平台下直接写入UNICODE字符串 */
	char *hex = hexdump(fm->content, fm->length);
    EXPECT_STREQ(hex, "16 7F 01 78 4B 6D D5 8B ");
	xfree(hex);
	}
#endif
	free_file_mem(fm);
	xfree(mbcs);

	/* WCS -> MBCS */
	mbcs = wcs_to_mbcs(wcs);
	printf("WCS -> MBCS: %s\n", mbcs);
	xfree(wcs);

	xfree(mbcs);
}

/* 宽字符串UTF-8转换 */
TEST_F(Charset, WCS_UTF8)
{
	const char *gbk = "這是GBK編碼";
	char* utf8 = NULL;
	wchar_t* wcs = NULL;
	char *hex;
	size_t ulen;

	//UTF-8
	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), &utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	CHECK_UTF8();

	//UTF-8 -> WIDE
	wcs = utf8_to_wcs(utf8, 0);
	hex = hexdump(wcs, wcslen(wcs) * sizeof(wchar_t));
#ifdef _WIN32
	EXPECT_STREQ(hex, UTF16_HEX);
#else
	EXPECT_STREQ(hex, UTF32_HEX);
#endif
	xfree(hex);

	xfree(utf8);
	xfree(wcs);
}

/* 多字节UTF-8转换 */
TEST_F(Charset, MBCS_UTF8)
{
	const char *gbk = "這是GBK編碼";
	char* utf8 = NULL, *mbcs = NULL;
	char *hex;
	size_t ulen;

	//UTF-8
	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), &utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	//UTF-8 -> MBCS
	mbcs = utf8_to_mbcs(utf8, 1);
	printf ("MBCS: %s\n", mbcs);
	xfree(utf8);

	//MBCS -> UTF-8
	utf8 = mbcs_to_utf8(mbcs, 1);
	CHECK_UTF8();

	xfree(utf8);
	xfree(mbcs);
}

/* 字符编码转换 */
TEST_F(Charset, Convert)
{
	const char *gbk = "這是GBK編碼";
	char* utf8 = NULL;
	char* big5 = NULL;
	char *hex;
	size_t ulen;

	//UTF-8
	if (!convert_to_charset("GBK", "UTF-8", gbk, strlen(gbk), &utf8, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	CHECK_UTF8();

	//big5
	if (!convert_to_charset("UTF-8", "BIG5", utf8, strlen(utf8), &big5, &ulen, 1))
	{
		EXPECT_TRUE(0);
		return;
	}

	hex = hexdump(big5, strlen(big5));
	EXPECT_STREQ(hex, "B3 6F AC 4F 47 42 4B BD 73 BD 58 ");
	xfree(hex);

	xfree(utf8);
	xfree(big5);
}
