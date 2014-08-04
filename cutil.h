/* 
* @file: cutil.h
* @desc: ����ֲϵͳƽ̨���ݲ�
*		 ��Ҫ����ϵͳAPI����׼C�⣬��Чʵ��
* @auth: liwei (www.leewei.org)
* @mail: ari.feng@qq.com
* @date: 2012/04/22
* @mdfy: 2013/12/01
*/

/*
* ����ƽ̨��
* slackware 13.1 32-bit + GCC 4.4.4 
* ubuntu 12.04 64-bit + GCC 4.6.3 
* WinXP 32-bit + VC 6.0
* Win7 32/64-bit + VS2010
* Qt Mingw g++
* Qt arm-linux-androideabi-g++
* Mac OS X + Xcode / QtCreator
*/

/*
 * ע�⣺
 * һ����ʹ�ñ��⣬Ӧ�ڳ�����ʼ�ͽ���λ�÷ֱ����cutil_init()��cutil_exit()������
 * �������������ִ�гɹ������int�͵ķ���ֵ��ʾ����ô1��ʾ�ɹ���0��ʾʧ�ܣ�
 * ���������ļ�ϵͳ����غ�����
 *		��Windows���ַ�������Ĭ��ʹ��ϵͳ���ֽڱ���(��GBK)�������USE_UTF8_STR��������ٶ�ʹ��UTF-8���룻
 *		��Linux��Ӧ������ʹ��UTF-8���룬����ĳЩ����(��create_directories)������Ϊ�쳣��
 * �ġ�������Ҫ��������������Ȳ����ĺ���(��file_size_readable)��Ӧʹ���Ƽ���������С�ĺ꣨�� FILE_SIZE_BUFSIZE����
 * �塢���������Ҫ����һ������������ô���һ�����һ����ʾ��������С�Ĳ�����
 * ������debugģʽ�£����� NOT_REACHED(), NOT_IMPLEMENTED,log_[d]printf(LOG_FATAL,...)��ASSERT/VERIFYʧ�� ����ʹ��ǰ���̼�¼��ջ��Ϣ�������˳���
 * �ߡ�ʹ�ô�д��ASSERT��VERFITY�������ö��Զ�ջ��¼���ܣ���Сд��assert����ASSERT���ڵ���ģʽ����Ч����VERIFY��Releaseģʽ��Ҳ��Ч�����ҵ���ʧ�ܻ�ʹ���̱�����
 * �ˡ�����Ŀ¼·���ĺ�������֤��·���ָ�����β��
 */

/*
 * ��������ģ�飺
 *
 * һ������ͷ�ļ�
 * ����������������
 * ������ѧ�����ͺ�
 * �ġ��ڴ����
 * �塢�ַ�������
 * �����ļ�ϵͳ
 * �ߡ�ʱ��Ͷ�ʱ��
 * �ˡ��ַ�����
 * �š������
 * ʮ���߳�ͬ���뻥��
 * ʮһ�����߳�
 * ʮ������־ϵͳ
 * ʮ�����ڴ���(����ʱ)
 * ʮ�ġ��������
 * ʮ�塢�汾����
 * ʮ��������
 */

#ifndef UTILS_CUTIL_H
#define UTILS_CUTIL_H

/* ѡ�������ļ� */
#include "../utils_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ��ʼ�����⣬�ڳ���ͷ���� */
void cutil_init();

/* �����⣬�ڳ������ǰ���� */
void cutil_exit();

/* ���ñ��������� */
typedef void (*crash_handler)(const char* dump_file);
void set_crash_handler(crash_handler handler);

/* ����Ĭ�ϵı��������� */
void set_default_crash_handler();

/* ����������ƣ����ڴ�����ʱ�ļ���Ŀ¼�� */
/* ����Ƿ�Ӣ�����ƣ�����Ӧ��USE_UTF8_STRһ�£��255���ַ� */
void set_product_name(const char* product_name);

/* ��ȡ���õ�������� */
const char* get_product_name();

/************************************************************************/
/*                    Common Headers ����ͷ�ļ�                         */
/************************************************************************/

/* ϵͳͷ�ļ� */
#ifdef OS_POSIX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#else
#include <windows.h>
#endif

/* C���׼ͷ�ļ� */
#include <stdio.h>
#include <stdlib.h>

/************************************************************************/
/*                         Data types ��������                           */
/************************************************************************/

#include <limits.h>					/* INT_MAX, LONG_MAX, LLONG_MAX... */

/* �޷������Ͷ��� */
typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;		/* Avoid using long/ulong */

/* �̶�����������Ͷ��� */
#if defined(COMPILER_MSVC) && _MSC_VER <= MSVC6
typedef __int64 int64_t;			/* ����VC��������֧��C99��׼<stdint.h> */
typedef __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#define INT64_C(x) x##I64
#define UINT64_C(x) x##UI64
#elif defined(COMPILER_MSVC) && _MSC_VER <= MSVC10
MSVC_PUSH_DISABLE_WARNING(4005)		/* VC2010��֮ǰ�汾ͬʱ����<stdint.h>��<intsafe.h>��������ض��徯�� */
#include <intsafe.h>
#include <stdint.h>
MSVC_POP_WARNING()
#else
#include <stdint.h>					/* GCC����VC2012���ϰ汾����ֱ�Ӱ���<stdint.h> */
#endif

/* 64λ���γ�������� */
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

#if __WORDSIZE == 64
typedef int64_t ssize_t;
#else
typedef int ssize_t;
#endif

#ifndef OS_WIN
#define INFINITE 0xFFFFFFFFU
#endif

/* �������������ֵ(gnulib) */
#define TYPE_MAXIMUM(t) ((t) (~ (~ (t) 0 << (sizeof (t) * CHAR_BIT - 1))))

