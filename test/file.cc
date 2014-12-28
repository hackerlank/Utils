#include "../cutil.h"

#include <gtest/gtest.h>
#include "gnu-md5.h"

#define SEP PATH_SEP_STR
#define GBK_BOOK "data" SEP "Four Famous Novels" SEP "Journey to the West (GBK).txt"
#define UTF8_BOOK "data" SEP "Four Famous Novels" SEP "The Dream of the Red Chamber (UTF-8).txt"
#define UTF8BOM_BOOK "data" SEP "Four Famous Novels" SEP "The Romance of the Three Kindoms (UTF-8 BOM).txt"
#define UTF16LE_BOOK "data" SEP "Four Famous Novels" SEP "The Story by the Water Margin (UTF-16LE).txt"

#define MD5_LEN 16

// 比较两个文件的MD5值是否一致
static int md5_is_same(const char* file1, const char* file2)
{
    char md5_1[MD5_LEN], md5_2[MD5_LEN];

    FILE* fp1 = fopen(file1, "rb");
    if (!fp1)
        return 0;

    md5_stream(fp1, md5_1);
    fclose(fp1);

    FILE* fp2 = fopen(file2, "rb");
    if (!fp2)
        return 0;

    md5_stream(fp2, md5_2);
    fclose(fp2);

    return !memcmp(md5_1, md5_2, MD5_LEN);
}

class FileTest : public testing::Test {
protected:
    virtual void SetUp() OVERRIDE {
        path_find_directory(__FILE__, gbk_book, sizeof(gbk_book));
        strlcat(gbk_book, GBK_BOOK, sizeof(gbk_book));
        ASSERT_TRUE(path_is_file(gbk_book));

        path_find_directory(__FILE__, utf8_book, sizeof(utf8_book));
        strlcat(utf8_book, UTF8_BOOK, sizeof(utf8_book));
        ASSERT_TRUE(path_is_file(utf8_book));
    }

    virtual void TearDown() OVERRIDE {

    }

protected:
    char gbk_book[MAX_PATH];
    char utf8_book[MAX_PATH];
};

TEST_F(FileTest, CopyMove)
{
    char book1[MAX_PATH];
    strlcpy(book1, "temp" SEP, sizeof(book1));
    strlcat(book1, path_find_file_name(GBK_BOOK), sizeof(book1));

    ASSERT_TRUE(touch(book1));
    EXPECT_FALSE(copy_file(gbk_book, book1, 0));

    ASSERT_TRUE(copy_file(gbk_book, book1, 1));
    EXPECT_TRUE(md5_is_same(gbk_book, book1));

    // 移动文件
    char book2[MAX_PATH];
    path_insert_before_extension(book1, " (1)", book2, sizeof(book2));

    ASSERT_TRUE(touch(book2));
    EXPECT_FALSE(move_file(book1, book2, 0));
    EXPECT_TRUE(path_file_exists(book1));

    ASSERT_TRUE(move_file(book1, book2, 1));
    EXPECT_FALSE(path_file_exists(book1));
    EXPECT_TRUE(md5_is_same(gbk_book, book2));

    ASSERT_TRUE(delete_file(book2));
    EXPECT_FALSE(path_file_exists(book2));
}

TEST_F(FileTest, Size)
{
    ASSERT_EQ(INT64_C(1296952), file_size(gbk_book));
    ASSERT_EQ(INT64_C(1298432), get_file_disk_usage(gbk_book));

    ASSERT_EQ(4096, get_file_block_size(gbk_book));

    char buf[12];
    file_size_readable(file_size(gbk_book), buf, sizeof(buf));
    ASSERT_STREQ("1.24 MB", buf);
}

#define ASTER_POS 3021
#define UTF8_BOOK_LEN 2610700

