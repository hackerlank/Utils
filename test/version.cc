#include "../cutil.h"

#include <gtest/gtest.h>

/* 测试版本号解析 */
TEST(Vesion, Parse)
{
    struct version_info vi;

    EXPECT_FALSE(version_parse("", &vi));
    EXPECT_FALSE(version_parse(NULL, &vi));
    EXPECT_FALSE(version_parse("1.0", NULL));

#define EXPECT_VERINFO(a,b,c,d,e) \
    EXPECT_EQ(a, vi.major);       \
    EXPECT_EQ(b, vi.minor);       \
    EXPECT_EQ(c, vi.revision);    \
    EXPECT_EQ(d, vi.build);       \
    EXPECT_STREQ(e, vi.suffix);

    ASSERT_TRUE(version_parse("1", &vi));
    EXPECT_VERINFO(1, 0, 0, 0, "");

    ASSERT_TRUE(version_parse("1.2", &vi));
    EXPECT_VERINFO(1, 2, 0, 0, "");

    ASSERT_TRUE(version_parse("1.2.120 RC", &vi));
    EXPECT_VERINFO(1, 2, 120, 0, "RC");

    ASSERT_TRUE(version_parse("3.2.5.8848   alpha1", &vi));
    EXPECT_VERINFO(3, 2, 5, 8848, "alpha1");

    ASSERT_TRUE(version_parse("26.0.2343.1", &vi));
    EXPECT_VERINFO(26, 0, 2343, 1, "");

    ASSERT_TRUE(version_parse("3.5.0.3305  (新年版)", &vi));
    EXPECT_VERINFO(3, 5, 0, 3305, "(新年版)");

#undef EXPECT_VERSION
}

/* 测试版本号比较 */
TEST(Version, Compare)
{
    EXPECT_EQ(-2, version_compare(NULL, ""));

    EXPECT_EQ(-1, version_compare("0", "0.1"));
    EXPECT_EQ(-1, version_compare("1", "1.1"));
    EXPECT_EQ(-1, version_compare("1.0", "1.0.2"));
    EXPECT_EQ(0, version_compare("1.0", "1.0 alpha1"));
    EXPECT_EQ(1, version_compare("1.8.7", "1.8.5.233"));
    EXPECT_EQ(0, version_compare("3.5.0.3333", "3.5.0.3333"));
    EXPECT_EQ(-1, version_compare("3.4.2.3137", "3.5.0.3333  (元旦版)"));
}
