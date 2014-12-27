#include "../cutil.h"

#include <gtest/gtest.h>

#define SEP PATH_SEP_STR

/* 目录相关 */
TEST(Directory, Basic)
{
	/* 创建目录test/dirtest并验证其存在 */
    const char* single_dir = "temp" SEP "foo";
    EXPECT_TRUE(create_directory(single_dir));
    EXPECT_TRUE(path_is_directory(single_dir));
    EXPECT_TRUE(is_empty_dir(single_dir));
    EXPECT_TRUE(delete_directory(single_dir));
    EXPECT_FALSE(path_file_exists(single_dir));

	/* create_directory无法创建多级目录，而create_directories可以 */
    const char* multi_dir = "temp"SEP"foo"SEP"bar"SEP"abc"SEP;
	EXPECT_FALSE(create_directory(multi_dir));
	EXPECT_TRUE(create_directories(multi_dir));
	EXPECT_TRUE(path_is_directory(multi_dir));
    EXPECT_TRUE(is_empty_dir(multi_dir));
	
    const char* parent_dir = "temp" SEP "foo" SEP "bar";
    EXPECT_FALSE(is_empty_dir(parent_dir));

    /* delete_directory无法删除多级目录 */
    EXPECT_FALSE(delete_directory(parent_dir));
    EXPECT_TRUE(path_file_exists(parent_dir));

    /* delete_directories可以删除多级目录 */
    EXPECT_TRUE(delete_directories(parent_dir, NULL, NULL));
    EXPECT_FALSE(path_file_exists(parent_dir));
}

TEST(Directory, DeleteEmpty)
{
    const char* multi_dir = "temp"SEP"foo"SEP"bar"SEP"abc"SEP;

    // 没有任何文件，所有目录将被删除
    ASSERT_TRUE(create_directories(multi_dir));
    EXPECT_TRUE(delete_empty_directories("temp"SEP"foo"));
    EXPECT_FALSE(path_file_exists("temp"SEP"foo"));

    // 在最底层目录下新建一个文件，一个目录也不会被删除
    char buf[MAX_PATH];
    ASSERT_TRUE(create_directories(multi_dir));
    ASSERT_TRUE(get_temp_file_under(multi_dir, "test", buf, sizeof(buf)));
    EXPECT_FALSE(delete_empty_directories("temp"SEP"foo"));
    EXPECT_TRUE(path_is_file(buf));
    EXPECT_TRUE(path_is_directory("temp"SEP"foo"));

    // 在倒数第二层目录下新建一个文件，仅最后一级目录被删除
    ASSERT_TRUE(delete_file(buf));
    ASSERT_TRUE(get_temp_file_under("temp"SEP"foo"SEP"bar", "test", buf, sizeof(buf)));
    EXPECT_FALSE(delete_empty_directories("temp"SEP"foo"));
    EXPECT_FALSE(path_file_exists(multi_dir));
    EXPECT_TRUE(path_is_file(buf));
    EXPECT_TRUE(path_is_directory("temp"SEP"foo"SEP"bar"));

    EXPECT_TRUE(delete_directories("temp"SEP"foo", NULL, NULL));
}

static int copy_dir_hander(const char* src, const char *dst,
    int action, int succ, void *arg) {
    return 1;
}

/* 目录遍历 */
TEST(Directory, Walk)
{
    int i, j;
    char buf[MAX_PATH];

    ASSERT_TRUE(create_directories("temp" SEP "foo" SEP));

    /* 在temp/foo目录下创建100个文件和100个目录 */
    for (i = 0; i < 100; i++) {
        snprintf(buf, MAX_PATH, "temp" SEP "foo" SEP "folder%d", i);
        EXPECT_TRUE(create_directory(buf));

        snprintf(buf, MAX_PATH, "temp" SEP "foo" SEP "%d.txt", i);
        EXPECT_TRUE(touch(buf));
    }

    /* 拷贝foo目录到foo1下 */
    EXPECT_TRUE(create_directory("temp" SEP "foo1"));
    EXPECT_TRUE(copy_directories("temp" SEP "foo", "temp" SEP "foo1", copy_dir_hander, NULL));

    /* 检查foot1目录下的内容 */
    struct walk_dir_context *ctx;
    ctx = walk_dir_begin("temp" SEP "foo1" SEP "foo");
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
}

static int handle_each_file(const char* fpath, void *arg) {
    *((int*)arg)+=1;
    return 1;
}

static int handle_each_dir(const char* fpath, void *arg) {
    *((int*)arg)+=1;
    return 1;
}

static int g_deleted_files;
static int g_deleted_dirs;

static int delete_dir_handler(const char* path, int action, int succ, void *arg) {
    if (action == 0)
        g_deleted_files++;
    else if (action == 1)
        g_deleted_dirs++;

    return 1;
}

TEST(Directory, Foreach)
{
    /* 在folder0下新建100个文件以检查递归行为 */
    char buf[MAX_PATH];
    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(get_temp_file_under("temp" SEP "foo" SEP "folder0", "test", buf, sizeof(buf)));
    }

    /* 不递归遍历文件 */
    int count = 0;
    EXPECT_TRUE(foreach_file("temp" SEP "foo", handle_each_file, 0, 1, &count));
    EXPECT_EQ(100, count);

    /* 递归遍历文件 */
    count = 0;
    EXPECT_TRUE(foreach_file("temp" SEP "foo", handle_each_file, 1, 1, &count));
    EXPECT_EQ(200, count);

    /* 遍历目录 */
    count = 0;
    EXPECT_TRUE(foreach_dir("temp" SEP "foo", handle_each_dir, &count));
    EXPECT_EQ(100, count);

    /* 最后清除目录 */
    g_deleted_dirs = g_deleted_files = 0;
    EXPECT_TRUE(delete_directories("temp" SEP "foo", delete_dir_handler, NULL));
    EXPECT_EQ(200, g_deleted_files);
    EXPECT_EQ(100, g_deleted_dirs);
    EXPECT_TRUE(delete_directories("temp" SEP "foo1", delete_dir_handler, NULL));
    EXPECT_FALSE(path_file_exists("temp" SEP "foo"));
    EXPECT_FALSE(path_file_exists("temp" SEP "foo1"));
}