/* 
 * ��ʽ���������
 * �÷��� printf ("value=%" PRId64 "\n", i64);
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

/* ������������������Linux/Windows��32/64λ�������µĳ��ȼ�printf�����ʽ��
________________________________________________________________________
|����		|	Linux32	|	Linux64	|	Win32	|	Win64	|��ע		|
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
|void*		|	4(p)	|	8(p)	|	4(p)	|	8(p)	| ָ��		|
|size_t		|	4		|	8		|	4		|	8		|			|
|wchar_t	|	4		|	4		|	2		|	2		| ���ַ�	|
|_______________________________________________________________________|
*/

/************************************************************************/
/*                            Math ��ѧ                                  */
/************************************************************************/

#include "deps/min-max.h"		/* xmin, xmax */

/* ����Ԫ�ظ��� */
#define countof(arr) (sizeof(arr) / sizeof((arr)[0]))

/* ȡ��/��32λ */
#define UPPER_32_BITS(n) ((uint32_t)(((n) >> 16) >> 16))
#define LOWER_32_BITS(n) ((uint32_t)(n))

/* �����ַ� */
#define ISDIGIT(c) (c >= '0' && c <= '9')
#define ISXDIGIT(c) (((c >= '0' && c <= '9') || ((c & ~0x20) >= 'A' && (c & ~0x20) <= 'F')) ? 1 : 0)

/* ת��һ��ʮ�������ַ�(0~9,A~F)����Ӧ��ֵ(0~15) */
#define XDIGIT_TO_NUM(h) ((h) < 'A' ? (h) - '0' : toupper (h) - 'A' + 10)
#define X2DIGITS_TO_NUM(h1, h2) ((XDIGIT_TO_NUM (h1) << 4) + XDIGIT_TO_NUM (h2))

/* ת��һ��[0, 16)��Χ�ڵ���ֵ����ʮ�����Ƶ�ASCII���ʾ�ַ� */
#define XNUM_TO_DIGIT(x) ("0123456789ABCDEF"[x] + 0)
#define XNUM_TO_digit(x) ("0123456789abcdef"[x] + 0)

/************************************************************************/
/*                         Memory �ڴ����                               */
/************************************************************************/

#if defined(COMPILER_MSVC) && _MSC_VER <= MSVC6
#define __FUNCTION__ ""
#endif

/* �ڴ���Ժ��� */
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

/* �����ͷ����ڴ� */
#define XMALLOC(type) ((type *)xmalloc(sizeof (type)))
#define XCALLOC(type) ((type *)xcalloc(1, sizeof (type)))
#define XALLOCA(type) ((type *)alloca(sizeof(type)))

#define XNMALLOC(n, type) ((type *)xmalloc((n) * sizeof (type)))
#define XNCALLOC(n, type) ((type *)xcalloc((n), sizeof (type)))
#define XNALLOCA(n, type) ((type *)alloca((n) * sizeof (type)))

/************************************************************************/
/*                         String �ַ�������                             */
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

/* �ַ����ȽϺ� */
#define STREQ(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)						/* ���ִ�Сд�Ƚ������ַ��� */
#define STRIEQ(a, b) \
	(toupper(*(a)) == toupper(*(b)) && strcasecmp((a), (b)) == 0)				/* �����ִ�Сд�Ƚ������ַ��� */

/* 
 * ��ȫ���ַ�����ʽ�����
 * ���κ�����£����ᱣ֤��������'\0'��β
 * �ڴ����UNIX/Linux���߰汾VC�£��ú���������д����ַ������ȣ������ظ�ֵ
 * ����ĳЩʵ�֣���VC6�����ڻ�����������������Ҳ�᷵��-1����� xasprintf 
 */
int xvsnprintf(char* buffer, size_t size, const char* format, va_list args) PRINTF_FORMAT(3, 0) WUR;
int xsnprintf(char* buffer, size_t size, const char* format, ...) PRINTF_FORMAT(3, 4) WUR;

/* ��ʽ��������·�����ַ��� */
/* GLIBC��_GNU_SOURCE�걻�����������ѵ����˺��� */
int xasprintf (char** out, const char* format, ...) PRINTF_FORMAT(2, 3) WUR;	/* ���ⲿ�ͷ� */

/* BSD�����ַ��������͸��Ӻ��� */
/* �� str[n]cpy �� str[n]cat �������ȫ���Ƚϼ��������� */
/* http://www.gratisoft.us/todd/papers/strlcpy.html */
size_t xstrlcpy(char* dst, const char* src, size_t dst_size);					/* ��ȫ���ַ����������� */
size_t xstrlcat(char* dst, const char *src, size_t dst_size);					/* ��ȫ������ַ������Ӻ��� */

/* ��Сдת�� */
int lowercase_str (char *str);													/* ���ַ�����ΪСд�������ַ������ı䷵��1�����򷵻�0 */
int uppercase_str (char *str);													/* ���ַ�����Ϊ��д������ֵͬ�� */

char *strdup_lower (const char *s);												/* ��ȡһ���ַ�����Сд�濽�� */
char *strdup_upper (const char *s);												/* ��ȡһ���ַ����Ĵ�д�濽�� */

char *substrdup (const char *beg, const char *end);								/* �����ַ�����һ���ִ�����'\0'��β�����ⲿ�ͷ� */

/* �ַ����Ƚϡ����Ҳ��� */
#ifndef __GLIBC__																/* GLIBCʵ�ֵĵ�������δʵ�ֵ�ʵ�ú��� */

char* strndup(const char* s, size_t n);											/* ����strdup��ֻ���ֻ����n���ֽڣ���󸽼�'\0' */
char* strsep(char **stringp, const char* delim);								/* �ָ��ַ��������õ�strtok */

