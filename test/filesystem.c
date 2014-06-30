#include "filesystem.h"
#include "../cutil.h"

#define SEP PATH_SEP_STR

/* 路径查找 */
void fs_path_find()
{
	char buf[MAX_PATH];

	/* path_find_file_name */
#ifdef _WIN32
	CU_ASSERT_STRING_EQUAL(path_find_file_name("C:\\myfolder\\abc.txt"), "abc.txt");		/* 普通文件 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("C:\\myfolder\\abc"), "abc");				/* 普通文件(无扩展名) */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("C:\\myfolder\\"), "myfolder\\");			/* 目录(有分隔符) */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("C:\\myfolder\\\\"), "myfolder\\\\");		/* 目录(多分隔符) */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("C:\\myfolder"), "myfolder");				/* 目录 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("C:\\"), "C:\\");							/* 根目录 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("..\\abc.txt"), "abc.txt");					/* 相对路径 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("abc.txt"), "abc.txt");						/* 仅文件名 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("abc"), "abc");								/* 仅文件名(无扩展名) */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("\\"), "\\");								/* 仅分隔符 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("\\\\"), "\\\\");							/* 仅分隔符 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name(""), "");									/* 空字符串 */
	CU_ASSERT_STRING_EQUAL(path_find_file_name("  "), "  ");								/* 空字符串 */
	CU_ASSERT_PTR_EQUAL(path_find_file_name(NULL), NULL);									/* NULL指针 */
#endif

	/* path_find_extension */
#ifdef _WIN32
	CU_ASSERT_STRING_EQUAL(path_find_extension("C:\\a.txt"), ".txt");						/* 普通文件 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("C:\\a."), ".");								/* 仅点号 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("C:\\a"), "");								/* 无扩展名 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("C:\\"), "");								/* 根目录 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("a..txt"), ".txt");							/* 多个点号 */
	CU_ASSERT_STRING_EQUAL(path_find_extension(".txt"), ".txt");							/* 仅扩展名 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("abc"), "");									/* 仅文件名(无扩展名) */
	CU_ASSERT_STRING_EQUAL(path_find_extension("\\"), "");									/* 仅分隔符 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("\\\\"), "");								/* 仅分隔符 */
	CU_ASSERT_STRING_EQUAL(path_find_extension(""), "");									/* 空字符串 */
	CU_ASSERT_STRING_EQUAL(path_find_extension("  "), "");									/* 空字符串 */
	CU_ASSERT_PTR_EQUAL(path_find_extension(NULL), NULL);									/* NULL指针 */
#endif

	/* path_find_directory */
#ifdef _WIN32
	CU_ASSERT_TRUE(path_find_directory("C:\\folder\\a.txt", buf, MAX_PATH));				/* 普通文件 */
	CU_ASSERT_STRING_EQUAL(buf, "C:\\folder\\");
	CU_ASSERT_TRUE(path_find_directory("C:\\folder\\another\\", buf, MAX_PATH));			/* 普通目录 */
	CU_ASSERT_STRING_EQUAL(buf, "C:\\folder\\");
	CU_ASSERT_FALSE(path_find_directory("\\\\", buf, MAX_PATH));							/* 全分隔符 */
	CU_ASSERT_FALSE(path_find_directory("C:\\", buf, MAX_PATH));							/* 根目录 */
	CU_ASSERT_FALSE(path_find_directory(NULL, buf, MAX_PATH));								/* NULL指针 */
#endif
}

