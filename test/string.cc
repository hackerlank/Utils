#include "../cutil.h"

#include <set>
#include <string>
#include <gtest/gtest.h>

TEST(String, Equal) {
    EXPECT_TRUE(STREQ("", ""));
    EXPECT_TRUE(STREQ("abc", "abc"));
    EXPECT_FALSE(STREQ("abc", "ABC"));
    EXPECT_FALSE(STREQ("abc", "123"));

    EXPECT_TRUE(STRIEQ("", ""));
    EXPECT_TRUE(STRIEQ("abc", "abc"));
    EXPECT_TRUE(STRIEQ("abc", "ABC"));
    EXPECT_FALSE(STRIEQ("abc", "123"));
}

TEST(String, Printf) {
    char buf[1024];
    int i;

    EXPECT_EQ(xsnprintf(buf, 0, "%s", "abc"), -1);
    i = xsnprintf(buf, 1, "%s", "abc");
    EXPECT_EQ(3, i);
    EXPECT_STREQ("", buf);
    i = xsnprintf(buf, 2, "%s", "abc");
    EXPECT_EQ(3, i);
    EXPECT_STREQ("a", buf);
    i = xsnprintf(buf, 4, "%s", "abc");
    EXPECT_EQ(3, i);
    EXPECT_STREQ("abc", buf);
    i = xsnprintf(buf, 5, "%s", "abc");
    EXPECT_EQ(3, i);
    EXPECT_STREQ("abc", buf);
    i = xsnprintf(buf, 10, "%s", "abc");
    EXPECT_EQ(3, i);
    EXPECT_STREQ("abc", buf);

    char *output;
    ASSERT_TRUE(xasprintf(&output, "%s", "abc"));
    EXPECT_STREQ("abc", output);
    xfree(output);

    ASSERT_TRUE(xasprintf(&output, "%s%" PRIu64 "_%d\n", "abc", INT64_C(1234567890), 888));
    EXPECT_STREQ("abc1234567890_888\n", output);
    xfree(output);

    ASSERT_TRUE(xasprintf(&output, "%s %d - %.2f + %x", "Hello world", 1989, 2134.2311, 255));
    EXPECT_STREQ(output, "Hello world 1989 - 2134.23 + ff");
    xfree(output);

    const size_t len = 64 * 1024 * 1024;
    char *random = (char*)xmalloc(len);
    ASSERT_NE((char*)NULL, random);
    for (size_t i = 0; i < len-1; i++)
        random[i] = 'a';
    random[len-1] = '\0';
    EXPECT_FALSE(xasprintf(&output, "%s", random));
    xfree(random);
}

TEST(String, lcpycat) {
    char buf[16];

    EXPECT_EQ(5, xstrlcpy(buf, "12345", 0));
    EXPECT_EQ(5, xstrlcpy(buf, "12345", 1));
    EXPECT_STREQ(buf, "");
    EXPECT_EQ(5, xstrlcpy(buf, "12345", 2));
    EXPECT_STREQ(buf, "1");
    EXPECT_EQ(5, xstrlcpy(buf, "12345", 3));
    EXPECT_STREQ(buf, "12");
    EXPECT_EQ(5, xstrlcpy(buf, "12345", 5));
    EXPECT_STREQ(buf, "1234");
    EXPECT_EQ(5, xstrlcpy(buf, "12345", 6));
    EXPECT_STREQ(buf, "12345");
    EXPECT_EQ(5, xstrlcpy(buf, "12345", 16));
    EXPECT_STREQ(buf, "12345");

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 5));
    EXPECT_STREQ("12345", buf);

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 6));
    EXPECT_STREQ("12345", buf);

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 7));
    EXPECT_STREQ("12345a", buf);

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 8));
    EXPECT_STREQ("12345ab", buf);

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 10));
    EXPECT_STREQ("12345abcd", buf);

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 11));
    EXPECT_STREQ("12345abcde", buf);

    EXPECT_EQ(5, xstrlcpy(buf, "12345", sizeof(buf)));
    EXPECT_EQ(10, xstrlcat(buf, "abcde", 16));
    EXPECT_STREQ("12345abcde", buf);
}

