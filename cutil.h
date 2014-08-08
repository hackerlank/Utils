/* 
* @file: cutil.h
* @desc: 可移植系统平台兼容层
*		 主要基于系统API及标准C库，高效实用
* @auth: liwei (www.leewei.org)
* @mail: ari.feng@qq.com
* @date: 2012/04/22
* @mdfy: 2013/12/01
*/

/*
 * 包括以下模块：
 *
 * 一、常用头文件
 * 二、基本数据类型
 * 三、数学函数和宏
 * 四、内存管理
 * 五、字符串操作
 * 六、文件系统
 * 七、时间和定时器
 * 八、字符编码
 * 九、多进程
 * 十、线程同步与互斥
 * 十一、多线程
 * 十二、日志系统
 * 十三、内存检测(运行时)
 * 十四、调试相关
 * 十五、版本管理
 * 十六、其他
 */

#ifndef UTILS_CUTIL_H
#define UTILS_CUTIL_H

/* 选项配置文件 */
#include "utils_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化本库，在程序开头调用 */
void cutil_init();

/* 清理本库，在程序结束前调用 */
void cutil_exit();

/* 设置崩溃处理函数 */
typedef void (*crash_handler)(const char* dump_file);
void set_crash_handler(crash_handler handler);

/* 设置默认的崩溃处理函数 */
void set_default_crash_handler();

/* 设置软件名称，用于创建临时文件和目录等 */
/* 如果是非英文名称，编码应与USE_UTF8_STR一致，最长255个字符 */
void set_product_name(const char* product_name);

/* 获取设置的软件名称 */
const char* get_product_name();

/************************************************************************/
/*                    Common Headers 常用头文件                         */
/************************************************************************/

/* 系统头文件 */
#ifdef OS_POSIX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#else
#include <windows.h>
#endif

/* C库标准头文件 */
#include <stdio.h>
#include <stdlib.h>

/************************************************************************/
/*                         Data types 数据类型                           */
/************************************************************************/

#include <limits.h>					/* INT_MAX, LONG_MAX, LLONG_MAX... */

/* 无符号类型定义 */
typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;		/* Avoid using long/ulong */

/* 固定宽度数据类型定义 */
#if defined(COMPILER_MSVC) && _MSC_VER <= MSVC6
typedef __int64 int64_t;			/* 早期VC编译器不支持C99标准<stdint.h> */
typedef __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#define INT64_C(x) x##I64
#define UINT64_C(x) x##UI64
#elif defined(COMPILER_MSVC) && _MSC_VER <= MSVC10
MSVC_PUSH_DISABLE_WARNING(4005)		/* VC2010及之前版本同时包含<stdint.h>和<intsafe.h>会产生宏重定义警告 */
#include <intsafe.h>
#include <stdint.h>
MSVC_POP_WARNING()
#else
#include <stdint.h>					/* GCC或者VC2012以上版本可以直接包含<stdint.h> */
#endif

/* 64位整形常量定义宏 */
#ifndef INT64_C
#define INT64_C(x) x##LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x##ULL
#endif

#ifndef INT64_MAX
#define INT64_MAX INT64_C(0x7FFFFFFFFFFFFFFF)
#define INT64_MIN INT64_C(0x8000000000000000)
#endif
#ifndef UINT64_MAX
#define UINT64_MAX UINT64_C(0xFFFFFFFFFFFFFFFF)
#endif

#ifndef SIZE_T_MAX
#if __WORDSIZE == 64
#define SIZE_T_MAX UINT64_MAX
#else
#define SIZE_T_MAX UINT_MAX
#endif
#endif

#ifdef OS_WIN
#if __WORDSIZE == 64
typedef int64_t ssize_t;
#else
typedef int ssize_t;
#endif
#endif

#ifndef OS_WIN
#define INFINITE 0xFFFFFFFFU
#endif

/* 带符号类型最大值(gnulib) */
#define TYPE_MAXIMUM(t) ((t) (~ (~ (t) 0 << (sizeof (t) * CHAR_BIT - 1))))

/* 
 * 格式化输入输出
 * 用法如 printf ("value=%" PRId64 "\n", i64);
 */
#if defined(OS_POSIX) || defined(__MINGW32__)
#ifdef __cplusplus
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS		/* Use C99 in C++ */
#endif
#endif
#include <inttypes.h>				/* PRId64, PRIu64... */
#define PRIlf		"Lf"			/* long double */
#elif defined (OS_WIN)
#define PRIu64		"I64u"			/* uint64_t */
#define PRId64		"I64d"			/* int64_t */
#define PRIlf		"llf"			/* long double */
#endif
 
#ifndef PRIuS
#ifdef OS_WIN
#if __WORDSIZE == 64
#define PRIuS		PRIu64			/* size_t is 64bit */
#else
#define PRIuS		"u"				/* size_t is 32bit */
#endif
#else /* POSIX */
#define PRIuS       "zu"
#endif
#endif

/* 附：各种数据类型在Linux/Windows、32/64位编译器下的长度及printf输出格式：
________________________________________________________________________
|类型		|	Linux32	|	Linux64	|	Win32	|	Win64	|附注		|
|_______________________________________________________________________|
|char		|	1(c)	|	1(c)	|	1(c)	|	1(c)	|			|
|short		|	2(hd)	|	2(hd)	|	2(hd)	|	2(hd)	|			|
|int		|	4(d)	|	4(d)	|	4(d)	|	4(d)	|			|
|long		|	4(ld)	|	8(ld)	|	4(ld)	|	4(ld)	|			|
|long long	|	8(lld)	|	8(ld)	|	8(lld)	|	8(ld)	| C99		|
|float		|	4(f)	|	4(f)	|	4(f)	|	4(f)	|			|
|double		|	8(lf)	|	8(lf)	|	8(lf)	|	8(lf)	|			|
|long double|	12(Lf)	|	16(Lf)	|	8(llf)	|	8(llf)	| C99		|
|			|			|			|			|			|			|
|void*		|	4(p)	|	8(p)	|	4(p)	|	8(p)	| 指针		|
|size_t		|	4		|	8		|	4		|	8		|			|
|wchar_t	|	4		|	4		|	2		|	2		| 宽字符	|
|_______________________________________________________________________|
*/

