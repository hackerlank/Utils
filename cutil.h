/* 
* @file: cutil.h
* @desc: ����ֲϵͳƽ̨���ݲ�
*        ��Ҫ����ϵͳAPI����׼C�⣬��Чʵ��
* @auth: liwei (www.leewei.org)
* @mail: ari.feng@qq.com
* @date: 2012/04/22
* @mdfy: 2013/12/01
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

#include "deps/platform.h"
#include "deps/compiler.h"

#if !defined(_DEBUG) && !defined(NDEBUG)
#error "No build mode specified"
#elif defined(_DEBUG) && defined(NDEBUG)
#error "Only one build mode can be specified"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ��ʼ�����⣬�ڳ���ͷ���� */
void cutil_init();

/* �����⣬�ڳ������ǰ���� */
void cutil_exit();

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

#include <limits.h>     /* INT_MAX, LONG_MAX, LLONG_MAX... */

/* �޷������Ͷ��� */
typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;  /* Avoid using long/ulong */

/* �̶�����������Ͷ��� */
#if defined(COMPILER_MSVC) && _MSC_VER <= MSVC6
/* ����VC��������֧��C99��׼<stdint.h> */
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#define INT64_C(x) x##I64
#define UINT64_C(x) x##UI64
#elif defined(COMPILER_MSVC) && _MSC_VER <= MSVC10
/* VC2010��֮ǰ�汾ͬʱ����<stdint.h>��<intsafe.h>��������ض��徯�� */
MSVC_PUSH_DISABLE_WARNING(4005)
#include <intsafe.h>
#include <stdint.h>
MSVC_POP_WARNING()
#else
/* GCC����VC2012���ϰ汾����ֱ�Ӱ���<stdint.h> */
#include <stdint.h>
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

/* �������������ֵ(gnulib) */
#define TYPE_MAXIMUM(t) ((t) (~ (~ (t) 0 << (sizeof (t) * CHAR_BIT - 1))))

/*
 * ��ʽ���������
 * �÷��� printf ("value=%" PRId64 "\n", i64);
 */
#if defined(OS_POSIX) || defined(__MINGW32__)
#ifdef __cplusplus
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS  /* Use C99 in C++ */
#endif
#endif
#include <inttypes.h>         /* PRId64, PRIu64... */
#define PRIlf  "Lf"           /* long double */
#elif defined (OS_WIN)
#define PRIu64  "I64u"        /* uint64_t */
#define PRId64  "I64d"        /* int64_t */
#define PRIlf  "llf"          /* long double */
#endif

#ifndef PRIuS
#ifdef OS_WIN
#if __WORDSIZE == 64
#define PRIuS  PRIu64         /* size_t is 64bit */
#else
#define PRIuS  "u"            /* size_t is 32bit */
#endif
#else /* POSIX */
#define PRIuS       "zu"
#endif
#endif

/************************************************************************/
/*                            Math ��ѧ                                  */
/************************************************************************/

/* ���ֵ/��Сֵ */
#define xmin(x, y) ((x) > (y) ? (y) : (x))
#define xmax(x, y) ((x) > (y) ? (x) : (y))
#define xmin3(x, y, z) xmin(xmin(x, y), z)
#define xmax3(x, y, z) xmax(xmax(x, y), z)

/* ����Ԫ�ظ��� */
#define countof(arr) (sizeof(arr) / sizeof((arr)[0]))

/* ȡ��/��32λ */
#define UPPER_32_BITS(n) ((uint32_t)(((n) >> 16) >> 16))
#define LOWER_32_BITS(n) ((uint32_t)(n))

/* �Ƿ���ʮ�������ַ� */
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

#if (defined DBG_MEM_LOG || defined DBG_MEM_RT)
#define DBG_MEM
#else
#undef DBG_MEM
#endif

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

#define xisxdigit(c) ISXDIGIT(c)
#define xisascii(c) ((unsigned)c < 0x80)
#define xisdigit(c) ((unsigned)c < 0xFF && isdigit(c))
#define xisspace(c) ((unsigned)c < 0xFF && isspace(c))
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

#ifdef OS_WIN
#define strlcat xstrlcat
#define strlcpy xstrlcpy
#endif

/* �ַ����ȽϺ� */
/* ���ִ�Сд�Ƚ������ַ��� */
#define STREQ(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)

/* �����ִ�Сд�Ƚ������ַ��� */
#define STRIEQ(a, b) \
 (toupper(*(a)) == toupper(*(b)) && strcasecmp((a), (b)) == 0)

/*
 * ��ȫ���ַ�����ʽ�����
 * ���ظ�ʽ�����������ַ����ĳ��ȣ������ظ�ֵ����֤��������'\0'��β
 * ֻ�з���ֵ�ĳ��ȴ���0��С�����뻺�����ĳ��ȣ����������ĳɹ����
 * ע��ĳЩʵ���ڻ�����������������Ҳ�᷵��-1��ͬʱ��
 * VC�߰汾��errno��ΪERANGE����VC6����ô����ĳЩposixʵ�ֽ�errno��ΪEOVERFLOW
 * ���崦��ʽ�ɲμ� xasprintf ������ʵ��
 */
int xvsnprintf(char* buffer, size_t size, const char* format, va_list args) PRINTF_FMT(3, 0);
int xsnprintf(char* buffer, size_t size, const char* format, ...) PRINTF_FMT(3, 4);

/* ��ʽ��������·�����ַ���, ���ⲿ�ͷ� */
/* GLIBC��_GNU_SOURCE�걻�����������ѵ����˺��� */
int xasprintf (char** out, const char* format, ...) PRINTF_FMT(2, 3) WUR;

