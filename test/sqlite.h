/* 测试SQLite数据库 */

#ifndef TEST_SQLITE_H
#define TEST_SQLITE_H

#ifdef __cplusplus
extern "C" {
#endif

void sqlite_insert();			/* 插入速度测试 */
void sqlite_select();			/* 查询速度测试 */

#ifdef __cplusplus
}
#endif

#endif
