#include "../cutil.h"

#include <gtest/gtest.h>

#ifndef OS_MACOSX

TEST(FileSystem, Usage)
{
    struct fs_usage usage;
#ifdef OS_WIN
    ASSERT_TRUE(get_fs_usage("C:\\Windows", &usage));
#else
    ASSERT_TRUE(get_fs_usage("/home", &usage));
    EXPECT_GT(usage.fsu_files, 0);
    EXPECT_GT(usage.fsu_ffree, 0);
#endif
    EXPECT_GT(usage.fsu_total, 0);
    EXPECT_GT(usage.fsu_free, 0);
    EXPECT_GT(usage.fsu_avail, 0);
}

#endif

TEST(FileSystem, SpecialPath)
{
    char buf[MAX_PATH];
    xstrlcpy(buf, get_execute_dir(), sizeof(buf));
    xstrlcat(buf, get_execute_name(), sizeof(buf));
    EXPECT_STREQ(get_execute_path(), buf);
#ifdef OS_WIN
    EXPECT_STREQ(get_module_path(), buf);
#endif
}