/* 
 * BSD�����ַ��������͸��Ӻ���
 * �������Ǳ�֤��������'\0'��β(ֻҪ dst_size > 0) 
 * ����ֵ����Ϊ����Ҫ������ַ����ĳ���(��������β��'\0')
 * �� str[n]cpy �� str[n]cat �������ȫ���Ƚϼ���������
 * http://www.gratisoft.us/todd/papers/strlcpy.html 
 */
size_t xstrlcpy(char* dst, const char* src, size_t dst_size);
size_t xstrlcat(char* dst, const char *src, size_t dst_size);

/* ��Сдת�� */
/* ���ַ�����Ϊ��/Сд�������ַ������ı䷵��1�����򷵻�0 */
int uppercase_str (char *str);
int lowercase_str (char *str);

/* ��ȡһ���ַ����Ĵ�/Сд�濽�� */
char *strdup_upper (const char *s);
char *strdup_lower (const char *s);

/* �����ַ�����һ���ִ�����'\0'��β�����ⲿ�ͷ� */
char *substrdup (const char *beg, const char *end);

/* �ַ����Ƚϡ����Ҳ��� */
/* GLIBCʵ�ֵĵ�������δʵ�ֵ�һЩ���� */
#ifndef __GLIBC__

/* ����strdup��ֻ���ֻ����n���ֽڣ���󸽼�'\0' */
char* strndup(const char* s, size_t n);

/* �ָ��ַ��������õ�strtok������ı�Դ�ַ��� */
char* strsep(char **stringp, const char* delim);

#ifndef __MINGW32__
#if defined(COMPILER_MSVC)
/* ʹ��MFC��stricmpϵ�к��� */
#define strcasecmp stricmp
#define strncasecmp strnicmp
#else
/* �����ִ�Сд�Ƚ������ַ��� */
int strcasecmp (const char *s1, const char *s2);

/* �����ִ�Сд�Ƚ������ַ�������Ƚ�n���ַ� */
int strncasecmp (const char *s1, const char *s2, size_t n);
#endif
#endif

/* ���ַ���s��ǰslen���ֽ��в���find */
char* strnstr(const char *s, const char *find, size_t slen);

/* ��������ڴ滺���� */
void* memrchr(const void* s, int c, size_t n);

/* �򵥿���ؼ���һ���ڴ� */
void* memfrob(void *mem, size_t length);

#endif /* !__GLIBC__ */

#if !defined(__GLIBC__) || !defined(_GNU_SOURCE)
/* �����ִ�Сд���ַ���s�в����ַ���find */
/* Linux��ֻ�ж�����_GNU_SOURCE��Żᵼ���˺��� */
char* strcasestr(const char *s, const char *find);
#endif

/* �����ִ�Сд���ַ���s��ǰslen���ֽ����в����ַ��� */
char *strncasestr(const char *s, const char *find, size_t slen);

/* �ַ���hashɢ���㷨 */
size_t hash_pjw (const char *s, size_t tablesize);

/************************************************************************/
/*                         Filesystem �ļ�ϵͳ���                       */
/************************************************************************/

#ifdef OS_POSIX
#include <dirent.h>
#define MAX_PATH   1024 /* PATH_MAX is too large? */
#endif

#define MAX_PATH_WIN  260

/* ·����� */
#ifdef OS_WIN
#define PATH_SEP_CHAR   '\\'
#define PATH_SEP_WCHAR  L'\\'
#define PATH_SEP_STR    "\\"
#define PATH_SEP_WSTR   L"\\"
#define MIN_PATH        3          /* "C:\" */
#define IS_PATH_SEP(c) ((c) == '\\' || (c) == '/')
#else
#define PATH_SEP_CHAR   '/'
#define PATH_SEP_WCHAR  L'/'
#define PATH_SEP_STR    "/"
#define PATH_SEP_WSTR   L"/"
#define MIN_PATH        1         /* "/" */
#define IS_PATH_SEP(c) ((c) == '/')
#endif

/* fopen��д���ļ�(>2GB) */
/* 32λLinux���ڰ���ͷ�ļ�֮ǰ����_FILE_OFFSET_BIT=64 */
#if defined(OS_POSIX) || defined(__MINGW32__)
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

/************************* ·������ *************************/

/* �ж�path�Ƿ��Ǹ�Ŀ¼(/��X:\)  */
int is_root_path(const char* path);

/* �ж�path��ָ��·���Ƿ��Ǿ���·�� */
int is_absolute_path(const char* path);

#ifdef OS_WIN
/* �Ƿ���Windows���繲��·��
 * �� \\servername\sharename\directory\filename */
int is_unc_path(const char* path);
#endif

/* ��ȡfull_path�����base_path�����·�� */
int relative_path(const char* base_path, const char* full_path,
    char* buf, size_t len) WUR;

/* 
 * ��ȡrel_path�����base_path�ľ���·�� 
 * ���rel_path������Ǿ���·����ֱ�ӷ���rel_path
 * ���base_pathΪ�գ�ʹ�õ�ǰ����Ŀ¼; base_path���������·��
 */
int absolute_path(const char* base_path, const char* rel_path,
    char* abs_path, size_t len) WUR;

/* ����·�����ļ�������ײ�Ŀ¼���������������壬��ͬ */
const char* path_find_file_name(const char* path);

/* �����ļ�����չ�� */
/* ���pathû����չ�����ؿ��ַ��������PATHΪNULL����NULL */
const char* path_find_extension(const char* path);