/************************************************************************/
/*                            Math 数学                                  */
/************************************************************************/

#include "deps/min-max.h"		/* xmin, xmax */

/* 数组元素个数 */
#define countof(arr) (sizeof(arr) / sizeof((arr)[0]))

/* 取高/低32位 */
#define UPPER_32_BITS(n) ((uint32_t)(((n) >> 16) >> 16))
#define LOWER_32_BITS(n) ((uint32_t)(n))

/* 特殊字符 */
#define ISDIGIT(c) (c >= '0' && c <= '9')
#define ISXDIGIT(c) (((c >= '0' && c <= '9') || ((c & ~0x20) >= 'A' && (c & ~0x20) <= 'F')) ? 1 : 0)

/* 转换一个十六进制字符(0~9,A~F)到相应数值(0~15) */
#define XDIGIT_TO_NUM(h) ((h) < 'A' ? (h) - '0' : toupper (h) - 'A' + 10)
#define X2DIGITS_TO_NUM(h1, h2) ((XDIGIT_TO_NUM (h1) << 4) + XDIGIT_TO_NUM (h2))

/* 转换一个[0, 16)范围内的数值到其十六进制的ASCII码表示字符 */
#define XNUM_TO_DIGIT(x) ("0123456789ABCDEF"[x] + 0)
#define XNUM_TO_digit(x) ("0123456789abcdef"[x] + 0)

/************************************************************************/
/*                         Memory 内存管理                               */
/************************************************************************/

#if defined(COMPILER_MSVC) && _MSC_VER <= MSVC6
#define __FUNCTION__ ""
#endif

/* 内存调试函数 */
#ifdef DBG_MEM
extern int g_xalloc_count;

void free_d(void *, const char*, const char*, int);
void *malloc_d(size_t, const char*, const char*, int);
void *realloc_d(void *, size_t, const char*, const char*, int);
void *calloc_d(size_t, size_t, const char*, const char*, int);
char *strdup_d(const char*, size_t, const char*, const char*, int);

#define xmalloc(s) malloc_d(s, path_find_file_name(__FILE__), __FUNCTION__, __LINE__)
#define xrealloc(p,s) realloc_d(p, s, path_find_file_name(__FILE__), __FUNCTION__, __LINE__)
#define xcalloc(c,s) calloc_d(c, s, path_find_file_name(__FILE__), __FUNCTION__, __LINE__)
#define xstrdup(s) strdup_d(s, -1, path_find_file_name(__FILE__), __FUNCTION__, __LINE__)
#define xstrndup(s,n)strdup_d (s, n, path_find_file_name(__FILE__), __FUNCTION__, __LINE__)
#define xfree(p) free_d(p, path_find_file_name(__FILE__), __FUNCTION__, __LINE__)
#else
#define xmalloc malloc
#define xrealloc realloc
#define xcalloc calloc
#define xstrdup strdup
#define xstrndup strndup
#define xfree free
#endif

/* 按类型分配内存 */
#define XMALLOC(type) ((type *)xmalloc(sizeof (type)))
#define XCALLOC(type) ((type *)xcalloc(1, sizeof (type)))
#define XALLOCA(type) ((type *)alloca(sizeof(type)))

#define XNMALLOC(n, type) ((type *)xmalloc((n) * sizeof (type)))
#define XNCALLOC(n, type) ((type *)xcalloc((n), sizeof (type)))
#define XNALLOCA(n, type) ((type *)alloca((n) * sizeof (type)))

/************************************************************************/
/*                         String 字符串操作                             */
/************************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#ifdef OS_WIN
#include <Shlwapi.h> /* StrDuaA */
#endif

#define xisdigit(c) ISDIGIT(c)
#define xisxdigit(c) ISXDIGIT(c)
#define xisascii(c) ((unsigned)c < 0x80)
#define xisalpha(c) ((unsigned)c < 0xFF && isalpha(c))
#define xisalnum(c) ((unsigned)c < 0xFF && isalnum(c))
#define xisupper(c) ((unsigned)c < 0xFF && isupper(c))
#define xislower(c) ((unsigned)c < 0xFF && islower(c))

#ifdef COMPILER_MSVC
#define alloca _alloca
#define strdupa StrDupA
#define vsnprintf xvsnprintf
#define snprintf xsnprintf
#define asprintf xasprintf
#endif

/* 字符串比较宏 */
#define STREQ(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)						/* 区分大小写比较两个字符串 */
#define STRIEQ(a, b) \
	(toupper(*(a)) == toupper(*(b)) && strcasecmp((a), (b)) == 0)				/* 不区分大小写比较两个字符串 */

/* 
 * 安全的字符串格式化输出
 * 在任何情况下，都会保证缓冲区以'\0'结尾
 * 在大多数UNIX/Linux及高版本VC下，该函数返回欲写入的字符串长度，出错返回负值
 * 对于某些实现（如VC6），在缓冲区不够大的情况下也会返回-1，详见 xasprintf 
 */
int xvsnprintf(char* buffer, size_t size, const char* format, va_list args) PRINTF_FORMAT(3, 0) WUR;
int xsnprintf(char* buffer, size_t size, const char* format, ...) PRINTF_FORMAT(3, 4) WUR;

/* 格式化输出到新分配的字符串 */
/* GLIBC在_GNU_SOURCE宏被定义的情况下已导出此函数 */
int xasprintf (char** out, const char* format, ...) PRINTF_FORMAT(2, 3) WUR;	/* 需外部释放 */