#ifndef __MINGW32__
#if defined(COMPILER_MSVC)
#define strcasecmp stricmp														/* ʹ��MFC��stricmpϵ�к��� */
#define strncasecmp strnicmp
#else
int strcasecmp (const char *s1, const char *s2);								/* �����ִ�Сд�Ƚ������ַ��� */
int	strncasecmp (const char *s1, const char *s2, size_t n);						/* �����ִ�Сд�Ƚ������ַ�������Ƚ�n���ַ� */
#endif
#endif

char* strnstr(const char *s, const char *find, size_t slen);					/* ���ַ���s��ǰslen���ֽ��в���find */
char* strcasestr(const char *s, const char *find);								/* �����ִ�Сд���ַ���s�в����ַ���find */

void* memfrob(void *mem, size_t length);										/* �򵥿���ؼ���һ���ڴ� */

#endif /* !__GLIBC__ */

char *strncasestr(const char *s, const char *find, size_t slen);				/* �����ִ�Сд���ַ���s��ǰslen���ֽ����в����ַ��� */

size_t hash_pjw (const char *s, size_t tablesize);						        /* �ַ���hashɢ���㷨 */

/************************************************************************/
/*                         Filesystem �ļ�ϵͳ���	                    */
/************************************************************************/

#ifdef OS_POSIX
#include <dirent.h>
#define MAX_PATH			1024	/* PATH_MAX is too large? */
#endif

#define MAX_PATH_WIN		260

/* ·����� */
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

/* fopen��д���ļ�(>2GB) */
/* 32λLinux���ڰ���ͷ�ļ�֮ǰ����_FILE_OFFSET_BIT=64 */
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

int			is_root_path(const char* path);											/* �ж�path�Ƿ��Ǹ�Ŀ¼(/��X:\)  */
int			is_absolute_path(const char* path);										/* �ж�path��ָ��·���Ƿ��Ǿ���·�� */

int			absolute_path(const char* relpath, char* buf, size_t len) WUR;			/* ��ȡ�ļ�/Ŀ¼����ڵ�ǰ����Ŀ¼�ľ���·�� */
int			relative_path(const char* base_path, const char* full_path, char sep,
							char* buf, size_t len) WUR;								/* ��ȡfull_path�����base_path�����·�� */

const char* path_find_file_name(const char* path);									/* ����·�����ļ�������ײ�Ŀ¼���������������壬��ͬ */
const char* path_find_extension(const char* path);									/* �����ļ�����չ����Ŀ¼����NULL */
int			path_find_directory(const char* path, char* buf, size_t len) WUR;		/* ����·����ָĿ¼/�ļ����ϼ�Ŀ¼·��(·������ΪUTF-8����) */

int			path_file_exists(const char* path);										/* ·����ָ�ļ�/Ŀ¼�Ƿ���� */
int			path_is_directory(const char* path);									/* ·���Ƿ�����ЧĿ¼ */
int			path_is_file(const char* path);											/* ·���Ƿ�����Ч�ļ� */

int			unique_file(const char* path, char *buf, size_t len) WUR;				/* ��ȡ��ǰ���õ��ļ�·�� */
int			unique_dir(const char* path, char *buf, size_t len) WUR;				/* ��ȡ��ǰ���õ�Ŀ¼·�� */

/* ·���Ϸ��� */
#define PATH_UNIX		1
#define PATH_WINDOWS	2

#ifdef OS_WIN
#define PATH_PLATFORM PATH_WINDOWS
#else
#define PATH_PLATFORM PATH_UNIX
#endif

char*		path_escape(const char* path, int platform, int reserve_separator);		/* ��·���еķǷ��ַ��滻Ϊ%HH����ʽ�����ⲿ�ͷ� */
void		path_illegal_blankspace(char *path, int platform, int reserve_separator);	/* ��·���еķǷ��ַ��滻Ϊ�ո�� */

/* ����Ŀ¼ */
struct walk_dir_context;

struct walk_dir_context* walk_dir_begin(const char* dir) WUR;						/* ��ʼ���� */
int			walk_dir_next(struct walk_dir_context *ctx) WUR;						/* ��ȡһ�� */
void		walk_dir_end(struct walk_dir_context *ctx);								/* �������� */

const char*	walk_entry_name(struct walk_dir_context *ctx);							/* �ļ�/Ŀ¼�� */
int			walk_entry_path(struct walk_dir_context *ctx, char* buf, size_t len) WUR;	/* ����·���� */

int			walk_entry_is_dot(struct walk_dir_context *ctx) WUR;					/* ������.(��ǰĿ¼) */
int			walk_entry_is_dotdot(struct walk_dir_context *ctx) WUR;					/* ������..(�ϼ�Ŀ¼) */
int			walk_entry_is_dir(struct walk_dir_context *ctx) WUR;					/* ����������Ŀ¼ */
int			walk_entry_is_file(struct walk_dir_context *ctx) WUR;					/* �����������ļ� */
int			walk_entry_is_regular(struct walk_dir_context *ctx) WUR;				/* ������������ͨ�ļ� */

/* Ŀ¼���� */
int			create_directory(const char *dir) WUR;									/* ��������Ŀ¼����Ŀ¼������ڣ�,�ɹ�����1 */
int			create_directories(const char* dir) WUR;								/* �ݹ鴴��Ŀ¼�ṹ*/

int			delete_directory(const char* dir) WUR;									/* ɾ��һ��Ŀ¼������Ϊ�գ� */

typedef	int	(*delete_dir_cb)(const char* path, int type, int succ, void *arg);		/* ɾ������Ŀ¼�ص�������type����0��1�ֱ��ʾ�ļ���Ŀ¼ */
int			delete_directories(const char* dir, delete_dir_cb func, void *arg) WUR;	/* ɾ��һ��Ŀ¼���������������� */

