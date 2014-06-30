#include "string.h"
#include "../cutil.h"

/* ×Ö·ûÀàÐÍÅÐ¶Ï */
void string_ctype()
{
	CU_ASSERT_TRUE(xisascii('a'));
	CU_ASSERT_TRUE(xisascii('6'));
	CU_ASSERT_TRUE(xisascii('*'));
	CU_ASSERT_TRUE(xisascii('\n'));
	CU_ASSERT_TRUE(xisascii(' '));
	CU_ASSERT_FALSE(xisascii(-1));

	CU_ASSERT_TRUE(xiscntrl(0));
	CU_ASSERT_TRUE(xiscntrl('\t'));
	CU_ASSERT_TRUE(xiscntrl(12));
}

/* ´óÐ¡Ð´×ª»» */
void string_case()
{
	char p[] = "I am very glade to see U!";

	lowercase_str(p);
	CU_ASSERT_STRING_EQUAL(p, "i am very glade to see u!");
	uppercase_str(p);
	CU_ASSERT_STRING_EQUAL(p, "I AM VERY GLADE TO SEE U!");
}

/* ×Ö·û´®¸´ÖÆ */
void string_duplicate()
{
	char *s = "no matching symbolic information FOUND.";

	/* strndup */
	char *p = xstrndup(s, 10);
	CU_ASSERT_STRING_EQUAL(p, "no matchin");
	xfree(p);

	p = xstrndup(s, 0);
	CU_ASSERT_STRING_EQUAL(p, "");
	xfree(p);

	p = xstrndup(s, 1024);
	CU_ASSERT_STRING_EQUAL(p, "no matching symbolic information FOUND.");
	xfree(p);

	/* strdup_lower */
	p = strdup_lower(s);
	CU_ASSERT_STRING_EQUAL(p, "no matching symbolic information found.");
	xfree(p);

	/* strdup_uppder */
	p = strdup_upper(s);
	CU_ASSERT_STRING_EQUAL(p, "NO MATCHING SYMBOLIC INFORMATION FOUND.");
	xfree(p);

	/* substrdup */
	p = substrdup(s, s + 20);
	CU_ASSERT_STRING_EQUAL(p, "no matching symbolic");
	xfree(p);

	p = substrdup(s, s);
	CU_ASSERT_STRING_EQUAL(p, "");
	xfree(p);

	p = substrdup(s, s + 1024);
	CU_ASSERT_STRING_EQUAL(p , "no matching symbolic information FOUND.");
	xfree(p);
}

/* ×Ö·û´®·Ö¸î */
void string_split()
{
	char *s = "1, 22,  ,, 333,44 44,";
	char *sa = strdupa(s);
	char *p = sa, *q;
	int i;

	i = 0;
	while (q = strsep(&p, ","))
	{
		switch(++i){
		case 1:
			CU_ASSERT_STRING_EQUAL(q, "1");
			break;
		case 2:
			CU_ASSERT_STRING_EQUAL(q, " 22");
			break;
		case 3:
			CU_ASSERT_STRING_EQUAL(q, "  ");
			break;
		case 4:
			CU_ASSERT_STRING_EQUAL(q, "");
			break;
		case 5:
			CU_ASSERT_STRING_EQUAL(q, " 333");
			break;
		case 6:
			CU_ASSERT_STRING_EQUAL(q, "44 44");
			break;
		case 7:
			CU_ASSERT_STRING_EQUAL(q, "");
			break;
		default:
			CU_ASSERT_TRUE(0);
			break;
		}
	}
}

/* ×Ö·û´®¸ñÊ½»¯ */
void string_format()
{
	/* asprintf */
	char *p = asprintf("%s %d - %.2f + %x", "Hello world", 1989, 2134.2311, 255);
	CU_ASSERT_STRING_EQUAL(p, "Hello world 1989 - 2134.23 + ff");
	xfree(p);
}

/* ×Ö·û´®±È½Ï */
void string_compare()
{
	/* strcasecmp */
	CU_ASSERT_TRUE(!strcasecmp("a2bc4d*/", "a2bc4d*/"));
	CU_ASSERT_TRUE(!strcasecmp("a2bc4d*/", "A2bC4d*/"));
	CU_ASSERT_TRUE(!strcasecmp("", ""));
	
	/* strncasecmp */
	CU_ASSERT_TRUE(!strncasecmp("a2bc4d", "a2bc4d*/", 6));
	CU_ASSERT_TRUE(!strncasecmp("a2bc4d", "A2bC4d*/", 6));
	CU_ASSERT_FALSE(!strncasecmp("a2bc4d", "A2bC4d*/", 7));
	CU_ASSERT_FALSE(!strncasecmp("", "12", 2));
}

/* ×Ö·û´®²éÕÒ */
void string_search()
{
	char *s = "modify=20120620105939;perm=flcdmpe;type=dir;unique=FD00U6A88C26;UNIX.group=680;UNIX.mode=0711;UNIX.owner=679; domains";

	/* strcasestr */
	CU_ASSERT_PTR_EQUAL(strcasestr(s, "TYPE"), s + 35);
	CU_ASSERT_PTR_EQUAL(strcasestr(s, ""), s);
	CU_ASSERT_PTR_EQUAL(strcasestr(s, "HELLO"), NULL)

	/* strncasestr */
	CU_ASSERT_PTR_NOT_EQUAL(strncasestr(s, "TYPE", 35), s + 35);
	CU_ASSERT_PTR_NOT_EQUAL(strncasestr(s, "TYPE", 38), s + 35);
	CU_ASSERT_PTR_EQUAL(strncasestr(s, "TYPE", 39), s + 35);
	CU_ASSERT_PTR_EQUAL(strncasestr(s, "HELLO", 100), NULL);
}

/* ×Ö·û´®¹þÏ£Öµ */
void string_hash()
{
	
}