/* BSD风格的字符串拷贝和附加函数 */
/* 比 str[n]cpy 和 str[n]cat 更快更安全，比较见以下链接 */
/* http://www.gratisoft.us/todd/papers/strlcpy.html */
size_t xstrlcpy(char* dst, const char* src, size_t dst_size);					/* 安全的字符串拷贝函数 */
size_t xstrlcat(char* dst, const char *src, size_t dst_size);					/* 安全方便地字符串附加函数 */

/* 大小写转换 */
int lowercase_str (char *str);													/* 将字符串改为小写，若有字符发生改变返回1，否则返回0 */
int uppercase_str (char *str);													/* 将字符串改为大写，返回值同上 */

char *strdup_lower (const char *s);												/* 获取一个字符串的小写版拷贝 */
char *strdup_upper (const char *s);												/* 获取一个字符串的大写版拷贝 */

char *substrdup (const char *beg, const char *end);								/* 返回字符串的一个字串，以'\0'结尾，需外部释放 */

/* 字符串比较、查找操作 */
#ifndef __GLIBC__																/* GLIBC实现的但其他库未实现的实用函数 */

char* strndup(const char* s, size_t n);											/* 类似strdup但只最多只复制n个字节，最后附加'\0' */
char* strsep(char **stringp, const char* delim);								/* 分割字符串，更好的strtok */

#ifndef __MINGW32__
#if defined(COMPILER_MSVC)
#define strcasecmp stricmp														/* 使用MFC的stricmp系列函数 */
#define strncasecmp strnicmp
#else
int strcasecmp (const char *s1, const char *s2);								/* 不区分大小写比较两个字符串 */
int	strncasecmp (const char *s1, const char *s2, size_t n);						/* 不区分大小写比较两个字符串，最长比较n个字符 */
#endif
#endif

char* strnstr(const char *s, const char *find, size_t slen);					/* 在字符串s的前slen个字节中查找find */
char* strcasestr(const char *s, const char *find);								/* 不区分大小写在字符串s中查找字符串find */

void* memfrob(void *mem, size_t length);										/* 简单可逆地加密一块内存 */

#endif /* !__GLIBC__ */

char *strncasestr(const char *s, const char *find, size_t slen);				/* 不区分大小写在字符串s的前slen个字节中中查找字符串 */

size_t hash_pjw (const char *s, size_t tablesize);						        /* 字符串hash散列算法 */

/************************************************************************/
/*                         Filesystem 文件系统相关	                    */
/************************************************************************/

#ifdef OS_POSIX
#include <dirent.h>
#define MAX_PATH			1024	/* PATH_MAX is too large? */
#endif

#define MAX_PATH_WIN		260

/* 路径相关 */
#ifdef OS_WIN
#define PATH_SEP_CHAR		'\\'
#define PATH_SEP_WCHAR		L'\\'
#define PATH_SEP_STR		"\\"
#define PATH_SEP_WSTR		L"\\"
#define LINE_END_STR		"\r\n"
#define MIN_PATH			3		/* "C:\" */
#else
#define PATH_SEP_CHAR		'/'
#define PATH_SEP_WCHAR		L'/'
#define PATH_SEP_STR		"/"
#define PATH_SEP_WSTR		L"/"
#define LINE_END_STR		"\r"
#define MIN_PATH			1		/* "/" */
#endif

/* fopen读写大文件(>2GB) */
/* 32位Linux需在包含头文件之前定义_FILE_OFFSET_BIT=64 */
#ifdef OS_POSIX
#define fseek fseeko
#define ftell ftello
#define seek_off_t int64_t
#elif defined(COMPILER_MSVC) && _MSC_VER >= MSVC8
#define fseek _fseeki64
#define ftell _ftelli64
#define seek_off_t int64_t
#else
#define seek_off_t long
#pragma message("Can not fseek/ftell large file (>2GB)")
#endif

int			is_root_path(const char* path);											/* 判断path是否是根目录(/或X:\)  */
int			is_absolute_path(const char* path);										/* 判断path所指的路径是否是绝对路径 */

int			absolute_path(const char* relpath, char* buf, size_t len) WUR;			/* 获取文件/目录相对于当前工作目录的绝对路径 */
int			relative_path(const char* base_path, const char* full_path, char sep,
							char* buf, size_t len) WUR;								/* 获取full_path相对于base_path的相对路径 */

const char* path_find_file_name(const char* path);									/* 返回路径的文件名或最底层目录名，例见函数定义，下同 */
const char* path_find_extension(const char* path);									/* 返回文件的扩展名，目录返回NULL */
int			path_find_directory(const char* path, char* buf, size_t len) WUR;		/* 返回路径所指目录/文件的上级目录路径(路径名需为UTF-8编码) */

int			path_file_exists(const char* path);										/* 路径所指文件/目录是否存在 */
int			path_is_directory(const char* path);									/* 路径是否是有效目录 */
int			path_is_file(const char* path);											/* 路径是否是有效文件 */

int			unique_file(const char* path, char *buf, size_t len) WUR;				/* 获取当前可用的文件路径 */
int			unique_dir(const char* path, char *buf, size_t len) WUR;				/* 获取当前可用的目录路径 */

/* 路径合法化 */
#define PATH_UNIX		1
#define PATH_WINDOWS	2

#ifdef OS_WIN
#define PATH_PLATFORM PATH_WINDOWS
#else
#define PATH_PLATFORM PATH_UNIX
#endif

char*		path_escape(const char* path, int platform, int reserve_separator);		/* 将路径中的非法字符替换为%HH的形式，需外部释放 */
void		path_illegal_blankspace(char *path, int platform, int reserve_separator);	/* 将路径中的非法字符替换为空格符 */

/* 遍历目录 */
struct walk_dir_context;

struct walk_dir_context* walk_dir_begin(const char* dir) WUR;						/* 开始遍历 */
int			walk_dir_next(struct walk_dir_context *ctx) WUR;						/* 获取一项 */
void		walk_dir_end(struct walk_dir_context *ctx);								/* 结束遍历 */