int			is_empty_dir(const char* dir);											/* �ж�Ŀ¼�Ƿ��ǿ�Ŀ¼ */
int			delete_empty_directories(const char* dir) WUR;							/* ɾ��һ��Ŀ¼�µ����в������ļ���Ŀ¼����ɾ������Ŀ¼���� */

typedef int (*copy_dir_cb)(const char* src, const char *dst,						/* ��������Ŀ¼�ص�����*/																					
							int action, int succ, void *arg);						/* action����0��1��2�ֱ��ʾ�����ļ�������Ŀ¼�ʹ���Ŀ¼  */
int			copy_directories(const char* srcdir, const char* dstdir,				/* ��srcĿ¼������dstĿ¼�£��÷���ע���copy_directories�� */
							copy_dir_cb func, void *arg) WUR;

typedef		int (*foreach_file_func_t)(const char* fpath, void *arg);				/* �ļ������� */
typedef		foreach_file_func_t foreach_dir_func_t;									/* Ŀ¼������ */

int			foreach_file(const char* dir, foreach_file_func_t func,					/* ���Ҳ�����Ŀ¼�µ�ÿ���ļ� */
						int recursively, int regular_only, void *arg) WUR;
int			foreach_dir(const char* dir, foreach_dir_func_t func, void *arg) WUR;	/* ���Ҳ�����Ŀ¼�µ�ÿ��Ŀ¼(���ݹ�) */

/* �ļ����� */
int			copy_file(const char* exists, const char* newfile, int overwritten) WUR;/* �����ļ� */
int			move_file(const char* exists, const char* newfile, int overwritten) WUR;/* �ƶ��ļ� */

int			delete_file(const char* path) WUR;										/* ɾ���ļ� */
int			delete_file_empty_updir(const char* path, const char* top_dir) WUR;		/* ɾ���ļ������ϵĿ�Ŀ¼ֱ��ָ��Ŀ¼ */

int64_t		file_size(const char* path);											/* ��ȡָ���ļ��ĳ��ȣ����8EB... */
void		file_size_readable(int64_t size, char *outbuf, int outlen);				/* ��ȡ�ɶ���ǿ���ļ���С����3.5GB������outlen>64 */

int64_t		get_file_disk_usage(const char *absolute_path);							/* ��ȡ�ļ���Ŀ¼ʵ��ռ�ô��̿ռ�Ĵ�С */
int			get_file_block_size(const char *absolute_path);							/* ��ȡ�ļ���Ŀ¼�����ļ�ϵͳ��������Ĵ�С */
int64_t		compute_file_disk_usage(int64_t real_size, int block_size);				/* �����ļ�ʵ�ʴ�С�ͷ������С��ȡʵ��ռ�ô��̿ռ� */

FILE*		xfopen(const char* file, const char* mode);								/* ���ļ� */
int			xfread(FILE* fp, int separator, size_t max_count,							
					char** outbuf, size_t* outlen) WUR;								/* ��ȡ�ļ������ⲿ�ͷ� */
void		xfclose(FILE *fp);														/* �ر��ļ� */

/* �ļ�������Ϣ */
struct file_mem {
	char*	content;																/* �ļ����� */
	size_t	length;																	/* �ļ����� */
};

/* ���������ļ� */
struct file_mem* read_file_mem(const char* file, size_t max_size);					/* ���ļ����ݶ�ȡ���ڴ��� */
void	free_file_mem(struct file_mem *fm);											/* �ͷŶ�ȡ���ڴ��е��ļ� */

/* д�������ļ� */
int		write_mem_file(const char *file, const void *data, size_t len) WUR;			/* ���ڴ��е�����д���ļ� */
int		write_mem_file_safe(const char *file, const void *data, size_t len) WUR;	/* ͬ�ϣ�����д���������������ļ��� */
#define touch(file) write_mem_file(file, "", 0)

size_t	get_delim(char **lineptr, size_t *n, int delim, FILE *fp);					/* ���ļ��ж�ȡ��delimΪ�ָ�����һ������ */
size_t	get_line (char **lineptr, size_t *n, FILE *fp);								/* ���ļ��ж�ȡһ�� */

typedef int (*foreach_block_cb)(char *line, size_t len, size_t nblock, void *arg);	/* �ɹ�����˷ָ����鷵��1��������0����ֹ */
typedef int (*foreach_line_cb)(char *line, size_t len, size_t nline, void *arg);	/* �ɹ�������з���1��������0�����ٴ����Ժ���� */

int		foreach_block(FILE* fp, foreach_block_cb func, int delim, void *arg) WUR;	/* ��ȡ�ļ�ÿ���ָ�������в��������ɹ����������з���1 */
int		foreach_line (FILE* fp, foreach_line_cb func, void *arg) WUR;				/* ��ȡ�ļ���ÿһ�н��в��������ɹ����������з���1 */

/* ���̿ռ� */
struct fs_usage {
	uint64_t fsu_total;																/* �ܹ��ֽ��� */
	uint64_t fsu_free;																/* �����ֽ����������û��� */
	uint64_t fsu_avail;																/* �����ֽ�����һ���û��� */
	uint64_t fsu_files;																/* �ļ��ڵ��� */
	uint64_t fsu_ffree;																/* ���ýڵ��� */
};

int	get_fs_usage(const char* absolute_path, struct fs_usage *fsp) WUR;				/* ��ȡ·�������ļ�ϵͳ��ʹ��״̬ */

/* ����Ŀ¼/�ļ� */
const char* get_execute_path();														/* ��ȡ����·����(��argv[0]) */
const char* get_execute_name();														/* ��ȡ�����ļ��� */
const char* get_execute_dir();														/* ��ȡ�����ļ�����Ŀ¼ */

const char* get_module_path();														/* ��ȡ��ǰģ��·�� */

const char* get_current_dir();														/* ��ȡ���̵�ǰ����Ŀ¼ */
int			set_current_dir(const char *dir) WUR;									/* ���ý��̵�ǰ����Ŀ¼ */

