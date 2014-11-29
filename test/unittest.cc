#include "gtest/include/gtest/gtest.h"
#include "../cutil.h"

int main(int argc, char* argv[])
{
    set_disable_debug_log();
    cutil_init();

	delete_directories("temp", NULL, NULL);
	create_directory("temp");

    testing::InitGoogleTest(&argc, argv);

    int rv = RUN_ALL_TESTS();

	delete_directories("temp", NULL, NULL);
    cutil_exit();

    return rv;
}