const char*	walk_entry_name(struct walk_dir_context *ctx);							/* 文件/目录名 */
int			walk_entry_path(struct walk_dir_context *ctx, char* buf, size_t len) WUR;	/* 完整路径名 */

int			walk_entry_is_dot(struct walk_dir_context *ctx) WUR;					/* 该项是.(当前目录) */
int			walk_entry_is_dotdot(struct walk_dir_context *ctx) WUR;					/* 该项是..(上级目录) */
int			walk_entry_is_dir(struct walk_dir_context *ctx) WUR;					/* 该项类型是目录 */
int			walk_entry_is_file(struct walk_dir_context *ctx) WUR;					/* 该项类型是文件 */
int			walk_entry_is_regular(struct walk_dir_context *ctx) WUR;				/* 该项类型是普通文件 */

/* 目录操作 */
int			create_directory(const char *dir) WUR;									/* 创建单层目录（父目录必须存在）,成功返回1 */
int			create_directories(const char* dir) WUR;								/* 递归创建目录结构*/

int			delete_directory(const char* dir) WUR;									/* 删除一个目录（必须为空） */

typedef	int	(*delete_dir_cb)(const char* path, int type, int succ, void *arg);		/* 删除整个目录回调函数，type参数0、1分别表示文件和目录 */
int			delete_directories(const char* dir, delete_dir_cb func, void *arg) WUR;	/* 删除一个目录及包含的所有内容 */

int			is_empty_dir(const char* dir);											/* 判断目录是否是空目录 */
int			delete_empty_directories(const char* dir) WUR;							/* 删除一个目录下的所有不包含文件的目录（不删除参数目录本身） */

typedef int (*copy_dir_cb)(const char* src, const char *dst,						/* 拷贝整个目录回调函数*/																					
							int action, int succ, void *arg);						/* action参数0、1、2分别表示拷贝文件、拷贝目录和创建目录  */
int			copy_directories(const char* srcdir, const char* dstdir,				/* 将src目录拷贝到dst目录下（用法及注意见copy_directories） */
							copy_dir_cb func, void *arg) WUR;

typedef		int (*foreach_file_func_t)(const char* fpath, void *arg);				/* 文件处理函数 */
typedef		foreach_file_func_t foreach_dir_func_t;									/* 目录处理函数 */

int			foreach_file(const char* dir, foreach_file_func_t func,					/* 查找并处理目录下的每个文件 */
						int recursively, int regular_only, void *arg) WUR;
int			foreach_dir(const char* dir, foreach_dir_func_t func, void *arg) WUR;	/* 查找并处理目录下的每个目录(不递归) */

/* 文件操作 */
int			copy_file(const char* exists, const char* newfile, int overwritten) WUR;/* 复制文件 */
int			move_file(const char* exists, const char* newfile, int overwritten) WUR;/* 移动文件 */
int			delete_file(const char* path) WUR;										/* 删除文件 */

int64_t		file_size(const char* path);											/* 获取指定文件的长度，最大8EB... */
void		file_size_readable(int64_t size, char *outbuf, int outlen);				/* 获取可读性强的文件大小，如3.5GB，建议outlen>64 */

int64_t		get_file_disk_usage(const char *absolute_path);							/* 获取文件或目录实际占用磁盘空间的大小 */
int			get_file_block_size(const char *absolute_path);							/* 获取文件或目录所在文件系统磁盘区块的大小 */

#define		CALC_DISK_USAGE(real_size, block_size) \
			(((real_size) + (block_size)) & (~((block_size) - 1)))					/* 根据文件实际大小和分区块大小计算实际占用的磁盘空间 */

FILE*		xfopen(const char* file, const char* mode);								/* 打开文件 */
int			xfread(FILE* fp, int separator, size_t max_count,							
					char** outbuf, size_t* outlen) WUR;								/* 读取文件，需外部释放 */
void		xfclose(FILE *fp);														/* 关闭文件 */

/* 文件内容信息 */
struct file_mem {
	char*	content;																/* 文件内容 */
	size_t	length;																	/* 文件长度 */
};

/* 读入整个文件 */
struct file_mem* read_file_mem(const char* file, size_t max_size);					/* 将文件内容读取到内存中 */
void	free_file_mem(struct file_mem *fm);											/* 释放读取到内存中的文件 */

/* 写入整个文件 */
int		write_mem_file(const char *file, const void *data, size_t len) WUR;			/* 将内存中的数据写入文件 */
int		write_mem_file_safe(const char *file, const void *data, size_t len) WUR;	/* 同上，避免写入错误或不完整导致文件损坏 */
#define touch(file) write_mem_file(file, "", 0)

size_t	get_delim(char **lineptr, size_t *n, int delim, FILE *fp);					/* 从文件中读取以delim为分隔符的一块数据 */
size_t	get_line (char **lineptr, size_t *n, FILE *fp);								/* 从文件中读取一行 */

typedef int (*foreach_block_cb)(char *line, size_t len, size_t nblock, void *arg);	/* 成功处理此分隔符块返回1，若返回0将终止 */
typedef int (*foreach_line_cb)(char *line, size_t len, size_t nline, void *arg);	/* 成功处理此行返回1，若返回0将不再处理以后的行 */

int		foreach_block(FILE* fp, foreach_block_cb func, int delim, void *arg) WUR;	/* 读取文件每个分隔符块进行操作，若成功处理所有行返回1 */
int		foreach_line (FILE* fp, foreach_line_cb func, void *arg) WUR;				/* 读取文件的每一行进行操作，若成功处理所有行返回1 */

/* 磁盘空间 */
struct fs_usage {
	uint64_t fsu_total;																/* 总共字节数 */
	uint64_t fsu_free;																/* 空闲字节数（超级用户） */
	uint64_t fsu_avail;																/* 可用字节数（一般用户） */
	uint64_t fsu_files;																/* 文件节点数 */
	uint64_t fsu_ffree;																/* 可用节点数 */
};

int	get_fs_usage(const char* absolute_path, struct fs_usage *fsp) WUR;				/* 获取路径所在文件系统的使用状态 */

