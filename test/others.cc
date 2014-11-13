#include "others.h"
#include "../cutil.h"

/* ≤‚ ‘∞Ê±æ∫≈Ω‚Œˆ */
void other_ver_parse()
{
	int major, minor, revision, build;
	const size_t slen = 128;
	char suffix[128];

	version_parse("1.0", &major, &minor, &revision, &build, suffix, slen);
	CU_ASSERT_EQUAL(major, 1);
	CU_ASSERT_EQUAL(minor, 0);
	CU_ASSERT_EQUAL(revision, 0);
	CU_ASSERT_EQUAL(build, 0);
	CU_ASSERT_STRING_EQUAL(suffix, "");

	version_parse("3.2.5.8848 alpha1", &major, &minor, &revision, &build, suffix, slen);
	CU_ASSERT_EQUAL(major, 3);
	CU_ASSERT_EQUAL(minor, 2);
	CU_ASSERT_EQUAL(revision, 5);
	CU_ASSERT_EQUAL(build, 8848);
	CU_ASSERT_STRING_EQUAL(suffix, "alpha1");

	version_parse("26.0.2343.1", &major, &minor, NULL, NULL, NULL, 0);
	CU_ASSERT_EQUAL(major, 26);
	CU_ASSERT_EQUAL(minor, 0);
}

/* ≤‚ ‘∞Ê±æ∫≈±»Ωœ */
void other_ver_cmp()
{
	CU_ASSERT_EQUAL(version_compare("1.0", "1.1"), -1);
	CU_ASSERT_EQUAL(version_compare("1.0", "1.0.2"), -1);
	CU_ASSERT_EQUAL(version_compare("1.0", "1.0 alpha1"), 0);
	CU_ASSERT_EQUAL(version_compare("1.8.7", "1.8.5.233"), 1);
	CU_ASSERT_EQUAL(version_compare("1.8.7.1234", "1.8.7.3333"), -1);
}