const char* get_home_dir();															/* ��ȡ��ǰ��¼�û�����Ŀ¼ */
const char* get_app_data_dir();														/* ��ȡӦ�ó�������Ŀ¼�������ļ�������ȣ� */

/* ��ʱĿ¼ / �ļ� */
const char* get_temp_dir();															/* ��ȡӦ�ó������ʱĿ¼ */
int			get_temp_file_under(const char* dir, const char *prefix, 				/* ��ָ��Ŀ¼�»�ȡ���õ���ʱ�ļ� */
	char *outbuf, size_t outlen) WUR;
const char* get_temp_file(const char* prefix);										/* ��ȡĬ����ʱĿ¼�¿��õ���ʱ�ļ������̰߳�ȫ�� */


/************************************************************************/
/*                         Time & Timer ʱ��Ͷ�ʱ��                     */
/************************************************************************/

#include <time.h>

/* ʱ�������ַ��� */
const char* time_str (time_t);														/* ����һ�� HH:MM:SS ��ʽ��ʱ���ַ��������̰߳�ȫ */
const char* date_str (time_t);														/* ����һ�� YY-MM-DD ��ʽ�������ַ��������̰߳�ȫ */
const char* datetime_str (time_t);													/* ����һ�� YY-MM-DD HH:MM:SS ��ʽ��ʱ�������ַ��������̰߳�ȫ */
const char* timestamp_str(time_t);													/* ����һ�� YYMMDDHHMMSS ��ʽ��ʱ����ַ��������̰߳�ȫ */

#define TIME_UNIT_MAX	20

void time_unit_localize(const char* year, const char* month, const char* day,		/* ���ñ��ػ���ʱ�䵥λ */
						const char* hour, const char* minute, const char* second,
						int plural);

void time_span_readable(int64_t seconds, int accurate, char* outbuf, size_t outlen);/* ����һ���׶���ʱ���(��2Сʱ51��13��)��cutoff��ȷ����ʱ�䵥λ */

#define TIME_BUFSIZE		64														/* ʱ���ַ�����������С */
#define TIME_SPAN_BUFSIZE	128														/* ʱ��������С */

/* ˯�� */
void	ssleep(int seconds);														/* ˯��ָ������ */
void	msleep(int milliseconds);													/* ˯��ָ�������� */

/* 
 * ��ʱ�� 
 * ������ʱ�����㼰�������ܷ���
 * ��߾�ȷ��΢�뼶��
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

void	time_meter_start(time_meter_t *timer);												/* ��ʼ��ʱ */
void	time_meter_stop(time_meter_t *timer);												/* ������ʱ */

/* �Ӽ�ʱ����ʼ����ֹ������ʱ�� */
double	time_meter_elapsed_us(time_meter_t *meter);											/* ����ʱ��(΢��) */
double	time_meter_elapsed_ms(time_meter_t *meter);											/* ����ʱ��(����) */
double	time_meter_elapsed_s(time_meter_t *meter);											/* ����ʱ��(��) */

/* �Ӽ�ʱ����ʼ�����ھ�����ʱ�� */
double time_meter_elapsed_us_till_now(time_meter_t *meter);
double time_meter_elapsed_ms_till_now(time_meter_t *meter);
double time_meter_elapsed_s_till_now(time_meter_t *meter);

/************************************************************************/
/*                         Charset �ַ������ຯ��							 */
/************************************************************************/

typedef unsigned int	UTF32;	/* at least 32 bits */
typedef unsigned short	UTF16;	/* at least 16 bits */
typedef unsigned char	UTF8;	/* typically 8 bits */
typedef unsigned char	UTF7;	/* typically 8 bits */
typedef unsigned char	Boolean; /* 0 or 1 */

#define MAX_CHARSET	10																/* �ַ�������󳤶� */

/* GB2312����ASCII�룬GBK����GB2312���� */
int	is_ascii(const void* str, size_t size);											/* �ַ����Ǵ�ASCII���� */
int is_utf8(const void* str, size_t size);											/* �ַ����Ƿ���UTF-8���� */
int	is_gb2312(const void* str, size_t size);										/* �ַ����Ƿ���GB2312���� */
int	is_gbk(const void* str, size_t size);											/* �ַ����Ƿ���GBK���� */

int get_charset(const void* str, size_t size, 
				char *outbuf, size_t outlen, int can_ascii) WUR;					/* ��ȡ�ַ������ַ���(ASCII,UTF-8,GB2312,GBK,GB18030) */
int get_file_charset(const char* file, char *outbuf, size_t outlen,					/* ̽���ļ����ַ��� */
				double* probability, int max_line) WUR;

int	read_file_bom(FILE *fp, char *outbuf, size_t outlen) WUR;						/* ��ȡ�ļ���BOMͷ */
int write_file_bom(FILE *fp, const char* charset) WUR;								/* д���ַ�����BOMͷ */

int utf8_len(const char* u8);														/* ��ȡUTF-8�����ַ������ַ��� (���ֽ���)�����أ�1��ʾ��ЧUTF-8���� */
int utf8_trim(const char* u8, char* outbuf, size_t max_byte);						/* ���ո���������ֽ�����ȡUTF-8�ַ������������ַ����ĳ���, outbuf���ȱ������max_byte */
int utf8_abbr(const char* u8, char* outbuf, size_t max_byte,						/* ��дUTF-8�ַ�����ָ����󳤶ȣ�������ǰ�������ַ����м���...��ʾʡ�� */
			size_t last_reserved_words);											/* last_reserved_wordsָ���Ӧ�������ٸ��ַ���ע�ⲻ���ֽڣ� */

wchar_t* mbcs_to_wcs(const char* src);												/* �����ֽ��ַ���ת��Ϊ���ַ��������ⲿ�ͷ� */
char*    wcs_to_mbcs(const wchar_t* src);											/* �����ַ���ת��Ϊ���ֽ��ַ��������ⲿ�ͷ� */

