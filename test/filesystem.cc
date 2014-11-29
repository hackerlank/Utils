#include "../cutil.h"

#include <gtest/gtest.h>

#define SEP PATH_SEP_STR

static int copy_dir_hander(const char* src, const char *dst,																				
    int action, int succ, void *arg) {
	return 1;
}

static int delete_dir_handler(const char* path, int action, int succ, void *arg) {
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