/* ����·����ָĿ¼/�ļ����ϼ�Ŀ¼·��(·������ΪUTF-8����) */
int path_find_directory(const char* path, char* buf, size_t len) WUR;

/* ·����ָ�ļ�/Ŀ¼�Ƿ���� */
int path_file_exists(const char* path);

/* ·���Ƿ�����ЧĿ¼/�ļ� */
int path_is_directory(const char* path);
int path_is_file(const char* path);

/* ����׺����ӵ��ļ�������չ��ǰ */
/* ����ļ�û����չ��������·���ָ�����β��ֱ�Ӹ��ӵ�·����� */
int path_insert_before_extension(const char* path, const char*suffix, char* buf, size_t len) WUR;

/* ��ȡ��ǰ���õ��ļ�/Ŀ¼·�� */
/* ���create_now����Ϊ�棬����������һ���յ��ļ�/Ŀ¼ */
/* path����Ϊ����·�� */
int unique_file(const char* path, char *buf, size_t len, int create_now) WUR;
int unique_dir(const char* path, char *buf, size_t len, int create_now) WUR;

/* ·���Ϸ��� */
#define PATH_UNIX  1
#define PATH_WINDOWS 2

#ifdef OS_WIN
#define PATH_PLATFORM PATH_WINDOWS
#else
#define PATH_PLATFORM PATH_UNIX
#endif

/* ��·���еķǷ��ַ��滻Ϊ%HH����ʽ�����ⲿ�ͷ� */
char* path_escape(const char* path, int platform, int reserve_separator);

/* ��·���еķǷ��ַ��滻Ϊ�ո�� */
void path_illegal_blankspace(char *path, int platform, int reserve_separator);

/* ��ָ���ַ����в���·���ָ���������strchr��strrchr */
char* strpsep(const char* path);
char* strrpsep(const char* path);

/************************* Ŀ¼���� *************************/

/* ����Ŀ¼ */
struct walk_dir_context;

/* ��ʼ����Ŀ¼����ȡ��һ�� */
struct walk_dir_context* walk_dir_begin(const char* dir) WUR;

/* ��ȡ��һ��Ŀ¼���� */
int walk_dir_next(struct walk_dir_context *ctx) WUR;

/* �������� */
void walk_dir_end(struct walk_dir_context *ctx);

/* ��ȡ�����������/����·�� */
const char* walk_entry_name(struct walk_dir_context *ctx);
int walk_entry_path(struct walk_dir_context *ctx, char* buf, size_t len) WUR;

int walk_entry_is_dot(struct walk_dir_context *ctx) WUR;      /* ������.(��ǰĿ¼) */
int walk_entry_is_dotdot(struct walk_dir_context *ctx) WUR;   /* ������..(�ϼ�Ŀ¼) */
int walk_entry_is_dir(struct walk_dir_context *ctx) WUR;      /* ����������Ŀ¼ */
int walk_entry_is_file(struct walk_dir_context *ctx) WUR;     /* �����������ļ� */
int walk_entry_is_regular(struct walk_dir_context *ctx) WUR;  /* ������������ͨ�ļ� */

/* ��������Ŀ¼����Ŀ¼������ڣ�,�ɹ�����1 */
int create_directory(const char *dir) WUR;

/*
 * �����༶Ŀ¼
 * ���·������"\"��β�������Ĳ��ִ���ΪĿ¼
 * ע��linux��dir����ΪUTF-8����
 */
int create_directories(const char* dir) WUR;

/* ɾ��һ����Ŀ¼ */
int delete_directory(const char* dir) WUR;

/* ɾ������Ŀ¼�ص�������type����0��1�ֱ��ʾ�ļ���Ŀ¼ */
typedef int (*delete_dir_cb)(const char* path, int type, int succ, void *arg);

/* ɾ��һ��Ŀ¼���������������� */
int  delete_directories(const char* dir, delete_dir_cb func, void *arg) WUR;

/* �ж�Ŀ¼�Ƿ��ǿ�Ŀ¼ */
int  is_empty_dir(const char* dir);

/* 
 * �ݹ�ɾ��Ŀ¼�µ����п�Ŀ¼
 * ���û���κ��ļ���ָ��Ŀ¼��Ҳ�ᱻɾ��
 * ����Ŀ¼��û���κ��ļ���Ŀ¼����Ҳ�ɹ�ɾ��ʱ����1�����򷵻�0
 */
int  delete_empty_directories(const char* dir) WUR;

/* ��������Ŀ¼�ص�����*/
/* action����0��1��2�ֱ��ʾ�����ļ�������Ŀ¼�ʹ���Ŀ¼  */
typedef int (*copy_dir_cb)(const char* src, const char *dst,
    int action, int succ, void *arg);

/* ��srcĿ¼������dstĿ¼�£��÷���ע���copy_directories�� */
int  copy_directories(const char* srcdir, const char* dstdir,
       copy_dir_cb func, void *arg) WUR;

/* �ļ������� */
typedef int (*foreach_file_func_t)(const char* fpath, void *arg);

/* Ŀ¼������ */
typedef foreach_file_func_t foreach_dir_func_t;

/* ���Ҳ�����Ŀ¼�µ�ÿ���ļ� */
int  foreach_file(const char* dir, foreach_file_func_t func,
      int recursively, int regular_only, void *arg) WUR;

/* ���Ҳ�����Ŀ¼�µ�ÿ��Ŀ¼(���ݹ�) */
int  foreach_dir(const char* dir, foreach_dir_func_t func, void *arg) WUR;

/************************* �ļ����� *************************/

