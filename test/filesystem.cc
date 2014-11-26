#include "../cutil.h"

#include <gtest/gtest.h>

#define SEP PATH_SEP_STR

/* 路径查找 */
TEST(FileSystem, PathFind)
{
	char buf[MAX_PATH];

	/* path_find_file_name */
#ifdef _WIN32
	EXPECT_STREQ(path_find_file_name("C:\\myfolder\\abc.txt"), "abc.txt");		/* 普通文件 */
	EXPECT_STREQ(path_find_file_name("C:\\myfolder\\abc"), "abc");				/* 普通文件(无扩展名) */
	EXPECT_STREQ(path_find_file_name("C:\\myfolder\\"), "myfolder\\");			/* 目录(有分隔符) */
	EXPECT_STREQ(path_find_file_name("C:\\myfolder\\\\"), "myfolder\\\\");		/* 目录(多分隔符) */
	EXPECT_STREQ(path_find_file_name("C:\\myfolder"), "myfolder");				/* 目录 */
	EXPECT_STREQ(path_find_file_name("C:\\"), "C:\\");							/* 根目录 */
	EXPECT_STREQ(path_find_file_name("..\\abc.txt"), "abc.txt");				/* 相对路径 */
	EXPECT_STREQ(path_find_file_name("abc.txt"), "abc.txt");					/* 仅文件名 */
	EXPECT_STREQ(path_find_file_name("abc"), "abc");							/* 仅文件名(无扩展名) */
	EXPECT_STREQ(path_find_file_name("\\"), "\\");								/* 仅分隔符 */
	EXPECT_STREQ(path_find_file_name("\\\\"), "\\\\");							/* 仅分隔符 */
	EXPECT_STREQ(path_find_file_name(""), "");									/* 空字符串 */
	EXPECT_STREQ(path_find_file_name("  "), "  ");								/* 空字符串 */
	EXPECT_EQ(path_find_file_name(NULL), (const char*)NULL);					/* NULL指针 */
#endif

	/* path_find_extension */
#ifdef _WIN32
	EXPECT_STREQ(path_find_extension("C:\\a.txt"), ".txt");							/* 普通文件 */
	EXPECT_STREQ(path_find_extension("C:\\a."), ".");								/* 仅点号 */
	EXPECT_STREQ(path_find_extension("C:\\a"), "");									/* 无扩展名 */
	EXPECT_STREQ(path_find_extension("C:\\"), "");									/* 根目录 */
	EXPECT_STREQ(path_find_extension("a..txt"), ".txt");							/* 多个点号 */
	EXPECT_STREQ(path_find_extension(".txt"), ".txt");								/* 仅扩展名 */
	EXPECT_STREQ(path_find_extension("abc"), "");									/* 仅文件名(无扩展名) */
	EXPECT_STREQ(path_find_extension("\\"), "");									/* 仅分隔符 */
	EXPECT_STREQ(path_find_extension("\\\\"), "");									/* 仅分隔符 */
	EXPECT_STREQ(path_find_extension(""), "");										/* 空字符串 */
	EXPECT_STREQ(path_find_extension("  "), "");									/* 空字符串 */
	EXPECT_EQ(path_find_extension(NULL), (const char*)NULL);						/* NULL指针 */
#endif

	/* path_find_directory */
#ifdef _WIN32
	EXPECT_TRUE(path_find_directory("C:\\folder\\a.txt", buf, MAX_PATH));				/* 普通文件 */
	EXPECT_STREQ(buf, "C:\\folder\\");
	EXPECT_TRUE(path_find_directory("C:\\folder\\another\\", buf, MAX_PATH));			/* 普通目录 */
	EXPECT_STREQ(buf, "C:\\folder\\");
	EXPECT_FALSE(path_find_directory("\\\\", buf, MAX_PATH));							/* 全分隔符 */
	EXPECT_FALSE(path_find_directory("C:\\", buf, MAX_PATH));							/* 根目录 */
	EXPECT_FALSE(path_find_directory(NULL, buf, MAX_PATH));								/* NULL指针 */
#endif
}