UTF16* utf8_to_utf16(const UTF8* in, int strict);									/* ��UTF-8������ַ���ת��ΪUTF-16LE�ַ��������ⲿ�ͷ� */
UTF8* utf16_to_utf8(const UTF16* in, int strict);									/* ��UTF-16LE�ַ���ת��ΪUTF-8������ַ��������ⲿ�ͷ� */

UTF32* utf8_to_utf32(const UTF8* in,  int strict);									/* ��UTF-8������ַ���ת��ΪUTF-32LE�ַ��������ⲿ�ͷ� */
UTF8* utf32_to_utf8(const UTF32* in,  int strict);									/* ��UTF-32LE�ַ���ת��ΪUTF-8������ַ��������ⲿ�ͷ� */

UTF32* utf16_to_utf32(const UTF16* in, int strict);									/* ��UTF-16LE������ַ���ת��ΪUTF-32LE�ַ��������ⲿ�ͷ� */
UTF16* utf32_to_utf16(const UTF32* in, int strict);									/* ��UTF-32LE�ַ���ת��ΪUTF-16LE������ַ��������ⲿ�ͷ� */

UTF7* utf8_to_utf7(const UTF8* in);                                     			/* ��UTF-8�����ַ���ת��ΪUTF-7�ַ�������Ҫ�ⲿ�ͷţ�strict������ */
UTF8* utf7_to_utf8(const UTF7* in);             									/* ��UTF-7�����ַ���ת��ΪUTF-8�ַ�������Ҫ�ⲿ�ͷţ�strict������ */

#if defined(WCHAR_T_IS_UTF16)
#define utf8_to_wcs(in, s) (wchar_t*)utf8_to_utf16((UTF8*)in, s)					/* ��ϵͳ���ַ���(wchar_t*)ת��ΪUTF-8������ַ��������ⲿ�ͷ� */
#define wcs_to_utf8(in, s) (char*)utf16_to_utf8((UTF16*)in, s)						/* ��UTF-8������ַ���ת��Ϊϵͳ���ַ���(wchar_t*)�����ⲿ�ͷ� */
#elif defined(WCHAR_T_IS_UTF32)
#define utf8_to_wcs(in, s) (wchar_t*)utf8_to_utf32((UTF8*)in, s)
#define wcs_to_utf8(in, s) (char*)utf32_to_utf8((UTF32*)in, s)
#endif

char* utf8_to_mbcs(const char* utf8, int strict);									/* ��UTF-8������ַ���ת��Ϊϵͳ���ֽڱ��� */
char* mbcs_to_utf8(const char* mbcs, int strict);									/* ��ϵͳ���ֽڱ�����ַ���ת��ΪUTF-8���� */

ssize_t utf16_len(const UTF16* u16);													/* ��ȡUTF-16LE�����ַ������ַ��� (���ֽ���) */
ssize_t utf32_len(const UTF32* u32);													/* ��ȡUTF-32LE�����ַ������ַ��� (���ֽ���) */

#ifndef OS_ANDROID
const char* get_locale();															/* ��ȡϵͳĬ���ַ���(��UTF-8��CP936) */
#endif

const char* get_language();															/* ��ȡϵͳĬ�ϵ����ԣ���zh_CN��en_US) */

#ifdef _LIBICONV_H

int convert_to_charset(const char* from, const char* to,							/* ����ת���������ɹ�����1��ʧ�ܷ���0 */
						 const char* inbuf, size_t inlen,
						 char **outbuf, size_t *outlen, int strict) WUR;										//����ģʽ
#endif

/************************************************************************/
/*                         Process  �����                              */
/************************************************************************/

#ifdef OS_WIN
typedef HANDLE process_t;
#define INVALID_PROCESS NULL
#else
typedef pid_t process_t;
#define INVALID_PROCESS 0
#endif

process_t process_create(const char* cmd_with_param, int show);						/* �����µĽ��� */
int	process_wait(process_t process, int milliseconds, int *exit_code) WUR;			/* �ȴ����̽��� */
int process_kill(process_t process, int exit_code, int wait) WUR;					/* ɱ������ */

/*
 * show: 0 ����ʾ��1 ��ʾ��2 �����ʾ
 * wait_timeout: 0 ���ȴ���INFINITE ���޵ȴ����������� �ȴ��ĺ�����
 * ������������˫����Ҫʹ����һ��˫������ת�壬�� \"\"\"quoted text\"\"\"" ���� "quoted text"
 */
int shell_execute(const char* cmd, const char* param, int show, int wait_timeout);	/* ִ��һ���ⲿ�����һ���ļ������������һ����ַ�� */

char* popen_readall(const char* command);											/* �������ȴ���ȡ�ӽ��̵�������������ֶ��ͷ� */

#ifdef OS_WIN
FILE* popen(const char *command, const char *mode);									/* �����ӽ��̺����ӹܵ� */
#define pclose _pclose

#endif

/************************************************************************/
/*                       Mutex & Sync �߳�ͬ���뻥��                    */
/************************************************************************/

/* ԭ�ӱ��� */
/* ���в��������ز���֮���ֵ */
typedef struct {
	volatile long counter;
} atomic_t;

long atomic_get(const atomic_t *v);
void atomic_set(atomic_t *v, long i);
void atomic_add(long i, atomic_t *v);
void atomic_sub(long i, atomic_t *v);
void atomic_inc(atomic_t *v);
void atomic_dec(atomic_t *v);

/* ������ʵ��, �߳��ڻ�ȡ������ʱ��æ�ȴ� */
/* ��Ϊ���ᱻ�ں��л�״̬��������Ƚ�С */
typedef struct {
	volatile long spin;
}spin_lock_t;