/* 常用目录/文件 */
const char* get_execute_path();														/* 获取进程路径名(即argv[0]) */
const char* get_execute_name();														/* 获取进程文件名 */
const char* get_execute_dir();														/* 获取进程文件所在目录 */

const char* get_module_path();														/* 获取当前模块路径 */

const char* get_current_dir();														/* 获取进程当前工作目录 */
int			set_current_dir(const char *dir) WUR;									/* 设置进程当前工作目录 */

const char* get_home_dir();															/* 获取当前登录用户的主目录 */
const char* get_app_data_dir();														/* 获取应用程序数据目录（配置文件，缓存等） */

/* 临时目录 / 文件 */
const char* get_temp_dir();															/* 获取应用程序的临时目录 */
int			get_temp_file_under(const char* dir, const char *prefix, 				/* 在指定目录下获取可用的临时文件 */
	char *outbuf, size_t outlen) WUR;
const char* get_temp_file(const char* prefix);										/* 获取默认临时目录下可用的临时文件（非线程安全） */


/************************************************************************/
/*                         Time & Timer 时间和定时器                     */
/************************************************************************/

#include <time.h>

/* 时间日期字符串 */
const char* time_str (time_t);														/* 返回一个 HH:MM:SS 样式的时间字符串，非线程安全 */
const char* date_str (time_t);														/* 返回一个 YY-MM-DD 样式的日期字符串，非线程安全 */
const char* datetime_str (time_t);													/* 返回一个 YY-MM-DD HH:MM:SS 样式的时间日期字符串，非线程安全 */
const char* timestamp_str(time_t);													/* 返回一个 YYMMDDHHMMSS 样式的时间戳字符串，非线程安全 */

#define TIME_UNIT_MAX	20

void time_unit_localize(const char* year, const char* month, const char* day,		/* 设置本地化的时间单位 */
						const char* hour, const char* minute, const char* second,
						int plural);

void time_span_readable(int64_t seconds, int accurate, char* outbuf, size_t outlen);/* 返回一个易读的时间差(如2小时51分13秒)，cutoff精确到的时间单位 */

#define TIME_BUFSIZE		64														/* 时间字符串缓冲区大小 */
#define TIME_SPAN_BUFSIZE	128														/* 时间差缓冲区大小 */

/* 睡眠 */
void	ssleep(int seconds);														/* 睡眠指定秒数 */
void	msleep(int milliseconds);													/* 睡眠指定毫秒数 */

/* 
 * 计时器 
 * 可用于时长计算及代码性能分析
 * 最高精确到微秒级别
 */
typedef struct{
#ifdef OS_WIN
	LARGE_INTEGER start;
	LARGE_INTEGER stop;
#else
	struct timeval start;
	struct timeval stop;
#endif
} time_meter_t;

void	time_meter_start(time_meter_t *timer);												/* 开始计时 */
void	time_meter_stop(time_meter_t *timer);												/* 结束计时 */

/* 从计时器开始到终止经历的时间 */
double	time_meter_elapsed_us(time_meter_t *meter);											/* 经历时间(微秒) */
double	time_meter_elapsed_ms(time_meter_t *meter);											/* 经历时间(毫秒) */
double	time_meter_elapsed_s(time_meter_t *meter);											/* 经历时间(秒) */

/* 从计时器开始到现在经历的时间 */
double time_meter_elapsed_us_till_now(time_meter_t *meter);
double time_meter_elapsed_ms_till_now(time_meter_t *meter);
double time_meter_elapsed_s_till_now(time_meter_t *meter);

/************************************************************************/
/*                         Charset 字符编码类函数							 */
/************************************************************************/

typedef unsigned int	UTF32;	/* at least 32 bits */
typedef unsigned short	UTF16;	/* at least 16 bits */
typedef unsigned char	UTF8;	/* typically 8 bits */
typedef unsigned char	UTF7;	/* typically 8 bits */
typedef unsigned char	Boolean; /* 0 or 1 */

#define MAX_CHARSET	10																/* 字符集名最大长度 */

/* GB2312兼容ASCII码，GBK兼容GB2312编码 */
int	is_ascii(const void* str, size_t size);											/* 字符串是纯ASCII编码 */
int is_utf8(const void* str, size_t size);											/* 字符串是否是UTF-8编码 */
int	is_gb2312(const void* str, size_t size);										/* 字符串是否是GB2312编码 */
int	is_gbk(const void* str, size_t size);											/* 字符串是否是GBK编码 */

int get_charset(const void* str, size_t size, 
				char *outbuf, size_t outlen, int can_ascii) WUR;					/* 获取字符串的字符集(ASCII,UTF-8,GB2312,GBK,GB18030) */
int get_file_charset(const char* file, char *outbuf, size_t outlen,					/* 探测文件的字符集 */
				double* probability, int max_line) WUR;

int	read_file_bom(FILE *fp, char *outbuf, size_t outlen) WUR;						/* 获取文件的BOM头 */
int write_file_bom(FILE *fp, const char* charset) WUR;								/* 写入字符集的BOM头 */

int utf8_len(const char* u8);														/* 获取UTF-8编码字符串的字符数 (非字节数)，返回－1表示无效UTF-8编码 */
int utf8_trim(const char* u8, char* outbuf, size_t max_byte);						/* 按照给定的最多字节数截取UTF-8字符串，返回新字符串的长度, outbuf长度必须大于max_byte */
int utf8_abbr(const char* u8, char* outbuf, size_t max_byte,						/* 简写UTF-8字符串到指定最大长度，保留最前和最后的字符，中间用...表示省略 */
			size_t last_reserved_words);											/* last_reserved_words指最后应保留多少个字符（注意不是字节） */

wchar_t* mbcs_to_wcs(const char* src);												/* 将多字节字符串转换为宽字符串，需外部释放 */
char*    wcs_to_mbcs(const wchar_t* src);											/* 将宽字符串转换为多字节字符串，需外部释放 */

