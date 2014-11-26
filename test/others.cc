#include "../cutil.h"

#include <gtest/gtest.h>

/* ≤‚ ‘∞Ê±æ∫≈Ω‚Œˆ */
TEST(Misc, Version)
{
	int major, minor, revision, build;
	const size_t slen = 128;
	char suffix[128];

	version_parse("1.0", &major, &minor, &revision, &build, suffix, slen);
	EXPECT_EQ(major, 1);
	EXPECT_EQ(minor, 0);
	EXPECT_EQ(revision, 0);
	EXPECT_EQ(build, 0);
	EXPECT_STREQ(suffix, "");

	version_parse("3.2.5.8848 alpha1", &major, &minor, &revision, &build, suffix, slen);
	EXPECT_EQ(major, 3);
	EXPECT_EQ(minor, 2);
	EXPECT_EQ(revision, 5);
	EXPECT_EQ(build, 8848);
	EXPECT_STREQ(suffix, "alpha1");

	version_parse("26.0.2343.1", &major, &minor, NULL, NULL, NULL, 0);
	EXPECT_EQ(major, 26);
	EXPECT_EQ(minor, 0);
}

/* ≤‚ ‘∞Ê±æ∫≈±»Ωœ */
void other_ver_cmp()
{
	EXPECT_EQ(version_compare("1.0", "1.1"), -1);
	EXPECT_EQ(version_compare("1.0", "1.0.2"), -1);
	EXPECT_EQ(version_compare("1.0", "1.0 alpha1"), 0);
	EXPECT_EQ(version_compare("1.8.7", "1.8.5.233"), 1);
	EXPECT_EQ(version_compare("1.8.7.1234", "1.8.7.3333"), -1);
}