void spin_init(spin_lock_t* lock);													/* ��ʼ�� */
void spin_lock(spin_lock_t* lock);													/* ���� */
void spin_unlock(spin_lock_t* lock);												/* ���� */
int  spin_trylock(spin_lock_t* lock);												/* ���Լ��� */
int  spin_is_lock(spin_lock_t* lock);												/* �Ƿ��Ѽ��� */

/* �������̼߳���������ȡ������ʱ�ں˻���������߳����� */
/* ͬһ���߳̿��Զ�λ�ȡ�����ݹ��������������ͷ���ͬ�Ĵ��� */
/* VC6��֧��mutex_trylock */
#ifdef OS_WIN
typedef CRITICAL_SECTION mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#endif

void mutex_init(mutex_t *lock);														/* ��ʼ�������� */
void mutex_lock(mutex_t *lock);														/* ���������� */
void mutex_unlock(mutex_t *lock);													/* ���������� */
int  mutex_trylock(mutex_t *lock);													/* ���Լ������ɹ�����1 */
void mutex_destroy(mutex_t* lock);													/* ���ٻ����� */

#ifdef USE_READ_WRITE_LOCK

/* ��д��ʵ�� */
/* ������߳����ڶ��������߳�Ҳ���Զ���������д */
/* ������߳�����д����ô���������̶߳����ܶ�д */
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
 * ��������, �����̼߳��ͬ��
 * Windows XP��Server 2003�����²�֧��
 */
#ifdef OS_POSIX
typedef pthread_cond_t cond_t;
#elif WINVER > WINVISTA
typedef CONDITION_VARIABLE cond_t;
#else
/* not supported */
#endif

void cond_init(cond_t *cond);														/* ��ʼ���������� */
void cond_wait(cond_t *cond, mutex_t *mutex);										/* �ȴ��������������ı� */
int  cond_wait_time(cond_t *cond, mutex_t *mutex, int milliseconds);				/* ��ָ����ȴ�ʱ��,��ʱ����0 */
void cond_signal(cond_t *cond);														/* ���ѵȴ������е�һ���߳� */
void cond_signal_all(cond_t *cond);													/* �������еȴ��߳� */
void cond_destroy(cond_t *cond);													/* ������������ */

#endif

/************************************************************************/
/*                         Thread  ���߳�                               */
/************************************************************************/

/*
 * ����ֲ�߳̿��ʵ��
 * û��ʵ��thread_terminate����Ϊ������õĳ����������ʹ���������
 * �������thread_join���ȴ��߳̽��������������߳���Դ�޷��ͷ�
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

int	 thread_create1(thread_t* t, thread_proc_t proc, void *arg, int stacksize) WUR;	/* �����߳� */
void thread_exit(size_t exit_code);													/* �߳����˳� */
int  thread_join(thread_t t, thread_ret_t *exit_code);								/* �ȴ��߳� */

/* �̱߳��ش洢 */
/* Thread locale storage */
#ifdef OS_WIN
typedef DWORD thread_tls_t;
#else
typedef pthread_key_t thread_tls_t;
#endif

int	 thread_tls_create(thread_tls_t *tls) WUR;										/* ����TLS�� */
int  thread_tls_set(thread_tls_t tls, void *data) WUR;								/* ����TLSֵ */								
int  thread_tls_get(thread_tls_t tls, void **data) WUR;								/* ��ȡTLSֵ */
int  thread_tls_free(thread_tls_t tls) WUR;											/* �ͷ�TLS */

/* �߳�һ�γ�ʼ�� */
/* Thread once Initialize */
typedef struct{
	int inited;
	mutex_t lock;
}thread_once_t;

typedef int (*thread_once_func)();

void thread_once_init(thread_once_t* once);											/* ��ʼ���ṹ�� */
int thread_once(thread_once_t* once, thread_once_func func);						/* ִ���߳�һ�γ�ʼ�� */

/************************************************************************/
/*							 Debugging  ����					            */
/************************************************************************/