/* ´óÐ¡Ð´×ª»» */
TEST(String, Case)
{
	char p[] = "I am very glade to see U!";

    EXPECT_EQ(1, lowercase_str(p));
	EXPECT_STREQ(p, "i am very glade to see u!");
    EXPECT_EQ(0, lowercase_str(p));
    EXPECT_STREQ(p, "i am very glade to see u!");
    EXPECT_EQ(1, uppercase_str(p));
	EXPECT_STREQ(p, "I AM VERY GLADE TO SEE U!");
    EXPECT_EQ(0, uppercase_str(p));
    EXPECT_STREQ(p, "I AM VERY GLADE TO SEE U!");

    const char *s = "no matching symbolic information FOUND.";

    char *q = strdup_lower(s);
    ASSERT_STREQ(q, "no matching symbolic information found.");
    xfree(q);
    q = strdup_upper(s);
    ASSERT_STREQ(q, "NO MATCHING SYMBOLIC INFORMATION FOUND.");
    xfree(q);
}

/* ×Ö·û´®¸´ÖÆ */
TEST(String, Duplicate)
{
	const char *s = "no matching symbolic information FOUND.";
    char* p;

    /* substrdup */
    p = substrdup(s, s);
    ASSERT_STREQ(p, "");
    xfree(p);
    p = substrdup(s, s + 1);
    ASSERT_STREQ(p, "n");
    xfree(p);
    p = substrdup(s, s + 20);
    ASSERT_STREQ(p, "no matching symbolic");
    xfree(p);
    p = substrdup(s, s + strlen(s) + 10);
    ASSERT_STREQ(p, "no matching symbolic information FOUND.");
    xfree(p);

	/* strndup */
    p = xstrndup(s, 0);
    ASSERT_STREQ(p, "");
    xfree(p);
    p = xstrndup(s, 1);
    ASSERT_STREQ(p, "n");
    xfree(p);
	p = xstrndup(s, 10);
    ASSERT_STREQ(p, "no matchin");
	xfree(p);
    p = xstrndup(s, strlen(s));
    ASSERT_STREQ(p, "no matching symbolic information FOUND.");
	xfree(p);
    p = xstrndup(s, strlen(s) + 10);
    ASSERT_STREQ(p, "no matching symbolic information FOUND.");
    xfree(p);
}

/* ×Ö·û´®·Ö¸î */
TEST(String, Split)
{
	const char *s = "1, 22,  ,, 333,44 44,";
	char *sa = xstrdup(s);
	char *p = sa, *q;
	int i;

	i = 0;
	while ((q = strsep(&p, ",")))
	{
		switch(++i){
		case 1:
			EXPECT_STREQ(q, "1");
			break;
		case 2:
			EXPECT_STREQ(q, " 22");
			break;
		case 3:
			EXPECT_STREQ(q, "  ");
			break;
		case 4:
			EXPECT_STREQ(q, "");
			break;
		case 5:
			EXPECT_STREQ(q, " 333");
			break;
		case 6:
			EXPECT_STREQ(q, "44 44");
			break;
		case 7:
			EXPECT_STREQ(q, "");
			break;
		default:
			EXPECT_TRUE(0);
			break;
		}
	}

	xfree(sa);
}