/* 路径检测 */
void fs_path_is()
{
	/* path_file_exists */
#ifdef _WIN32
	CU_ASSERT_TRUE(path_file_exists("C:\\"));
	CU_ASSERT_TRUE(path_file_exists("C:\\windows"));
	CU_ASSERT_TRUE(path_file_exists("C:\\Windows\\"));
	CU_ASSERT_TRUE(path_file_exists("C:\\Windows\\explorer.exe"));
	CU_ASSERT_FALSE(path_file_exists("D:\\not_exist_but_coincidence"));
#endif

	/* path_is_file */
#ifdef _WIN32
	CU_ASSERT_TRUE(path_is_file("C:\\Windows\\explorer.exe"));

// 	CU_ASSERT_FALSE(path_is_file("not_exist_but_coincidence"));
// 	CU_ASSERT_TRUE(touch("not_exist_but_coincidence", 1,0,0,0,0));
// 	CU_ASSERT_TRUE(path_is_file("not_exist_but_coincidence"));
// 	CU_ASSERT_TRUE(delete_file("not_exist_but_coincidence"));
// 	CU_ASSERT_FALSE(path_is_file("not_exist_but_coincidence"));
#endif

	/* path_is_directory */
#ifdef _WIN32
	CU_ASSERT_TRUE(path_is_directory("C:\\"));
	CU_ASSERT_TRUE(path_is_directory("C:\\windows"));
	CU_ASSERT_TRUE(path_is_directory("C:\\Windows\\"));
#endif
}

/* 相对路径 */
void fs_path_relative()
{
	char buf[MAX_PATH];

#ifdef _WIN32
	CU_ASSERT_TRUE(relative_path("C:\\a.txt", "C:\\f1\\f2\\b.txt", PATH_SEP_CHAR, buf, MAX_PATH));
	CU_ASSERT_STRING_EQUAL(buf, "f1\\f2\\b.txt");
	CU_ASSERT_TRUE(relative_path("C:\\f1\\f2\\b.txt", "C:\\a.txt", PATH_SEP_CHAR, buf, MAX_PATH));
	CU_ASSERT_STRING_EQUAL(buf, "..\\..\\a.txt");
	CU_ASSERT_TRUE(relative_path("a.txt", "f1\\f2\\b.txt", PATH_SEP_CHAR, buf, MAX_PATH));
	CU_ASSERT_STRING_EQUAL(buf, "f1\\f2\\b.txt");
	CU_ASSERT_TRUE(relative_path("f1\\f2\\b.txt", "a.txt", PATH_SEP_CHAR, buf, MAX_PATH));
	CU_ASSERT_STRING_EQUAL(buf, "..\\..\\a.txt");
#endif
}

/* 唯一路径 */
void fs_path_unique()
{
	char buf[MAX_PATH], buf1[MAX_PATH], buf2[MAX_PATH];
	char *p = "temp"SEP"unique_file_test.txt";
	char *q = "temp"SEP"unique_dir_test";

	/* unique_file */
	unique_file(p, buf, MAX_PATH);
	CU_ASSERT_STRING_EQUAL(buf, "temp"SEP"unique_file_test.txt");
	touch(buf);

	unique_file(buf, buf1, MAX_PATH);
	CU_ASSERT_STRING_EQUAL(buf1, "temp"SEP"unique_file_test (1).txt");
	touch(buf1);

	unique_file(buf, buf2, MAX_PATH);
	CU_ASSERT_STRING_EQUAL(buf2, "temp"SEP"unique_file_test (2).txt");
	touch(buf2);

	delete_file(buf);
	delete_file(buf1);
	delete_file(buf2);

	/* unique_dir */
	unique_dir(q, buf, MAX_PATH);
	CU_ASSERT_STRING_EQUAL(buf, "temp"SEP"unique_dir_test");
	touch(buf);
	
	unique_file(buf, buf1, MAX_PATH);
	CU_ASSERT_STRING_EQUAL(buf1, "temp"SEP"unique_dir_test (1)");
	touch(buf1);
	
	unique_file(buf, buf2, MAX_PATH);
	CU_ASSERT_STRING_EQUAL(buf2, "temp"SEP"unique_dir_test (2)");
	touch(buf2);
	
	delete_directory(buf);
	delete_directory(buf1);
	delete_directory(buf2);
}

