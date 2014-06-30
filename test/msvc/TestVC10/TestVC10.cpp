#include "../../test-c.h"
#include "../../test-cpp.h"

#include "stdafx.h"

#ifdef _DEBUG
#pragma comment(lib, "../../../third_party/iconv/iconvd.lib")
#pragma comment(lib, "../../../third_party/CUnit/CUnitLibD.lib")
#pragma comment(lib, "../../../third_party/zlib/zlibd.lib")
#else
#pragma comment(lib, "../../../third_party/iconv/iconv.lib")
#pragma comment(lib, "../../../third_party/CUnit/CUnitLib.lib")
#pragma comment(lib, "../../../third_party/zlib/zlib.lib")
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	do_test_c(argc, argv);
	do_test_cpp(argc, argv);

	getchar();
	return 0;
}
