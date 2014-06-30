#include "sqlite.h"
#include "../cutil.h"
#include "../third_party/sqlite3.h"

#ifdef OS_WIN
#define DB_FILE "G:\\sqlite_test.db"
#define SRC_DATA_DIR "G:\\Chrome\\"
#else
#define DB_FILE "/tmp/sqlite_test.db"
#define SRC_DATA_DIR "/usr/include/"
#endif

int insert_file_proc(const char *path, void *arg)
{
	sqlite3 *db = (sqlite3*)arg;
	sqlite3_stmt* stmt = NULL;  
	int64_t size;
	utimer_t timer;
	char sql[1024], fsize[128];
	int ret;

	const int max_inserts = 100000;			//最多插入次数
	static int num_inserts = 0;				//当前已插入次数
	static int64_t total_size = 0;			//已插入文件大小的总大小
	static double total_time = 0;			//插入所需的总时间(ms)

	//测试读取数据库时无匹配情况
	static int file_id = 0;
	if (++file_id % 5 == 0)
		return 1;

	size = file_size(path);
	file_size_readable(size, fsize, sizeof(fsize));
	snprintf(sql, sizeof(sql), "INSERT INTO t VALUES(NULL, '%s', %d, 'testdatatestdata');", path, (int)size);

	utimer_start(&timer);
	ret = sqlite3_prepare(db, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK)
		goto exit;

// 	ret = sqlite3_bind_blob(stmt, 1, fm->content, fm->length, NULL);
// 	if (ret != SQLITE_OK)
// 		goto exit;

	ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	utimer_stop(&timer);

	num_inserts++;
	total_size += size;
	total_time += utimer_elapsed_ms(&timer);

	log_dprintf(LOG_INFO, "%d: INSERT %s %s(%s) (%.2fms avg %.2lfms)\n", 
		num_inserts, 
		ret == SQLITE_DONE ? "SUCCESS" : "FAILED", 
		path, 
		fsize, 
		utimer_elapsed_ms(&timer), 
		total_time/num_inserts);

exit:
// 	if (fm)
// 		free_file_mem(fm);

	if (num_inserts >= max_inserts)
		return 0;

	return 1;
}

/* 插入速度测试 */
void sqlite_insert()
{
	sqlite3 *db;
	char *errmsg;
	int ret;

	delete_file(DB_FILE);

	if (sqlite3_open_v2(DB_FILE, &db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL))
	{
		printf("couldn't open db to write.\n");
		return;
	}

	ret = sqlite3_exec(db, "CREATE TABLE t(id INTEGER PRIMARY KEY AUTOINCREMENT, path VARCHAR(255), size INTEGER, content BLOB)", NULL, 0, &errmsg);
	if (ret != SQLITE_OK)
	{
		printf("create table failed : %s\n", errmsg);
		sqlite3_free(errmsg);
		return;
	}

	printf("SQLite INSERT test started...\n");
	foreach_file(SRC_DATA_DIR, insert_file_proc, 1, 1, db);
	printf("SQLite INSERT test complete!\n");

	sqlite3_close_v2(db);
}

int select_file_proc(const char *path, void *arg)
{
	sqlite3 *db = (sqlite3*)arg;
	sqlite3_stmt* stmt = NULL;  
	utimer_t timer;
	char sql[1024], fsize[128];
	int64_t size;
	int ret;

	const int max_select = 100000;			//最多查询次数
	static int num_select = 0;				//当前已查询次数
	static int64_t total_size = 0;			//已查询文件大小的总大小
	static double total_time = 0;			//查询所需的总时间(ms)

	snprintf(sql, sizeof(sql), "select * from t WHERE path='%s';", path);

	utimer_start(&timer);
	ret = sqlite3_prepare(db, sql, -1, &stmt, 0);
	if (ret != SQLITE_OK)
		goto exit;

	ret = sqlite3_step(stmt);
	if (ret != SQLITE_ROW)
		goto exit;

	//得到纪录中的BLOB字段
	//content = sqlite3_column_blob(stmt, 3);

	//得到字段中数据的长度
	size = sqlite3_column_int(stmt, 2);
	sqlite3_finalize(stmt);

	utimer_stop(&timer);

	num_select++;
	//total_size += size;
	total_time += utimer_elapsed_ms(&timer);
	file_size_readable(size, fsize, sizeof(fsize));

	log_dprintf(LOG_INFO, "%d: SELECT %s %s(%s) (%.2fms avg %.2lfms)\n", 
		num_select, 
		"SUCCESS", 
		path,
		fsize, 
		utimer_elapsed_ms(&timer), 
		total_time/num_select);

exit:
	if (num_select >= max_select)
		return 0;

	return 1;
}

/* 查询速度测试 */
void sqlite_select()
{
	sqlite3 *db;

	if (sqlite3_open_v2(DB_FILE, &db, SQLITE_OPEN_READONLY, NULL))
	{
		printf("couldn't open db to read.\n");
		return;
	}

	printf("SQLite SELECT test started...\n");
	foreach_file(SRC_DATA_DIR, select_file_proc, 1, 1, db);
	printf("SQLite SELECT test complete!\n");

	sqlite3_close_v2(db);
}