/* 路径检测 */
TEST(FileSystem, PathIsExists)
{
	/* path_file_exists */
#ifdef _WIN32
	EXPECT_TRUE(path_file_exists("C:\\"));
	EXPECT_TRUE(path_file_exists("C:\\windows"));
	EXPECT_TRUE(path_file_exists("C:\\Windows\\"));
	EXPECT_TRUE(path_file_exists("C:\\Windows\\explorer.exe"));
	EXPECT_FALSE(path_file_exists("D:\\not_exist_but_coincidence"));
#endif

	/* path_is_file */
#ifdef _WIN32
	EXPECT_TRUE(path_is_file("C:\\Windows\\explorer.exe"));

 	EXPECT_FALSE(path_is_file("not_exist_but_coincidence"));
 	EXPECT_TRUE(touch("not_exist_but_coincidence"));
 	EXPECT_TRUE(path_is_file("not_exist_but_coincidence"));
 	EXPECT_TRUE(delete_file("not_exist_but_coincidence"));
 	EXPECT_FALSE(path_is_file("not_exist_but_coincidence"));
#endif

	/* path_is_directory */
#ifdef _WIN32
	EXPECT_TRUE(path_is_directory("C:\\"));
	EXPECT_TRUE(path_is_directory("C:\\windows"));
	EXPECT_TRUE(path_is_directory("C:\\Windows\\"));
#endif
}

/* 相对路径 */
TEST(FileSystem, RelativePath)
{
	char buf[MAX_PATH];

#ifdef _WIN32
	EXPECT_EQ(relative_path("C:\\a.txt", "C:\\f1\\f2\\b.txt", buf, MAX_PATH), 1);
	EXPECT_STREQ(buf, "f1\\f2\\b.txt");
	EXPECT_EQ(relative_path("C:\\f1\\f2\\b.txt", "C:\\a.txt", buf, MAX_PATH), 1);
	EXPECT_STREQ(buf, "..\\..\\a.txt");
	EXPECT_EQ(relative_path("a.txt", "f1\\f2\\b.txt", buf, MAX_PATH), 1);
	EXPECT_STREQ(buf, "f1\\f2\\b.txt");
	EXPECT_EQ(relative_path("f1\\f2\\b.txt", "a.txt", buf, MAX_PATH), 1);
	EXPECT_STREQ(buf, "..\\..\\a.txt");
#endif
}

/* 唯一路径 */
TEST(FileSystem, Unique)
{
	char buf[MAX_PATH], buf1[MAX_PATH], buf2[MAX_PATH];
	char *p = "temp"SEP"unique_file_test.txt";
	char *q = "temp"SEP"unique_dir_test";

	/* unique_file */
	EXPECT_EQ(unique_file(p, buf, MAX_PATH, 0), 1);
	EXPECT_STREQ(buf, "temp"SEP"unique_file_test.txt");
	touch(buf);

	EXPECT_EQ(unique_file(buf, buf1, MAX_PATH, 0), 1);
	EXPECT_STREQ(buf1, "temp"SEP"unique_file_test (1).txt");
	touch(buf1);

	EXPECT_EQ(unique_file(buf, buf2, MAX_PATH, 0), 1);
	EXPECT_STREQ(buf2, "temp"SEP"unique_file_test (2).txt");
	touch(buf2);

	delete_file(buf);
	delete_file(buf1);
	delete_file(buf2);

	/* unique_dir */
	EXPECT_EQ(unique_dir(q, buf, MAX_PATH, 0), 1);
	EXPECT_STREQ(buf, "temp"SEP"unique_dir_test");
	touch(buf);
	
	EXPECT_EQ(unique_file(buf, buf1, MAX_PATH, 0), 1);
	EXPECT_STREQ(buf1, "temp"SEP"unique_dir_test (1)");
	touch(buf1);
	
	EXPECT_EQ(unique_file(buf, buf2, MAX_PATH, 0), 1);
	EXPECT_STREQ(buf2, "temp"SEP"unique_dir_test (2)");
	touch(buf2);
	
	EXPECT_EQ(delete_file(buf), 1);
	EXPECT_EQ(delete_file(buf1), 1);
	EXPECT_EQ(delete_file(buf2), 1);
}