TEST_F(FileTest, Read)
{
    FILE* fp = xfopen(utf8_book, "rb");
    ASSERT_NE(fp, (FILE*)NULL);

    // 读取到指定的字符结束，使用提供的缓冲区
    char buf[4094];
    char *p = buf;
    size_t n = sizeof(buf);
    xfread(fp, '*', 0, &p, &n);
    EXPECT_EQ(ASTER_POS, n);
    EXPECT_EQ(p, buf);

    // 如果提供的缓冲区过小，将自动分配内存
    p = buf;
    n = 1024;
    fseek(fp, 0, SEEK_SET);
    ASSERT_TRUE(xfread(fp, '*', 0, &p, &n));
    EXPECT_EQ(ASTER_POS, n);
    EXPECT_NE(p, buf);
    xfree(p);

    // 如果没有提供缓冲区，将自动分配内存
    p = NULL;
    fseek(fp, 0, SEEK_SET);
    ASSERT_TRUE(xfread(fp, '*', 0, &p, &n));
    EXPECT_EQ(ASTER_POS, n);
    EXPECT_NE(p, buf);
    xfree(p);

    // 读取指定的字节数
    p = buf;
    n = sizeof(buf);
    memset(buf, 0, sizeof(buf));
    fseek(fp, 0, SEEK_SET);
    ASSERT_TRUE(xfread(fp, -1, ASTER_POS, &p, &n));
    EXPECT_EQ(ASTER_POS, n);
    EXPECT_EQ('*', buf[ASTER_POS-1]);
    EXPECT_EQ('\0', buf[ASTER_POS]);
    EXPECT_EQ('\0', buf[ASTER_POS+1]);
    EXPECT_EQ(p, buf);

    // 读取一行
    p = buf;
    n = sizeof(buf);
    memset(buf, 0, sizeof(buf));
    fseek(fp, 0, SEEK_SET);
    ASSERT_TRUE(get_line(fp, &p, &n));
    EXPECT_EQ(17, n);
    EXPECT_EQ('\r', buf[15]);
    EXPECT_EQ('\n', buf[16]);
    EXPECT_EQ('\0', buf[17]);
    EXPECT_EQ('\0', buf[18]);

    xfclose(fp);
}

TEST_F(FileTest, ReadWriteMem)
{
    struct file_mem* fm = read_file_mem(utf8_book, 0);
    ASSERT_NE((struct file_mem*)NULL, fm);
    EXPECT_EQ(UTF8_BOOK_LEN, fm->length);
    EXPECT_NE((char*)NULL, fm->content);
    free_file_mem(fm);

    fm = read_file_mem(utf8_book, 4096);
    ASSERT_EQ((struct file_mem*)NULL, fm);

    const char *tmpf = "temp" SEP "write_mem_file";
    EXPECT_FALSE(write_mem_file(tmpf, NULL, 100));
    EXPECT_TRUE(write_mem_file(tmpf, "", 100));

    const char content[] = "some thing to write, haha\n";
    EXPECT_TRUE(write_mem_file(tmpf, content, sizeof(content)));
    EXPECT_EQ(sizeof(content), file_size(tmpf));
}

static int g_line_count;
static int g_length;

static int handle_line(char *line, size_t len, size_t nline, void *arg)
{
    g_line_count++;
    g_length += len;
    return 1;
}

static int handle_line_1000(char *line, size_t len, size_t nline, void *arg)
{
    if (++g_line_count >= 1000)
        return 0;

    return 1;
}

static int handle_block(char *line, size_t len, size_t nline, void *arg)
{
    g_line_count++;
    g_length += len;
    return 1;
}

TEST_F(FileTest, Foreach)
{
    FILE* fp = xfopen(utf8_book, "rb");
    ASSERT_NE(fp, (FILE*)NULL);

    ASSERT_TRUE(foreach_line(fp, handle_line, NULL));
    EXPECT_EQ(3629, g_line_count);
    EXPECT_EQ(UTF8_BOOK_LEN, g_length);

    g_line_count = g_length = 0;
    fseek(fp, 0, SEEK_SET);
    ASSERT_FALSE(foreach_line(fp, handle_line_1000, NULL));
    EXPECT_EQ(1000, g_line_count);

    g_line_count = g_length = 0;
    fseek(fp, 0, SEEK_SET);
    ASSERT_TRUE(foreach_block(fp, handle_block, '*', NULL));
    EXPECT_EQ(39, g_line_count);
    EXPECT_EQ(UTF8_BOOK_LEN, g_length);

    fclose(fp);
}