/* �����ļ� */
int  copy_file(const char* exists, const char* newfile, int overwritten) WUR;

/* �ƶ��ļ� */
int  move_file(const char* exists, const char* newfile, int overwritten) WUR;

/* ɾ���ļ� */
int  delete_file(const char* path) WUR;

/* ��ȡָ���ļ��ĳ��ȣ����8EB... */
int64_t file_size(const char* path);

/* ��ȡ�ɶ���ǿ���ļ���С����3.5GB������outlen>64 */
void file_size_readable(int64_t size, char *outbuf, int outlen);

/* ��ȡ�ļ���Ŀ¼ʵ��ռ�ô��̿ռ�Ĵ�С */
int64_t get_file_disk_usage(const char *absolute_path);

/* ��ȡ�ļ���Ŀ¼�����ļ�ϵͳ��������Ĵ�С */
int  get_file_block_size(const char *absolute_path);

/* �����ļ�ʵ�ʴ�С�ͷ������С����ʵ��ռ�õĴ��̿ռ� */
#define CALC_DISK_USAGE(real_size, block_size) \
  (((real_size) + (block_size)) & (~((block_size) - 1)))

/* ���ļ��������Ѵ򿪵��ļ����� */
FILE* xfopen(const char* file, const char* mode);

/* �ر��ļ����ݼ��ļ����� */
void xfclose(FILE *fp);

/*
 * ���ļ��ж�ȡ����
 * �����ȡ�����У�����ָ���ָ������ﵽָ������ȡ�ֽ���������ļ�ĩβ�����
 * �������������separatorΪ-1���򲻱ȽϷָ�����
 * ���max_bytes����Ϊ0��-1��Ҳ��ʾ�����ƶ�ȡ���ֽ���
 * ���*lineptr��ΪNULL,�Ҷ������ݵĳ���С��*n,������д����ԭ�������У�
 * ���򽫶�̬����һ���ڴ����ڴ�Ŷ������ݣ����*lineptrΪNULL�������Ƕ�̬�����ڴ棻
 * ��ע�⡿������Ļ�������������󽫲��ᱻ�ͷ�(��ͬ��GLIBC��getline),
 * ����ֵ�����سɹ�������ֽ����������ָ���������������β��'\0'���ļ�Ϊ�ջ���󷵻�0
 */
size_t xfread(FILE *fp, int separator, size_t max_bytes,
 char **lineptr, size_t *n) WUR;

/*
 * ���ļ��ж���һ��
 * ������GLIBC��getline������C++Ҳʵ����ȫ�ֵ�getline
 * �����foreach_line�����ܺõ�ڹ�������ʹ�ô˺���
 */
size_t get_line(FILE *fp, char **lineptr, size_t *n) WUR;

/* �ɹ�����˷ָ����鷵��1��������0����ֹ */
typedef int (*foreach_block_cb)(char *line, size_t len, size_t nblock, void *arg);

/* �ɹ�������з���1��������0�����ٴ����Ժ���� */
typedef int (*foreach_line_cb)(char *line, size_t len, size_t nline, void *arg);

/* ��ȡ�ļ�ÿ���ָ�������в��������ɹ����������з���1 */
int  foreach_block(FILE* fp, foreach_block_cb func, int delim, void *arg) WUR;

/* ��ȡ�ļ���ÿһ�н��в��������ɹ����������з���1 */
int  foreach_line (FILE* fp, foreach_line_cb func, void *arg) WUR;

/* �ļ�������Ϣ */
struct file_mem {
 char* content;               /* �ļ����� */
 size_t length;               /* �ļ����� */
};

/* ���ļ������ڴ� */
/* ����ļ������ڻ����1GB��������NULL */
/* ���max_size�����㣬������ļ���СС��ָ��ֵʱ�ż��� */
/* ����ļ��ĳ���Ϊ0����contentΪNULL����length����0 */
/* ��������ڴ�ɹ�������������'\0'��β */
struct file_mem* read_file_mem(const char* file, size_t max_size);
void free_file_mem(struct file_mem *fm);

/* д�������ļ� */
/* ���ڴ��е�����д���ļ� */
int  write_mem_file(const char *file, const void *data, size_t len) WUR;

/* ͬ�ϣ�����д���������������ļ��� */
int  write_mem_file_safe(const char *file, const void *data, size_t len) WUR;

/* ����һ�����ļ� */
#define touch(file) write_mem_file(file, "", 0)

/************************* �ļ�ϵͳ *************************/

/* ���̿ռ� */
struct fs_usage {
 uint64_t fsu_total;                /* �ܹ��ֽ��� */
 uint64_t fsu_free;                 /* �����ֽ����������û��� */
 uint64_t fsu_avail;                /* �����ֽ�����һ���û��� */
 uint64_t fsu_files;                /* �ļ��ڵ��� */
 uint64_t fsu_ffree;                /* ���ýڵ��� */
};

/* ��ȡ·�������ļ�ϵͳ��ʹ��״̬ */
int get_fs_usage(const char* absolute_path, struct fs_usage *fsp) WUR;

/************************* ����·�� *************************/

/* ��ȡ����·����(��argv[0]) */
const char* get_execute_path();

/* ��ȡ�����ļ��� */
const char* get_execute_name();

/* ��ȡ�����ļ�����Ŀ¼ */
const char* get_execute_dir();

/* ��ȡ��ǰģ��·�� */
const char* get_module_path();

/* ��ȡ���̵�ǰ����Ŀ¼ */
const char* get_current_dir();

