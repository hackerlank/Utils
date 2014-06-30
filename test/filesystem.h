/* 文件系统相关测试 */

#ifndef TEST_FS_H
#define TEST_FS_H

#ifdef __cplusplus
extern "C" {
#endif

void	fs_path_find();			/* 路径查找 */
void	fs_path_is();			/* 路径检测 */
void	fs_path_relative();		/* 相对路径 */
void	fs_path_unique();		/* 唯一路径 */
void	fs_path_illegal();		/* 合法路径 */
void	fs_directory();			/* 目录相关 */
void	fs_file();				/* 文件相关 */

#ifdef __cplusplus
}
#endif

#endif
