#include "../cutil.h"

#include <gtest/gtest.h>

#define SEP PATH_SEP_STR

TEST(FilePath, PathSeparator) {
#ifdef OS_WIN
    EXPECT_TRUE(IS_PATH_SEP('\\'));
    EXPECT_TRUE(IS_PATH_SEP('/'));
#else
    EXPECT_FALSE(IS_PATH_SEP('\\'));
    EXPECT_TRUE(IS_PATH_SEP('/'));
#endif

    /* 行为需与strchr和strrchr一致 */
    EXPECT_STREQ(NULL, strpsep(""));
    EXPECT_STREQ("/", strpsep("/"));
    EXPECT_STREQ("//", strpsep("//"));
    EXPECT_STREQ("/def", strpsep("abc/def"));
    EXPECT_STREQ("/def/", strpsep("abc/def/"));

    EXPECT_STREQ(NULL, strrpsep(""));
    EXPECT_STREQ("/", strrpsep("/"));
    EXPECT_STREQ("/", strrpsep("//"));
    EXPECT_STREQ("/def", strrpsep("abc/def"));
    EXPECT_STREQ("/ghi", strrpsep("abc/def/ghi"));

#ifdef OS_WIN
    EXPECT_STREQ(NULL, strpsep(""));
    EXPECT_STREQ("\\", strpsep("\\"));
    EXPECT_STREQ("\\\\", strpsep("\\\\"));
    EXPECT_STREQ("\\def", strpsep("abc\\def"));
    EXPECT_STREQ("\\def\\", strpsep("abc\\def\\"));

    EXPECT_STREQ(NULL, strrpsep(""));
    EXPECT_STREQ("\\", strrpsep("\\"));
    EXPECT_STREQ("\\", strrpsep("\\\\"));
    EXPECT_STREQ("\\def", strrpsep("abc\\def"));
    EXPECT_STREQ("\\ghi", strrpsep("abc\\def\\ghi"));
#endif
}

TEST(FilePath, IsRootOrAbsolutePath) {
#ifdef OS_WIN
    EXPECT_TRUE(is_root_path("C:\\"));
    EXPECT_FALSE(is_root_path("C:\\abc"));
    EXPECT_TRUE(is_absolute_path("C:\\"));
    EXPECT_TRUE(is_absolute_path("C:\\123"));
    EXPECT_FALSE(is_absolute_path("123"));
#else
    EXPECT_TRUE(is_root_path("/"));
    EXPECT_FALSE(is_root_path("/abc"));
    EXPECT_TRUE(is_absolute_path("/"));
    EXPECT_TRUE(is_absolute_path("/123"));
    EXPECT_FALSE(is_absolute_path("123"));
#endif
}

#ifdef OS_WIN
TEST(FilePath, UNCPath) {
    EXPECT_FALSE(is_unc_path(NULL));
    EXPECT_FALSE(is_unc_path(""));
    EXPECT_FALSE(is_unc_path("\\"));
    EXPECT_FALSE(is_unc_path("\\\\"));
    EXPECT_FALSE(is_unc_path("\\\\\\"));
    EXPECT_TRUE(is_unc_path("\\\\server"));
    EXPECT_TRUE(is_unc_path("//server"));
}
#endif

/* 绝对路径 */
TEST(FilePath, AbsolutePath) {
    char buf[MAX_PATH], abspath[MAX_PATH];

#ifdef OS_MACOSX
    IGNORE_RESULT(touch("abc"));
#endif
    ASSERT_TRUE(absolute_path("abc", buf, sizeof(buf)));
    xstrlcpy(abspath, get_current_dir(), sizeof(abspath));
    xstrlcat(abspath, "abc", sizeof(abspath));
    EXPECT_STREQ(abspath, buf);

#ifdef OS_WIN
    EXPECT_TRUE(absolute_path("C:\\abc\\123.txt", buf, sizeof(buf)));
    EXPECT_STREQ("C:\\abc\\123.txt", buf);
    EXPECT_TRUE(absolute_path("C:\\abc", buf, sizeof(buf)));
    EXPECT_STREQ("C:\\abc", buf);
#elif defined OS_MACOSX
    EXPECT_TRUE(absolute_path("/private/tmp", buf, sizeof(buf)));
    EXPECT_STREQ("/private/tmp", buf);
    IGNORE_RESULT(touch("/private/tmp/123.txt"));
    EXPECT_TRUE(absolute_path("/private/tmp/123.txt", buf, sizeof(buf)));
    EXPECT_STREQ("/private/tmp/123.txt", buf);
#else
    EXPECT_TRUE(absolute_path("/tmp", buf, sizeof(buf)));
    EXPECT_STREQ("/tmp", buf);
    EXPECT_TRUE(absolute_path("/tmp/123.txt", buf, sizeof(buf)));
    EXPECT_STREQ("/tmp/123.txt", buf);
#endif
}

