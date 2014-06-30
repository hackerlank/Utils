#include "test-c.h"
#include "../cutil.h"
#include <locale.h>

#include "datatype.h"
#include "math.h"
#include "string.h"
#include "filesystem.h"
#include "charset.h"
#include "sqlite.h"
#include "zlibtest.h"
#include "others.h"

#pragma warning(disable: 4113)

static int add_test_suite_case()
{
	/* 数据类型 */
	CU_TestInfo datatype_cases[] = {
        {"Interger Limit", datetype_limits},
        {"Custom Unsigned", datetype_unsigned},
		{"C99 Format Style", datetype_format},
		{"Type Length", datetype_length},
        CU_TEST_INFO_NULL
	};
	
	/* 数学相关 */
	CU_TestInfo math_cases[] = {
		{"Basic", math_basic},
		{"Hex", math_hex},
		CU_TEST_INFO_NULL
	};

	/* 字符串处理 */
	CU_TestInfo string_cases[] = {
		{"Char type", string_ctype},
		{"Case conv", string_case},
		{"Duplicate", string_duplicate},
		{"Split", string_split},
		{"Format", string_format},
		{"Compare", string_compare},
		{"Search", string_search},
		{"Hash", string_hash},
		CU_TEST_INFO_NULL
	};

	/* 文件系统 */
	CU_TestInfo filesystem_cases[] = {
		{"Path Find", fs_path_find},
		{"Path Detect", fs_path_is},
		{"Path Relative", fs_path_relative},
		{"Path Unique", fs_path_unique},
		{"Path Illegal", fs_path_illegal},
		{"Directory", fs_directory},
		{"File", fs_file},
		CU_TEST_INFO_NULL
	};

	/* 字符编码 */
	CU_TestInfo charset_cases[] = {
		{"Detect", charset_detect},
		{"Detect File", charset_file},
		{"UTF-8 Operation", charset_utf8},
		{"UTF-8  UTF-16 | UTF-32", charset_unicode},
		{"MBCS | Wide", charset_wcs_mbcs},
		{"UTF-8 | Wide", charset_wcs_utf8},
		{"MBCS | UTF-8", charset_mbcs_utf8},
		{"Locale Charset", charset_locale},
		{"Charset Conversion", charset_convert},
		CU_TEST_INFO_NULL
	};

	/* SQLite */
	CU_TestInfo sqlite_cases[] = {
		{"Insert", sqlite_insert},
		{"Select", sqlite_select},
		CU_TEST_INFO_NULL
	};

	/* zlib */
	CU_TestInfo zlib_cases[] = {
		{"Deflate Inflate", zlib_deflate_inflate},
		{"Deflate2 Inflate2", zlib_deflate2_inflate2},
		{"Compress Uncompress", zlib_compress_uncompress},
		{"Gzip Ungzip", zlib_gzip_ungzip},
		{"Zip Unzip", zlib_zip_unzip},
		{"Zlib-cutil", zlib_cutil},
		CU_TEST_INFO_NULL
	};

	/* 其他 */
	CU_TestInfo other_cases[] = {
		{"Parse", other_ver_parse},
		{"Compare", other_ver_cmp},
		CU_TEST_INFO_NULL
	};

	CU_SuiteInfo suites[] = {
		{"Data type", NULL, NULL, datatype_cases},
		{"Math", NULL, NULL, math_cases},
		{"String", NULL, NULL, string_cases},
		{"File system", NULL, NULL, filesystem_cases},
		{"Charset", NULL, NULL, charset_cases},
		{"SQLite", NULL, NULL, sqlite_cases},
		{"Zlib", NULL, NULL, zlib_cases},
		{"Others", NULL, NULL, other_cases},
		CU_SUITE_INFO_NULL
	};

	if(CU_register_suites(suites) != CUE_SUCCESS)
		return 0;

	return 1;
}

int do_test_c(int arc, char** argv)
{
	if(CU_initialize_registry() != CUE_SUCCESS)
		return 1;

	assert(CU_get_registry());
	assert(!CU_is_test_running()); 

	if (!add_test_suite_case())
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	cutil_init();

	/* 建立临时文件夹 */
	if (path_is_directory("temp"))
		delete_directories("temp", NULL, NULL);
	else if (path_is_file("temp"))
		delete_file("temp");

	create_directory("temp");

	/* 执行测试 */
	CU_set_output_filename("TestC");
	CU_automated_run_tests();

	delete_directories("temp", NULL, NULL);

	CU_cleanup_registry();
	
	cutil_exit();

	return 0;
}
