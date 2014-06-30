#include "zlibtest.h"
#include "../cutil.h"

/* simply taken from Google+ */
static const char *original = "I only knew there had been drama related to it through the Arch community.\
							  Many people are resistant to change, especially those who can recite or create \
							  init scripts in their sleep. It's an understandable source of frustration. \
							  The benefits outweigh the need to retain the comfort level, imho, but that's subjective. ";

void zlib_deflate_inflate()
{

}

void zlib_deflate2_inflate2()
{

}

/* 测试解压缩 */
#define TEST_UNCOMPRESS() do{ \
	ulong uncompbufsize = orgsize; \
	ulong uncompsize = uncompbufsize;\
	char *unbuffer = (char*)xmalloc((size_t)uncompbufsize);\
	CU_ASSERT_EQUAL(uncompress(unbuffer, &uncompsize, buffer, compsize), Z_OK);\
	CU_ASSERT_EQUAL(uncompbufsize, orgsize);\
	xfree(unbuffer);\
}while(0);

void zlib_compress_uncompress()
{
	/* 测试压缩 */
	ulong orgsize = strlen(original);
	ulong compbufsize = compressBound(orgsize);
	ulong compsize = compbufsize;
	char *buffer = (char*)xmalloc((size_t)compbufsize);
	
	/* 默认压缩方式，压缩至221字节 */
	CU_ASSERT_EQUAL(compress(buffer, &compsize, original, orgsize), Z_OK);
	CU_ASSERT_TRUE(compsize < orgsize);
	TEST_UNCOMPRESS();

	/* 无压缩，附加了头部信息 */
	compsize = compbufsize;
	CU_ASSERT_EQUAL(compress2(buffer, &compsize, original, orgsize, Z_NO_COMPRESSION), Z_OK);
	CU_ASSERT_TRUE(compsize > orgsize);
	TEST_UNCOMPRESS();

	xfree(buffer);
}

#define GZFILE "temp"PATH_SEP_STR"test.gz"

void zlib_gzip_ungzip()
{
	gzFile gzfp;
	size_t orglen = strlen(original);

	gzfp = gzopen(GZFILE, "wb");
	if (!gzfp)
	{
		CU_ASSERT_FALSE(1);
		return;
	}

	/* 设置压缩可用的缓冲区大小，越大压缩越大，默认8192字节 */
	gzbuffer(gzfp, 128 * 1024);

	/* 设置压缩率和源数据属性，同deflate2 */
	gzsetparams(gzfp, Z_BEST_COMPRESSION, Z_DEFAULT_STRATEGY);

	/* 写入压缩数据 */
	CU_ASSERT_EQUAL(gzwrite(gzfp, original, orglen), orglen);
	gzclose(gzfp);
	CU_ASSERT_TRUE(file_size(GZFILE) > 0);
	CU_ASSERT_TRUE(file_size(GZFILE) < orglen);

	/* 测试读取gz压缩文件 */
	gzfp = gzopen(GZFILE, "rb");
	if (!gzfp)
	{
		CU_ASSERT_FALSE(1);
		return;
	}

	/* 读出压缩数据 */
	{
		char *uncompbuf = (char*)xmalloc(orglen+1);
		CU_ASSERT_EQUAL(gzread(gzfp, uncompbuf, orglen), orglen);
		uncompbuf[orglen] = '\0';
		xfree(uncompbuf);
	}

	gzclose(gzfp);
	delete_file(GZFILE);
}

void zlib_zip_unzip()
{
	/* zlib 不直接支持zip格式 */
}

void zlib_cutil()
{
	size_t orglen = strlen(original);
	char *compbuf = NULL, *uncompbuf = NULL;
	ulong complen = 0, uncomplen = 0;
	char compbuffer[512], uncompbuffer[512];

	/* 不传入缓冲区 */
	if (!zlib_compress(original, orglen, &compbuf, &complen, Z_DEFAULT_COMPRESSION))
	{
		CU_ASSERT_FALSE(1);
		return;
	}

	uncomplen = orglen;
	if (!zlib_uncompress(compbuf, complen, &uncompbuf, &uncomplen))
	{
		CU_ASSERT_FALSE(1);
		xfree(compbuf);
		return;
	}

	CU_ASSERT_EQUAL(uncomplen, orglen);
	CU_ASSERT_TRUE(strncmp(original, uncompbuf, orglen) == 0);

	xfree(compbuf);
	xfree(uncompbuf);

	/* 传入缓冲区 */
	compbuf = compbuffer;
	complen = sizeof(compbuffer);
	if (!zlib_compress(original, orglen, &compbuf, &complen, Z_DEFAULT_COMPRESSION))
	{
		CU_ASSERT_FALSE(1);
		return;
	}

	uncompbuf = uncompbuffer;
	uncomplen = sizeof(uncompbuffer);
	if (!zlib_uncompress(compbuf, complen, &uncompbuf, &uncomplen))
	{
		if (compbuf != compbuffer)
			xfree(compbuf);

		CU_ASSERT_FALSE(1);
		return;
	}

	CU_ASSERT_EQUAL(uncomplen, orglen);
	CU_ASSERT_TRUE(strncmp(original, uncompbuf, orglen) == 0);

	if (compbuf != compbuffer)
		xfree(compbuf);
}