UTF16* utf8_to_utf16(const UTF8* in, int strict);									/* 将UTF-8编码的字符串转换为UTF-16LE字符串，需外部释放 */
UTF8* utf16_to_utf8(const UTF16* in, int strict);									/* 将UTF-16LE字符串转换为UTF-8编码的字符串，需外部释放 */

UTF32* utf8_to_utf32(const UTF8* in,  int strict);									/* 将UTF-8编码的字符串转换为UTF-32LE字符串，需外部释放 */
UTF8* utf32_to_utf8(const UTF32* in,  int strict);									/* 将UTF-32LE字符串转换为UTF-8编码的字符串，需外部释放 */

UTF32* utf16_to_utf32(const UTF16* in, int strict);									/* 将UTF-16LE编码的字符串转换为UTF-32LE字符串，需外部释放 */
UTF16* utf32_to_utf16(const UTF32* in, int strict);									/* 将UTF-32LE字符串转换为UTF-16LE编码的字符串，需外部释放 */

UTF7* utf8_to_utf7(const UTF8* in);                                     			/*将UTF-8编码字符串转换为UTF-7字符串，需要外部释放，strict无意义*/
UTF8* utf7_to_utf8(const UTF7* in);             									/*将UTF-7编码字符串转换为UTF-8字符串，需要外部释放，strict无意义*/

#if defined(WCHAR_T_IS_UTF16)
#define utf8_to_wcs(in, s) (wchar_t*)utf8_to_utf16((UTF8*)in, s)					/* 将系统宽字符串(wchar_t*)转换为UTF-8编码的字符串，需外部释放 */
#define wcs_to_utf8(in, s) (char*)utf16_to_utf8((UTF16*)in, s)						/* 将UTF-8编码的字符串转换为系统宽字符串(wchar_t*)，需外部释放 */
#elif defined(WCHAR_T_IS_UTF32)
#define utf8_to_wcs(in, s) (wchar_t*)utf8_to_utf32((UTF8*)in, s)
#define wcs_to_utf8(in, s) (char*)utf32_to_utf8((UTF32*)in, s)
#endif

char* utf8_to_mbcs(const char* utf8, int strict);									/* 将UTF-8编码的字符串转换为系统多字节编码 */
char* mbcs_to_utf8(const char* mbcs, int strict);									/* 将系统多字节编码的字符串转换为UTF-8编码 */

ssize_t utf16_len(const UTF16* u16);													/* 获取UTF-16LE编码字符串的字符数 (非字节数) */
ssize_t utf32_len(const UTF32* u32);													/* 获取UTF-32LE编码字符串的字符数 (非字节数) */

#ifndef OS_ANDROID
const char* get_locale();															/* 获取系统默认字符集(如UTF-8，CP936) */
#endif

const char* get_language();															/* 获取系统默认的语言（如zh_CN，en_US）*/

#ifdef _LIBICONV_H

int convert_to_charset(const char* from, const char* to,							/* 编码转换函数，成功返回1，失败返回0 */
						 const char* inbuf, size_t inlen,
						 char **outbuf, size_t *outlen, int strict) WUR;										//宽松模式
#endif

/************************************************************************/
/*                         Process  多进程                              */
/************************************************************************/

#ifdef OS_WIN
typedef HANDLE process_t;
#define INVALID_PROCESS NULL
#else
typedef pid_t process_t;
#define INVALID_PROCESS 0
#endif

process_t process_create(const char* cmd_with_param, int show);						/* 创建新的进程 */
int	process_wait(process_t process, int milliseconds, int *exit_code) WUR;			/* 等待进程结束 */
int process_kill(process_t process, int exit_code, int wait) WUR;					/* 杀死进程 */

/*
 * show: 0 不显示，1 显示，2 最大化显示
 * wait_timeout: 0 不等待，INFINITE 无限等待，其他正数 等待的毫秒数
 * 参数中若带有双引号要使用另一对双引号来转义，如 \"\"\"quoted text\"\"\"" 代表 "quoted text"
 */
int shell_execute(const char* cmd, const char* param, int show, int wait_timeout);	/* 执行一个外部命令，打开一个文件，用浏览器打开一个网址等 */

char* popen_readall(const char* command);											/* 创建并等待读取子进程的所有输出，需手动释放 */

#ifdef OS_WIN
FILE* popen(const char *command, const char *mode);									/* 创建子进程和连接管道 */
#define pclose _pclose

#endif

/************************************************************************/
/*                       Mutex & Sync 线程同步与互斥                    */
/************************************************************************/

/* 原子变量 */
/* 所有操作均返回操作之后的值 */
typedef struct {
	volatile long counter;
} atomic_t;

long atomic_get(const atomic_t *v);
void atomic_set(atomic_t *v, long i);
void atomic_add(long i, atomic_t *v);
void atomic_sub(long i, atomic_t *v);
void atomic_inc(atomic_t *v);
void atomic_dec(atomic_t *v);

/* 自旋锁实现, 线程在获取不到锁时会忙等待 */
/* 因为不会被内核切换状态因而开销比较小 */
typedef struct {
	volatile long spin;
}spin_lock_t;

void spin_init(spin_lock_t* lock);													/* 初始化 */
void spin_lock(spin_lock_t* lock);													/* 加锁 */
void spin_unlock(spin_lock_t* lock);												/* 解锁 */
int  spin_trylock(spin_lock_t* lock);												/* 尝试加锁 */
int  spin_is_lock(spin_lock_t* lock);												/* 是否已加锁 */

/* 互斥量线程加锁，当获取不到锁时内核会调度其他线程运行 */
/* 同一个线程可以多次获取锁（递归锁），但必须释放相同的次数 */
/* VC6不支持mutex_trylock */
#ifdef OS_WIN
typedef CRITICAL_SECTION mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#endif