/* ���ý��̵�ǰ����Ŀ¼ */
int set_current_dir(const char *dir) WUR;

/* ��ȡ��ǰ��¼�û�����Ŀ¼ */
const char* get_home_dir();

/* ��ȡӦ�ó�������Ŀ¼�������ļ�������ȣ� */
/* �ɹ������Ѵ���Ŀ¼��·����ʧ�ܷ��ؿ��ַ��� */
const char* get_app_data_dir();

/************************* ��ʱĿ¼/�ļ� *************************/

/* ��ȡӦ�ó������ʱĿ¼ */
const char* get_temp_dir();

/* 
 * ��ָ��Ŀ¼�»�ȡ���õ���ʱ�ļ� 
 * �ɹ�����1, ���Ѵ����յ���ʱ�ļ�
 */
int get_temp_file_under(const char* dir, const char *prefix,
                        char *outbuf, size_t outlen) WUR;

/* ��ȡĬ����ʱĿ¼�¿��õ���ʱ�ļ������̰߳�ȫ�� */
/* �ɹ�������ʱ�ļ���·��������ʱ�ļ��Ѵ���Ϊ���ļ� */
const char* get_temp_file(const char* prefix);

/************************************************************************/
/*                         Time & Timer ʱ��Ͷ�ʱ��                     */
/************************************************************************/

#include <time.h>

/* ʱ�������ַ��� */
/* ���� HH:MM:SS ��ʽ��ʱ���ַ��������̰߳�ȫ */
const char* time_str (time_t);

/* ���� YY-MM-DD ��ʽ�������ַ��������̰߳�ȫ */
const char* date_str (time_t);

/* ���� YY-MM-DD HH:MM:SS ��ʽ��ʱ�������ַ��������̰߳�ȫ */
const char* datetime_str (time_t);

/* ���� YYMMDDHHMMSS ��ʽ��ʱ����ַ��������̰߳�ȫ */
const char* timestamp_str(time_t);

/*
 * ����ʱ�������ַ���
 * ֧�� ISO ��׼��ʽ���� 2014-09-24 12:59:30��2014/09/24 12:59:30
 * �Լ� __DATE__ __TIME__ ��, �� Sep 24 2014 12:59:30
 * �ɹ������� 1970/1/1 ������������ʧ�ܷ���0
 */
time_t parse_datetime(const char* datetime_str);

#define TIME_UNIT_MAX 20

/* ���ñ��ػ���ʱ�䵥λ */
void time_unit_localize(const char* year, const char* month, const char* day,
      const char* hour, const char* minute, const char* second,
      int plural);

/* ����һ���׶���ʱ���(��2Сʱ51��13��)��cutoff��ȷ����ʱ�䵥λ */
void time_span_readable(int64_t seconds, int accurate, char* outbuf, size_t outlen);

#define TIME_BUFSIZE  64       /* ʱ���ַ�����������С */
#define TIME_SPAN_BUFSIZE 128  /* ʱ��������С */

/* ˯��ָ������ */
void ssleep(int seconds);

/* ˯��ָ�������� */
void msleep(int milliseconds);

/************************* ��ʱ�� *************************/

/*
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

/* ��ʼ/������ʱ */
void time_meter_start(time_meter_t *timer);
void time_meter_stop(time_meter_t *timer);

/* �Ӽ�ʱ����ʼ����ֹ������ʱ�䣨΢�롢���롢�룩 */
double time_meter_elapsed_us(time_meter_t *meter);
double time_meter_elapsed_ms(time_meter_t *meter);
double time_meter_elapsed_s(time_meter_t *meter);

/* �Ӽ�ʱ����ʼ�����ھ�����ʱ�� */
double time_meter_elapsed_us_till_now(time_meter_t *meter);
double time_meter_elapsed_ms_till_now(time_meter_t *meter);
double time_meter_elapsed_s_till_now(time_meter_t *meter);

/************************************************************************/
/*                         Charset �ַ������ຯ��                        */
/************************************************************************/

/************************* ������ *************************/

#define MAX_CHARSET 10       /* �ַ�������󳤶� */

/* GB2312����ASCII�룬GBK����GB2312���� */
int is_ascii(const void* str, size_t size);          /* �ַ����Ǵ�ASCII���� */
int is_utf8(const void* str, size_t size);           /* �ַ����Ƿ���UTF-8���� */
int is_gb2312(const void* str, size_t size);         /* �ַ����Ƿ���GB2312���� */
int is_gbk(const void* str, size_t size);            /* �ַ����Ƿ���GBK���� */

/* ��ȡ�ַ������ַ���(ASCII,UTF-8,GB2312,GBK,GB18030) */
int get_charset(const void* str, size_t size,
    char *outbuf, size_t outlen, int can_ascii) WUR;

/* ̽���ļ����ַ��� */
int get_file_charset(const char* file, char *outbuf, size_t outlen,
    double* probability, int max_line) WUR;

/* ��ȡ�ļ���BOMͷ */
int read_file_bom(FILE *fp, char *outbuf, size_t outlen) WUR;

/* д���ַ�����BOMͷ */
int write_file_bom(FILE *fp, const char* charset) WUR;

/************************* ����ת�� *************************/

typedef unsigned int UTF32;      /* at least 32 bits */
typedef unsigned short UTF16;    /* at least 16 bits */
typedef unsigned char UTF8;      /* typically 8 bits */
typedef unsigned char UTF7;      /* typically 8 bits */
typedef unsigned char Boolean;   /* 0 or 1 */

