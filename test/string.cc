#include "../cutil.h"

#include <gtest/gtest.h>

/* ×Ö·ûÀàÐÍÅÐ¶Ï */
TEST(String, Type)
{
	EXPECT_TRUE(xisascii('a'));
	EXPECT_TRUE(xisascii('6'));
	EXPECT_TRUE(xisascii('*'));
	EXPECT_TRUE(xisascii('\n'));
	EXPECT_TRUE(xisascii(' '));
	EXPECT_FALSE(xisascii(-1));
}

/* ´óÐ¡Ð´×ª»» */
TEST(String, Case)
{
	char p[] = "I am very glade to see U!";

	lowercase_str(p);
	EXPECT_STREQ(p, "i am very glade to see u!");
	uppercase_str(p);
	EXPECT_STREQ(p, "I AM VERY GLADE TO SEE U!");
}

/* ×Ö·û´®¸´ÖÆ */
TEST(String, Duplicate)
{
	const char *s = "no matching symbolic information FOUND.";

	/* strndup */
	char *p = xstrndup(s, 10);
	EXPECT_STREQ(p, "no matchin");
	xfree(p);

	p = xstrndup(s, 0);
	EXPECT_STREQ(p, "");
	xfree(p);

	p = xstrndup(s, 1024);
	EXPECT_STREQ(p, "no matching symbolic information FOUND.");
	xfree(p);

	/* strdup_lower */
	p = strdup_lower(s);
	EXPECT_STREQ(p, "no matching symbolic information found.");
	xfree(p);

	/* strdup_uppder */
	p = strdup_upper(s);
	EXPECT_STREQ(p, "NO MATCHING SYMBOLIC INFORMATION FOUND.");
	xfree(p);

	/* substrdup */
	p = substrdup(s, s + 20);
	EXPECT_STREQ(p, "no matching symbolic");
	xfree(p);

	p = substrdup(s, s);
	EXPECT_STREQ(p, "");
	xfree(p);

	p = substrdup(s, s + 1024);
	EXPECT_STREQ(p , "no matching symbolic information FOUND.");
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

/* ×Ö·û´®¸ñÊ½»¯ */
TEST(String, Format)
{
	/* asprintf */
    char *p;
    ASSERT_TRUE(xasprintf(&p, "%s %d - %.2f + %x", "Hello world", 1989, 2134.2311, 255));
	EXPECT_STREQ(p, "Hello world 1989 - 2134.23 + ff");
	xfree(p);
}

/* ×Ö·û´®±È½Ï */
TEST(String, Compare)
{
	/* strcasecmp */
	EXPECT_TRUE(!strcasecmp("a2bc4d*/", "a2bc4d*/"));
	EXPECT_TRUE(!strcasecmp("a2bc4d*/", "A2bC4d*/"));
	EXPECT_TRUE(!strcasecmp("", ""));

	/* strncasecmp */
	EXPECT_TRUE(!strncasecmp("a2bc4d", "a2bc4d*/", 6));
	EXPECT_TRUE(!strncasecmp("a2bc4d", "A2bC4d*/", 6));
	EXPECT_FALSE(!strncasecmp("a2bc4d", "A2bC4d*/", 7));
	EXPECT_FALSE(!strncasecmp("", "12", 2));
}

/* ×Ö·û´®²éÕÒ */
TEST(String, Search)
{
	const char *s = "modify=20120620105939;perm=flcdmpe;type=dir;unique=FD00U6A88C26;UNIX.group=680;UNIX.mode=0711;UNIX.owner=679; domains";

	/* strcasestr */
	EXPECT_EQ(strcasestr(s, "TYPE"), s + 35);
	EXPECT_EQ(strcasestr(s, ""), s);
    EXPECT_EQ(strcasestr(s, "HELLO"), (char*)NULL);

	/* strncasestr */
	EXPECT_NE(strncasestr(s, "TYPE", 35), s + 35);
	EXPECT_NE(strncasestr(s, "TYPE", 38), s + 35);
	EXPECT_EQ(strncasestr(s, "TYPE", 39), s + 35);
    EXPECT_EQ(strncasestr(s, "HELLO", 100), (char*)NULL);
}

/* ×Ö·û´®¹þÏ£Öµ */
TEST(String, Hash)
{

}