void mutex_init(mutex_t *lock);														/* 初始化互斥锁 */
void mutex_lock(mutex_t *lock);														/* 加锁互斥锁 */
void mutex_unlock(mutex_t *lock);													/* 解锁互斥锁 */
int  mutex_trylock(mutex_t *lock);													/* 尝试加锁，成功返回1 */
void mutex_destroy(mutex_t* lock);													/* 销毁互斥锁 */

#ifdef USE_READ_WRITE_LOCK

/* 读写锁实现 */
/* 如果有线程正在读，其他线程也可以读，但不能写 */
/* 如果有线程正在写，那么所有其他线程都不能读写 */
typedef struct {
#ifdef OS_POSIX
	pthread_rwlock_t rwlock;
#elif WINVER >= WINVISTA
	SRWLOCK	srwlock;
#else
	mutex_t write_lock;
	long reader_count;
	HANDLE no_readers;
#endif
} rwlock_t;

void rwlock_init(rwlock_t *rwlock);
void rwlock_rdlock(rwlock_t *rwlock);
void rwlock_wrlock(rwlock_t *rwlock);
void rwlock_rdunlock(rwlock_t *rwlock);
void rwlock_wrunlock(rwlock_t *rwlock);
void rwlock_destroy(rwlock_t *rwlock);

#endif

#ifdef USE_CONDITION_VARIABLE

/*
 * 条件变量, 用于线程间的同步
 * Windows XP、Server 2003及以下不支持
 */
#ifdef OS_POSIX
typedef pthread_cond_t cond_t;
#elif WINVER > WINVISTA
typedef CONDITION_VARIABLE cond_t;
#else
/* not supported */
#endif

void cond_init(cond_t *cond);														/* 初始化条件变量 */
void cond_wait(cond_t *cond, mutex_t *mutex);										/* 等待条件变量发生改变 */
int  cond_wait_time(cond_t *cond, mutex_t *mutex, int milliseconds);				/* 可指定最长等待时间,超时返回0 */
void cond_signal(cond_t *cond);														/* 唤醒等待队列中的一个线程 */
void cond_signal_all(cond_t *cond);													/* 唤醒所有等待线程 */
void cond_destroy(cond_t *cond);													/* 销毁条件变量 */

#endif

/************************************************************************/
/*                         Thread  多线程                               */
/************************************************************************/

/*
 * 可移植线程库的实现
 * 没有实现thread_terminate，因为设计良好的程序从来不会使用这个函数
 * 必须调用thread_join来等待线程结束，否则会造成线程资源无法释放
 */

#ifdef OS_WIN
typedef HANDLE thread_t;
typedef unsigned thread_ret_t;
#define THREAD_CALLTYPE __stdcall
#define INVALID_THREAD NULL
#else /* POSIX */
#include <pthread.h>
typedef pthread_t thread_t;
typedef void* thread_ret_t;
#define THREAD_CALLTYPE
#define INVALID_THREAD 0
#endif /* OS_WIN */

typedef thread_ret_t (THREAD_CALLTYPE *thread_proc_t)(void*);

int	 thread_create1(thread_t* t, thread_proc_t proc, void *arg, int stacksize) WUR;	/* 创建线程 */
void thread_exit(size_t exit_code);													/* 线程内退出 */
int  thread_join(thread_t t, thread_ret_t *exit_code);								/* 等待线程 */

/* 线程本地存储 */
/* Thread locale storage */
#ifdef OS_WIN
typedef DWORD thread_tls_t;
#else
typedef pthread_key_t thread_tls_t;
#endif

int	 thread_tls_create(thread_tls_t *tls) WUR;										/* 创建TLS键 */
int  thread_tls_set(thread_tls_t tls, void *data) WUR;								/* 设置TLS值 */								
int  thread_tls_get(thread_tls_t tls, void **data) WUR;								/* 获取TLS值 */
int  thread_tls_free(thread_tls_t tls) WUR;											/* 释放TLS */

/* 线程一次初始化 */
/* Thread once Initialize */
typedef struct{
	int inited;
	mutex_t lock;
}thread_once_t;

typedef int (*thread_once_func)();

void thread_once_init(thread_once_t* once);											/* 初始化结构体 */
int thread_once(thread_once_t* once, thread_once_func func);						/* 执行线程一次初始化 */

/************************************************************************/
/*							 Debugging  调试					            */
/************************************************************************/