/* 合法路径 */
TEST(FileSystem, Illegal)
{
	char p[] = "\\/:*?\"<>|abc!@%^&()+-=[]{};,.123";
	
#ifdef _WIN32
	path_illegal_blankspace(p, PATH_WINDOWS, 0);
	EXPECT_STREQ(p, " abc!@%^&()+-=[]{};,.123");
#else
	path_illegal_blankspace(p, PATH_UNIX, 0);
	EXPECT_STREQ(p, "\\ :*?\"<>|abc!@%^&()+-=[]{};,.123");
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
TEST(FileSystem, Directory)
{
	int i,j;
	char buf[MAX_PATH], multi_dir[MAX_PATH];

	snprintf(multi_dir, MAX_PATH, "%stemp%sfoo%sbar%sabc%s", get_current_dir(), SEP, SEP, SEP, SEP);

	/* 创建目录test/dirtest并验证其存在 */
	EXPECT_TRUE(create_directory("temp"SEP"foo"));
	EXPECT_TRUE(path_is_directory("temp"SEP"foo"));

	/* create_directory无法创建多级目录，而create_directories可以 */
	EXPECT_FALSE(create_directory(multi_dir));
	EXPECT_TRUE(create_directories(multi_dir));
	EXPECT_TRUE(path_is_directory(multi_dir));
	
	/* delete_directory无法删除多级目录，而delete_directories可以 */
	EXPECT_TRUE(delete_directories("temp"SEP"foo"SEP"bar", NULL, NULL));
	EXPECT_FALSE(path_file_exists("temp"SEP"foo"SEP"bar"));

	/* 创建更多的目录和文件 */
	for (i = 0; i < 100; i++)
	{
		snprintf(buf, MAX_PATH, "temp"SEP"foo"SEP"folder%d", i);
		EXPECT_TRUE(create_directory(buf));

		snprintf(buf, MAX_PATH, "temp"SEP"foo"SEP"%d.txt", i);
		EXPECT_TRUE(touch(buf));
	}

	/* 拷贝目录，最多拷贝50个目录和80个文件 */
	EXPECT_TRUE(create_directory("temp"SEP"foo1"));
	EXPECT_TRUE(copy_directories("temp"SEP"foo", "temp"SEP"foo1", copy_dir_hander, NULL));

	/* 遍历目录 */
	struct walk_dir_context *ctx;
	ctx = walk_dir_begin("temp"SEP"foo1"SEP"foo");
	EXPECT_NE(ctx, (struct walk_dir_context *)NULL);

	i = j = 0;
	do
	{
		if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
			continue;

		EXPECT_EQ(walk_entry_path(ctx, buf, MAX_PATH), 1);

		if (walk_entry_is_file(ctx))
			i++;
		else if (walk_entry_is_dir(ctx))
			j++;
		else
			EXPECT_TRUE(0);

	} while (walk_dir_next(ctx));

	walk_dir_end(ctx);

	/* 验证遍历到的文件/目录数量 */
	EXPECT_EQ(i, 100);
	EXPECT_EQ(j, 100);

	/* TODO: foreach_file, foreach_dir */

	/* 最后清除目录 */
	EXPECT_TRUE(delete_directories("temp"SEP"foo", delete_dir_handler, NULL));
	EXPECT_TRUE(delete_directories("temp"SEP"foo1", delete_dir_handler, NULL));
	EXPECT_FALSE(path_file_exists("temp"SEP"foo"));
	EXPECT_FALSE(path_file_exists("temp"SEP"foo1"));
}

/* 文件相关 */
TEST(FileSystem, File)
{
	
}