/* ���ֽ��ַ��� <=> ���ַ���ת�������ⲿ�ͷ� */
wchar_t* mbcs_to_wcs(const char* src);
char*    wcs_to_mbcs(const wchar_t* src);

/* UTF-8�ַ��� <=> UTF-16LE�ַ���ת�������ⲿ�ͷ� */
UTF16* utf8_to_utf16(const UTF8* in, int strict);
UTF8*  utf16_to_utf8(const UTF16* in, int strict);

/* UTF-8�ַ��� <=> UTF-32LE�ַ���ת�������ⲿ�ͷ� */
UTF32* utf8_to_utf32(const UTF8* in,  int strict);
UTF8*  utf32_to_utf8(const UTF32* in,  int strict);

/* UTF-16LE�ַ��� <=> UTF-32LE�ַ���ת�������ⲿ�ͷ� */
UTF32* utf16_to_utf32(const UTF16* in, int strict);
UTF16* utf32_to_utf16(const UTF32* in, int strict);

/* UTF-8�ַ��� <=> UTF-7�ַ���ת������Ҫ�ⲿ�ͷ� */
UTF7* utf8_to_utf7(const UTF8* in);
UTF8* utf7_to_utf8(const UTF7* in);

/* ���ַ���(wchar_t*) <=> UTF-8�ַ���ת�������ⲿ�ͷ� */
#if defined(WCHAR_T_IS_UTF16)
#define utf8_to_wcs(in, s) (wchar_t*)utf8_to_utf16((UTF8*)in, s)
#define wcs_to_utf8(in, s) (char*)utf16_to_utf8((UTF16*)in, s)
#elif defined(WCHAR_T_IS_UTF32)
#define utf8_to_wcs(in, s) (wchar_t*)utf8_to_utf32((UTF8*)in, s)
#define wcs_to_utf8(in, s) (char*)utf32_to_utf8((UTF32*)in, s)
#endif

/* UTF-8�ַ��� <=> ϵͳ���ֽ��ַ���ת�������ⲿ�ͷ� */
char* utf8_to_mbcs(const char* utf8, int strict);
char* mbcs_to_utf8(const char* mbcs, int strict);

#ifdef _LIBICONV_H

/* ����ת���������ɹ�����1��ʧ�ܷ���0 */
int convert_to_charset(const char* from, const char* to,
        const char* inbuf, size_t inlen,
        char **outbuf, size_t *outlen, int strict) WUR;
#endif

/************************* ������� *************************/

/* ��ȡUTF-8�����ַ������ַ��� (���ֽ���) */
/* ��ָ�롢���ַ������UTF-8���뷵��0 */
size_t utf8_len(const char* u8);

/* ���ո���������ֽ�����ȡUTF-8�ַ��� */
/* outbuf���ȱ������max_byte, �ɹ��������ַ����ĳ���, ʧ�ܷ���0 */
size_t utf8_trim(const char* u8, char* outbuf, size_t max_byte);

/*
 * ��дUTF-8�ַ�����ָ����󳤶ȣ�������ǰ�������ַ����м���...��ʾʡ��
 *
 * ���� "c is a wonderful language" ��дΪ20���ֽڣ��������3���ַ�
 * ���Ϊ "c is a wonderf...age"
 * ע��1��"..."�ĳ��ȣ�3���ֽڣ������� max_byte ������
 * 2�����last_reserved_wordsΪ0����"..."�������������
 *    ���last_reserved_words���ڵ���max_byte-3����"..."����������ǰ
 * 3���ܶ��ֽڱ����Ӱ�죬���շ����ַ����ĳ��ȿ���С�� max_byte�������
 * 4��outbuf����Ϊ�������С������� max_byte (���'\0')
 * �ɹ�����1��ʧ�ܷ���0
 */
size_t utf8_abbr(const char* u8, char* outbuf, size_t max_byte,
    size_t last_reserved_words);

/* ��ȡUTF-16LE/32LE�����ַ������ַ��� (���ֽ���) */
/* ��ָ�롢���ַ�������0 */
size_t utf16_len(const UTF16* u16);
size_t utf32_len(const UTF32* u32);

/* ��ȡϵͳĬ���ַ���(��UTF-8��CP936) */
#ifndef OS_ANDROID
const char* get_locale();
#endif

/* ��ȡϵͳĬ�ϵ����ԣ���zh_CN��en_US��*/
const char* get_language();

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

/* �����µĽ��� */
process_t process_create(const char* cmd_with_param, int show);

/* �ȴ����̽��� */
int process_wait(process_t process, int milliseconds, int *exit_code) WUR;

/* ɱ������ */
int process_kill(process_t process, int exit_code, int wait) WUR;

/*
 * ִ��һ���ⲿ�����һ���ļ������������һ����ַ��
 * show: 0 ����ʾ��1 ��ʾ��2 �����ʾ
 * wait_timeout: 0 ���ȴ���INFINITE ���޵ȴ����������� �ȴ��ĺ�����
 * ������������˫����Ҫʹ����һ��˫������ת�壬�� \"\"\"quoted text\"\"\"" ���� "quoted text"
 */
int shell_execute(const char* cmd, const char* param, int show, int wait_timeout);

/* �������ȴ���ȡ�ӽ��̵�������������ֶ��ͷ� */
char* popen_readall(const char* command);

/* �����ӽ��̺����ӹܵ� */
#if defined(OS_WIN) && !defined(__MINGW32__)
FILE* popen(const char *command, const char *mode);
#define pclose _pclose
#endif

/************************************************************************/
/*                       Mutex & Sync �߳�ͬ���뻥��                    */
/************************************************************************/

/************************* ԭ�ӱ��� *************************/

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