/* 合法路径 */
void fs_path_illegal()
{
	char *p = "\\/:*?\"<>|abc!@%^&()+-=[]{};,.123";

	char *q = strdupa(p);
	path_char_blankspace(q);
#ifdef _WIN32
	CU_ASSERT_STRING_EQUAL(q, "         abc!@%^&()+-=[]{};,.123");
#else
	CU_ASSERT_STRING_EQUAL(q, "\\ :*?\"<>|abc!@%^&()+-=[]{};,.123");
#endif	
}

static int copy_dir_hander(const char* src, const char *dst,																				
					int action, int succ, void *arg)
{
	return 1;
}

static int delete_dir_handler(const char* path, int action, int succ, void *arg)
{
	return 1;
}

/* 目录相关 */
void fs_directory()
{
	int i,j;
	char buf[MAX_PATH], multi_dir[MAX_PATH];
	struct trav_dir_context ctx;

	snprintf(multi_dir, MAX_PATH, "%stemp%sfoo%sbar%sabc%s", get_current_dir(), SEP, SEP, SEP, SEP);

	/* 创建目录test/dirtest并验证其存在 */
	CU_ASSERT_TRUE(create_directory("temp"SEP"foo"));
	CU_ASSERT_TRUE(path_is_directory("temp"SEP"foo"));

	/* create_directory无法创建多级目录，而create_directories可以 */
	CU_ASSERT_FALSE(create_directory(multi_dir));
	CU_ASSERT_TRUE(create_directories(multi_dir));
	CU_ASSERT_TRUE(path_is_directory(multi_dir));
	
	/* delete_directory无法删除多级目录，而delete_directories可以 */
	CU_ASSERT_TRUE(delete_directories("temp"SEP"foo"SEP"bar", NULL, NULL));
	CU_ASSERT_FALSE(path_file_exists("temp"SEP"foo"SEP"bar"));

	/* 创建更多的目录和文件 */
	for (i = 0; i < 100; i++)
	{
		snprintf(buf, MAX_PATH, "temp"SEP"foo"SEP"folder%d", i);
		CU_ASSERT_TRUE(create_directory(buf));

		snprintf(buf, MAX_PATH, "temp"SEP"foo"SEP"%d.txt", i);
		CU_ASSERT_TRUE(touch(buf));
	}

	/* 拷贝目录，最多拷贝50个目录和80个文件 */
	CU_ASSERT_TRUE(create_directory("temp"SEP"foo1"));
	CU_ASSERT_TRUE(copy_directories("temp"SEP"foo", "temp"SEP"foo1", copy_dir_hander, NULL));

	/* 遍历目录 */
	CU_ASSERT_TRUE(trav_dir_begin("temp"SEP"foo1"SEP"foo", &ctx));

	i = j = 0;
	do 
	{
		if (trav_entry_is_dot(&ctx) || trav_entry_is_dotdot(&ctx))
			continue;

		CU_ASSERT_TRUE(trav_entry_path(&ctx, buf, MAX_PATH));

		if (trav_entry_is_file(&ctx))
			i++;
		else if (trav_entry_is_dir(&ctx))
			j++;
		else 
			CU_ASSERT_TRUE(0);

	} while (trav_dir_next(&ctx));

	CU_ASSERT_TRUE(trav_dir_end(&ctx));

	/* 验证遍历到的文件/目录数量 */
	CU_ASSERT_EQUAL(i, 100);
	CU_ASSERT_EQUAL(j, 100);

	/* TODO: foreach_file, foreach_dir */

	/* 最后清除目录 */
	CU_ASSERT_TRUE(delete_directories("temp"SEP"foo", delete_dir_handler, NULL));
	CU_ASSERT_TRUE(delete_directories("temp"SEP"foo1", delete_dir_handler, NULL));
	CU_ASSERT_FALSE(path_file_exists("temp"SEP"foo"));
	CU_ASSERT_FALSE(path_file_exists("temp"SEP"foo1"));
}

/* 文件相关 */
void fs_file()
{
	
}