/* ×Ö·û´®±È½Ï */
TEST(String, Compare)
{
	/* strcasecmp */
    EXPECT_TRUE(!strcasecmp("", ""));
	EXPECT_TRUE(!strcasecmp("a2bc4d*/", "a2bc4d*/"));
	EXPECT_TRUE(!strcasecmp("a2bc4d*/", "A2bC4d*/"));
    EXPECT_FALSE(!strcasecmp("123", "abc"));

	/* strncasecmp */
    EXPECT_TRUE(!strncasecmp("", "", 0));
    EXPECT_TRUE(!strncasecmp("", "", 1));
    EXPECT_TRUE(!strncasecmp("a2bc4d!@#", "a2bc4d*/", 6));
    EXPECT_TRUE(!strncasecmp("a2bc4d!@#", "A2bC4d*/", 6));
    EXPECT_FALSE(!strncasecmp("a2bc4d!@#", "A2bC4d*/", 7));
    EXPECT_FALSE(!strncasecmp("a2bc4d!@#", "A2bC4d*/", 100));
	EXPECT_FALSE(!strncasecmp("abc", "123", 2));
}

/* ×Ö·û´®²éÕÒ */
TEST(String, Search)
{
	const char *s = "modify=20120620105939;perm=flcdmpe;type=dir;unique=FD00U6A88C26;UNIX.group=680;UNIX.mode=0711;UNIX.owner=679; domains";

    /* strncasestr */
    EXPECT_EQ((char*)NULL, strncasestr(s, "TYPE", 0));
    EXPECT_EQ((char*)NULL, strncasestr(s, "TYPE", 34));
    EXPECT_EQ((char*)NULL, strncasestr(s, "HELLO", 100));
    EXPECT_EQ((char*)NULL, strncasestr(s, "TYPE", 35));
    EXPECT_EQ((char*)NULL, strncasestr(s, "TYPE", 38));
    EXPECT_EQ(s + 35, strncasestr(s, "TYPE", 39));
    EXPECT_EQ(s + 35, strncasestr(s, "TYPE", 40));
    EXPECT_EQ(s + 35, strncasestr(s, "TYPE", strlen(s) + 100));

    EXPECT_EQ((char*)NULL, strncasestr(s, "type", 0));
    EXPECT_EQ((char*)NULL, strncasestr(s, "type", 34));
    EXPECT_EQ((char*)NULL, strncasestr(s, "hello", 100));
    EXPECT_EQ((char*)NULL, strncasestr(s, "type", 35));
    EXPECT_EQ((char*)NULL, strncasestr(s, "type", 38));
    EXPECT_EQ(s + 35, strncasestr(s, "type", 39));
    EXPECT_EQ(s + 35, strncasestr(s, "type", 40));
    EXPECT_EQ(s + 35, strncasestr(s, "type", strlen(s) + 100));

	/* strcasestr */
    EXPECT_EQ(s + 35, strcasestr(s, "type"));
    EXPECT_EQ(s + 35, strcasestr(s, "TYPE"));
	EXPECT_EQ(strcasestr(s, ""), s);
    EXPECT_EQ(strcasestr(s, "HELLO"), (char*)NULL);
}

TEST(String, Encrypt) {
    char buf[] = "Hello, world!";
    char* orig = xstrdup(buf);

    EXPECT_EQ(buf, memfrob(buf, sizeof(buf)));
    EXPECT_NE(0, memcmp(buf, orig, sizeof(buf)));
    EXPECT_EQ(buf, memfrob(buf, sizeof(buf)));
    EXPECT_EQ(0, memcmp(buf, orig, sizeof(buf)));

    xfree(orig);
}

/* ×Ö·û´®¹þÏ£Öµ */
TEST(String, Hash)
{
    char table[] = "askfjwueriwnjv87s8923nrfsafsd*^%$#^%(*ISBHfbadfasdfd";
    std::set<size_t> hashes;
    std::string h;
    for (int i = 0; i < 1000; i++) {
        h.clear();
        for (int j = 0; j < i; j++) {
            int k = i % xmax(j, 1);
            h.append(1, table[k]);
        }
        
        hashes.insert(hash_pjw(h.c_str(), SIZE_T_MAX));
    }

    EXPECT_EQ(1000, hashes.size());
}
