#include "gtest/include/gtest/gtest.h"
#include "../cutil.h"

int main(int argc, char* argv[])
{
    set_disable_debug_log();
    cutil_init();

    IGNORE_RESULT(delete_directories("temp", NULL, NULL));
	IGNORE_RESULT(create_directory("temp"));

    testing::InitGoogleTest(&argc, argv);

    int rv = RUN_ALL_TESTS();

	IGNORE_RESULT(delete_directories("temp", NULL, NULL));
    cutil_exit();

    getchar();
    return rv;
}
