#include "stdafx.h"
#include "../../gtest/include/gtest/gtest.h"
#include "../../../cutil.h"

int _tmain(int argc, _TCHAR* argv[])
{
    cutil_init();

    testing::InitGoogleTest(&argc, argv);

    int rv = RUN_ALL_TESTS();

    cutil_exit();

    getchar();
    return rv;
}