/* ���� */
#define ASSERTION(expr, name)	\
	if (!(expr)) {fatal_exit(name" {%s %s %d} %s", \
	path_find_file_name(__FILE__), __FUNCTION__, __LINE__, #expr);}

#undef ASSERT
#undef VERIFY

/* ASSERT ���ڵ���ģʽ����Ч���� VERITY ������Ч */
#ifdef _DEBUG
#define ASSERT(expr) ASSERTION(expr, "[ASSERT]")
#else
#define ASSERT(expr) {;}
#endif

#define VERIFY(expr) ASSERTION(expr, "[VERIFY]")

/* �쳣���� */
#ifdef _DEBUG
#define NOT_HANDLED(type) fatal_exit("%s {%s %s %d} %s", \
		datetime_str(time(NULL)), path_find_file_name(__FILE__), __FUNCTION__, __LINE__, type)
#else
#define NOT_HANDLED(type) 
#endif

#define NOT_REACHED()	  NOT_HANDLED("not reached")
#define NOT_IMPLEMENTED() NOT_HANDLED("not implemented")

void	fatal_exit(const char *fmt, ...) PRINTF_FORMAT(1,2);					/* ��������˳� */

/* ������Ϣ */
#ifdef OS_WIN
const char* get_last_error_win32();												/* ��ȡ��WIN32 APIʧ�ܺ�Ĵ�����Ϣ(GetLastError()) */
#endif
const char* get_last_error_std();												/* ��ȡC��׼�⺯�������Ĵ�����Ϣ(errno) */
const char*	get_last_error();													/* linux����std�棬windows����winapi�� */

/* �ڴ�鿴 */
char*		hexdump(const void *data, size_t len);								/* ��ʮ�����Ƶ���ʽ�鿴�����������ݣ����ֶ��ͷ� */

/************************************************************************/
/*                         	Logging ��־ϵͳ    							*/
/************************************************************************/

/* 
 * Linux������־��¼
 * ��ͬʱ�򿪶����־�ļ������̰߳�ȫ
 * 2012/03/21 08:15:21 [ERROR] {main.c main 10} foo bar.
 */

#define	MAX_LOGS		100														/* �û����ɴ���־�� */
#define LOG_INVALID		-1														/* ��Ч����־������(��ʼ������) */
#define DEBUG_LOG		0														/* ������־��ID */

/* ��¼�ȼ� */
enum LogSeverity{
		LOG_DEBUG = 0, 															/* ���� */
		LOG_INFO,																/* ��Ϣ */
		LOG_NOTICE,																/* ע�� */
		LOG_WARNING,															/* ���� */
		LOG_ERROR,																/* ���� */
		LOG_CRIT,																/* ���� */
		LOG_ALERT,																/* Σ�� */
		LOG_FATAL,																/* ���� */
};

void	log_init();																/* ��ʼ����־ģ�� */
void	log_severity(int severity);												/* �����¼����С���صȼ� */

int		log_open(const char *path, int append, int binary);						/* ���û���־�ļ� */

void	log_printf0(int id, const char *fmt, ...) PRINTF_FORMAT(2,3);			/* ��ʽ���������־ */
void	log_printf(int id, int severity, const char *, ...) PRINTF_FORMAT(3,4);	/* ����ʱ��͵Ǽ���Ϣ */

void	log_flush(int log_id);													/* ����־����������д����� */

void	log_close(int log_id);													/* �ر��û���־�ļ� */
void	log_close_all();														/* �ر����д򿪵���־�ļ� */

void	set_debug_log_to_stderr(int enable);									/* ����״̬�µ�����־��Ϣ�����stderr */
int		is_debug_log_set_to_stderr();

/* ��¼������־����¼�ļ��������������кţ� */
/* ע�����ܶ�fmt����ʹ��gettext���ʻ� */
#define log_dprintf

/************************************************************************/
/*              Memory Runtime Check ����ʱ�ڴ���                      */
/************************************************************************/

#ifdef DBG_MEM_RT

/* �ڴ�������� */
enum MEMRT_MTD_C{ 
	MEMRT_MALLOC = 0, MEMRT_CALLOC, MEMRT_REALLOC, MEMRT_STRDUP, MEMRT_FREE, MEMRT_C_END,
};

/* ���ش���ֵ */
enum MEMRT_ERR_C{
	MEMRTE_OK,
	MEMRTE_NOFREE,																/* �ڴ�δ���ͷ�(й¶) */
	MEMRTE_UNALLOCFREE,															/* �ڴ�δ����ȴ���ͷ� */
	MEMRTE_MISRELIEF,															/* û�����ʹ��malloc/free */
	MEMRTE_C_END,
};

/* ����һ���ڴ���� */
struct MEMRT_OPERATE{
	int		method;																/* �������� */
	void*	address;															/* ������ַ */
	size_t	size;																/* �ڴ��С */

	char*	file;																/* �ļ��� */
	char*	func;																/* ������ */
	int		line;																/* ��  �� */

	char*	desc;																/* �籣������ */
	void	(*msgfunc)(int error, struct MEMRT_OPERATE *rtp);

	struct MEMRT_OPERATE *next;
};

typedef void (*memrt_msg_callback)(int error, struct MEMRT_OPERATE *rtp);

/* �ⲿ�ӿ� */
extern void memrt_init();
extern int  memrt_check();

/* ����ʹ�� */
extern int  __memrt_alloc(int mtd, void* addr, size_t sz, const char* file, 
	const char* func, int line, const char* desc, memrt_msg_callback pmsgfun);
extern int  __memrt_release(int mtd, void* addr, size_t sz, const char* file, 
	const char* func, int line, const char*desc, memrt_msg_callback pmsgfun);
extern void __memrt_printf(const char *fmt, ...);
extern int g_xalloc_count;
#endif /* DBG_MEM_RT */

/************************************************************************/
/*						    Version �汾����		                        */
/************************************************************************/

int	version_parse(const char* version, int *major, int *minor,			/* ����һ���Ե�ָ��İ汾�ַ������ַ�����ʽ��ʵ�� */
				int *revision, int *build, char *suffix, size_t plen) WUR;
int	version_compare(const char* v1, const char* v2) WUR;				/* �Ƚ�v1��v2�����汾�ţ�v1��v2�·���1����ȷ���0 */

/************************************************************************/
/*                         Others ����		                            */
/************************************************************************/

/* ���̻������� */
int			set_env(const char* key, const char* val) WUR;						/* ���û������� */
int			unset_env(const char* key) WUR;										/* ɾ���������� */
const char*	get_env(const char* key);											/* ��ȡ�������� */

/* �ַ���=>���� */
uint		atou(const char* str);												/* ת��Ϊuint */
size_t		atos(const char* str);												/* ת��Ϊsize_t */
int64_t		atoi64(const char* str);											/* ת��Ϊint64_t */
uint64_t	atou64(const char* str);											/* ת��Ϊuint64_t */

/* ָ��<=>�ַ��� */
int			ptr_to_str(void *ptr, char* buf, int len) WUR;						/* ��ָ��ת����ʮ�����Ƶ��ַ��������ֶ��ͷ� */
void*		str_to_ptr(const char *str);										/* ������ָ���ʮ�����Ƶ��ַ���ת��Ϊָ��ֵ */

int			number_of_processors();												/* ��ȡCPU������Ŀ */

int			num_bits(int64_t number);											/* ����һ�������Ĵ�ӡλ�� */
int			get_random();														/* ��ȡ����ʱ���α����� */

#ifdef __cplusplus
}
#endif

#endif /* UTILS_CUTIL_H */

/*
 * vim: et ts=4 sw=4
 */