/* 相对路径 */
TEST(FilePath, RelativePath)
{
    char buf[MAX_PATH];

#ifdef OS_WIN
    EXPECT_EQ(relative_path("C:\\a.txt", "C:\\f1\\f2\\b.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "f1\\f2\\b.txt");
    EXPECT_EQ(relative_path("C:\\f1\\f2\\b.txt", "C:\\a.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "..\\..\\a.txt");
    EXPECT_EQ(relative_path("a.txt", "f1\\f2\\b.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "f1\\f2\\b.txt");
    EXPECT_EQ(relative_path("f1\\f2\\b.txt", "a.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "..\\..\\a.txt");
#else
    EXPECT_EQ(relative_path("/a.txt", "/f1/f2/b.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "f1/f2/b.txt");
    EXPECT_EQ(relative_path("/f1/f2/b.txt", "/a.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "../../a.txt");
    EXPECT_EQ(relative_path("a.txt", "f1/f2/b.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "f1/f2/b.txt");
    EXPECT_EQ(relative_path("f1/f2/b.txt", "a.txt", buf, sizeof(buf)), 1);
    EXPECT_STREQ(buf, "../../a.txt");
#endif
}

/* 路径查找 */
TEST(FilePath, PathFindFileName)
{
    /* 在Windows下结果需与Win32 API PathFindExtension 一致 */
#ifdef OS_WIN
#undef EXPECT_STREQ_W
#define EXPECT_STREQ_W(path, expect) \
    EXPECT_STREQ(expect, path_find_file_name(path)); \
    EXPECT_STREQ(expect, PathFindFileNameA(path));
#else
#define EXPECT_STREQ_W(path, expect) EXPECT_STREQ(expect, path_find_file_name(path))
#endif

    /* path_find_file_name */
    EXPECT_STREQ_W("", "");									/* 空字符串 */
    EXPECT_STREQ_W("  ", "  ");								/* 空字符串 */
    EXPECT_STREQ_W("abc.txt", "abc.txt");					/* 仅文件名 */
    EXPECT_STREQ_W("abc", "abc");							/* 仅文件名(无扩展名) */
    EXPECT_EQ(path_find_file_name(NULL), (const char*)NULL);/* NULL指针 */

#ifdef OS_WIN
    // MSDN的测试用例 http://msdn.microsoft.com/en-us/library/windows/desktop/bb773589(v=vs.85).aspx
    EXPECT_STREQ_W("c:\\path\\file", "file");
    EXPECT_STREQ_W("c:\\path", "path");
    EXPECT_STREQ_W("c:\\path\\", "path\\");
    EXPECT_STREQ_W("c:\\", "c:\\");
    EXPECT_STREQ_W("c:", "c:");
    EXPECT_STREQ_W("path", "path");
    // 其他测试用例
    EXPECT_STREQ_W("C:\\myfolder\\abc.txt", "abc.txt");		/* 普通文件 */
    EXPECT_STREQ_W("C:\\\\myfolder\\\\abc.txt", "abc.txt");  /* 双分隔符 */
    EXPECT_STREQ_W("C:\\myfolder\\abc", "abc");				/* 普通文件(无扩展名) */
    EXPECT_STREQ_W("C:\\myfolder\\", "myfolder\\");			/* 目录(有分隔符) */
    EXPECT_STREQ_W("C:\\myfolder\\\\", "myfolder\\\\");		/* 目录(多分隔符) */
    EXPECT_STREQ_W("C:\\myfolder", "myfolder");				/* 目录 */
    EXPECT_STREQ_W("C:\\", "C:\\");							/* 根目录 */
    EXPECT_STREQ_W("..\\abc.txt", "abc.txt");				/* 相对路径 */
    EXPECT_STREQ_W("\\", "\\");								/* 仅分隔符 */
    EXPECT_STREQ_W("\\\\", "\\\\");							/* 仅分隔符 */
#else /* POSIX */
    EXPECT_STREQ_W("/path/file", "file");
    EXPECT_STREQ_W("//path//file", "file");
    EXPECT_STREQ_W("/path", "path");
    EXPECT_STREQ_W("/path/", "path/");
    EXPECT_STREQ_W("/", "/");
    EXPECT_STREQ_W("//", "//");
    EXPECT_STREQ_W("path", "path");
#endif
}

TEST(FilePath, PathFindExtension) {
    /* 在Windows下结果需与Win32 API PathFindExtension 一致 */
#undef EXPECT_STREQ_W
#ifdef OS_WIN
#undef EXPECT_STREQ_W
#define EXPECT_STREQ_W(path, expect) \
    EXPECT_STREQ(expect, path_find_extension(path)); \
    EXPECT_STREQ(expect, PathFindExtensionA(path));
#else
#define EXPECT_STREQ_W(path, expect) EXPECT_STREQ(expect, path_find_extension(path))
#endif

    EXPECT_STREQ_W("abc.", ".");
    EXPECT_STREQ_W(".", ".");
    EXPECT_STREQ_W("..", ".");
    EXPECT_STREQ_W("a..txt", ".txt");							/* 多个点号 */
    EXPECT_STREQ_W(".txt", ".txt");								/* 仅扩展名 */
    EXPECT_STREQ_W(".tar.gz", ".gz");							/* 多个扩展名 */
    EXPECT_STREQ_W("abc", "");									/* 仅文件名(无扩展名) */
    EXPECT_STREQ_W("", "");										/* 空字符串 */
    EXPECT_STREQ_W("  ", "");									/* 空字符串 */
    EXPECT_EQ(path_find_extension(NULL), (const char*)NULL);	/* NULL指针 */

#ifdef OS_WIN
    EXPECT_STREQ_W("C:\\a.txt", ".txt");						/* 普通文件 */
    EXPECT_STREQ_W("C:\\a.", ".");								/* 仅点号 */
    EXPECT_STREQ_W("C:\\a", "");								/* 无扩展名 */
    EXPECT_STREQ_W("C:\\", "");									/* 根目录 */
    EXPECT_STREQ_W("\\", "");									/* 仅分隔符 */
    EXPECT_STREQ_W("\\\\", "");									/* 仅分隔符 */
#else
    EXPECT_STREQ_W("/a.txt", ".txt");
    EXPECT_STREQ_W("/a.", ".");
    EXPECT_STREQ_W("/a", "");
    EXPECT_STREQ_W("/", "");
    EXPECT_STREQ_W("//", "");
#endif
}

TEST(FilePath, PathFindDirectory) {
    char buf[MAX_PATH];

    EXPECT_FALSE(path_find_directory(NULL, buf, MAX_PATH));
    EXPECT_FALSE(path_find_directory("", buf, MAX_PATH));

#ifdef OS_WIN
    EXPECT_TRUE(path_find_directory("C:\\folder\\a.txt", buf, MAX_PATH));				/* 普通文件 */
    EXPECT_STREQ(buf, "C:\\folder\\");
    EXPECT_TRUE(path_find_directory("C:\\folder\\another\\", buf, MAX_PATH));			/* 普通目录 */
    EXPECT_STREQ(buf, "C:\\folder\\");
    EXPECT_FALSE(path_find_directory("\\\\", buf, MAX_PATH));							/* 全分隔符 */
    EXPECT_FALSE(path_find_directory("C:\\", buf, MAX_PATH));							/* 根目录 */
#else
    EXPECT_TRUE(path_find_directory("/folder/a.txt", buf, MAX_PATH));
    EXPECT_STREQ(buf, "/folder/");
    EXPECT_TRUE(path_find_directory("/folder/another/", buf, MAX_PATH));
    EXPECT_STREQ(buf, "/folder/");
    EXPECT_FALSE(path_find_directory("/", buf, MAX_PATH));
    EXPECT_FALSE(path_find_directory("//", buf, MAX_PATH));
#endif
}

/* 路径检测 */
TEST(FilePath, PathExists)
{
    /* path_file_exists */
#ifdef OS_WIN
    EXPECT_TRUE(path_file_exists("C:\\"));
    EXPECT_TRUE(path_file_exists("C:\\windows"));
    EXPECT_TRUE(path_file_exists("C:\\Windows\\"));
    EXPECT_TRUE(path_file_exists("C:\\Windows\\explorer.exe"));
    EXPECT_FALSE(path_file_exists("D:\\not_exist_but_coincidence"));
#else
    EXPECT_TRUE(path_file_exists("/"));
    EXPECT_TRUE(path_file_exists("/etc"));
    EXPECT_TRUE(path_file_exists("/etc/"));
    EXPECT_TRUE(path_file_exists("/etc/passwd"));
    EXPECT_FALSE(path_file_exists("/etc/rkqwkjsjdfas"));
#endif

    const char* random_name = "not_exist_but_coincidence";

    /* path_is_file */
    EXPECT_FALSE(path_is_file(random_name));
    EXPECT_TRUE(touch(random_name));
    EXPECT_TRUE(path_is_file(random_name));
    EXPECT_TRUE(delete_file(random_name));
    EXPECT_FALSE(path_is_file(random_name));

#ifdef OS_WIN
    EXPECT_TRUE(path_is_file("C:\\Windows\\explorer.exe"));
    EXPECT_FALSE(path_is_file("C:\\Windows\\explorer11223123.exe"));
#else
    EXPECT_TRUE(path_is_file("/etc/passwd"));
    EXPECT_FALSE(path_is_file("/etc/fstab2k421k4jf"));
#endif

    /* path_is_directory */
#ifdef OS_WIN
    EXPECT_TRUE(path_is_directory("C:\\"));
    EXPECT_TRUE(path_is_directory("C:\\windows"));
    EXPECT_TRUE(path_is_directory("C:\\Windows\\"));
    EXPECT_FALSE(path_is_directory("C:\\Windowsqwejrklj"));
#else
    EXPECT_TRUE(path_is_directory("/"));
    EXPECT_TRUE(path_is_directory("/etc"));
    EXPECT_TRUE(path_is_directory("/etc/"));
    EXPECT_FALSE(path_is_directory("/etcalk23k4j12"));
#endif
}

TEST(FilePath, InsertBeforeExtension) {
    char buf[MAX_PATH];
    EXPECT_FALSE(path_insert_before_extension(NULL, "abc", buf, sizeof(buf)));
    EXPECT_FALSE(path_insert_before_extension("", " (1)", buf, sizeof(buf)));
    EXPECT_FALSE(path_insert_before_extension("abc", NULL, buf, sizeof(buf)));
    EXPECT_FALSE(path_insert_before_extension(NULL, NULL, buf, sizeof(buf)));

    EXPECT_TRUE(path_insert_before_extension("abc", "", buf, sizeof(buf)));
    EXPECT_STREQ("abc", buf);
    EXPECT_TRUE(path_insert_before_extension("abc.txt", " (1)", buf, sizeof(buf)));
    EXPECT_STREQ("abc (1).txt", buf);
    EXPECT_TRUE(path_insert_before_extension("abc", " (1)", buf, sizeof(buf)));
    EXPECT_STREQ("abc (1)", buf);
    EXPECT_TRUE(path_insert_before_extension("abc.", " (1)", buf, sizeof(buf)));
    EXPECT_STREQ("abc (1).", buf);

#ifdef OS_WIN
    EXPECT_TRUE(path_insert_before_extension("C:\\dir\\abc.txt", " (1)", buf, sizeof(buf)));
    EXPECT_STREQ("C:\\dir\\abc (1).txt", buf);
    EXPECT_TRUE(path_insert_before_extension("C:\\dir\\abc", " (1)", buf, sizeof(buf)));
    EXPECT_STREQ("C:\\dir\\abc (1)", buf);
    EXPECT_TRUE(path_insert_before_extension("C:\\dir\\abc.", " (1)", buf, sizeof(buf)));
    EXPECT_STREQ("C:\\dir\\abc (1).", buf);
#else

#endif
}

/* 唯一路径 */
TEST(FilePath, UniqueFile)
{
    char buf[MAX_PATH], buf1[MAX_PATH], buf2[MAX_PATH];
    const char *p = "temp" SEP "unique_file_test.txt";

    EXPECT_FALSE(unique_file(NULL, buf, MAX_PATH, 0));
    EXPECT_FALSE(unique_file(p, NULL, MAX_PATH, 0));
    EXPECT_FALSE(unique_file("", NULL, MAX_PATH, 0));

    EXPECT_EQ(unique_file(p, buf, MAX_PATH, 0), 1);
    EXPECT_STREQ(buf, "temp" SEP "unique_file_test.txt");
    EXPECT_EQ(unique_file(p, buf, MAX_PATH, 1), 1);
    ASSERT_TRUE(path_file_exists(buf));

    EXPECT_EQ(unique_file(buf, buf1, MAX_PATH, 0), 1);
    EXPECT_STREQ(buf1, "temp" SEP "unique_file_test (1).txt");
    EXPECT_EQ(unique_file(buf, buf1, MAX_PATH, 1), 1);
    ASSERT_TRUE(path_file_exists(buf1));

    EXPECT_EQ(unique_file(buf, buf2, MAX_PATH, 0), 1);
    EXPECT_STREQ(buf2, "temp" SEP "unique_file_test (2).txt");
    EXPECT_EQ(unique_file(buf, buf2, MAX_PATH, 1), 1);
    ASSERT_TRUE(path_file_exists(buf2));

    EXPECT_TRUE(delete_file(buf));
    EXPECT_TRUE(delete_file(buf1));
    EXPECT_TRUE(delete_file(buf2));
}

TEST(FilePath, UniqueDir)
{
    char buf[MAX_PATH], buf1[MAX_PATH], buf2[MAX_PATH];
    const char *q = "temp" SEP "unique_dir_test" SEP;

    EXPECT_FALSE(unique_dir(NULL, buf, MAX_PATH, 0));
    EXPECT_FALSE(unique_dir(q, NULL, MAX_PATH, 0));
    EXPECT_FALSE(unique_dir("", NULL, MAX_PATH, 0));

    EXPECT_EQ(unique_dir(q, buf, MAX_PATH, 0), 1);
    EXPECT_STREQ(buf, "temp" SEP "unique_dir_test" SEP);
    EXPECT_EQ(unique_dir(q, buf, MAX_PATH, 1), 1);
    ASSERT_TRUE(path_is_directory(buf));

    EXPECT_EQ(unique_dir(buf, buf1, MAX_PATH, 0), 1);
    EXPECT_STREQ(buf1, "temp" SEP "unique_dir_test (1)" SEP);
    EXPECT_EQ(unique_dir(buf, buf1, MAX_PATH, 1), 1);
    ASSERT_TRUE(path_is_directory(buf1));

    EXPECT_EQ(unique_dir(buf, buf2, MAX_PATH, 0), 1);
    EXPECT_STREQ(buf2, "temp" SEP "unique_dir_test (2)" SEP);
    EXPECT_EQ(unique_dir(buf, buf2, MAX_PATH, 1), 1);
    ASSERT_TRUE(path_is_directory(buf2));

    EXPECT_EQ(delete_directory(buf), 1);
    EXPECT_EQ(delete_directory(buf1), 1);
    EXPECT_EQ(delete_directory(buf2), 1);
}

/* 合法路径 */
TEST(FilePath, BlankIllegalChar)
{
    char p[] = "\\/:*?\"<>|abc!@%^&()+-=[]{};,.123";

    char* t = xstrdup(p);
    path_illegal_blankspace(t, PATH_WINDOWS, 0);
    EXPECT_STREQ(" abc!@%^&()+-=[]{};,.123", t);
    xfree(t);

    t = xstrdup(p);
    path_illegal_blankspace(t, PATH_WINDOWS, 1);
    EXPECT_STREQ("\\/ abc!@%^&()+-=[]{};,.123", t);
    xfree(t);

    t = xstrdup(p);
    path_illegal_blankspace(t, PATH_UNIX, 0);
    EXPECT_STREQ("\\ :*?\"<>|abc!@%^&()+-=[]{};,.123", t);
    xfree(t);

    t = xstrdup(p);
    path_illegal_blankspace(t, PATH_UNIX, 1);
    EXPECT_STREQ("\\/:*?\"<>|abc!@%^&()+-=[]{};,.123", t);
    xfree(t);
}

TEST(FilePath, EscapeIllegalChar)
{
    char p[] = "\\/:*?\"<>|abc!@%^&()+-=[]{};,.123";

    char* t = path_escape(p, PATH_WINDOWS, 0);
    EXPECT_STREQ("%5C%2F%3A%2A%3F%22%3C%3E%7Cabc!@%^&()+-=[]{};,.123", t);
    xfree(t);

    t = path_escape(p, PATH_WINDOWS, 1);
    EXPECT_STREQ("\\/%3A%2A%3F%22%3C%3E%7Cabc!@%^&()+-=[]{};,.123", t);
    xfree(t);

    t = path_escape(p, PATH_UNIX, 0);
    EXPECT_STREQ("\\%2F:*?\"<>|abc!@%^&()+-=[]{};,.123", t);
    xfree(t);

    t = path_escape(p, PATH_UNIX, 1);
    EXPECT_STREQ("\\/:*?\"<>|abc!@%^&()+-=[]{};,.123", t);
    xfree(t);
}