/* 断言 */
#define ASSERTION(expr, name)	\
	if (!(expr)) {fatal_exit(name" {%s %s %d} %s", \
	path_find_file_name(__FILE__), __FUNCTION__, __LINE__, #expr);}

#undef ASSERT
#undef VERIFY

/* ASSERT 仅在调试模式下生效，而 VERITY 总是有效 */
#ifdef _DEBUG
#define ASSERT(expr) ASSERTION(expr, "[ASSERT]")
#else
#define ASSERT(expr) {;}
#endif

#define VERIFY(expr) ASSERTION(expr, "[VERIFY]")

/* 异常处理 */
#ifdef _DEBUG
#define NOT_HANDLED(type) fatal_exit("%s {%s %s %d} %s", \
		datetime_str(time(NULL)), path_find_file_name(__FILE__), __FUNCTION__, __LINE__, type)
#else
#define NOT_HANDLED(type) 
#endif

#define NOT_REACHED()	  NOT_HANDLED("not reached")
#define NOT_IMPLEMENTED() NOT_HANDLED("not implemented")

void	fatal_exit(const char *fmt, ...) PRINTF_FORMAT(1,2);					/* 软件致命退出 */

/* 错误信息 */
#ifdef OS_WIN
const char* get_last_error_win32();												/* 获取用WIN32 API失败后的错误信息(GetLastError()) */
#endif
const char* get_last_error_std();												/* 获取C标准库函数错误后的错误信息(errno) */
const char*	get_last_error();													/* linux下用std版，windows下用winapi版 */

/* 内存查看 */
char*		hexdump(const void *data, size_t len);								/* 以十六进制的形式查看缓冲区的内容，需手动释放 */

/************************************************************************/
/*                         	Logging 日志系统    							*/
/************************************************************************/

/* 
 * Linux风格的日志记录
 * 可同时打开多个日志文件，多线程安全
 * 2012/03/21 08:15:21 [ERROR] {main.c main 10} foo bar.
 */

#define	MAX_LOGS		100														/* 用户最多可打开日志数 */
#define LOG_INVALID		-1														/* 无效的日志描述符(初始化定义) */
#define DEBUG_LOG		0														/* 调试日志的ID */

/* 记录等级 */
enum LogSeverity{
		LOG_DEBUG = 0, 															/* 调试 */
		LOG_INFO,																/* 信息 */
		LOG_NOTICE,																/* 注意 */
		LOG_WARNING,															/* 警告 */
		LOG_ERROR,																/* 错误 */
		LOG_CRIT,																/* 紧急 */
		LOG_ALERT,																/* 危急 */
		LOG_FATAL,																/* 致命 */
};

void	log_init();																/* 初始化日志模块 */
void	log_severity(int severity);												/* 允许记录的最小严重等级 */

int		log_open(const char *path, int append, int binary);						/* 打开用户日志文件 */

void	log_printf0(int id, const char *fmt, ...) PRINTF_FORMAT(2,3);			/* 格式化输出到日志 */
void	log_printf(int id, int severity, const char *, ...) PRINTF_FORMAT(3,4);	/* 附加时间和登记信息 */

void	log_flush(int log_id);													/* 将日志缓冲区数据写入磁盘 */

void	log_close(int log_id);													/* 关闭用户日志文件 */
void	log_close_all();														/* 关闭所有打开的日志文件 */

void	set_debug_log_to_stderr(int enable);									/* 调试状态下调试日志信息输出到stderr */
int		is_debug_log_set_to_stderr();

/* 记录调试日志（记录文件名、函数名、行号） */
/* 注：不能对fmt参数使用gettext国际化 */
#if (defined _DEBUG) && (!defined COMPILER_MSVC || _MSC_VER > MSVC6)
#define log_dprintf(level, fmt, ...) log_printf(DEBUG_LOG, level, "{%s %s %d} " fmt, \
		path_find_file_name(__FILE__), __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define log_dprintf(level, fmt, ...) log_printf(DEBUG_LOG, level, fmt, ##__VA_ARGS__)
#endif

/************************************************************************/
/*              Memory Runtime Check 运行时内存检测                      */
/************************************************************************/

#ifdef DBG_MEM_RT

/* 内存操作类型 */
enum MEMRT_MTD_C{ 
	MEMRT_MALLOC = 0, MEMRT_CALLOC, MEMRT_REALLOC, MEMRT_STRDUP, MEMRT_FREE, MEMRT_C_END,
};

/* 返回错误值 */
enum MEMRT_ERR_C{
	MEMRTE_OK,
	MEMRTE_NOFREE,																/* 内存未被释放(泄露) */
	MEMRTE_UNALLOCFREE,															/* 内存未分配却被释放 */
	MEMRTE_MISRELIEF,															/* 没有配对使用malloc/free */
	MEMRTE_C_END,
};

/* 代表一次内存操作 */
struct MEMRT_OPERATE{
	int		method;																/* 操作类型 */
	void*	address;															/* 操作地址 */
	size_t	size;																/* 内存大小 */

	char*	file;																/* 文件名 */
	char*	func;																/* 函数名 */
	int		line;																/* 行  号 */

	char*	desc;																/* 如保存类名 */
	void	(*msgfunc)(int error, struct MEMRT_OPERATE *rtp);

	struct MEMRT_OPERATE *next;
};

typedef void (*memrt_msg_callback)(int error, struct MEMRT_OPERATE *rtp);

/* 外部接口 */
extern void memrt_init();
extern int  memrt_check();

/* 仅库使用 */
extern int  __memrt_alloc(int mtd, void* addr, size_t sz, const char* file, 
	const char* func, int line, const char* desc, memrt_msg_callback pmsgfun);
extern int  __memrt_release(int mtd, void* addr, size_t sz, const char* file, 
	const char* func, int line, const char*desc, memrt_msg_callback pmsgfun);
extern void __memrt_printf(const char *fmt, ...);
extern int g_xalloc_count;
#endif /* DBG_MEM_RT */

/************************************************************************/
/*						    Version 版本管理		                        */
/************************************************************************/

int	version_parse(const char* version, int *major, int *minor,			/* 分析一个以点分隔的版本字符串，字符串格式见实现 */
				int *revision, int *build, char *suffix, size_t plen) WUR;
int	version_compare(const char* v1, const char* v2) WUR;				/* 比较v1和v2两个版本号，v1比v2新返回1，相等返回0 */

/************************************************************************/
/*                         Others 其他		                            */
/************************************************************************/

/* 进程环境变量 */
int			set_env(const char* key, const char* val) WUR;						/* 设置环境变量 */
int			unset_env(const char* key) WUR;										/* 删除环境变量 */
const char*	get_env(const char* key);											/* 获取环境变量 */

/* 字符串=>数字 */
uint		atou(const char* str);												/* 转换为uint */
size_t		atos(const char* str);												/* 转换为size_t */
int64_t		atoi64(const char* str);											/* 转换为int64_t */
uint64_t	atou64(const char* str);											/* 转换为uint64_t */

/* 指针<=>字符串 */
int			ptr_to_str(void *ptr, char* buf, int len) WUR;						/* 将指针转换成十六进制的字符串，需手动释放 */
void*		str_to_ptr(const char *str);										/* 将代表指针的十六进制的字符串转换为指针值 */

int			number_of_processors();												/* 获取CPU核心数目 */

int			num_bits(int64_t number);											/* 计算一个整数的打印位数 */
int			get_random();														/* 获取基于时间的伪随机数 */

#ifdef __cplusplus
}
#endif

#endif /* UTILS_CUTIL_H */

/*
 * vim: et ts=4 sw=4
 */