/************************* ������ *************************/

/* ������ʵ��, �߳��ڻ�ȡ������ʱ��æ�ȴ� */
/* ��Ϊ���ᱻ�ں��л�״̬��������Ƚ�С */
typedef struct {
 volatile long spin;
}spin_lock_t;

void spin_init(spin_lock_t* lock);             /* ��ʼ�� */
void spin_lock(spin_lock_t* lock);             /* ���� */
void spin_unlock(spin_lock_t* lock);           /* ���� */
int  spin_trylock(spin_lock_t* lock);          /* ���Լ��� */
int  spin_is_lock(spin_lock_t* lock);          /* �Ƿ��Ѽ��� */

/************************* ������ *************************/

/* �������̼߳���������ȡ������ʱ�ں˻���������߳����� */
/* ͬһ���߳̿��Զ�λ�ȡ�����ݹ��������������ͷ���ͬ�Ĵ��� */
/* VC6��֧��mutex_trylock */
#ifdef OS_WIN
typedef CRITICAL_SECTION mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#endif

void mutex_init(mutex_t *lock);                /* ��ʼ�������� */
void mutex_lock(mutex_t *lock);                /* ���������� */
void mutex_unlock(mutex_t *lock);              /* ���������� */
int  mutex_trylock(mutex_t *lock);             /* ���Լ������ɹ�����1 */
void mutex_destroy(mutex_t* lock);             /* ���ٻ����� */

/************************* ��д�� *************************/

#ifdef USE_READ_WRITE_LOCK

