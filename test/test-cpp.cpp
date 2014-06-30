#include "test-cpp.h"
#include "../ccutil.h"

#include "cppstring.h"

static int add_test_suite_case()
{
	/* 数据类型 */
	CU_TestInfo string_cases[] = {
        {"Wstring | string", wstring_string},
        {"Wstring | UTF8 string", wstring_utf8string},
		{"string | UTF8 string", string_utf8string},
		{"string16 | UTF8 string", string16_utf8string},
		{"string16 | Wstring", string16_wstring},
		{"string16 | string", string16_string},
		{"string conversions", string_conversions},
        CU_TEST_INFO_NULL
	};
	
	CU_SuiteInfo suites[] = {
		{"String", NULL, NULL, string_cases},
		CU_SUITE_INFO_NULL
	};

	if(CU_register_suites(suites) != CUE_SUCCESS)
		return 0;

	return 1;
}

int do_test_cpp(int arc, char** argv)
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
	CU_set_output_filename("TestCpp");
	CU_automated_run_tests();

	delete_directories("temp", NULL, NULL);

	CU_cleanup_registry();
	
	cutil_exit();

	return 0;
}
