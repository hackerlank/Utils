#include "../cutil.h"

#include <gtest/gtest.h>

TEST(FileSystem, Usage)
{
    struct fs_usage usage;
#ifdef OS_WIN
    get_fs_usage("C:\\Windows", &usage);
#else
    get_fs_usage("/home", &usage);
    EXPECT_GT(usage.fsu_files, 0);
    EXPECT_GT(usage.fsu_ffree, 0);
#endif
    EXPECT_GT(usage.fsu_total, 0);
    EXPECT_GT(usage.fsu_free, 0);
    EXPECT_GT(usage.fsu_avail, 0);
}

TEST(FileSystem, SpecialPath)
{
    char buf[MAX_PATH];
    strlcpy(buf, get_execute_dir(), sizeof(buf));
    strlcat(buf, get_execute_name(), sizeof(buf));
    EXPECT_STREQ(get_execute_path(), buf);
    EXPECT_STREQ(get_module_path(), buf);
}