/* ��д��ʵ�� */
/* ������߳����ڶ��������߳�Ҳ���Զ���������д */
/* ������߳�����д����ô���������̶߳����ܶ�д */
typedef struct {
#ifdef OS_POSIX
 pthread_rwlock_t rwlock;
#elif WINVER >= WINVISTA
 SRWLOCK srwlock;
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

/************************* �������� *************************/

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

void cond_init(cond_t *cond);                   /* ��ʼ���������� */
void cond_wait(cond_t *cond, mutex_t *mutex);   /* �ȴ��������������ı� */
int  cond_wait_time(cond_t *cond, mutex_t *mutex, int milliseconds);    /* ��ָ����ȴ�ʱ��,��ʱ����0 */
void cond_signal(cond_t *cond);                 /* ���ѵȴ������е�һ���߳� */
void cond_signal_all(cond_t *cond);             /* �������еȴ��߳� */
void cond_destroy(cond_t *cond);                /* ������������ */

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
typedef HANDLE uthread_t;
#define THREAD_CALLTYPE __stdcall
#define INVALID_THREAD NULL
#else /* POSIX */
#include <pthread.h>
typedef pthread_t uthread_t;
#define THREAD_CALLTYPE
#define INVALID_THREAD 0
#endif /* OS_WIN */

typedef int (THREAD_CALLTYPE *uthread_proc_t)(void*);

/* �����߳� */
int  uthread_create(uthread_t* t, uthread_proc_t proc, void *arg, int stacksize) WUR;

/* �߳����˳� */
void uthread_exit(size_t exit_code);

/* �ȴ��߳� */
int  uthread_join(uthread_t t, int *exit_code);

/************************* �̱߳��ش洢 *************************/

#ifdef OS_WIN
typedef DWORD thread_tls_t;
#else
typedef pthread_key_t thread_tls_t;
#endif

int  thread_tls_create(thread_tls_t *tls) WUR;          /* ����TLS�� */
int  thread_tls_set(thread_tls_t tls, void *data) WUR;  /* ����TLSֵ */
int  thread_tls_get(thread_tls_t tls, void **data) WUR; /* ��ȡTLSֵ */
int  thread_tls_free(thread_tls_t tls) WUR;             /* �ͷ�TLS */

/************************* �߳�һ�γ�ʼ�� *************************/

typedef struct {
  int inited;
  mutex_t lock;
} thread_once_t;

typedef int (*thread_once_func)();

/* ��ʼ���ṹ�� */
void thread_once_init(thread_once_t* once);

/* ִ���߳�һ�γ�ʼ�� */
int thread_once(thread_once_t* once, thread_once_func func);

/************************************************************************/
/*                            Debugging  ����                           */
/************************************************************************/

/* ���� */
#define ASSERTION(expr, name) \
 if (!(expr)) {backtrace_here(LOG_WARNING, name" %s in %s at %d: %s", \
 path_find_file_name(__FILE__), __FUNCTION__, __LINE__, #expr);}

#undef ASSERT
#undef VERIFY

/* ASSERT ���ڵ���ģʽ����Ч���� VERITY ������Ч */
#define VERIFY(expr) ASSERTION(expr, "[Verify]")

#ifdef _DEBUG
#define ASSERT(expr) ASSERTION(expr, "[Assert]")
#define NOT_REACHED()   ASSERTION(0, "[Not Reached]")
#define NOT_IMPLEMENTED() ASSERTION(0, "[Not Implemented]")
#else
#define ASSERT(expr)
#define NOT_REACHED()
#define NOT_IMPLEMENTED()
#endif

/* ���ö�ջ������ */
/* title�ĸ�ʽΪ"[Tag] " */
typedef void(*backtrace_handler)(int level, const char* title, const char* content);
void set_backtrace_handler(backtrace_handler handler);

/* ���ñ��������� */
typedef void(*crash_handler)(const char* dump_file, const char* log_file);
void set_crash_handler(crash_handler handler);

/* ����Ĭ�ϵı��������� */
void set_default_crash_handler();

/*
 * ��ӡ��ǰִ�ж�ջ����ʼ����
 * ���fatal����Ϊ�棬�����³����˳�
 */
void backtrace_here(int level, const char *fmt, ...) PRINTF_FMT(2, 3);

/*
 * ����ʹ��������Ի�ȡ���ö�ջ��dump�ļ�
 */
void crash_here();

/* ������Ϣ */
/* ��ȡ��WIN32 APIʧ�ܺ�Ĵ�����Ϣ(GetLastError()) */
#ifdef OS_WIN
const char* get_last_error_win32();
#endif

 /* ��ȡC��׼�⺯�������Ĵ�����Ϣ(errno) */
const char* get_last_error_std();

 /* linux����std�棬windows����winapi�� */
const char* get_last_error();

/* ��ʮ�����Ƶ���ʽ�鿴�����������ݣ����ֶ��ͷ� */
char*  hexdump(const void *data, size_t len);

/************************************************************************/
/*                          Logging ��־ϵͳ                            */
/************************************************************************/

/*
 * Linux������־��¼
 * ��ͬʱ�򿪶����־�ļ������̰߳�ȫ
 * 2012/03/21 08:15:21 [ERROR] {main.c main 10} foo bar.
 */

/* ��¼�ȼ� */
enum LogLevel {
  LOG_FATAL = 0,             /* ���� */
  LOG_ALERT,                 /* Σ�� */
  LOG_CRITICAL,              /* ���� */
  LOG_ERROR,                 /* ���� */
  LOG_WARNING,               /* ���� */
  LOG_NOTICE,                /* ע�� */
  LOG_INFO,                  /* ��Ϣ */
  LOG_DEBUG,                 /* ���� */
};

/* �����Զ�����־������ */
/* �Զ��崦��������1��ʾ������¼������0��ʾ����¼������־ */
typedef int (*log_handler)(const char* name, int level, const char* message);
void set_log_handler(log_handler handler);

/* ���ü�¼����͵ȼ� */
void set_log_level(int min_level);

/* ����Ĭ�ϵĵ�����־ */
void set_disable_debug_log();

/* ��������־��Ϣ�����stderr */
void set_log_to_console();
int  is_log_to_console();

/* ���û���־�ļ� */
int  log_open(const char* name, const char *path, int append, int binary);

/* ��ʽ���������־ */
void log_printf0(const char* name, const char *fmt, ...) PRINTF_FMT(2, 3);

/* ͬ�ϣ���ͬʱ����ʱ��͵ȼ���Ϣ */
void log_printf(const char* name, int level, const char *, ...) PRINTF_FMT(3, 4);

/* ����־����������д����� */
void log_flush(const char* name);

/* �ر��û���־�ļ� */
int log_close(const char* name);

/* ��¼������־����¼�ļ��������������кţ� */
/* ע�����ܶ�fmt����ʹ��gettext���ʻ� */
void log_dprintf(int severity, const char *, ...);

/************************************************************************/
/*              Memory Runtime Check ����ʱ�ڴ���                      */
/************************************************************************/

#ifdef DBG_MEM_RT

/* �ڴ�������� */
enum MEMRT_MTD_C {
 MEMRT_MALLOC = 0, MEMRT_CALLOC, MEMRT_REALLOC, MEMRT_STRDUP, MEMRT_FREE, MEMRT_C_END,
};

/* ���ش���ֵ */
enum MEMRT_ERR_C {
  MEMRTE_OK,
  MEMRTE_NOFREE,              /* �ڴ�δ���ͷ�(й¶) */
  MEMRTE_UNALLOCFREE,         /* �ڴ�δ����ȴ���ͷ� */
  MEMRTE_MISRELIEF,           /* û�����ʹ��malloc/free */
  MEMRTE_C_END,
};

typedef void(*memrt_msg_callback)(int error,
    const char* file, const char* func, int line,
    void* address, size_t size, int method);

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
/*                           Version �汾����                           */
/************************************************************************/

struct version_info {
    int major;         /* ���汾�� */
    int minor;         /* �ΰ汾�� */
    int revision;      /* �����汾�� */
    int build;         /* ����/�ύ������ */
    char suffix[32];   /* �汾��׺������Alpha/Beta�� */
};

 /* ����һ���Ե�ָ��İ汾�ַ������ַ�����ʽ��ʵ�� */
int version_parse(const char* version, struct version_info* parsed) WUR;

/* �Ƚ�v1��v2�����汾�ţ�v1��v2�·���1����ȷ���0 */
int version_compare(const char* v1, const char* v2) WUR;

/************************************************************************/
/*                         Others ����                                  */
/************************************************************************/

/* ���̻������� */
int set_env(const char* key, const char* val) WUR;   /* ���û������� */
int unset_env(const char* key) WUR;                  /* ɾ���������� */
const char* get_env(const char* key);                /* ��ȡ�������� */

/* �ַ���=>���� */
uint  atou(const char* str);                         /* ת��Ϊuint */
size_t  atos(const char* str);                       /* ת��Ϊsize_t */
int64_t  atoi64(const char* str);                    /* ת��Ϊint64_t */
uint64_t atou64(const char* str);                    /* ת��Ϊuint64_t */

int number_of_processors();                          /* ��ȡCPU������Ŀ */
int num_bits(int64_t number);                        /* ����һ�������Ĵ�ӡλ�� */
int get_random();                                    /* ��ȡ����ʱ���α����� */

#ifdef __cplusplus
}
#endif

#endif /* UTILS_CUTIL_H */

/*
 * vim: et ts=4 sw=4
 */
