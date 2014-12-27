#include "cutil.h"

#include <locale.h>
#include <wchar.h>
#include <errno.h>
#include <stdarg.h>

#ifdef OS_WIN

#include <io.h> /* copy_file */
#include <direct.h> /* _mkdir */
#include <sys/utime.h> /* utime */
#include <sys/stat.h> /* _stati64, _wstati64 */
#include <ShlObj.h> /* SHGetSpecialFolderPath */
#include <dbghelp.h> /* minidump */
#include <crtdbg.h> /* _CrtSetReportMode */
#include <ShellAPI.h> /* ShellExecuteEx */
#include <shlwapi.h> /* path, str.etc */
#include <process.h> /* _beginthreadex.etc */

#ifdef _MSC_VER
#define inline _inline

#pragma comment(lib, "dbghelp.lib") /* minidump */
#pragma comment(lib, "shlwapi.lib") /* path, str.etc */
#pragma comment(lib, "User32.lib") /* MessageBox.etc */
#pragma comment(lib, "kernel32.lib") /* SRW Lock.etc */
#pragma comment(lib, "shell32.lib")
#endif

/* MBCS <-> Unicode */
#define MBCS2UNI(m, w, s) MultiByteToWideChar(CP_ACP, 0, m, -1, w, s)
#define UNI2MBCS(w, m, s) WideCharToMultiByte(CP_ACP, 0, w, -1, m, s, NULL, NULL)

/* UTF-8 <-> Unicode */
#define UTF82UNI(m, w, s) MultiByteToWideChar(CP_UTF8, 0, m, -1, w, (int)s)
#define UNI2UTF8(w, m, s) WideCharToMultiByte(CP_UTF8, 0, w, -1, m, (int)s, NULL, NULL)

static LONG WINAPI CrashDumpHandler(EXCEPTION_POINTERS *pException);

#else /* OS_POSIX */

#include <sys/types.h>        /* mkdir */
#include <sys/stat.h>        /* stat */
#include <sys/time.h>        /* gettimeofday */
#include <sys/wait.h>        /* waitpid */
#include <sys/resource.h>    /* setrlimit */
#include <fcntl.h>            /* copy_file */
#include <utime.h>            /* utime */
#include <signal.h>            /* SIGCHLD.etc */
#include <stddef.h>         /* ptrdiff_t type */

#ifdef OS_MACOSX
#include <sys/mount.h>      /* statfs */
#include <libproc.h>        /* proc_pidpath */
#else
#include <sys/statfs.h>        /* statfs */
#endif

#ifndef OS_ANDROID
#include <execinfo.h>        /* backtrace */
#endif

void crash_signal_handler(int n, siginfo_t *siginfo, void *act);

#endif /* OS_WIN */

/* stat大文件(>2GB) */
#if defined(COMPILER_MSVC)
#define STAT_STRUCT struct _stati64
#define stat _stati64
#else
#define STAT_STRUCT struct stat
#endif

/* 软件名称 */
static char g_product_name[256];

/* 本库已初始化标志 */
static int g_cutil_inited;

/* 已打开的文件数 */
static int g_opened_files;

/* 是否禁用调试日志 */
static int g_disable_debug_log;

/* 外部堆栈处理函数 */
static backtrace_handler g_backtrace_hander;

/* 外部崩溃处理函数 */
static crash_handler g_crash_handler;

/* 中断信号处理函数 */
typedef void (*interrupt_handler_func)(int);
static interrupt_handler_func g_interrupt_handler;

/* 设置默认的中断处理函数 */
int set_default_interrupt_handler();

/************************************************************************/
/*                         初始化/结束使用本库                          */
/************************************************************************/

void cutil_init()
{
    int ret ALLOW_UNUSED;
    g_cutil_inited = 1;

    /* 将当前工作目录为可执行文件所在目录 */
    ret = set_current_dir(get_execute_dir());
    ASSERT(ret);

    /* 初始化日志模块 */
    log_init();

    /* 运行时内存调试 */
#ifdef DBG_MEM_RT
    memrt_init();
#endif

    /* 初始化软件名称 */
    xstrlcpy(g_product_name, "UtilsApp", sizeof(g_product_name));

    /* 中断处理函数 */
    ret = set_default_interrupt_handler();
    ASSERT(ret);

    /* 程序崩溃处理 */
#ifdef USE_CRASH_HANDLER
    set_default_crash_handler();
#endif

#ifdef OS_WIN
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

#if defined(OS_POSIX)
    /* 多字节/宽字符串转换 */
    /* 将进程的locale设置为系统的locale，进程启动时默认为"C" */
    /* GLIBC实现为依次查询LC_ALL,LC_CTYPE,LANG环境变量 */
    setlocale(LC_CTYPE, "");

#ifndef OS_ANDROID
    /* 检查系统默认编码 */
    const char *locale = get_locale();
    if (strcasecmp(locale, "UTF-8"))
        log_dprintf(LOG_WARNING, "The current system locale is %s, \
                                 which can cause some function to behave abnormally. \
                                 UTF-8 is strongly recommended.", locale);
#endif
#endif /* OS_POSIX */
}

void cutil_exit()
{
    /* 删除临时目录 */
    if (path_is_directory(get_temp_dir()))
        IGNORE_RESULT(delete_directories(get_temp_dir(), 0, 0));

    /* 检查文件使用情况 */
    if (g_opened_files)
        log_dprintf(LOG_NOTICE, "%d files still open!", g_opened_files);

    /* 检查内存使用情况 */
#ifdef DBG_MEM_RT
    if (g_xalloc_count)
        log_dprintf(LOG_NOTICE, "Detected %d memory leaks!", g_xalloc_count);
    memrt_check();
#endif

    /* 关闭日志模块 */
    log_close_all();

#ifdef OS_WIN
    CoUninitialize();
#endif
}

static inline
int check_cutil_init()
{
    if (!g_cutil_inited)
    {
        const char* msg = "cutil hasn't been initialized ! call cutil_init() first.";
#ifdef OS_WIN
        MessageBoxA(NULL, msg, "fatal error", MB_OK);
#else
        fprintf(stderr, "%s\n", msg);
#endif
        exit(3);
    }
    return 1;
}

#define CHECK_INIT() ASSERT(check_cutil_init())

void set_disable_debug_log()
{
    g_disable_debug_log = 1;
}

void set_product_name(const char* product_name)
{
    if (product_name)
        xstrlcpy(g_product_name, product_name, sizeof(g_product_name));
}

const char* get_product_name()
{
    return g_product_name;
}

#ifdef OS_WIN
BOOL WINAPI InterruptHandlerWin(DWORD dwCtrlType)
{
    if (g_interrupt_handler)
        g_interrupt_handler(dwCtrlType);

    return TRUE;
}
#else
void interrupt_signal_handler(int n ALLOW_UNUSED, siginfo_t *siginfo, void *act ALLOW_UNUSED)
{
    if (g_interrupt_handler)
        g_interrupt_handler(siginfo->si_signo);
}
#endif

/*
 * 设置程序收到中断信号（如Ctrl^C）时的处理函数
 * 若未设置此处理函数，进程在收到此类信号的默认行为是终止进程
 * 注册后再收到中断消息将不会终止当前进程，除非在处理函数中显式退出（如调用exit)
 * 处理函数的int参数代表消息类型（如CTRL_C_EVENT或SIGINT，见实现），一般可以忽略
 * 返回值：注册成功返回1，失败返回0
 */
int set_interrupt_handler(interrupt_handler_func handler)
{
#ifdef OS_WIN
    if (!SetConsoleCtrlHandler(InterruptHandlerWin, TRUE))
        return 0;
    g_interrupt_handler = handler;
    return 1;
#else
    struct sigaction act;
    int error = 0;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = interrupt_signal_handler;
    error = sigaction(SIGINT, &act, NULL);    //Interrupt from keyboard
    error += sigaction(SIGQUIT, &act, NULL); //Termination signal
    error += sigaction(SIGTERM, &act, NULL);    //Quit from keyboard
    if (error)
        return 0;
    g_interrupt_handler = handler;
    return 1;
#endif
}

/* 默认的中断信号处理函数 */
/* 即在终端响铃后安全退出 */
static void default_interrupt_handler(int type)
{
    char msg[128];

#ifdef OS_WIN
    switch(type) {
    case CTRL_C_EVENT:
        xstrlcpy(msg, "Ctrl-C pressed.", sizeof(msg));
        Beep(750, 300);
        break;
    case CTRL_BREAK_EVENT:
        Beep(900, 200);
        xstrlcpy(msg, "Ctrl-Break pressed.", sizeof(msg));
        break;
    case CTRL_CLOSE_EVENT:
        Beep(600, 200);
        xstrlcpy(msg, "Console closed.", sizeof(msg));
        break;
    case CTRL_LOGOFF_EVENT:
        Beep(1000, 200);
        xstrlcpy(msg, "User logs off.", sizeof(msg));
        break;
    case CTRL_SHUTDOWN_EVENT:
        Beep(750, 500);
        xstrlcpy(msg, "System shutdown.", sizeof(msg));
        break;
    default:
        xstrlcpy(msg, "Unknown interrupt event", sizeof(msg));
        break;
    }
#else
    switch(type) {
    case SIGINT:
        xstrlcpy(msg, "SIGINT signal received!", sizeof(msg));
        break;
    case SIGQUIT:
        xstrlcpy(msg, "SIGQUIT signal received!", sizeof(msg));
        break;
    case SIGTERM:
        xstrlcpy(msg, "SIGTERM signal received!", sizeof(msg));
        break;
    default:
        xstrlcpy(msg, "unknwon interrupt signal received!", sizeof(msg));
        break;
    }
#endif

    printf("%s Exit now...\n", msg);
    log_dprintf(LOG_ALERT, "%s Exit now...\n", msg);

    cutil_exit();
    exit(1);
}

/* 设置默认的中断处理函数 */
/* 即调用cutil_exit()后结束当前进程 */
int set_default_interrupt_handler()
{
    return set_interrupt_handler(default_interrupt_handler);
}

#ifdef USE_CRASH_HANDLER

/* 设置默认的崩溃处理函数 */
/* Windows下生成minidump文件，且如果存在pdb将堆栈显示在弹出对话框中 */
/* Linux下生成coredump文件，且如果是debug版打印堆栈到标准错误输出(需使用-rdynamic链接选项) */
void set_default_crash_handler()
{
#ifdef OS_WIN
    /* 加载调试符号 */
    /* Release版本如果有pdb文件也可以打印堆栈 */
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
        log_dprintf(LOG_WARNING, "Initialize debug symbol failed! %s", get_last_error_win32());

    //_CrtSetReportMode(_CRT_ASSERT, 0);    /* disable default assert dialog */

    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CrashDumpHandler);
#else /* POSIX */
    struct rlimit lmt;
    lmt.rlim_cur = RLIM_INFINITY;
    lmt.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &lmt);

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = crash_signal_handler;
    sigaction(SIGSEGV, &act, NULL); //Invalid memory segment access
    sigaction(SIGFPE, &act, NULL);  //Floating point error
    sigaction(SIGABRT, &act, NULL);    //abort(), C++ exception.etc

    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
#endif /* OS_WIN */
}

#endif /* #ifdef USE_CRASH_HANDLER */

/************************************************************************/
/*                         Memory 内存管理                               */
/************************************************************************/

#ifdef DBG_MEM

int g_xalloc_count = 0;

#ifdef DBG_MEM_RT
static void memrt_msg_c(int error,
    const char* file, const char* func, int line,
    void* address, size_t size, int method);
#endif

/* 释放内存 */
void free_d(void *ptr, const char* file, const char* func, int line)
{
    if (!ptr)
    {
        /* FATAL日志消息会记录调用堆栈并exit */
        log_dprintf(LOG_FATAL, "{%s %s %d} Free NULL at %p.\n", file, func, line, ptr);
    }

#ifdef DBG_MEM_LOG
    log_dprintf(LOG_DEBUG, "{%s %s %d} Free at %p.\n", file, func, line, ptr);
#endif

#ifdef DBG_MEM_RT
    __memrt_release(MEMRT_FREE, ptr, 0, file, func, line, NULL, memrt_msg_c);
#endif

    g_xalloc_count--;
    free(ptr);
}

/* 分配内存 */
void *malloc_d(size_t size, const char* file, const char* func, int line)
{
    register void *memptr = NULL;

    if (size > 0 && ((memptr = malloc(size)) == NULL))
        log_dprintf(LOG_FATAL, "{%s %s %d} Malloc memory exhausted.\n", file, func, line);
    else
    {
#ifdef DBG_MEM_LOG
        log_dprintf(LOG_DEBUG, "{%s %s %d} Malloc at %p size %"PRIuS".\n", file, func, line, memptr, size);
#endif
#ifdef DBG_MEM_RT
        __memrt_alloc(MEMRT_MALLOC, memptr, size, file, func, line, NULL, memrt_msg_c);
#endif
        g_xalloc_count++;
    }

    return (memptr);
}

void *realloc_d(void *ptr, size_t size, const char* file, const char* func, int line)
{
    register void *memptr = NULL;

    ASSERT(ptr || size);

    /* 如果指针为空，等效于malloc */
    if (!ptr)
        return malloc_d(size, file, func, line);

    /* 如果大小为0，等效于free */
    if (!size)
    {
        free_d(ptr, file, func, line);
        return NULL;
    }

    if (size > 0 && ((memptr = realloc(ptr, size)) == NULL))
        log_dprintf(LOG_FATAL, "{%s %s %d} Realloc memory exhausted.\n", file, func, line);
    else
    {
#ifdef DBG_MEM_LOG
        log_dprintf(LOG_DEBUG, "{%s %s %d} Realloc from %p to %p size %"PRIuS".\n", file, func, line, ptr, memptr, size);
#endif
#ifdef DBG_MEM_RT
        __memrt_release(MEMRT_FREE, ptr, 0, file, func, line, NULL, memrt_msg_c);
        __memrt_alloc(MEMRT_REALLOC, memptr, size, file, func, line, NULL, memrt_msg_c);
#endif
    }

    return (memptr);
}

void *calloc_d(size_t count, size_t size, const char* file, const char* func, int line)
{
    register void *memptr = NULL;

    if (size > 0 && ((memptr = calloc(count, size)) == NULL))
        log_dprintf(LOG_FATAL, "{%s %s %d} Calloc memory exhausted.\n", file, func, line);
    else
    {
#ifdef DBG_MEM_LOG
        log_dprintf(LOG_DEBUG, "{%s %s %d} Calloc at %p count %"PRIuS" size %"PRIuS".\n", file, func, line, memptr, count, size);
#endif
#ifdef DBG_MEM_RT
        __memrt_alloc(MEMRT_CALLOC, memptr, count * size, file, func, line, NULL, memrt_msg_c);
#endif
        g_xalloc_count++;
    }

    return (memptr);
}

/* 分配字符串 */
char *strdup_d(const char* s, size_t n,  const char* file, const char* func, int line)
{
    char *p;

    if (n != INFINITE)
        p = strndup(s, n);
    else
        p = strdup(s);

    if (!p)
        log_dprintf(LOG_FATAL, "{%s %s %d} Strdup memory failed.\n", file, func, line);
    else
    {
#ifdef DBG_MEM_LOG
        log_dprintf(LOG_DEBUG, "{%s %s %d} Strdup at %p size %"PRIuS".\n", file, func, line, p, strlen(p));
#endif
#ifdef DBG_MEM_RT
        __memrt_alloc(MEMRT_STRDUP, p, strlen(s), file, func, line, NULL, memrt_msg_c);
#endif
        g_xalloc_count++;
    }

    return p;
}

#endif /* DBG_MEM */

/************************************************************************/
/*                         String 字符串操作                             */
/************************************************************************/

/* 将字符串改为小写，若有字符发生改变返回1，否则返回0 */
int lowercase_str (char *str)
{
    int changed = 0;
    for (; *str; str++)
        if (xisupper (*str))
        {
            changed = 1;
            *str = tolower (*str);
        }

    return changed;
}

/* 将字符串改为大写，若有字符发生改变返回1，否则返回0 */
int uppercase_str (char *str)
{
    int changed = 0;
    for (; *str; str++)
        if (xislower (*str))
        {
            changed = 1;
            *str = toupper (*str);
        }

    return changed;
}

/* 获取一个字符串的小写版拷贝 */
char *strdup_lower (const char *s)
{
    char *copy, *p;

    copy = xstrdup(s);
    for (p = copy; *p; p++)
        *p = tolower (*p);

    return copy;
}

/* 获取一个字符串的大写版拷贝 */
char *strdup_upper (const char *s)
{
    char *copy, *p;

    copy = xstrdup (s);
    for (p = copy; *p; p++)
        *p = toupper (*p);

    return copy;
}

/* 拷贝一个字符串的字串 */
char *
substrdup(const char *beg, const char *end)
{
  char *res;

  if (!beg || !end)
      return NULL;

  ASSERT(end >= beg);

  res = (char*)xmalloc(end - beg + 1);
  memcpy (res, beg, end - beg);
  res[end - beg] = '\0';
  return res;
}

/* 安全的vsnprintf */
int xvsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
#if defined(OS_WIN)
#if defined(__MINGW32__) || _MSC_VER <= MSVC6
    // MingW的实现当缓冲区过小时总是返回-1
    // 低版本的VC不支持_vsprintf_p等函数
    // 因此使用功能最相近的_vsnprintf函数
    int i = _vsnprintf(buffer, size, format, args);
    if (size > 0)
        buffer[size - 1] = '\0';
    return i;
#else
    /* 模拟snprintf的标准行为 */
    int length = _vsprintf_p(buffer, size, format, args);
    if (length < 0) {
        if (size > 0)
            buffer[size - 1] = '\0';
        return _vscprintf_p(format, args);
    }
    return length;
#endif
#else
    return vsnprintf(buffer, size, format, args);
#endif
}

/* 安全的snprintf */
int xsnprintf(char* buffer, size_t size, const char* format, ...)
{
    int result;
    va_list arguments;

    if (!size)
        return -1;

    va_start(arguments, format);
    result = xvsnprintf(buffer, size, format, arguments);
    va_end(arguments);

    return result;
}

/* 安全可移植的 asprintf */
int xasprintf(char** out, const char *fmt, ...)
{
    int size = 128;
    char *str = (char*)xmalloc(size);

    ASSERT(out);

    while (1)
    {
        int n;
        va_list args;

        errno = 0;

        va_start (args, fmt);
        n = xvsnprintf (str, size, fmt, args);
        va_end (args);

        /* Upon successful return, these functions return the number of characters printed
            (excluding the null byte used to end output to strings). [Linux man page] */
        if (n >= 0 && n < size)
        {
            *out = str;
            return 1;
        }

        /* If the output was truncated due to this limit then the return value is the number of characters
         * (excluding the terminating null byte) which would have been written to the final if enough space
         * had been available. Thus, a return value of size or more means that string the output was truncated.
         * If an output error is encountered, a negative value is returned. [Linux man page] */

        if (n < 0)
        {
            if (errno != 0                /* VC6不会设置errno */
#ifdef OS_WIN
                && errno != ERANGE        /* VC高版本置为ERANGE */
#else
                && errno != EOVERFLOW    /* 某些POSIX实现置为EOVERFLOW */
#endif
                )
            {
                xfree(str);
                return 0;
            }

            size <<= 1;
        }
        else
            size = n + 1;

        if (size > 32 * 1024 * 1024) {
            xfree(str);
            return 0;
        }

        str = (char*)xrealloc (str, size);
    }
}

/* BSD strlcpy implmentation */
size_t xstrlcpy(char* dst, const char* src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;

    ASSERT(dst && src);

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';        /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);    /* count does not include NUL */
}

/* BSD strlcat implmentation */
size_t xstrlcat(char* dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;
    size_t dlen;

    ASSERT(dst && src);

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (*d != '\0' && n-- != 0)
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return(dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return(dlen + (s - src));    /* count does not include NUL */
}

#ifndef __GLIBC__

/* 拷贝一个字符串的前n个字节 */
char *strndup(const char* s, size_t n)
{
    size_t slen, len;
    char *p;

    ASSERT(s);

    slen = strlen(s);
    len = xmin(slen, n);
    p = (char*)malloc(len+1);    /* 不使用xmalloc以免重复计算内存申请次数 */
    if (!p)
        log_dprintf(LOG_FATAL, "strndup malloc failed");

    strncpy(p, s, len);
    p[len] = '\0';

    return p;
}

/*
 * 分割字符串
 * 该函数会改变源字符串，但可以处理空格的情况
 * 取自 GLIBC 2.16.0
 */
char *strsep (char **stringp, const char *delim)
{
  char *begin, *end;

  begin = *stringp;
  if (begin == NULL)
    return NULL;

  if (delim[0] == '\0' || delim[1] == '\0')
  {
      char ch = delim[0];

      if (ch == '\0')
    end = NULL;
      else
    {
      if (*begin == ch)
        end = begin;
      else if (*begin == '\0')
        end = NULL;
      else
        end = strchr (begin + 1, ch);
    }
  }
  else
    /* Find the end of the token.  */
    end = strpbrk (begin, delim);

  if (end)
  {
      /* Terminate the token and set *STRINGP past NUL character.  */
      *end++ = '\0';
      *stringp = end;
  }
  else
    *stringp = NULL;

  return begin;
}

#if !defined(__MINGW32__) && !defined(COMPILER_MSVC)

/* 无可替代函数，需要自己实现strcasecmp, strncasecmp */
static unsigned char charmap[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xe1, 0xe2, 0xe3, 0xe4, 0xc5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
};

/* 不区分大小写比较两个字符串 */
int strcasecmp(const char *s1, const char* s2)
{
    unsigned char u1, u2;

    if (!s1 || !s2)
        return -1;

    for ( ; ; s1++, s2++)
    {
        u1 = (unsigned char) *s1;
        u2 = (unsigned char) *s2;

        if ((u1 == '\0') || (charmap[u1] != charmap[u2]))
            break;
    }

    return charmap[u1] - charmap[u2];
}

int strncasecmp(const char *s1, const char*s2, size_t length)
{
    unsigned char u1, u2;

    if (!s1 || !s2)
        return -1;

    for (; length != 0; length--, s1++, s2++)
    {
        u1 = (unsigned char) *s1;
        u2 = (unsigned char) *s2;

        if (charmap[u1] != charmap[u2])
            return charmap[u1] - charmap[u2];

        if (u1 == '\0')
            return 0;
    }

    return 0;
}

#endif /* !COMPILER_MSVC */

/* 在字符串s的前slen个字节中查找find */
char *strnstr(const char *s, const char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

void* memrchr(const void* s, int c, size_t n)
{
    const unsigned char* p = (const unsigned char*)s;
    for (p += n; n > 0; n--)
        if (*--p == c)
            return (void*)p;

    return NULL;
}

/* 通过简单地将每个字节与00101010异或来实现 */
void* memfrob(void *mem, size_t length)
{
    char *p = (char*)mem;

    while (length-- > 0)
        *p++ ^= 42;

    return mem;
}

#endif /* !__GLIBC__ */

#if !defined(__GLIBC__) || !defined(_GNU_SOURCE)
/*
* 不区分大小写查找字符串是否包含指定字串
*/
char *strcasestr(const char *s, const char *find)
{
    char c, sc;
    size_t len;

    if (!s || !find)
        return NULL;

    if ((c = *find++) != 0) {
        c = tolower((unsigned char)c);
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0)
                    return NULL;
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}
#endif

/*
 * 不区分大小写查找字符串的前多少个字节是否包含指定字串
 */
char *strncasestr(const char* s, const char* find, size_t slen)
{
    char c, sc;
    size_t len;

    if (!s || !find)
        return NULL;

    if ((c = *find++) != '\0') {
        c = tolower((unsigned char)c);
        len = strlen(find);
        do {
            do {
                if (slen-- < 1 || (sc = *s++) == '\0')
                    return (NULL);
            } while ((char)tolower((unsigned char)sc) != c);
            if (len > slen)
                return (NULL);
        } while (strncasecmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

#define SIZE_BITS (sizeof(size_t) * 8)

/*
 * Taken from coreutils/lib/hash-pjw.c
 * Algorithm from http://www.haible.de/bruno/hashfunc.html
 */
size_t hash_pjw (const char *x, size_t tablesize)
{
    const char *s;
    size_t h = 0;

    for (s = x; *s; s++)
        h = *s + ((h << 9) | (h >> (SIZE_BITS - 9)));

    return h % tablesize;
}

#undef SIZE_BITS

/************************************************************************/
/*                         Filesystem 文件系统相关                        */
/************************************************************************/

/* 判断路径名是否有效 */
/* 若路径有效，返回路径名字符串的长度；否则返回0 */
static inline
size_t _path_valid(const char* path, int absolute)
{
    size_t len;

    /* 因路径缓冲区大都定义为MAX_PATH大小，因此路径名长度不能大于MAX_PATH -1 */
    if (!path || !(len = strlen(path)) || len + 1 > MAX_PATH)
        return 0;

    if (absolute) {
        if (len < MIN_PATH)
            return 0;

#ifdef OS_WIN
        if (!(xisalpha(path[0]) && path[1] == ':' && IS_PATH_SEP(path[2])) &&    /* 本地磁盘 */
            !(IS_PATH_SEP(path[0]) && IS_PATH_SEP(path[1])))                     /* UNC 网络共享 */
            return 0;
#else
        if (!IS_PATH_SEP(path[0]))
            return 0;
#endif
    }

    return len;
}

/* 保证目录路径以分隔符结尾，返回新路径的长度，失败返回0 */
/* path和outbuf可以相同，表示对某一路径缓冲区进行操作 */
static
size_t _end_with_slash(const char* path, char* outbuf, size_t outlen)
{
    size_t len = _path_valid(path, 0);
    if (!len || outlen <= len)
        return 0;

    if (path != outbuf)
        memcpy(outbuf, path, len + 1);

    if (!IS_PATH_SEP(path[len - 1])) {
        if (len + 1 == outlen)
            return 0;

        outbuf[len] = PATH_SEP_CHAR;
        outbuf[++len] = '\0';
    }

    return len;
}

/* 判断path所指的路径是否是绝对路径 */
int is_absolute_path(const char* path)
{
    return _path_valid(path, 1) != 0;
}

/* 判断path是否是Windows的 UNC (Universal Naming Convention) 路径 */
#ifdef OS_WIN
int is_unc_path(const char* path)
{
    return path && path[0] && path[1] && path[2] &&
        IS_PATH_SEP(path[0]) && IS_PATH_SEP(path[1]) && !IS_PATH_SEP(path[2]);
}
#endif

/* 判断路径是否是根路径 */
int is_root_path(const char* path)
{
    return is_absolute_path(path) && (path[MIN_PATH] == '\0');
}

/* 返回路径的文件名或最底层目录名
 * 例1: C:\myfolder\a.txt -> a.txt ; /var/a.out -> a.out
 * 例2: C:\myfolder\dir -> dir ; /var/temp -> temp
 * 例3: C:\myfolder\dir\ -> dir\ ; /var/temp/ -> temp/
 * 例4: C:\ -> C:\ ; / -> /
 */
/* 注：如果是linux系统环境且locale不是UTF8，结果可能不正确 */
const char* path_find_file_name(const char* path)
{
#if (defined OS_WIN) && (!defined USE_UTF8_STR)
    return PathFindFileNameA(path);
#else //UTF-8
    const char *p;
    size_t len;

    if (!path)
        return NULL;

    len = strlen(path);
    if (!len)
        return path;            /* 空字符串 */

    p = path + len - 1;

    while (IS_PATH_SEP(*p) && p != path)
        --p;
    if (p == path)
        return p;                /*  全分隔符 */

    while (!IS_PATH_SEP(*p) && p != path)
        --p;

    if (p == path && !IS_PATH_SEP(*p))  /* 相对路径 */
        return p;
    else
        return p+1;
#endif
}

/* 返回路径的扩展名 */
/* 例1：C:\\1.txt -> .txt */
/* 例2：C:\\abc -> "" */
/* 例4：C:\\abc. -> . */
/* 例5: NULL -> NULL */
/* 例6："" -> "" */
/* 注：如果是linux系统环境且locale不是UTF8，结果可能不正确 */
const char* path_find_extension(const char* path)
{
#if (defined OS_WIN) && (!defined USE_UTF8_STR)
    return PathFindExtensionA(path);
#else //UTF-8
    const char *p, *pe;
    size_t len;

    if (!path)
        return NULL;

    len = strlen(path);
    if (!len)
        return path;

    pe = path + len;
    p = pe - 1;

    if (*p == '.')
        return p;

    while (!IS_PATH_SEP(*p) && *p != '.' && p != path)
        -- p;

    if (*p == '.')
        return p;

    return pe;
#endif
}

/* 返回路径所指目录/文件的上级目录路径(包含最后的路径分隔符) */
/* path和outbuf可以相同，表示在某一路径缓冲区中进行操作 */
/* 例1: C:\a.txt -> C:\ */
/* 例2: /usr/include/ -> /usr/ */
/* 返回值: 成功返回1，错误返回0并且outbuf为"" */
/* 注：如果是linux系统环境且locale不是UTF8，结果可能不正确 */
int path_find_directory(const char *path, char* outbuf, size_t outlen)
{
    size_t plen, nlen;

    plen = _path_valid(path, 0);
    if (!plen)
        return 0;

    if (outbuf != path)
        memset(outbuf, 0, outlen);

#if (defined OS_WIN) && (!defined USE_UTF8_STR)
    {
        wchar_t wpath[MAX_PATH];
        const wchar_t* p;

        if (!MBCS2UNI(path, wpath, MAX_PATH))
            return 0;

        p = wpath + wcslen(wpath) - 1;
        while (*p == PATH_SEP_WCHAR && p != wpath)
            --p;

        while (*p != PATH_SEP_WCHAR && p != wpath)
            --p;

        if (p == wpath)
            return 0;

        nlen = p - wpath + 1 + 1;
        if (outlen < nlen)
            return 0;

        wpath[nlen-1] = L'\0';
        return UNI2MBCS(wpath, outbuf, outlen);
    }
#else //UTF-8
    {
        const char* p = path + plen - 1;

        while (IS_PATH_SEP(*p) && p != path)
            --p;

        while (!IS_PATH_SEP(*p) && p != path)
            --p;

        if (p == path)
            return 0;

        nlen = p - path + 1 + 1;
        if (outlen < nlen)
            return 0;

        if (outbuf == path)
            outbuf[nlen-1] = '\0';
        else
            xstrlcpy(outbuf, path, nlen);

        return 1;
    }
#endif
}

/* 将后缀名添加到文件名后扩展名前 */
/* 如果没有扩展名直接附加到路径最后 */
int path_insert_before_extension(const char* path,
    const char*suffix, char* buf, size_t outlen)
{
    const char* ext;
    size_t plen, elen, slen;

    plen = _path_valid(path, 0);
    if (!plen || !suffix)
        return 0;

    /* 获取文件扩展名 */
    ext = path_find_extension(path);
    elen = strlen(ext);
    slen = strlen(suffix);

    if (outlen < plen + slen + 1)
        return 0;

    /* 使用strncpy 以忽略扩展名 */
    strncpy(buf, path, plen - elen);
    sprintf(buf + plen - elen, "%s%s", suffix, ext);

    return 1;
}

/* 路径所指文件/目录是否存在 */
int path_file_exists(const char* path)
{
    if (!_path_valid(path, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
        wchar_t wpath[MAX_PATH];
        if (!UTF82UNI(path, wpath, MAX_PATH))
            return 0;
        return PathFileExistsW(wpath);
    }
#else
    return PathFileExistsA(path);
#endif
#else /* POSIX */
    {
        struct stat st;
        if(stat(path, &st))
            return 0;
        return 1;
    }
#endif
}

/* 路径是否是有效目录 */
int path_is_directory(const char* path)
{
    if (!_path_valid(path, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
        wchar_t wpath[MAX_PATH];
        if (!UTF82UNI(path, wpath, MAX_PATH))
            return 0;
        return PathIsDirectoryW(wpath);
    }
#else
    return PathIsDirectoryA(path);
#endif
#else /* POSIX */
    {
        struct stat st;
        if(!stat(path, &st) && (st.st_mode & S_IFDIR))
            return 1;

        return 0;
    }
#endif
}

/* 路径是否是文件（存在且不是目录） */
int path_is_file(const char* path)
{
    if (!_path_valid(path, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
        wchar_t wpath[MAX_PATH];
        if (!UTF82UNI(path, wpath, MAX_PATH))
            return 0;
        return PathFileExistsW(wpath) && !PathIsDirectoryW(wpath);
    }
#else
    return PathFileExistsA(path) && !PathIsDirectoryA(path);
#endif
#else
    {
        struct stat st;
        if(!stat(path, &st) && !(st.st_mode & S_IFDIR))
            return 1;

        return 0;
    }
#endif
}

/*
 * 获取文件/目录相对于当前工作目录的绝对路径
 */
int absolute_path(const char* relpath, char* buf, size_t len)
{
#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
        wchar_t wrpath[MAX_PATH], wapath[MAX_PATH];
        if (!UTF82UNI(relpath, wrpath, MAX_PATH))
            return 0;

        if (!GetFullPathNameW(wrpath, MAX_PATH, wapath, NULL))
            return 0;

        if (!UNI2UTF8(wapath, buf, len))
            return 0;

        return 1;
    }
#else
    if (!GetFullPathNameA(relpath, len, buf, NULL))
        return 0;

    return 1;
#endif
#else
    char abuf[PATH_MAX];
    size_t alen;
    if (!realpath(relpath, abuf))
        return 0;

    alen = strlen(abuf);
    if (alen >= len)
        return 0;

    memcpy(buf, abuf, alen + 1);
    return 1;
#endif
}

/*
* 获取src指向dst的相对路径
* eg. src="/root/a/b/1.txt", dst="/2.txt", ret="../../../2.txt"
* eg. src="2.txt", dst="root/a/b/1.txt", ret="root/a/b/1.txt"
* 注1：src和dst必须同时是相对或绝对链接
* 注2：如果同是绝对链接而且是在windows下，src和dst必须在同一盘符下
* 注3：src是否以路径分隔符结尾会产生不同的相对路径，如:
* eg. src="C:\\a", dst="C:\\a\\b\\1.txt", ret="a\\b\\1.txt"
* eg. src="C:\\a\\", dst="C:\\a\\b\\1.txt", ret="b\\1.txt"
 */
int relative_path(const char* src, const char* dst, char* outbuf, size_t slen)
{
  char sepbuf[3];
  char separator = '/';
  const char *b, *l;
  int i, basedirs;
  ptrdiff_t start;

  /* check param */
  if (!src || !dst || !outbuf || !slen)
      return 0;

  /* First, skip the initial directory components common to both
     files.  */
  start = 0;
  for (b = src, l = dst; *b == *l && *b != '\0'; ++b, ++l)
    {
      if (IS_PATH_SEP(*b)) {
          start = (b - src) + 1;
          separator = *b;
      }
    }
  src += start;
  dst += start;

  /* Count the directory components in B. */
  basedirs = 0;
  for (b = src; *b; b++)
    {
      if (IS_PATH_SEP(*b)) {
          ++basedirs;
          separator = *b;
      }
    }

  sepbuf[0] = '.';
  sepbuf[1] = '.';
  sepbuf[2] = separator;

  if (slen < 3 * basedirs + strlen (dst) + 1)
      return 0;
  memset(outbuf, 0, slen);

  /* Construct LINK as explained above. */
  for (i = 0; i < basedirs; i++)
    memcpy (outbuf + 3 * i, sepbuf, 3);

  strcpy (outbuf + 3 * i, dst);

  return 1;
}

/* 获取可用的文件路径 */
int unique_file(const char* path, char *buf, size_t len, int create_now)
{
    const char* ext;
    size_t plen, elen;
    int i;

    plen = _path_valid(path, 0);
    if (!plen || !buf)
        return 0;

    // 如果初始路径不存在，直接返回
    if (!path_file_exists(path))
    {
        if (xstrlcpy(buf, path, len) >= len)
            return 0;

        goto succeed;
    }

    /* 获取文件扩展名 */
    ext = path_find_extension(path);
    elen = strlen(ext);

    for (i = 1; i < 10000; i++)
    {
        if (len < plen + num_bits(i) + 4 + elen)    /* " ()"和'\0' */
            return 0;

        /* 使用strncpy 以忽略扩展名 */
        strncpy(buf, path, plen - elen);
        sprintf(buf + plen - elen, " (%d)%s", i, ext);

        if (!path_file_exists(buf))
            goto succeed;
    }

    NOT_REACHED();
    return 0;

succeed:
    if (create_now)
        return create_directories(buf) && touch(buf);

    return 1;
}

/* 获取可用的目录路径 */
/* 返回的路径名是否以分隔符结尾与原路径相同 */
int unique_dir(const char* path, char *buf, size_t len, int create_now)
{
    int has_slash, i;

    size_t plen = _path_valid(path, 0);
    if (!plen || !buf)
        return 0;

    /* 如果初始目录不存在，直接返回 */
    if (!path_file_exists(path))
    {
        if (xstrlcpy(buf, path, len) >= len)
            return 0;

        goto succeed;
    }

    /* 直接路径后加" (N)"重试 */
    has_slash = (IS_PATH_SEP(path[plen-1]) ? 1 : 0);
    if (has_slash)
        plen --;

    for (i = 1; i < 10000; i++)
    {
        /* 新路径名的长度，附加了 " (x)"和'\0' */
        size_t explen = plen + num_bits(i) + 4 + (has_slash ? 1 : 0);
        if (len < explen)
            return 0;

        xstrlcpy(buf, path, len);
        sprintf(buf + plen, " (%d)", i);
        if (has_slash) {
            buf[explen-2] = PATH_SEP_CHAR;
            buf[explen-1] = '\0';
        }

        if (!path_file_exists(buf))
            goto succeed;
    }

    NOT_REACHED();
    return 0;

succeed:
    if (create_now)
        return create_directories(buf);
    return 1;
}

#define U PATH_UNIX            /* Unix不可用： / and \0 */
#define W PATH_WINDOWS        /* Windows不可用： \|/<>?:*" */
#define C 4                    /* 控制字符：如 0-31 */

#define UW U|W
#define UWC U|W|C

static const unsigned char pathchr_table[256] =
{
UWC,  C,  C,  C,   C,  C,  C,  C,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
  C,  C,  C,  C,   C,  C,  C,  C,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
  C,  C,  C,  C,   C,  C,  C,  C,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  C,  C,  C,  C,   C,  C,  C,  C,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
  0,  0,  W,  0,   0,  0,  0,  0,   /* SP  !   "   #    $   %   &   '   */
  0,  0,  W,  0,   0,  0,  0, UW,   /* (   )   *   +    ,   -   .   /   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
  0,  0,  W,  0,   W,  0,  W,  W,   /* 8   9   :   ;    <   =   >   ?   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
  0,  0,  0,  0,   W,  0,  0,  0,   /* X   Y   Z   [    \   ]   ^   _   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
  0,  0,  0,  0,   W,  0,  0,  C,   /* x   y   z   {    |   }   ~   DEL */

  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,

  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};

#define PATH_CHAR_ILLEGAL(c, mask) (pathchr_table[(unsigned char)(c)] & (mask|C))

char* strpsep(const char* path) {
    const char* p = path;
    while (*p && !IS_PATH_SEP(*p))
        ++p;

    return *p != '\0' ? (char*)p : NULL;
}

char* strrpsep(const char* path) {
    const char* p;

    if (!path)
        return NULL;

    p = path + strlen(path) - 1;
    while (p >= path && !IS_PATH_SEP(*p))
        --p;

    return p >= path ? (char*)p : NULL;
}

static inline
int _is_path_sep(int platform, char c) {
    if (platform == PATH_WINDOWS)
        return c == '/' || c == '\\';
    else
        return c == '/';
}

/* 将路径中的非法字符替换为空格符 */
/* 如果被替换为多个连续的空格将被合并为一个 */
/* reserved所指定的字符将被保留(通常为路径分隔符) */
void path_illegal_blankspace(char *path, int platform, int reserve_separator)
{
    char *p, *q;
    for (p = q = path; *p; p++) {
        if (PATH_CHAR_ILLEGAL(*p, platform) &&
            (!reserve_separator || !_is_path_sep(platform, *p))) {
            *q = ' ';
            if (q == path || *(q-1) != ' ')
                q++;
        } else {
            *q++ = *p;
        }
    }
    *q = '\0';
}

/*
 * 将路径中的非法字符替换为%HH的形式，需外部释放
 * 注：path必须为UTF-8编码
 */
char* path_escape(const char* path, int platform, int reserve_separator)
{
    size_t quoted, outlen;
    const char *p, *pe;
    char *out, *q;

    pe = path + strlen(path);
    quoted = 0;

    for (p = path; p < pe; p++)
        if (PATH_CHAR_ILLEGAL(*p, platform) &&
            (!reserve_separator || !_is_path_sep(platform, *p)))
            ++quoted;

    if (!quoted)
        return xstrdup(path);

    outlen = (pe - path) + (2 * quoted) + 1;
    out = (char*)xmalloc(outlen);
    q = out;

    for (p = path; p < pe; p++) {
        if (PATH_CHAR_ILLEGAL(*p, platform) &&
            (!reserve_separator || !_is_path_sep(platform, *p))) {
            unsigned char ch = *p;
            *q++ = '%';
            *q++ = XNUM_TO_DIGIT (ch >> 4);
            *q++ = XNUM_TO_DIGIT (ch & 0xf);
        } else {
            *q++ = *p;
        }
    }
    *q = '\0';

    return out;
}

#undef U
#undef W
#undef C
#undef UW
#undef UWC

/* 遍历目录 */
struct walk_dir_context {
    /* 遍历的路径名，必须以路径分隔符结尾 */
    char    basedir[MAX_PATH];

#ifdef OS_WIN
    HANDLE                hFind;
#ifdef USE_UTF8_STR
    WIN32_FIND_DATAW    wfd;
    char                filename[MAX_PATH];
#else
    WIN32_FIND_DATAA    wfd;
#endif
#else
    DIR*                dir;
    struct dirent*        pentry;
#endif
};

/* 打开目录并获取第一项 */
struct walk_dir_context* walk_dir_begin(const char *dir)
{
    struct walk_dir_context* ctx = NULL;

#ifdef OS_WIN
    char buf[MAX_PATH + 3];   /* strlen("*.*") */
#endif

    ctx = XCALLOC(struct walk_dir_context);
    if (!_end_with_slash(dir, ctx->basedir, sizeof(ctx->basedir))) {
        xfree(ctx);
        return NULL;
    }

#ifdef OS_WIN
    xsnprintf(buf, sizeof(buf), "%s*.*", ctx->basedir);
#ifdef USE_UTF8_STR
    {
        wchar_t wbuf[MAX_PATH];
        if (!UTF82UNI(buf, wbuf, MAX_PATH)) {
            xfree(ctx);
            return NULL;
        }
        ctx->hFind = FindFirstFileW(wbuf, &ctx->wfd);
    }
#else
    ctx->hFind = FindFirstFileA(buf, &ctx->wfd);
#endif
    if (ctx->hFind == INVALID_HANDLE_VALUE) {
        xfree(ctx);
        return NULL;
    }
#else
    ctx->dir = opendir(dir);
    if (ctx->dir == NULL) {
        xfree(ctx);
        return NULL;
    }
    ctx->pentry = readdir(ctx->dir);
    if (ctx->pentry == NULL) {
        closedir(ctx->dir);
        xfree(ctx);
        return NULL;
    }
#endif
    return ctx;
}

/* 遍历目录中的下一项 */
int walk_dir_next(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    if (!FindNextFileW(ctx->hFind, &ctx->wfd))
        return 0;
#else
    if (!FindNextFileA(ctx->hFind, &ctx->wfd))
        return 0;
#endif
#else
    if ((ctx->pentry = readdir(ctx->dir)) == NULL)
        return 0;
#endif
    return 1;
}

/* 该项是否是.(当前目录) */
int walk_entry_is_dot(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    return !wcscmp(ctx->wfd.cFileName, L".");
#else
    return !strcmp(ctx->wfd.cFileName, ".");
#endif
#else
    return !strcmp(ctx->pentry->d_name, ".");
#endif
}

/* 该项是否是..(上级目录) */
int walk_entry_is_dotdot(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    return !wcscmp(ctx->wfd.cFileName, L"..");
#else
    return !strcmp(ctx->wfd.cFileName, "..");
#endif
#else
    return !strcmp(ctx->pentry->d_name, "..");
#endif
}

/* 该项是否是目录 */
int walk_entry_is_dir(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

#ifdef OS_WIN
    return ctx->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#else
    return ctx->pentry->d_type == DT_DIR;
#endif
}

/* 该项是否是文件 */
int walk_entry_is_file(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

    return !walk_entry_is_dir(ctx);
}

/* 普通文件的定义是：在Widnows系统上不是隐藏文件、系统文件等，
 * 在Linux系统上不是块设备、字符设备、FIFO等 */
int walk_entry_is_regular(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

#ifdef OS_WIN
    if (ctx->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
        || ctx->wfd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM
        || ctx->wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        return 0;
    return 1;
#else
    return ctx->pentry->d_type == DT_REG;
#endif
}

/* 获取此项路径名 */
const char* walk_entry_name(struct walk_dir_context *ctx)
{
    if (!ctx)
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    if (!UNI2UTF8(ctx->wfd.cFileName, ctx->filename, MAX_PATH))
        return NULL;

    return ctx->filename;
#else
    return ctx->wfd.cFileName;
#endif
#else
    return ctx->pentry->d_name;
#endif
}

/* 获取此项路径名 */
int walk_entry_path(struct walk_dir_context *ctx, char *buf, size_t len)
{
    const char* filename = walk_entry_name(ctx);
    if (!filename)
        return 0;

    if (xstrlcpy(buf, ctx->basedir, len) >= len ||
        xstrlcat(buf, filename, len) >= len)
        return 0;

    return 1;
}

/* 结束遍历 */
void walk_dir_end(struct walk_dir_context *ctx)
{
    if (!ctx)
        return;

#ifdef OS_WIN
    FindClose(ctx->hFind);
#else
    closedir(ctx->dir);
#endif
    xfree(ctx);
}

/*
 * 创建单层目录，成功返回1
 * 注意：Windows下创建目录时目录名两边的空格会被自动删除，
 * 文件夹名末尾的.也会被自动删除
 */
int create_directory(const char *dir)
{
    if (!_path_valid(dir, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
    wchar_t wbuf[MAX_PATH];
    if (!UTF82UNI(dir, wbuf, MAX_PATH))
        return 0;

    return CreateDirectoryW(wbuf, NULL);
    }
#else
    return CreateDirectoryA(dir, NULL);
#endif
#else //Linux
    return mkdir(dir, 0700) == 0;
#endif
}

/* 创建多级目录 */
int create_directories(const char* dir)
{
    char *pb, *p, *pe;
    char u8dir[MAX_PATH];
    size_t len;

#if (defined OS_WIN) && (!defined USE_UTF8_STR)
    char wbuf[MAX_PATH];
#endif

    len = _path_valid(dir, 0);
    if (!len)
        return 0;

    if (is_root_path(dir))
        return 1;

    /* 如果在Windows下使用多字节字符集，首先转为UTF-8字符集 */
#if (defined OS_WIN) && (!defined USE_UTF8_STR)
    {
    wchar_t wbuf[MAX_PATH];
    if (!MBCS2UNI(dir, wbuf, MAX_PATH) ||
        !UNI2UTF8(wbuf, u8dir, MAX_PATH))
        return 0;
    }
    len = strlen(u8dir);
#else
    memcpy(u8dir, dir, len + 1);
#endif

    pb = u8dir;
    pe = u8dir + len;

    /* 定位到路径的第一个有效文件[夹] */
#ifdef OS_WIN
    /* UNC路径要忽略主机名 */
	/* 如 \\192.168.1.6\shared\  */
	if (is_unc_path(pb)) {
        p = strpsep(pb + 2);
        if (!p)
            return 0;
        ++p;
    } else if (is_absolute_path(pb)) {
        p = pb + MIN_PATH;
    } else
        p = pb;
#else
    if (is_absolute_path(pb))
        p = pb + MIN_PATH;
    else
        p = pb;
#endif

    /* 逐级创建文件夹 */
    while((p = strpsep(p)))
    {
        *p = '\0';

        /* 如果在Windows下使用多字节字符集，
         * path_is_directory 等系列函数参数也应为多字节字符串，不能直接使用 */
#if (defined OS_WIN) && (!defined USE_UTF8_STR)
        if (!MBCS2UNI(pb, wbuf, MAX_PATH))
            return 0;
        if (!PathIsDirectoryW(wbuf) && !CreateDirectoryW(wbuf))
            return 0;
#else
        if (!path_is_directory(pb) && !create_directory(pb))
            return 0;
#endif

        *p = PATH_SEP_CHAR;

        /* 路径最后以"/"结尾 */
        if (++p >= pe)
            break;
    }

    return 1;
}

/* 删除一个空目录 */
int delete_directory(const char *dir)
{
    if (!_path_valid(dir, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
    wchar_t wbuf[MAX_PATH];
    if (!UTF82UNI(dir, wbuf, MAX_PATH))
        return 0;

    return RemoveDirectoryW(wbuf);
    }
#else
    return RemoveDirectoryA(dir);
#endif
#else //Linux
    return !rmdir(dir);
#endif
}

#define WALK_END_RETURN_0 do{    \
    walk_dir_end(ctx);        \
    return 0;                \
}while(0)

/* 递归删除目录下的内容 */
int delete_directories(const char *dir, delete_dir_cb func, void *arg)
{
    struct walk_dir_context* ctx = NULL;
    char buf[MAX_PATH];
    int succ;

    ctx = walk_dir_begin(dir);
    if (!ctx)
        return 0;

    do {
        if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
            continue;
        else if (likely(walk_entry_path(ctx, buf, MAX_PATH))) {
            if (!walk_entry_is_dir(ctx)) {        //文件
                succ = delete_file(buf);
                if ((func && !func(buf, 0, succ, arg)))
                    WALK_END_RETURN_0;
            } else {                            //目录
                succ = delete_directories(buf, func, arg);
                if ((func && !func(buf, 1, succ, arg)))
                    WALK_END_RETURN_0;
            }
        } else
            WALK_END_RETURN_0;
    } while(walk_dir_next(ctx));

    walk_dir_end(ctx);

    if (!delete_directory(dir))
        return 0;

    return 1;
}

/* 判断目录是否是空目录 */
int is_empty_dir(const char* dir)
{
    struct walk_dir_context* ctx = NULL;

    ctx = walk_dir_begin(dir);
    if (!ctx)
        return 0;

    do {
        if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
            continue;
        else
            WALK_END_RETURN_0;
    } while (walk_dir_next(ctx));

    walk_dir_end(ctx);
    return 1;
}

/*
* 递归删除目录下的所有空目录
*/
int delete_empty_directories(const char* dir)
{
    struct walk_dir_context* ctx;
    char buf[MAX_PATH];
    int empty = 1;

    ctx = walk_dir_begin(dir);
    if (ctx) {
        do {
            if (walk_entry_is_dot(ctx)|| walk_entry_is_dotdot(ctx))
                continue;
            else if (likely(walk_entry_path(ctx, buf, MAX_PATH))) {
                if (walk_entry_is_dir(ctx)) {        //目录
                    if (!delete_empty_directories(buf))
                        empty = 0;
                } else                                //文件
                    empty = 0;
            } else
                WALK_END_RETURN_0;
        } while(walk_dir_next(ctx));

        walk_dir_end(ctx);
    } else
        return 0;

    return empty && delete_directory(dir);
}


/*
 * 递归拷贝目录
 * curdir是指本次要遍历的目录，srcdir和dstdir是不变的，总是指向最初的目录，且必须都以路径分隔符结尾
 * 回调函数参数action：0表示拷贝文件，1表示拷贝目录，2表示创建目录
 */
static int _copy_directories(char *curdir, const char *srcdir, const char *dstdir,
                            copy_dir_cb func, void *arg)
{
    struct walk_dir_context* ctx = NULL;
    char spath[MAX_PATH+1];                // 遍历项路径
    char dpath[MAX_PATH+1];                // 拷贝到路径;
    size_t srclen = strlen(srcdir);
    size_t dstlen = strlen(dstdir);
    int succ;

    ctx = walk_dir_begin(curdir);
    if (!ctx)
        return 0;

    //遍历目录
    do {
        if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
            continue;
        else if (likely(walk_entry_path(ctx, spath, MAX_PATH))) {
            char *partial = spath + srclen;            // 文件相对于路径
            size_t    partlen = strlen(partial);        // 相对路径长度

            if (dstlen + partlen + 1 > sizeof(dpath))
                WALK_END_RETURN_0;

            snprintf(dpath, sizeof(dpath), "%s%s", dstdir, partial);

            if (!walk_entry_is_dir(ctx)) {            // 文件
                // 拷贝此文件
                succ = copy_file(spath, dpath, 1);
                if (func && !func(spath, dpath, 0, succ, arg))
                    WALK_END_RETURN_0;
            } else {                                // 目录
                // 创建新目录
                succ = create_directory(dpath);
                if (func && !func(spath, dpath, 2, succ, arg))
                    WALK_END_RETURN_0;

                // 递归拷贝目录
                succ = _copy_directories(spath, srcdir, dstdir, func, arg);
                if (func && !func(spath, dpath, 1, succ, arg))
                    WALK_END_RETURN_0;
            }
        } else
            WALK_END_RETURN_0;

    } while (walk_dir_next(ctx));

    walk_dir_end(ctx);

    return 1;
}

/* 递归复制目录(将源目录复制到目标目录下)
 * 如果目标目录不存在将被创建
 * 如将/a目录复制到/b目录下，则形成的目录是/b/a/

 * 注意1（失败情况）：
 * 1、目标目录下不能有同原目录名相同的目录，比如/b/a/之前就存在，那么会失败
 * 2、如果目标目录下有同源目录名相同名字的文件，比如/b/a是一个文件，那就也会失败
 *
 * 注意2 有两种情况下复制是不允许的：
 * 1、源目录和目标目标不能相同，总不能把自己拷贝到自己下面吧 -_-||
 * 2、目标目录是源目录的子目录，比如将/usr复制到/usr/include/中去，这样会造成死循环
 * 3、原目录已经在目标目录下，如将/usr/include/复制到/usr/下，这根本不用复制
 *
 * 注意3 在Windows下，如果直接将C:\复制到D:\，会创建D:\C_DRIVE\...
 * 在Linux下，是不可能把/复制到任意其他路径的，因为违反了规则2

 * 注意4 Linux的路径操作是区分大小写的，即a和A可同时存在于一个目录下
 * 而在Windows下操作时，不能在同一目录下创建已存在文件/目录名的大小写不同版本
 * 如果在Linux下为NTFS/FAT文件系统创建了仅大小写不同的文件/目录，再切换到Windows下是不可预测的
 * 为了编写可移植的应用程序，不应该以大小写来区分不同的文件或目录。
 */
int copy_directories(const char *src, const char *dst,
                    copy_dir_cb func, void *arg)
{
    char sdir[MAX_PATH], ddir[MAX_PATH];
    char target_dir[MAX_PATH];
    char *p;

    /* 确保目录以分隔符结尾 */
    size_t slen = _end_with_slash(src, sdir, sizeof(sdir));
    size_t dlen = _end_with_slash(dst, ddir, sizeof(ddir));
    if (!slen || !dlen)
        return 0;

    if (!path_is_directory(sdir) || !create_directories(ddir))
        return 0;

    /* 检查规则1 */
#if defined OS_WIN
    if (!strcasecmp(sdir, ddir))
        return 0;
#else
    if (!strcmp(sdir, ddir))
        return 0;
#endif

    /* 检查规则2 */
#ifdef OS_WIN
    p = strcasestr(ddir, sdir);
#else
    p = strstr(ddir, sdir);
#endif

    if (p == ddir)
        return 0;

    /* 合成目标目录 */
    {
        char dirname[MAX_PATH];
        const char* fn = path_find_file_name(sdir);
        if (!fn)
            return 0;

        strcpy(dirname, fn);

#ifdef OS_WIN
        if (strlen(fn) == MIN_PATH)        /* 源目录是根驱动器的情况 */
            xsnprintf(dirname, MAX_PATH, "%c_DRIVE", fn[0]);
#endif
        memcpy(target_dir, ddir, dlen + 1);
        if (xstrlcat(target_dir, dirname, MAX_PATH) >= MAX_PATH)
            return 0;
    }

    /* 如果目标目录下已经存在此目录，说明违反了规则3 */
    /* 如果已经存在同名文件，则无法创建目录 */
    if (path_file_exists(target_dir) ||
        !create_directory(target_dir))
        return 0;

    /* 递归复制 */
    return _copy_directories(sdir, sdir, target_dir, func, arg);
}

/*
 *  查找并处理目录下的每个文件
 * 【参数】recursively：是否递归查找文件，regular_only：是否仅处理普通文件
 * 【返回值】1表示成功处理每个发现的文件，0表示查找有误或处理函数返回0
 */
int foreach_file(const char* dir, foreach_file_func_t func, int recursively, int regular_only, void *arg)
{
    struct walk_dir_context* ctx = NULL;
    char buf[MAX_PATH];

    ctx = walk_dir_begin(dir);
    if (!ctx)
        return 0;

    do {
        if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
            continue;
        else if (likely(walk_entry_path(ctx, buf, MAX_PATH))) {
            if (walk_entry_is_dir(ctx)){                // 目录
                if (recursively){
                    if (!foreach_file(buf, func, recursively, regular_only, arg))
                        WALK_END_RETURN_0;
                }
            } else {                                    // 文件
                if (!regular_only || walk_entry_is_regular(ctx)) {
                    if (!(*func)(buf, arg))
                        WALK_END_RETURN_0;
                }
            }
        } else
            WALK_END_RETURN_0;
    } while (walk_dir_next(ctx));

    walk_dir_end(ctx);

    return 1;
}

/*
 *  查找并处理目录下的每个目录
 * 【返回值】1表示成功处理每个发现的目录，0表示查找有误或处理函数返回0
 */
int foreach_dir(const char* dir, foreach_dir_func_t func, void *arg)
{
    struct walk_dir_context* ctx = NULL;
    char buf[MAX_PATH];

    ctx = walk_dir_begin(dir);
    if (!ctx)
        return 0;

    do {
        if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
            continue;
        else if (walk_entry_is_dir(ctx)) {                // 目录
            if (walk_entry_path(ctx, buf, MAX_PATH)) {
                if (!(*func)(buf, arg))
                    WALK_END_RETURN_0;
            } else
                WALK_END_RETURN_0;
        }
    } while (walk_dir_next(ctx));

    walk_dir_end(ctx);

    return 1;
}

/* 删除文件 */
int delete_file(const char *path)
{
    if (!_path_valid(path, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
    wchar_t wpath[MAX_PATH];
    if (!UTF82UNI(path, wpath, MAX_PATH))
        return 0;

    return DeleteFileW(wpath);
    }
#else
    return DeleteFileA(path);
#endif
#else
    return !unlink(path);
#endif
}

/* 复制文件 */
int copy_file(const char *exists, const char *newfile, int overwritten)
{
    if (!_path_valid(exists, 0) || !_path_valid(newfile, 0))
        return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
    wchar_t wexist[MAX_PATH], wnew[MAX_PATH];
    if (!UTF82UNI(exists, wexist, MAX_PATH) || !(UTF82UNI(newfile, wnew, MAX_PATH)))
        return 0;

    return CopyFileW(wexist, wnew, !overwritten);
    }
#else
    return CopyFileA(exists, newfile, !overwritten);
#endif
#else
    int fd1,fd2;
    size_t size;
    char buf[4096];
    STAT_STRUCT file;

    if (path_file_exists(newfile) && !overwritten)
        return 0;

    if(stat(exists,&file) == -1)
        return 0;

    if((fd1 = HANDLE_FAILURE(open(exists, O_RDONLY))) == -1)
        return 0;

    if((fd2 = HANDLE_FAILURE(creat(newfile,file.st_mode)))==-1)
    {
        HANDLE_FAILURE(close(fd1));
        return 0;
    }

    while((size = HANDLE_FAILURE(read(fd1,buf,4096))) > 0)
    {
        if (HANDLE_FAILURE(write(fd2,buf,size)) != size)
        {
            HANDLE_FAILURE(close(fd1));
            HANDLE_FAILURE(close(fd2));
            return 0;
        }
    }

    HANDLE_FAILURE(close(fd1));
    HANDLE_FAILURE(close(fd2));
    return 1;
#endif
}

/*
 * 移动文件 (不能移动目录)
 * 如果两路径位于同一文件系统中，则仅对文件重命名并立即返回；
 * 否则，将源文件拷贝到新文件位置，并删除源文件。
 * 注意：1、新文件名所在的目录必须存在。
 * 2、如果源文件和目标文件相同，将成功返回
 * TODO:如果exists和newfile是指向同一个文件的硬链接如何处理
 */
int move_file(const char* exists, const char* newfile, int overwritten)
{
    if (!_path_valid(exists, 0) || !_path_valid(newfile, 0))
        return 0;

    if (!path_is_file(exists) || path_is_directory(newfile))
        return 0;

    if (path_is_file(newfile))
    {
        if (overwritten)
            IGNORE_RESULT(delete_file(newfile));
        else
            return 0;
    }

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
    wchar_t wexist[MAX_PATH], wnew[MAX_PATH];
    if (!UTF82UNI(exists, wexist, MAX_PATH) || !(UTF82UNI(newfile, wnew, MAX_PATH)))
        return 0;

    return MoveFileW(wexist, wnew);
    }
#else
    return MoveFileA(exists, newfile);
#endif
#else //Linux
    if (rename(exists, newfile))
    {
        /* 位于不同文件系统中 */
        if (errno == EXDEV && copy_file(exists, newfile, 0))
        {
            IGNORE_RESULT(delete_file(exists));
            return 1;
        }
        else
            return 0;
    }

    return 1;
#endif
}

/* 获取文件长度 */
int64_t file_size(const char* path)
{
#ifdef OS_WIN
    HANDLE handle;
    LARGE_INTEGER li;
#else
    struct stat st;
#endif

    if (!_path_valid(path, 0))
        return -1;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
        wchar_t wpath[MAX_PATH];
        if (!UTF82UNI(path, wpath, MAX_PATH))
            return -1;
        handle = CreateFileW(wpath, FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    }
#else /* MBCS */
    handle = CreateFileA(path, FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
#endif
    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    if (!GetFileSizeEx(handle, &li)) {
        CloseHandle(handle);
        return -1;
    }

    CloseHandle(handle);
    return li.QuadPart;
#else
    if (stat(path, &st))
        return -1;

    return st.st_size;
#endif
}

/* 获取文件或目录所在文件系统磁盘区块的大小 */
int get_file_block_size(const char *path)
{
#ifdef OS_WIN
        DWORD SectorsPerCluster;
        DWORD BytesPerSector;
        DWORD NumberOfFreeClusters;
        DWORD TotalNumberOfClusters;
        char drive[4];

        /* 检查是否是绝对路径 */
        if (!_path_valid(path, 1))
            return 0;

        strncpy(drive, path, 3);
        drive[3] = '\0';

        /* 获取磁盘分区簇/块大小 */
        if (!GetDiskFreeSpaceA(drive, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters))
            return 0;

        return SectorsPerCluster * BytesPerSector;
#else
        struct stat st;
        if (stat(path, &st))
            return 0;

        return st.st_blksize;
#endif
}

/* 获取文件实际占用磁盘空间的大小 */
/* 参数file必须是绝对路径 */
/* 在Linux下，目录文件会也会占用磁盘块，而Windows下不占用磁盘空间 */
/* 空文件（长度为0）均不占用磁盘空间 */
int64_t get_file_disk_usage(const char *path)
{
#ifdef OS_WIN
    int blocksize;
    int64_t fsize;

    blocksize = get_file_block_size(path);
    if (blocksize <= 0)
        return 0;

    /* 目录不占空间 */
    fsize = file_size(path);
    if (fsize <= 0)
        return 0;

    return CALC_DISK_USAGE(fsize, blocksize);
#else
    struct stat st;
    if (stat(path, &st))
        return 0;

    return st.st_blocks * 512;
#endif
}

/* 获取可读性强的文件大小 */
/* 如3.51GB, 2.2MB, INT64_MAX 最高可达 8EB */
void file_size_readable(int64_t size, char* outbuf, int outlen)
{
    static const int64_t Byte = 1;
    static const int64_t KB = (int64_t)1<<10;
    static const int64_t MB = (int64_t)1<<20;
    static const int64_t GB = (int64_t)1<<30;
    static const int64_t TB = (int64_t)1<<40;
    static const int64_t PB = (int64_t)1<<50;
    static const int64_t EB = (int64_t)1<<60;
    /* ZB, YB, DB, NB... */

#define reach_size_unit(fmt, unit)    \
    else if (size >= unit)    \
    snprintf(outbuf, outlen, fmt, (long double)size / (long double)unit);

    if (size < 0)
        outbuf[0] = '\0';
    else if (size == 0)
        snprintf(outbuf, outlen, "0B");
    reach_size_unit("%.6" PRIlf " EB", EB)
    reach_size_unit("%.5" PRIlf " PB", PB)
    reach_size_unit("%.4" PRIlf " TB", TB)
    reach_size_unit("%.3" PRIlf " GB", GB)
    reach_size_unit("%.2" PRIlf " MB", MB)
    reach_size_unit("%.1" PRIlf " KB", KB)
    reach_size_unit("%.0" PRIlf " B",Byte)

#undef reach_size_unit
}


FILE* xfopen(const char* file, const char* mode)
{
    FILE *fp = NULL;

#if (defined OS_WIN) && (defined USE_UTF8_STR)
    wchar_t wfile[MAX_PATH];
    wchar_t wmode[5];

    if (!UTF82UNI(file, wfile, MAX_PATH) || !UTF82UNI(mode, wmode, countof(wmode)))
        return NULL;

    if ((fp = _wfopen(wfile, wmode)))
        g_opened_files++;

    return fp;
#else
    if ((fp = fopen(file, mode)))
        g_opened_files++;
    return fp;
#endif
}

/* 关闭文件 */
void xfclose(FILE* fp)
{
    if (!fp)
    {
        ASSERT(!"xfclose fp is null");
        return;
    }

    g_opened_files--;
    fclose(fp);
}

size_t xfread(FILE *fp, int separator, size_t max_bytes,
                char **lineptr, size_t *n)
{
    char *origptr, *mallptr;
    size_t count, nm = 1024;
    int ch;

    origptr = *lineptr;
    mallptr = NULL;
    count = 0;

    while ((ch = getc(fp)) != EOF)
    {
        if (origptr && count < *n - 1)    // -1 to reserve space for '\0'
            origptr[count++] = ch;
        else if (origptr && !mallptr)    // count == n - 1
        {
            if (*n > SIZE_T_MAX / 2) {
                return 0;
            }

            nm = *n<<1;
            mallptr = (char*)xmalloc(nm);
            memcpy(mallptr, origptr, count);
            mallptr[count++] = ch;
        }
        else if (mallptr)
        {
            if (count >= nm - 1)
            {
                if (nm > SIZE_T_MAX / 2) {
                    xfree(mallptr);
                    return 0;
                }
                nm <<=1;
                mallptr = (char*)xrealloc(mallptr, nm);
            }
            mallptr[count++] = ch;
        }
        else
        {
            mallptr = (char*)xmalloc(nm);
            mallptr[count++] = ch;
        }

        // 遇到分隔符或达到最多可以读取的字节数
        if (ch == separator || --max_bytes == 0)
            break;
    }

    // 结束符
    if (mallptr)
        mallptr[count] = '\0';
    else if (origptr && *n > 0)
        origptr[count] = '\0';

    // 设置输出
    if (mallptr)
        *lineptr = mallptr;
    *n = count;

    return count;
}

size_t get_line(FILE *fp, char **lineptr, size_t *n)
{
    return xfread(fp, '\n', 0, lineptr, n);
}

/* 从文件中读取每个分隔符块进行操作 */
int    foreach_block(FILE* fp, foreach_block_cb func, int delim, void *arg)
{
    char buf[1024], *block;
    size_t len, nblock;
    int ret;

    block = buf;
    len = sizeof(buf);
    nblock = 0;

    while (xfread(fp, delim, 0, &block, &len) > 0)
    {
        ret = (*func)(block, len, nblock, arg);

        if (block != buf)
            xfree(block);

        if (!ret)
            return 0;

        block = buf;
        len = sizeof(buf);
        nblock++;
    }

    return 1;
}

/* 从文件流中读入每一行并进行操作 */
int    foreach_line(FILE* fp, foreach_line_cb func, void *arg)
{
    return foreach_block(fp, func, '\n', arg);
}

/* 将文件内容读取到内存中 */
struct file_mem* read_file_mem(const char* file, size_t max_size)
{
    struct file_mem *fm;
    FILE *fp;
    int64_t length;
    size_t readed;

    if (!_path_valid(file, 0))
        return NULL;

    /* 获取文件大小 */
    length = file_size(file);
    if (length < 0 || length > (1<<30) ||
        (max_size > 0 && length > (int64_t)max_size))
        return NULL;

    /* 打开读取文件 */
    fp = xfopen(file, "rb");
    if (!fp)
        return NULL;

    fm = XMALLOC(struct file_mem);
    fm->length = (size_t)length;
    if (fm->length == 0)
    {
        fm->content = xstrdup("");
        xfclose(fp);
        return fm;
    }

    /* 分配内存并读入 */
    fm->content = (char*)xmalloc(fm->length + 1);

    readed = fread(fm->content, 1, fm->length, fp);
    if (readed != fm->length) {
        xfree(fm->content);
        xfree(fm);
        xfclose(fp);
        return NULL;
    }

    fm->content[fm->length] = '\0';
    xfclose(fp);

    return fm;
}

/* 释放读取到内存中的文件 */
void free_file_mem(struct file_mem *fm)
{
    if (fm)
    {
        if (fm->content)
            xfree(fm->content);
        xfree(fm);
    }
}

/* 将内存中的数据写入文件 */
int write_mem_file(const char* file, const void *data, size_t len)
{
    FILE *fp;

    if (!data)
        return 0;

    if (!(fp = xfopen(file, "wb")))
        return 0;

    if (fwrite(data, 1, len, fp) != len)
    {
        xfclose(fp);
        return 0;
    }

    xfclose(fp);
    return 1;
}

/* 将内存中的数据写入文件 */
/* 该函数不会直接打开已存在的文件进行写入，因为这将会截断之前的文件 */
/* 首先在文件所在的文件系统中创建一临时文件，待写入临时文件成功后再重命名为指定文件 */
int write_mem_file_safe(const char *file, const void *data, size_t len)
{
    char tmpdir[MAX_PATH], tmpfile[MAX_PATH];

    if (!path_find_directory(file, tmpdir, MAX_PATH))
        return 0;

    if (!get_temp_file_under(tmpdir, "write_safe", tmpfile, MAX_PATH))
        return 0;

    if (!write_mem_file(tmpfile, data, len) ||
        !move_file(file, tmpfile, 1)) {
        IGNORE_RESULT(delete_file(tmpfile));
        return 0;
    }

    return 1;
}

/* 获取文件系统的使用状态 */
/* 参数path必须是绝对路径 */
int get_fs_usage(const char* path, struct fs_usage *fsp)
{
    if (!_path_valid(path, 1) || !fsp)
        return 0;

#ifdef OS_WIN
    {
        char drive[4];
        ULARGE_INTEGER ulavail;
        ULARGE_INTEGER ultotal;
        ULARGE_INTEGER ulfree;

        strncpy(drive, path, 3);
        drive[3] = '\0';
        if (!GetDiskFreeSpaceExA(drive, &ulavail, &ultotal, &ulfree))
            return 0;

        fsp->fsu_total = ultotal.QuadPart;
        fsp->fsu_free = ulfree.QuadPart;
        fsp->fsu_avail = ulavail.QuadPart;
        fsp->fsu_files = 0;
        fsp->fsu_ffree = 0;
    }
#else
    {
        struct statfs stf;
        if (statfs(path, &stf) < 0)
            return 0;

        fsp->fsu_total = stf.f_bsize * stf.f_blocks;
        fsp->fsu_free = stf.f_bsize * stf.f_bfree;
        fsp->fsu_avail = stf.f_bsize * stf.f_bavail;
        fsp->fsu_files = stf.f_files;
        fsp->fsu_ffree = stf.f_ffree;
    }
#endif

    return 1;
}

/* 获取当前进程文件的路径(argv[0]) */
/* 失败返回空字符串 */
const char* get_execute_path()
{
    static char path[MAX_PATH];
    static int init = 0;

    if (!init)
    {
#ifdef OS_WIN
#ifdef USE_UTF8_STR
        wchar_t wpath[MAX_PATH];
        GetModuleFileNameW(NULL, wpath, MAX_PATH);
        if(!UNI2UTF8(wpath, path, MAX_PATH))
            path[0] = '\0';
#else  /* MBCS */
        GetModuleFileNameA(NULL, path, MAX_PATH);
#endif /* USE_UTF8_STR */
#elif defined(OS_MACOSX)
    if (proc_pidpath (getpid(), path, sizeof(path)) <= 0)
        path[0] = '\0';
#else  /* OS_POSIX */
        char proc[32];
        ssize_t n;
        sprintf(proc, "/proc/%d/exe", getpid());
        n = readlink(proc, path, MAX_PATH);
        if (n > 0 && n < MAX_PATH)
            path[n] = '\0';
        else
            path[0] = '\0';
#endif /* OS_WIN */
        init = 1;
    }

    return path;
}

/* 获取进程的文件名(包括扩展名) */
/* 失败返回空字符串 */
const char* get_execute_name()
{
    const char *path, *slash_end;

    path = get_execute_path();
    if (!path[0])
        return path;

    slash_end = strrpsep(path);
    if (slash_end)
        return slash_end+1;

    return path + strlen(path);
}

/* 常用目录 */
/* 获取进程起始目录（可执行文件所在目录）,以路径分隔符结尾 */
/* 返回值：成功获取返回指向路径字符串，失败返回空字符串 */
const char* get_execute_dir()
{
    static char dir[MAX_PATH];
    static int init = 0;

    if (!init)
    {
        const char* path = get_execute_path();
        if (path[0])
        {
            const char *slash_end = strrpsep(path);
            if(slash_end)
                strncpy(dir, path, slash_end - path + 1);
            else
                dir[0] = '\0';
        }
        else
            dir[0] = '\0';

        init = 1;
    }

    return dir;
}

/* 获取当前运行模块的路径 */
/* 即可执行文件或动态链接库的路径 */
const char* get_module_path()
{
    static char path[MAX_PATH];

#ifdef OS_WIN
    HMODULE instance = NULL;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (char*)get_module_path,
        &instance);

#ifdef USE_UTF8_STR
    {
        wchar_t wpath[MAX_PATH];
        if (!GetModuleFileNameW(instance, wpath, MAX_PATH) ||
            !UNI2UTF8(wpath, path, MAX_PATH))
            path[0] = '\0';
    }
#else
    if (!GetModuleFileNameA(instance, path, MAX_PATH))
        path[0] = '\0';
#endif
#else
    NOT_IMPLEMENTED();
    path[0] = '\0';
#endif
    return path;
}

/* 获取进程当前工作目录 */
/* 返回值：成功获取返回指向路径字符串，失败返回空字符串 */
/* 注：因为当前工作目录可能改变，因此不能使用一次初始化 */
const char* get_current_dir()
{
    static char path[MAX_PATH];

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    wchar_t wpath[MAX_PATH];
    if (!GetCurrentDirectoryW(MAX_PATH, wpath)
        || !UNI2UTF8(wpath, path, MAX_PATH))
        path[0] = '\0';
#else /* MBCS */
    if (!GetCurrentDirectoryA(MAX_PATH, path))
        path[0] = '\0';
#endif /* USE_UTF8_STR */
#else /* OS_POSIX */
    if (!getcwd(path, MAX_PATH))
        path[0] = '\0';
#endif /* OS_WIN */

    if (!_end_with_slash(path, path, sizeof(path)))
        path[0] = '\0';

    return path;
}

/* 设置进程当前的工作目录 */
int set_current_dir(const char *dir)
{
#ifdef OS_WIN
#ifdef USE_UTF8_STR
    wchar_t wpath[MAX_PATH];
    if (!UTF82UNI(dir, wpath, MAX_PATH)
        || !SetCurrentDirectoryW(wpath))
        return 0;
#else /* MBCS */
    if (!SetCurrentDirectoryA(dir))
        return 0;
#endif /* USE_UTF8_STR */
#else /* OS_POSIX */
    if (chdir(dir))
        return 0;
#endif /* OS_WIN */

    return 1;
}

#ifdef OS_WIN
/* 获取Windows下特殊目录的路径, path的长度应至少为MAX_PATH */
static int GetShellSpecialFolder(int key, char* path) {
#ifdef USE_UTF8_STR
    wchar_t wpath[MAX_PATH];
    if (!SHGetSpecialFolderPathW(NULL, wpath, key, 0) ||
        !UNI2UTF8(wpath, path, MAX_PATH))
        return 0;
#else
    if (!SHGetSpecialFolderPathA(NULL, path, key, 0))
        return 0;
#endif
    return 1;
}
#endif

/* 获取当前用户的主目录，以路径分隔符结尾 */
/* 返回值：获取成功返回路径字符串，失败返回空字符串 */
const char*get_home_dir()
{
    static char path[MAX_PATH];

    if (_end_with_slash(get_env("HOME"), path, MAX_PATH))
        return path;

#ifdef OS_WIN
    {
        wchar_t wpath[MAX_PATH];
        if (!SHGetSpecialFolderPathW(NULL, wpath, CSIDL_MYDOCUMENTS, 0))
            goto failed;

#ifdef USE_UTF8_STR
        if (!UNI2UTF8(wpath, path, MAX_PATH))
#else /* MBCS */
        if (!UNI2MBCS(wpath, path, MAX_PATH))
#endif /* USE_UTF8_STR */
            goto failed;

        if (!path_find_directory(path, path, MAX_PATH))
            goto failed;

        return path;
    }

failed:
    path[0] = '\0';
    return path;

#else
    /* 一般POSIX系统都会设置HOME环境变量 */
    path[0] = '\0';
    return path;
#endif /* OS_WIN */
}

/* 获取程序的缓存文件目录 */
/* Windows下位于C:\Users\xxx\AppData\Local\PACKAGE_NAME\ */
/* Linux下位于~/.PACKAGE_NAME，同 get_config_dir() */
const char* get_app_data_dir()
{
    static char path[MAX_PATH];

#ifdef OS_WIN
    if (!GetShellSpecialFolder(CSIDL_LOCAL_APPDATA, path))
        goto failed;
    xstrlcat(path, PATH_SEP_STR, MAX_PATH);
#else /* OS_POSIX */
    if (!xstrlcpy(path, get_home_dir(), MAX_PATH))
        goto failed;
    xstrlcat(path, ".", MAX_PATH);
#endif
    if (xstrlcat(path, g_product_name, MAX_PATH) >= MAX_PATH ||
        xstrlcat(path, PATH_SEP_STR, MAX_PATH) >= MAX_PATH)
        goto failed;

    return path;

failed:
    path[0] = '\0';
    return path;
}

/* 获取应用程序临时目录 */
/* 在Windows下返回如 C:\Windows\Temp\SafeSite\ */
/* 在Linux下返回如 /tmp/SafeSite/ */
const char *get_temp_dir()
{
    static char path[MAX_PATH];
    static int init = 0;

    if (!init)
    {
        size_t len;
#ifdef OS_WIN
#ifdef USE_UTF8_STR
        wchar_t wpath[MAX_PATH];
        if (!GetTempPathW(MAX_PATH, wpath)
            || !UNI2UTF8(wpath, path, MAX_PATH))
            path[0] = '\0';
#else /* MBCS */
        if (!GetTempPathA(MAX_PATH, path))
            path[0] = '\0';
#endif /* USE_UTF8_STR */
        len = _end_with_slash(path, path, MAX_PATH);
        if (!len)
            path[0] = '\0';
#else /* OS_POSIX */
        strcpy(path, "/tmp/");
        len = 5;
#endif /* OS_WIN */

        if (len > 0 && (len + strlen(g_product_name) < MAX_PATH - 2))
        {
            strcpy(path + len, g_product_name);
            strcat(path, PATH_SEP_STR);
            if (path_is_file(path))
                IGNORE_RESULT(delete_file(path));
            if (!path_is_directory(path) &&
                !create_directory(path))
                path[0] = '\0';
        }

        init = 1;
    }

    return path;
}

/*
 * 获取指定目录下可用的临时文件路径
 *
 * 如果指定的临时目录不存在，将创建此目录。
 * 如果prefix为NULL或为空字符串，将使用g_product_name作为前缀
 *
 * 注1：即使不以路径分隔符结尾，tmpdir的最后一部分将被视为目录名
 * 注2：输出缓冲区的长度应至少为MAX_PATH
 * 注3：此函数仅保证在调用时该临时文件不存在
 */

int get_temp_file_under(const char* tmpdir, const char *prefix,
    char *outbuf, size_t outlen)
{
    char tempdir_use[MAX_PATH];
    const char *prefix_use = g_product_name;
    size_t dlen;

    if (!outbuf || outlen < MAX_PATH)
        return 0;

    dlen = _end_with_slash(tmpdir, tempdir_use, MAX_PATH);
    if (!dlen)
        return 0;

    if (!create_directories(tempdir_use))
        return 0;

    if (prefix && prefix[0])
        prefix_use = prefix;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
    {
        wchar_t wtmpdir[MAX_PATH], wtmpfile[MAX_PATH], wprefix[128];
        if (!UTF82UNI(tempdir_use, wtmpdir, MAX_PATH)
            || !UTF82UNI(prefix_use, wprefix, sizeof(wprefix))
            || !GetTempFileNameW(wtmpdir, wprefix, 0, wtmpfile)
            || !UNI2UTF8(wtmpfile, outbuf, outlen))
            return 0;
        return 1;
    }
#else /* MBCS */
    if (!GetTempFileNameA(tempdir_use, prefix_use, 0, tmpfile))
        return 0;
    return 1;
#endif
#else /* POSIX */
    if (xstrlcpy(outbuf, tempdir_use, outlen) >= outlen ||
        xstrlcat(outbuf, prefix_use, outlen) >= outlen ||
        xstrlcat(outbuf, ".XXXXXXXX", outlen) >= outlen)
        return 0;

    if (!mktemp(outbuf))
        return 0;

    return 1;
#endif
}

/* 在默认临时目录下获取临时文件路径 */
/* 非线程安全，失败返回空字符串 */
const char* get_temp_file(const char* prefix)
{
    static char tmpfile[MAX_PATH];

    if (!get_temp_file_under(get_temp_dir(), prefix, tmpfile, MAX_PATH))
        tmpfile[0] = '\0';

    return tmpfile;
}

/************************************************************************/
/*                         Time & Timer 时间和定时器                     */
/************************************************************************/

static
const char*fmttime (time_t t, const char *fmt)
{
  static char output[32];
  struct tm *tm = localtime(&t);
  if (!tm)
    return NULL;

  if (!strftime(output, sizeof(output), fmt, tm))
    return NULL;

  return output;
}

/* 返回一个 HH:MM:SS 样式的时间字符串，不可重入 */
const char *time_str (time_t t)
{
  return fmttime(t, "%H:%M:%S");
}

/* 返回一个 YY-MM-DD 样式的日期字符串，不可重入 */
const char *date_str (time_t t)
{
    return fmttime(t, "%Y-%m-%d");
}

/* 返回一个 YY-MM-DD HH:MM:SS 样式的时间日期字符串，不可重入 */
const char *datetime_str (time_t t)
{
  return fmttime(t, "%Y-%m-%d %H:%M:%S");
}

/* 返回一个 YYMMDDHHMMSS 样式的时间戳 */
const char* timestamp_str(time_t t)
{
    return fmttime(t, "%Y%m%d%H%M%S");
}

/* 解析时间日期字符串 */
time_t parse_datetime(const char* datetime_str) {
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int month, day, year, hour, minute, second;
    struct tm t;
    int rv;

    if (!datetime_str || !datetime_str[0])
        return 0;

    if (xisdigit(datetime_str[0])) {
        rv = sscanf(datetime_str, "%d-%d-%d %d:%d:%d",
            &year, &month, &day, &hour, &minute, &second);
        if (rv != 6) {
            rv = sscanf(datetime_str, "%d/%d/%d %d:%d:%d",
                &year, &month, &day, &hour, &minute, &second);
            if (rv != 6)
                return 0;
        }
    } else {
        char s_month[5];
        rv = sscanf(datetime_str, "%s %d %d %d:%d:%d",
            s_month, &day, &year, &hour, &minute, &second);
        if (rv != 6)
            return 0;

        month = (int)(strcasestr(month_names, s_month) - month_names) / 3;
        if (!month)
            return 0;
    }

    memset(&t, 0, sizeof(t));

    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    t.tm_isdst = -1;

    return mktime(&t);
}

struct time_units{
    char year[TIME_UNIT_MAX];
    char month[TIME_UNIT_MAX];
    char day[TIME_UNIT_MAX];
    char hour[TIME_UNIT_MAX];
    char minute[TIME_UNIT_MAX];
    char second[TIME_UNIT_MAX];
};

static struct time_units tu_locale_single;
static struct time_units tu_locale_plural;
static int tu_single_init = 0;
static int tu_plural_init = 0;

/*
 * 设置时间单位的单、复数形式
 * time_span_readable 将依据此单位构造国际化的字符串
 * plural为不为0表示设置复数形式
 */
void time_unit_localize(const char* year, const char* month, const char* day,
                        const char* hour, const char* minute, const char* second,
                        int plural)
{
    struct time_units *intl;

    ASSERT(year && month && day && hour && minute && second);

    if (plural){
        intl = &tu_locale_plural;
        tu_plural_init = 1;
    }else{
        intl = &tu_locale_single;
        tu_single_init = 1;
    }

    memset(intl, 0, sizeof(struct time_units));
    xstrlcpy(intl->year, year, TIME_UNIT_MAX);
    xstrlcpy(intl->month, month, TIME_UNIT_MAX);
    xstrlcpy(intl->day, day, TIME_UNIT_MAX);
    xstrlcpy(intl->hour, hour, TIME_UNIT_MAX);
    xstrlcpy(intl->minute, minute, TIME_UNIT_MAX);
    xstrlcpy(intl->second, second, TIME_UNIT_MAX);
}

/* 返回一个可读性强的时间差(一月作为30天)，如2小时15分，3天等 */
void time_span_readable(int64_t diff, int cutoff, char* outbuf, size_t outlen)
{
    static struct time_units tu_en_single = {" year", " month", " day", " hour", " minute", " second"};
    static struct time_units tu_en_plural = {" years"," months"," days"," hours"," minutes"," seconds"};

    static const int64_t secs_per_second = 1;
    static const int64_t secs_per_minute = 60;
    static const int64_t secs_per_hour = 60 * 60;
    static const int64_t secs_per_day = 60 * 60 * 24;
    static const int64_t secs_per_month = 60 * 60 * 24 * 30;
    static const int64_t secs_per_year = 60 * 60 * 24 * 30 * 12;

    int pos, cut;
    int64_t nyear,nmonth,nday,nhour,nminute,nsecond;
    const char *year_s, *month_s, *day_s, *hour_s, *minute_s, *second_s;
    struct time_units *units_single, *units_plural;

    /* 时间单位(单数形式) */
    if (tu_single_init)
        units_single = &tu_locale_single;
    else
        units_single = &tu_en_single;

    /* 时间单位(复数形式) */
    if (tu_plural_init)
        units_plural = &tu_locale_plural;
    else if (tu_single_init)
        units_plural = &tu_locale_single;
    else
        units_plural = &tu_en_plural;

    memset(outbuf, 0, outlen);
    if (!diff)
    {
        outbuf[0] = '0';
        return;
    }

    pos = 0;
    cut = cutoff > 0 ? cutoff : 10;

#define REACH_TIME_UNIT(unit)            \
    else if (diff >= secs_per_##unit){    \
    n##unit = diff / secs_per_##unit;    \
    diff %= secs_per_##unit;            \
    unit##_s = n##unit > 1 ? units_plural->unit : units_single->unit;\
    if (pos + num_bits(n##unit) + strlen(unit##_s) + 1 > outlen) break;    \
    pos += sprintf(outbuf + pos, "%d%s ", (int)n##unit, unit##_s);\
    }

    while (diff > 0 && cut > 0)
    {
        if (0);
        REACH_TIME_UNIT(year)
        REACH_TIME_UNIT(month)
        REACH_TIME_UNIT(day)
        REACH_TIME_UNIT(hour)
        REACH_TIME_UNIT(minute)
        REACH_TIME_UNIT(second)

        cut--;
    }

#undef REACH_TIME_UNIT
}

/* 睡眠指定秒数 */
void ssleep(int seconds)
{
#ifdef OS_POSIX
    sleep(seconds);
#else
    Sleep(seconds*1000);
#endif
}

/* 睡眠指定毫秒数 */
void msleep(int milliseconds)
{
#ifdef OS_POSIX
    int seconds;

    if (milliseconds >= 1000)
    {
        seconds = milliseconds / 1000;
        milliseconds %= 1000;
        sleep(seconds);
    }

    if (milliseconds > 0)
        usleep(milliseconds * 1000);

#else
    Sleep(milliseconds);
#endif
}

/* 微秒级计时器 */
void time_meter_start(time_meter_t *meter)
{
#ifdef OS_POSIX
    gettimeofday(&meter->start, NULL);
#else
    QueryPerformanceCounter(&meter->start);
#endif
}

void time_meter_stop(time_meter_t *meter)
{
#ifdef OS_POSIX
    gettimeofday(&meter->stop, NULL);
#else
    QueryPerformanceCounter(&meter->stop);
#endif
}

/* 从开始到停止经历的微秒数 */
double time_meter_elapsed_us(time_meter_t *meter)
{
#ifdef OS_POSIX
    return (meter->stop.tv_sec - meter->start.tv_sec) * 1000000.0
        + (meter->stop.tv_usec - meter->start.tv_usec);
#else
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (double)(meter->stop.QuadPart - meter->start.QuadPart) * 1000000.0 / (double)freq.QuadPart;
#endif
}

/* 从开始到停止经历的毫秒数 */
double time_meter_elapsed_ms(time_meter_t *meter)
{
    return time_meter_elapsed_us(meter) / 1000;
}

/* 从开始到停止经历的秒数 */
double time_meter_elapsed_s(time_meter_t *meter)
{
    return time_meter_elapsed_us(meter) / 1000000;
}

/* 从开始到现在经历的微妙数 */
double time_meter_elapsed_us_till_now(time_meter_t *meter)
{
#ifdef OS_POSIX
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - meter->start.tv_sec) * 1000000.0 \
        + (now.tv_usec - meter->start.tv_usec);
#else
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    return (double)(now.QuadPart - meter->start.QuadPart) * 1000000.0 / (double)freq.QuadPart;
#endif
}

double time_meter_elapsed_ms_till_now(time_meter_t *meter)
{
    return time_meter_elapsed_us_till_now(meter) / 1000;
}

double time_meter_elapsed_s_till_now(time_meter_t *meter)
{
    return time_meter_elapsed_us_till_now(meter) / 1000000;
}

/* 获取基于时间的伪随机数 */
int get_random()
{
#ifdef OS_POSIX
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + tv.tv_usec) % INT_MAX;
#else
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (int)(li.QuadPart % INT_MAX);
#endif
}

/************************************************************************/
/*                         Charset 字符编码类函数                        */
/************************************************************************/

/* UTF-7, UTF-8, UTF-16, UTF-32 编码转换 */
#include "deps/ConvertUTF-inl.h"

#if defined(OS_POSIX) && !defined(OS_ANDROID)
#include <langinfo.h>
#endif

//7位标准ASCII码
int is_ascii(const void* pBuffer, size_t size)
{
    int IsASCII = 1;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;

    while (start < end)
    {
        if (*start < 0x80)
            start++;
        else
        {
            IsASCII = 0;
            break;
        }
    }

    return IsASCII;
}

// 是否是UTF-8编码
//
// 128个US-ASCII字符只需一个字节编码（Unicode范围由U+0000至U+007F）。
// 带有附加符号的拉丁文、希腊文、西里尔字母、亚美尼亚语、希伯来文、阿拉伯文、叙利亚文及它拿字母则需要二个字节编码。
// 其他基本多文种平面（BMP）中的字符（这包含了大部分常用字）使用三个字节编码。
// 其他极少使用的Unicode 辅助平面的字符使用四字节编码。
//
// UCS-4编码    与 UTF-8字节流 对应关系如下：
// U+00000000 – U+0000007F [1] 0xxxxxxx
// U+00000080 – U+000007FF [2] 110xxxxx 10xxxxxx
// U+00000800 – U+0000FFFF [3] 1110xxxx 10xxxxxx 10xxxxxx
// U+00010000 – U+001FFFFF [4] 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// U+00200000 – U+03FFFFFF [5] 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U+04000000 – U+7FFFFFFF [6] 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
int is_utf8(const void* pBuffer, size_t size)
{
    int IsUTF8 = 1;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;
    while (start < end)
    {
        if (*start < 0x80)                /* 1字节 */
        {
            start++;
        }
        else if (*start < (0xC0))
        {
            IsUTF8 = 0;
            break;
        }
        else if (*start < (0xE0))        /* 2字节 */
        {
            if (start >= end - 1)
                break;
            if ((start[1] & (0xC0)) != 0x80)
            {
                IsUTF8 = 0;
                break;
            }
            start += 2;
        }
        else if (*start < (0xF0))        /* 3字节 */
        {
            if (start >= end - 2)
                break;
            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
            {
                IsUTF8 = 0;
                break;
            }
            start += 3;
        }
        else if (*start < (0xF8))        /* 4字节 */
        {
            if (start >= end - 3)
                break;
            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80 || (start[3] & (0xC0)) != 0x80)
            {
                IsUTF8 = 0;
                break;
            }
            start += 4;
        }
        else
        {
            IsUTF8 = 0;
            break;
        }
    }
    return IsUTF8;
}

// 是否是GB2312编码
int is_gb2312(const void* pBuffer, size_t size)
{
    int IsGB2312 = 1;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;

    while (start < end)
    {
        if (*start < 0x80)            // 0x00~0x7F ASCII码
            start++;
        else if (*start < 0xA1)        // 0x80~0xA0 空码
        {
            IsGB2312 = 0;
            break;
        }
        else if (*start < 0xAA)        // 0xA1~0xA9 字符部分
        {
            if (start >= end -1)
                break;

            // 低字节范围 0xA1~0xFE
            if (start[1] < 0xA1 || start[1] > 0xFE)
            {
                IsGB2312 = 0;
                break;
            }

            start += 2;
        }
        else if (*start < 0xB0)        // 0xAA~0xAF 空码
        {
            IsGB2312 = 0;
            break;
        }
        else if (*start < 0xF8)        // 0xB0~0xF7 是对GB2312汉字表的扩充
        {
            if (start >= end -1)
                break;

            // 低字节范围 0xA1~0xFE(剔除0x7F)
            if (start[1] < 0xA1 || start[1] > 0xFE)
            {
                IsGB2312 = 0;
                break;
            }

            start += 2;
        }
        else                        // 0xFF 空码
        {
            IsGB2312 = 0;
            break;
        }
    }

    return IsGB2312;
}

// 是否是GBK编码
int is_gbk(const void* pBuffer, size_t size)
{
    int IsGBK = 1;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;

    while (start < end)
    {
        if (*start < 0x80)            // 0x00~0x7F ASCII码
            start++;
        else if (*start < 0x81)        // 0x80 空码
        {
            IsGBK = 0;
            break;
        }
        else if (*start < 0xA1)        // 0x81~0xA0 字符表前的不常用汉字
        {
            if (start >= end -1)
                break;

            // 低字节范围 0x40~0xFE (剔除0x7F)
            if (start[1] < 0x40 || start[1] > 0xFE || start[1] == 0x7F)
            {
                IsGBK = 0;
                break;
            }

            start += 2;
        }
        else if (*start < 0xAA)        // 0xA1~0xA9 字符部分
        {
            if (start >= end -1)
                break;

            if (*start < 0xA8)        // 0xA1~0xA7 和GB2312一样
            {
                // 低字节范围 0xA1~0xFE
                if (start[1] < 0xA1 || start[1] > 0xFE)
                {
                    IsGBK = 0;
                    break;
                }
            }
            else                    // 0xA8~0xA9 扩充了GB2312符号
            {
                // 低字节范围 0x40~0xEF(剔除0x7F) (不是很精确)
                if (start[1] < 0x40 || start[1] > 0xEF || start[1] == 0x7F)
                {
                    IsGBK = 0;
                    break;
                }
            }

            start += 2;
        }
        else if (*start < 0xB0)        // 0xAA~0xAF 是GB2312汉字表前新添加的生僻字
        {
            if (start >= end -1)
                break;

            // 低字节范围 0x40~0xA0(剔除0x7F)
            if (start[1] < 0x40 || start[1] > 0xA0 || start[1] == 0x7F)
            {
                IsGBK = 0;
                break;
            }

            start += 2;
        }
        else if (*start < 0xF8)        // 0xB0~0xF7 是对GB2312汉字表的扩充
        {
            if (start >= end -1)
                break;

            // 低字节范围 0x40~0xFE(剔除0x7F)
            if (start[1] < 0x40 || start[1] > 0xFE || start[1] == 0x7F)
            {
                IsGBK = 0;
                break;
            }

            start += 2;
        }
        else if (*start < 0xFF)        // 0xF8~0xFE 是GB2312汉字表后新添加的生僻字
        {
            if (start >= end -1)
                break;

            // 低字节范围 0x40~0xA0(剔除0x7F)
            if (start[1] < 0x40 || start[1] > 0xA0 || start[1] == 0x7F)
            {
                IsGBK = 0;
                break;
            }

            start += 2;
        }
        else                        // 0xFF 空码
        {
            IsGBK = 0;
            break;
        }
    }

    return IsGBK;
}

#define MATCH_CHARSET_RETURN(csfunc, csname)\
    if (is_##csfunc(str, size)){        \
    xstrlcpy(outbuf, csname, outlen);    \
    return 1;}

/* 获取字符串的准确字符集(ASCII,UTF-8,GB2312,GBK,GB18030) */
/* 如果can_ascii为0，则返回包含ASCII字符集的最小字符集 */
int get_charset(const void* str, size_t size, char *outbuf, size_t outlen, int can_ascii)
{
    if (can_ascii){
        MATCH_CHARSET_RETURN(ascii, "ASCII");
    }

    /* 每个汉字字符占3个字节的UTF-8字符串很容易被误认为GB系列编码，反之则不然 */
    /* 因此就首先检测其是否是UTF-8字符串 */
    MATCH_CHARSET_RETURN(utf8, "UTF-8");
    MATCH_CHARSET_RETURN(gb2312, "GB2312");
    MATCH_CHARSET_RETURN(gbk, "GBK");

    return 0;
}

#undef MATCH_CHARSET_RETURN

/* 检测文件字符集上下文信息 */
struct file_charset_info{
    uint ascii;
    uint utf8;
    uint gb2312;
    uint gbk;
    uint gb18030;
    uint unknown;

    uint max_line;
    uint count;
};

/* 检测文件每一行的字符集 */
static inline
int get_line_charset(char *line, size_t len, size_t nline, void *arg)
{
    struct file_charset_info *fci = (struct file_charset_info*)arg;

    if (is_ascii(line, len))
        fci->ascii++;
    else if (is_utf8(line, len))
        fci->utf8++;
    else if (is_gb2312(line, len))
        fci->gb2312++;
    else if (is_gbk(line, len))
        fci->gbk++;
    else
        fci->unknown++;

    /* 已检测行数 */
    fci->count++;

    /* 最多检测行数 */
    if (fci->max_line > 0 && nline + 2 > fci->max_line)
        return 0;

    return 1;
}

/*
 * @fname: 探测文件的字符集
 * @param: outbuf: 返回探测到的字符集编码
 * @param: outlen: 传入缓冲区长度（应至少为MAX_CHARSET）
 * @param: probability: 是此字符集的可能性(0~1.0, 0.95以上可确信)，可传NULL
 * @param: max_line: 最多探测的行数 (0表示检测整个文件)
 * @return: 1: 成功返回，0: 无法获取指定文件的字符集
 */
int get_file_charset(const char* file, char *outbuf, size_t outlen,
                    double* probability, int max_line)
{
    FILE *fp;
    struct file_charset_info* fci;

    if (!_path_valid(file, 0) || !outbuf)
        return 0;

    fp = xfopen(file, "rb");
    if (!fp)
        return 0;

    /* 文件是否有BOM头 */
    if (read_file_bom(fp, outbuf, outlen))
    {
        xfclose(fp);
        *probability = 1.0;
        return 1;
    }

    fci = XALLOCA(struct file_charset_info);
    memset(fci, 0, sizeof(struct file_charset_info));

    /* 最多检测行数 */
    fci->max_line = max_line > 0 ? max_line : 0;

    /* 探测每一行 */
    IGNORE_RESULT(foreach_line(fp, get_line_charset, fci));
    xfclose(fp);

    /* 无有效数据 */
    if (!fci->count)
        return 0;

    /* 确定字符集 */
    {
        /* 每种字符集的可能性 */
        double prob_ascii = (double)fci->ascii / (double)fci->count;
        double prob_utf8 =    (double)fci->utf8 / (double)fci->count;
        double prob_gb2312 = (double)fci->gb2312 / (double)fci->count;
        double prob_gbk =    (double)fci->gbk     / (double)fci->count;

        double prob_gbx = prob_gb2312 + prob_gbk;    /* GB系列总值 */

        char charset[MAX_CHARSET];
        size_t cslen;
        double prob;

        if (prob_utf8 && prob_utf8 > prob_gbx)
        {
            prob = prob_utf8;
            xstrlcpy(charset, "UTF-8", MAX_CHARSET);
        }
        else if (prob_gbx)
        {
            if (prob_gbk)
            {
                prob = prob_gbk + prob_gb2312 + prob_ascii;
                xstrlcpy(charset, "GBK", MAX_CHARSET);
            }
            else
            {
                prob = prob_gb2312 + prob_ascii;
                xstrlcpy(charset, "GB2312", MAX_CHARSET);
            }
        }
        else if (prob_ascii)
        {
            prob = prob_ascii;
            xstrlcpy(charset, "ASCII", MAX_CHARSET);
        }
        else
        {
            /* 字符集未知 */
            return 0;
        }

        /* 已经获取到文件字符集 */
        cslen = strlen(charset);
        if (outlen > cslen)
        {
            /* 成功返回 */
            if (probability)
                *probability = prob;
            strcpy(outbuf, charset);
            return 1;
        }
        else
        {
            /* 输出缓冲区太小 */
            return 0;
        }
    }

    NOT_REACHED();
    return 0;
}

/* 常用编码的文件BOM头 */
#define UTF8_BOM        "\xEF\xBB\xBF"
#define UTF16LE_BOM        "\xFF\xFE"
#define UTF16BE_BOM        "\xFE\xFF"
#define UTF32LE_BOM        "\xFF\xFE\x00\x00"
#define UTF32BE_BOM        "\x00\x00\xFE\xFF"
#define GB18030_BOM        "\x84\x31\x95\x33"

/*
 * @fname: 读取文件的BOM头
 * @param: fp: 刚打开的文件流
 * @outbuf: 返回读取到得BOM头
 * @return: 成功返回1，outbuf存放所代表的字符集
 *            否则返回0，并将文件流定位到原来的位置
 */
int read_file_bom(FILE *fp, char *outbuf, size_t outlen)
{
    unsigned char buf[10];
    size_t cslen, len = 0;
    int utf16le = 0, utf16be = 0;

    if (!fp || !outbuf)
        return 0;

#define FOUND_FILE_BOM(s) {            \
    cslen = strlen(s);                \
    if (outlen < cslen + 1){        \
    fseek(fp, -1*len, SEEK_CUR);    \
    return 0;                        \
    }                                \
    strcpy(outbuf, s);                \
    outbuf[cslen] = '\0';            \
    return 1;                        \
    }

    while(!feof(fp)) {
        fread((char*)buf + len, 1, 1, fp);
        len++;

        if (len == 1) {
            continue;
        } else if (len == 2) {
            if (!memcmp(UTF16LE_BOM, buf, len))
                utf16le = 1;
            else if (!memcmp(UTF16BE_BOM, buf, len))
                utf16be = 1;
        } else if (len == 3) {
            if (!memcmp(UTF8_BOM, buf, len))
                FOUND_FILE_BOM("UTF-8")
        } else if (len == 4) {
            if (!memcmp(UTF32LE_BOM, buf, len))
                FOUND_FILE_BOM("UTF-32LE")
            else if (!memcmp(UTF32BE_BOM, buf, len))
                FOUND_FILE_BOM("UTF-32BE")
            else if (!memcmp(GB18030_BOM, buf, len))
                FOUND_FILE_BOM("GB18030")
            else {
                if (utf16le) {
                    fseek(fp, (seek_off_t)-2, SEEK_CUR);
                    FOUND_FILE_BOM("UTF-16LE")
                } else if (utf16be) {
                    fseek(fp, (seek_off_t)-2, SEEK_CUR);
                    FOUND_FILE_BOM("UTF-16BE")
                } else
                    break;
            }
        } else {
            NOT_REACHED();
            break;
        }
    }

#undef FOUND_FILE_BOM

    fseek(fp, (seek_off_t)-1*len, SEEK_CUR);
    return 0;
}

/*
 * 写入文件的BOM头
 * @param: charset: 文件的编码字符集
 * @return: 成功写入字符集所对应的BOM头返回1;
 *            如果字符集没有相应BOM头返回0;
 */
int write_file_bom(FILE *fp, const char* charset)
{
    byte bom[5];
    size_t len = 0;

    if (!fp || !charset)
        return 0;

#define ELSEIF_FILE_BOM(c, b)                \
    else if (!strcmp(charset, c)){            \
       xstrlcpy((char*)bom, b, sizeof(bom));\
       len = strlen(b);                        \
    }

    if (0);
    ELSEIF_FILE_BOM("UTF-8", UTF8_BOM)
    ELSEIF_FILE_BOM("UTF-16LE", UTF16LE_BOM)
    ELSEIF_FILE_BOM("UTF-16BE", UTF16BE_BOM)
    ELSEIF_FILE_BOM("UTF-32LE", UTF32LE_BOM)
    ELSEIF_FILE_BOM("UTF-32BE", UTF32BE_BOM)
    ELSEIF_FILE_BOM("GB18030", GB18030_BOM)

#undef ELSEIF_FILE_BOM

    if (!len)
        return 0;

    /* 写入BOM头 */;
    return fwrite(bom, len, 1, fp) == 1;
}

/* 表示非ASCII字符的多字节串的第一个字节总是在0xC0到0xFD的范围里 */
/* 多字节串的其余字节都在0x80到0xBF范围里 */
#define UTF8_ASCII(ch)  (((byte)ch)<=0x7F)
#define UTF8_FIRST(ch) ((((byte)ch)>=0xC0) && (((byte)ch)<=0xFD))
#define UTF8_OTHER(ch) ((((byte)ch)>=0x80) && (((byte)ch)<=0xBF))

size_t utf8_len(const char* utf8)
{
    byte *p = (byte*)utf8;
    size_t count = 0;

    if (!utf8)
        return 0;

    while (*p){
        if (UTF8_ASCII(*p) || (UTF8_FIRST(*p)))
            count++;
        else if (!UTF8_OTHER(*p))
            return 0;

        p++;
    }

    return count;
}

/* 按照给定的最大字节数截取UTF-8字符串，
 * 此函数决定最多可以保留的字符数，返回新字符串的字节数
 * outbuf可为NULL；如果不为NULL，其长度必须大于max_byte (以添加'\0')，且截取后的字串会被复制到outbuf中
 * 如果传入的UTF-8字符串无效，返回－1 */
size_t utf8_trim(const char* utf8, char* outbuf, size_t max_byte)
{
    size_t slen = strlen(utf8);
    const byte *begin = (byte*)utf8;
    const byte *p = begin + max_byte;

    if (!utf8)
        return 0;

    if (max_byte >= slen)
    {
        if (outbuf)
            strcpy(outbuf, utf8);
        return slen;
    }

    while (p >= begin){
        if (UTF8_ASCII(*p) || (UTF8_FIRST(*p)))
            break;
        else if (!UTF8_OTHER(*p))
            return 0;

        p--;
    }

    /* 找到截取位置 */
    if (p >= begin)
    {
        ptrdiff_t count = p - begin;
        if (outbuf)
        {
            strncpy(outbuf, utf8, count);
            outbuf[count] = '\0';
        }
        return count;
    }

    return 0;
}

#define ABBR_DOTS_LEN    3    /* strlen("...") */

size_t utf8_abbr(const char* utf8, char* outbuf, size_t max_byte,
    size_t last_reserved_words)
{
    const char* p, *plast;
    size_t origlen, word_count;
    size_t suffix_count, prefix_count, left_room;

    if (!utf8 || !outbuf)
        return 0;

    origlen = strlen(utf8);
    if (max_byte >= origlen) {
        strcpy(outbuf, utf8);
        return 1;
    }

    if (max_byte <= ABBR_DOTS_LEN) {
        strncpy(outbuf, "...", max_byte);
        outbuf[max_byte] = '\0';
        return 1;
    }

    /* 找到新字符串中"..."后面的字符在源字符串中的位置 */
    p = plast = utf8 + origlen - 1;
    suffix_count = 0;
    word_count = 0;

    while(1) {
        ++suffix_count;
        if (UTF8_ASCII(*p) || (UTF8_FIRST(*p))) {
            if (suffix_count > max_byte) {
                suffix_count -= plast - p;    /* 定位到上次未超出范围的字符处 */
                p = plast;
                break;
            }
            if (++word_count == last_reserved_words)
                break;
            plast = p;
        } else if (!UTF8_OTHER(*p))
            return 0;

        --p;
    }

    left_room = max_byte - suffix_count;
    if (left_room <= ABBR_DOTS_LEN) {
        strncpy(outbuf, "...", left_room);
        outbuf[left_room] = '\0';
        strcat(outbuf, p);
        return 1;
    }

    // 找到新字符串中"..."之前的字符数目
    prefix_count = utf8_trim(utf8, NULL, left_room - ABBR_DOTS_LEN);
    if (!prefix_count)
        return 0;

    strncpy(outbuf, utf8, prefix_count);
    outbuf[prefix_count] = '\0';
    strcat(outbuf, "...");
    strcat(outbuf, p);

    return 1;
}

#undef UTF8_ASCII
#undef UTF8_FIRST
#undef UTF8_OTHER

/* 将多字节字符串转换为宽字符串 */
wchar_t* mbcs_to_wcs(const char* src)
{
#ifdef OS_WIN
    wchar_t *buf;

    int outlen = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    if (outlen <= 0)
        return NULL;

    buf = xcalloc(outlen, sizeof(wchar_t));
    if (!MultiByteToWideChar(CP_ACP, 0, src, -1, buf, outlen))
    {
        xfree (buf);
        return NULL;
    }

    return buf;
#else
    CHECK_INIT();

    /* Multithread safe shift states */
    mbstate_t mbs;
    memset(&mbs, 0, sizeof(mbs));

    const char* p = src;
    size_t dstsize = mbsrtowcs(NULL, &p, 0, &mbs);
    if (dstsize == (size_t)-1)
        return NULL;

    ++dstsize; // '\0'
    wchar_t *dst = (wchar_t*)xcalloc(dstsize, sizeof(wchar_t));

    p = src;
    if (mbsrtowcs(dst, &p, dstsize, &mbs) == (size_t)-1)
    {
        xfree(dst);
        return NULL;
    }

    return dst;
#endif
}

/* 将宽字符串转换为多字节字符串 */
char* wcs_to_mbcs(const wchar_t* src)
{
#ifdef OS_WIN
    BOOL bUsed;
    char *buf;

    int outlen = WideCharToMultiByte(CP_ACP, 0,  src, -1, NULL, 0, NULL, &bUsed);
    if (outlen <= 0)
        return NULL;

    buf = xcalloc(1, outlen);
    if (!WideCharToMultiByte(CP_ACP, 0, src, -1, buf, outlen, NULL, &bUsed))
    {
        xfree (buf);
        return NULL;
    }

    return buf;
#else
    CHECK_INIT();

    mbstate_t mbs;
    memset(&mbs, 0, sizeof(mbs));

    const wchar_t* p = src;
    size_t dstsize = wcsrtombs(NULL, &p, 0, &mbs);
    if (dstsize == (size_t)-1)
        return NULL;

    ++dstsize; //'\0'
    char *dst = (char*)xcalloc(1, dstsize);

    p = src;
    if (wcsrtombs(dst, &p, dstsize, &mbs) == (size_t)-1)
    {
        xfree(dst);
        return NULL;
    }

    return dst;
#endif
}

/* 将UTF-8编码的字符串转换为UTF-16字符串，需外部释放 */
UTF16* utf8_to_utf16(const UTF8* utf8, int strict)
{
    ConversionResult cr;
    size_t nLen = strlen((const char*)utf8);
    const UTF8 *uBeg = utf8;

    UTF16* wBeg = (UTF16*)xcalloc((nLen+1), sizeof(UTF16));
    UTF16* wBegOrig = wBeg;

    cr = ConvertUTF8toUTF16(&uBeg, uBeg+nLen, &wBeg, wBeg+nLen,
                strict ? strictConversion : lenientConversion);
    if (cr == conversionOK)
    {
        *wBeg = L'\0';
        return wBegOrig;
    }

    xfree(wBegOrig);
    return NULL;
}

/* 将UTF-16字符串转换为UTF-8编码的字符串，需外部释放 */
/* 注：在UTF-8编码中每个字符一般最多占3个字节，但有可能最多占6个字节 */
UTF8* utf16_to_utf8(const UTF16* src, int strict)
{
    ConversionResult cr;
    const UTF16 *wBeg;
    UTF8 *uBeg, *uBegOrig, *uEnd;
    size_t nLen = utf16_len(src);
    size_t nBytes = 2;

    do{
        uBeg = (UTF8*)xcalloc(1, nLen * nBytes + 1);
        uBegOrig = uBeg;
        uEnd = uBeg + nLen * nBytes + 1;

        wBeg = src;

        cr = ConvertUTF16toUTF8(&wBeg, wBeg + nLen, &uBeg, uEnd,
            strict ? strictConversion : lenientConversion);
        if (cr == conversionOK)
        {
            if (uBeg < uEnd)
            {
                *uBeg = '\0';
                return uBegOrig;
            }

            /* 无法添加字符串结束符'\0' */
            cr = targetExhausted;
        }

        xfree(uBegOrig);
        nBytes ++;

    }while(cr == targetExhausted && nBytes <= 6);

    return NULL;
}

/* 将UTF-8编码的字符串转换为UTF-32字符串，需外部释放 */
UTF32* utf8_to_utf32(const UTF8* utf8, int strict)
{
    ConversionResult cr;
    size_t nLen = strlen((const char*)utf8);
    const UTF8 *uBeg = utf8;

    UTF32* wBeg = (UTF32*)xcalloc((nLen+1), sizeof(UTF32));
    UTF32* wBegOrig = wBeg;

    cr = ConvertUTF8toUTF32(&uBeg, uBeg+nLen, &wBeg, wBeg+nLen,
        strict ? strictConversion : lenientConversion);
    if (cr == conversionOK)
    {
        *wBeg = L'\0';
        return wBegOrig;
    }

    xfree(wBegOrig);
    return NULL;
}

/* 将UTF-32字符串转换为UTF-8编码的字符串，需外部释放 */
/* 注：在UTF-8编码中每个字符一般最多占3个字节，但有有可能最多占6个字节 */
UTF8* utf32_to_utf8(const UTF32* src, int strict)
{
    ConversionResult cr;
    const UTF32 *wBeg;
    UTF8 *uBeg, *uBegOrig, *uEnd;
    size_t nLen = utf32_len(src);
    size_t nBytes = 3;

    do{
        uBeg = (UTF8*)xcalloc(1, nLen * nBytes + 1);
        uBegOrig = uBeg;
        uEnd = uBeg + nLen * nBytes + 1;

        wBeg = src;

        cr = ConvertUTF32toUTF8(&wBeg, wBeg + nLen, &uBeg, uEnd,
            strict ? strictConversion : lenientConversion);
        if (cr == conversionOK)
        {
            if (uBeg < uEnd)
            {
                *uBeg = '\0';
                return uBegOrig;
            }

            /* 无法添加字符串结束符'\0' */
            cr = targetExhausted;
        }

        xfree(uBegOrig);
        nBytes ++;

    }while(cr == targetExhausted && nBytes <= 6);

    return NULL;
}

/* 将UTF-16编码的字符串转换为UTF-32字符串，需外部释放 */
UTF32* utf16_to_utf32(const UTF16* src, int strict)
{
    ConversionResult cr;
    UTF32* u32, *u32Orig;
    const UTF16 *u16 = src;
    size_t nLen = utf16_len(src);

    u32 = (UTF32*)xcalloc((nLen + 1), sizeof(UTF32));
    u32Orig = u32;

    cr = ConvertUTF16toUTF32(&u16, u16 + nLen, &u32, u32 + nLen,
        strict ? strictConversion : lenientConversion);
    if (cr == conversionOK)
    {
        *u32 = L'\0';
        return u32Orig;
    }

    xfree(u32Orig);
    return NULL;
}

/* 将UTF-32字符串转换为UTF-16编码的字符串，需外部释放 */
UTF16* utf32_to_utf16(const UTF32* src, int strict)
{
    ConversionResult cr;
    UTF16* u16, *u16Orig;
    const UTF32 *u32 = src;
    size_t nLen = utf32_len(src);

    u16 = (UTF16*)xcalloc((nLen+1), sizeof(UTF16));
    u16Orig = u16;

    cr = ConvertUTF32toUTF16(&u32, u32+nLen, &u16, u16+nLen,
        strict ? strictConversion : lenientConversion);
    if (cr == conversionOK)
    {
        *u16 = L'\0';
        return u16Orig;
    }

    xfree(u16Orig);
    return NULL;
}

/* 将UTF-8编码字符串转换为UTF-7字符串，需要外部释放，strict无意义 */
UTF7* utf8_to_utf7(const UTF8* src)
{
    ConversionResult cr;
    const UTF8 *u8 = src;
    size_t u8len = strlen((const char*)src);

    /*
     * In the worst case we convert 2 chars to 7 chars. For example:
     * "\x10&\x10&..." -> "&ABA-&-&ABA-&-...".
     */
    size_t u7len = (u8len / 2) * 7 + 6;
    UTF7* u7 = (UTF7*)xmalloc(u7len);
    UTF7* u7p = u7;

    cr = ConvertUTF8toUTF7IMAP(&u8, u8 + u8len, &u7p, u7 + u7len);
    if (cr == conversionOK)
        return u7;

    xfree(u7);
    return NULL;
}

/* 将UTF-7编码字符串转换为UTF-8字符串，需要外部释放，strict无意义 */
UTF8* utf7_to_utf8(const UTF7* src)
{
    ConversionResult cr;
    const UTF7 *u7 = src;
    size_t u7len = strlen((const char*)src);

    size_t u8len = u7len + u7len / 8 + 1;
    UTF8* u8 = (UTF8*)xmalloc(u8len);
    UTF8* u8p = u8;

    cr = ConvertUTF7IMAPtoUTF8(&u7, u7 + u7len, &u8p, u8 + u8len);
    if (cr == conversionOK)
        return u8;

    xfree(u8);
    return NULL;
}

/* 将UTF-8编码的字符串转换为系统多字节编码 */
char* utf8_to_mbcs(const char* utf8, int strict)
{
    wchar_t *wcs;
    char *mbcs;

    wcs = utf8_to_wcs(utf8, strict);
    if (!wcs)
        return NULL;

    mbcs = wcs_to_mbcs(wcs);
    xfree(wcs);

    return mbcs;
}

/* 将系统多字节编码的字符串转换为UTF-8编码 */
char* mbcs_to_utf8(const char* mbcs, int strict)
{
    wchar_t *wcs;
    char *utf8;

    wcs = mbcs_to_wcs(mbcs);
    if (!wcs)
        return NULL;

    utf8 = wcs_to_utf8(wcs, strict);
    xfree(wcs);

    return utf8;
}

size_t utf16_len(const UTF16* u16)
{
    const UTF16* p = u16;

    if (!u16)
        return 0;

    while (*p) p++;
    return p - u16;
}

size_t utf32_len(const UTF32* u32)
{
    const UTF32* p = u32;

    if (!u32)
        return 0;

    while (*p) p++;
    return p - u32;
}

#ifndef OS_ANDROID
/* 获取系统默认字符集(多字节编码所用字符集) */
const char *get_locale()
{
#ifdef OS_WIN
    static char lcs[20];
    snprintf(lcs, sizeof(lcs), "CP%u", GetACP());
    return lcs;
#elif defined(OS_MACOSX)
    /* Mac 下 nl_langinfo 返回的是US-ASCII */
    static const char utf8[] = "UTF-8";
    return utf8;
#else
    CHECK_INIT();
    return nl_langinfo(CODESET);
#endif
}
#endif

/* 获取系统默认语言 */
/* 返回值如 zh_CN，en_US */
const char *get_language()
{
    static char lang[64];
#ifdef OS_POSIX
    CHECK_INIT();
    char *dot;
    xstrlcpy(lang, setlocale(LC_CTYPE, NULL), sizeof(lang));
    dot = strchr(lang, '.');
    if (dot)
        *dot = '\0';
    return lang;
#else
    LCID lcid = GetSystemDefaultLCID();

    //http://msdn.microsoft.com/en-us/library/ms912047(WinEmbedded.10).aspx
    switch(lcid)
    {
    case 2052:    // 中文（中国）
        strcpy(lang, "zh_CN");
        break;
    case 1028:    // 中文（台湾)
    case 3076:    // 中文（香港)
        strcpy(lang, "zh_TW");
        break;
    case 1033:    // 英语 (美国)
        strcpy(lang, "en_US");
        break;
    default:
        strcpy(lang, "en_US");
        break;
    }
    return lang;
#endif
}

#ifdef _LIBICONV_H

/* 使用libiconv库转换字符编码
 * 成功返回转换到目标编码的长度
 * 失败返回-1并设置errno，分以下几种情况:
 * 1、无效的输入序列，设置errno为EILSEQ
 * 2、不完整的输入序列，设置errno为EINVAL
 * 3、输出缓冲区过小，设置errno为E2BIG
 */
static inline
size_t iconv_convert(const char* from_charset, const char* to_charset,
    const char* inbuf, size_t inlen,
    char* outbuf, size_t outlen,
    int strict)
{
    iconv_t cd;
    char** pin = (char**)&inbuf;
    char** pout = &outbuf;
    size_t leftlen = outlen;
    int lenient = !strict;

    cd = iconv_open(to_charset,from_charset);
    if(cd == (iconv_t)-1)
        return (size_t)-1;

    if(iconvctl(cd, ICONV_SET_DISCARD_ILSEQ, &lenient) == (size_t)-1)
        return (size_t)-1;

    memset(outbuf,0,outlen);
    if(iconv(cd, pin, &inlen, pout, &leftlen)== (size_t)-1)
        return (size_t)-1;

    ASSERT(inlen == 0 && leftlen >= 0);

    iconv_close(cd);
    return outlen - leftlen;
}

/*
 * 字符编码转换
 * 如果outbuf不为NULL且outlen的长度足够容纳转换后的字符
 * 则转换后的字符串存放于outbuf中；否则将新申请一块内存并通过outbuf返回
 * outlen为转换后的字节长度
 * 返回值：成功返回1，失败返回0
 * 注：虽然inbuf一般不以'\0'结尾，但会保证输出以'\0'结尾，以防误用
 */
int convert_to_charset(const char* from, const char* to,
                        const char* inbuf, size_t inlen,
                        char **outbuf, size_t *outlen,
                        int strict)
{
    size_t tlen, dlen;
    char *tbuf = NULL;

    if (!inbuf || !inlen|| !outbuf || !outlen)
        return 0;

    /* 原字符集和目标字符集相同 */
    if (!strcasecmp(from, to))
    {
        if (!*outbuf || *outlen <= inlen)
            *outbuf = (char*)xmalloc(inlen+1);

        memcpy(*outbuf, inbuf, inlen);
        (*outbuf)[inlen] = '\0';
        *outlen = inlen;

        return 1;
    }

    if (*outbuf && *outlen){
        tlen = *outlen;
        tbuf = *outbuf;
    }else{
        tlen = inlen * 2 + 1;
        tbuf = (char*)xmalloc(tlen);
    }

retry_convert:

    errno = 0;
    memset(tbuf, 0, tlen);

    dlen = iconv_convert(from, to, inbuf, inlen, tbuf, tlen, strict);

    if ((dlen == (size_t)-1 && errno == E2BIG)            // 转换失败，原因是输出缓冲区太小
        || (dlen != (size_t)-1 && tlen < dlen +1))        // 转换成功，但无法容纳附加的'\0'
    {
        tlen += inlen;

        if (tbuf == *outbuf)
            tbuf = (char*)xmalloc(tlen);                // 首次内部分配
        else
            tbuf = (char*)xrealloc(tbuf, tlen);            // 重新分配大小

        goto retry_convert;
    }
    else if (dlen == (size_t)-1)                            // EILSEQ, EINVAL
    {
        if (tbuf != *outbuf)
            xfree(tbuf);

        return 0;
    }

    tbuf[dlen] = '\0';

    // 设置输出参数
    *outlen = dlen;

    if (tbuf != *outbuf)
        *outbuf = tbuf;

    return 1;
}

#endif

/************************************************************************/
/*                         Process  多进程                              */
/************************************************************************/

/* 创建新的进程 */
/* 参数executable：将要执行的可执行文件路径 */
/* 参数param：传递给可执行文件的参数指针数组，最后一个元素必须为NULL */
/* 返回值：创建成功返回进程标识符(process_t)，失败返回INVALID_PROCESS */
process_t process_create(const char* cmd_with_param, int show ALLOW_UNUSED)
{
#ifdef OS_WIN
    PROCESS_INFORMATION proc_info;
#ifdef USE_UTF8_STR
    {
        STARTUPINFOW startup_info;
        size_t cmdlen = strlen(cmd_with_param) + 1;
        wchar_t *cmdlinew = (wchar_t *)xmalloc(cmdlen * sizeof(wchar_t));

        GetStartupInfoW(&startup_info);
        startup_info.cb = sizeof(startup_info);
        startup_info.dwFlags = STARTF_USESHOWWINDOW;
        startup_info.wShowWindow = show ? SW_SHOW : SW_HIDE;

        if (!UTF82UNI(cmd_with_param, cmdlinew, cmdlen)
            || !CreateProcessW(NULL, cmdlinew, NULL, NULL, 1, 0, NULL, NULL, &startup_info, &proc_info))
        {
            xfree(cmdlinew);
            return INVALID_PROCESS;
        }

        xfree(cmdlinew);
    }
#else /* MBCS */
    {
        char *cmdline;
        STARTUPINFOA startup_info;
        GetStartupInfoA(&startup_info);
        startup_info.cb = sizeof(startup_info);
        startup_info.dwFlags = STARTF_USESHOWWINDOW;
        startup_info.wShowWindow = show ? SW_SHOW : SW_HIDE;

        /* CreateProcess会修改exe_param参数，因此不能直接使用 */
        cmdline = xstrdup(cmd_with_param);
        if (!CreateProcessA(NULL, cmdline, NULL, NULL, 1, 0, NULL, NULL, &startup_info, &proc_info))
        {
            xfree(cmdline);
            return INVALID_PROCESS;
        }

        xfree(cmdline);
    }
#endif /* USE_UTF8_STR */
    CloseHandle(proc_info.hThread);
    return proc_info.hProcess;
#else /* POSIX */
    pid_t pid = fork();
    if (pid < 0)
    {
        log_dprintf(LOG_ERROR, "Fork failed.");
        return INVALID_PROCESS;
    }
    else if (pid == 0)
    {
        /* 子进程 */
        int null_fd, new_fd;

        null_fd = HANDLE_FAILURE(open("/dev/null", O_RDONLY));
        if (null_fd < 0) {
            //log_dprintf(LOG_ERROR, "Failed to open /dev/null.");
            _exit(127);
        }

        new_fd = HANDLE_FAILURE(dup2(null_fd, STDIN_FILENO));
        if (new_fd != STDIN_FILENO) {
            //log_dprintf(LOG_ERROR, "Failed to dup /dev/null for stdin");
            _exit(127);
        }
        /* FIXME: why? */
        HANDLE_FAILURE(close(null_fd));

        /* 重置信号响应函数 */
        signal(SIGHUP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGILL, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
        signal(SIGFPE, SIG_DFL);
        signal(SIGBUS, SIG_DFL);
        signal(SIGSEGV, SIG_DFL);
        signal(SIGSYS, SIG_DFL);
        signal(SIGTERM, SIG_DFL);

        /* 开始执行 */
        execl("/bin/sh", "sh", "-c", cmd_with_param, NULL);

        /* 如果执行成功，不会运行到这里 */
        _exit(127);
    }

    return pid;
#endif
}

/* 等待进程结束，若参数为 INFINITE 则无限等待 */
int    process_wait(process_t process, int milliseconds, int *exit_code)
{
#ifdef OS_WIN
    DWORD temp_code;
    if (WaitForSingleObject(process, milliseconds) != WAIT_OBJECT_0
        || !GetExitCodeProcess(process, &temp_code))
        return 0;

    if (exit_code)
        *exit_code = temp_code;

    CloseHandle(process);
    return 1;
#else /* POSIX */
    int status = -1;
    pid_t ret_pid = HANDLE_FAILURE(waitpid(process, &status, WNOHANG));
    const int wait_per_cycle = 10; /* 每次等待10毫秒 */
    int time_waited = 0;

    while (ret_pid == 0) {
        msleep(wait_per_cycle);

        ret_pid = HANDLE_FAILURE(waitpid(process, &status, WNOHANG));

        time_waited += wait_per_cycle;
        if (milliseconds != (int)INFINITE && time_waited > milliseconds)
            break;
    }

    if (unlikely(ret_pid == -1))
        return 0;

    /* 子进程被信号所终止 */
    if (WIFSIGNALED(status)) {
        if (exit_code)
            *exit_code = -1;
        return 1;
    }

    /* 子进程已退出 */
    if (WIFEXITED(status)) {
        if (exit_code)
            *exit_code = WEXITSTATUS(status);
        return 1;
    }

    /* 子进程尚未终止 */
    return 0;

#endif
}

/* 杀死进程 */
int process_kill(process_t process, int exit_code ALLOW_UNUSED, int wait)
{
#ifdef OS_WIN
    if (!TerminateProcess(process, exit_code))
        return 0;

    // 等待进程完成异步IO操作
    if (wait)
    {
        if (WaitForSingleObject(process, 60 * 1000) != WAIT_OBJECT_0)
            ASSERT(!"Error waiting for process exit");
    }

    CloseHandle(process);
    return 1;
#else /* POSIX */
    if (kill(process, SIGTERM))
        return 0;

    // 等待进程完成异步IO操作
    if (wait)
    {
        int tries = 60;
        unsigned sleep_ms = 4;
        const unsigned kMaxSleepMs = 1000;
        int exited = 0;

        while (tries-- > 0) {
            pid_t pid = HANDLE_FAILURE(waitpid(process, NULL, WNOHANG));
            if (pid == process) {
                exited = 1;
                break;
            }
            if (pid == -1) {
                if (errno == ECHILD) {
                    /* 另一个进程也在等待相同的pid */
                    exited = 1;
                    break;
                }
            }

            msleep(sleep_ms);
            if (sleep_ms < kMaxSleepMs)
                sleep_ms *= 2;
        }

        /* 杀不死？用SIGKILL信号再杀一次 */
        if (!exited)
        {
            ASSERT(!"Error waiting for process exit");
            if (kill(process, SIGKILL))
                return 0;
        }
    }

    return 1;
#endif
}

char* popen_readall(const char* command)
{
    char *buf = NULL;
    size_t len = 0;

    FILE* f = popen(command, "rb");
    if (!f)
        return NULL;

    if (!xfread(f, -1, 0, &buf, &len)) {
        pclose(f);
        return NULL;
    }

    IGNORE_RESULT(xfread(f, -1, 0, &buf, &len));

    pclose(f);
    return buf;
}

#if defined(OS_WIN) && !defined(__MINGW32__)
/*
 * 创建子进程以及与之连接的管道
 * 可以通过管道读取子进程输出(r)或写入管道作为子进程标准输入(w)
 */
FILE* popen(const char *cmd, const char *mode)
{
#ifdef USE_UTF8_STR
    wchar_t wcmd[MAX_PATH], wmode[10];
    if (!UTF82UNI(cmd, wcmd, MAX_PATH) ||
        !UTF82UNI(mode, wmode, countof(wmode)))
        return NULL;

    return _wpopen(wcmd, wmode);
#else
    return _popen(cmd, mode);
#endif
}
#endif

/*
 * TODO: linux, mac 支持
 */
int shell_execute(const char* cmd, const char* param, int show, int wait_timeout)
{
#ifdef OS_WIN
    wchar_t *wcmd = NULL, *wparam = NULL;
    SHELLEXECUTEINFOW sei;

    if (!cmd)
        return 0;

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);

    if (show <= 0)
        sei.nShow = SW_HIDE;
    else if (show == 1)
        sei.nShow = SW_NORMAL;
    else
        sei.nShow = SW_MAXIMIZE;

    sei.fMask = SEE_MASK_NOASYNC;
    if (wait_timeout > 0)
        sei.fMask |= SEE_MASK_NOCLOSEPROCESS;

#ifdef USE_UTF8_STR
    wcmd = XNMALLOC(MAX_PATH, wchar_t);
    if (!UTF82UNI(cmd, wcmd, MAX_PATH * sizeof(wchar_t)))
        return 0;

    if (param) {
        wparam = XNMALLOC(MAX_PATH, wchar_t);
        if (!UTF82UNI(param, wparam, MAX_PATH * sizeof(wchar_t)))
            return 0;
    }
#else
    wcmd = mbcs_to_wcs(cmd);
    if (!wcmd)
        return 0;

    if (param) {
        wparam = mbcs_to_wcs(wparam);
        if (!wparam)
            return 0;
    }
#endif

    sei.lpFile = wcmd;
    sei.lpParameters = wparam;

    if (!ShellExecuteExW(&sei)) {
        xfree(wcmd);
        if (wparam)
            xfree(wparam);
        return 0;
    }

    xfree(wcmd);
    if (wparam)
        xfree(wparam);

    if (wait_timeout > 0 && sei.hProcess) {
        WaitForSingleObject(sei.hProcess, wait_timeout);
    }

    if (sei.hProcess)
        CloseHandle(sei.hProcess);

    return 1;
#else
    NOT_IMPLEMENTED();
    return 0;
#endif
}

/************************************************************************/
/*                         Sync 线程与同步                               */
/************************************************************************/

// sync_fetch_and_* 操作然后返回之前的值
// sync_*_and_fetch 操作然后返回新的值
// http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// InterlockedExchangeAdd 返回操作之前的值
// InterlockedIncrement 返回操作之后的值

/* 原子变量操作 */
long atomic_get(const atomic_t *v)
{
#ifdef OS_WIN
    return InterlockedExchangeAdd((LONG*)&v->counter, 0);
#else
    return __sync_fetch_and_add((volatile long*)&v->counter, 0);
#endif
}

void atomic_set(atomic_t *v, long i)
{
#ifdef OS_WIN
    InterlockedExchange((LONG*)&v->counter, i);
#else
    __sync_lock_test_and_set(&v->counter, i);
#endif
}

void atomic_add(long i, atomic_t *v)
{
#ifdef OS_WIN
    InterlockedExchangeAdd((LONG*)&v->counter, i);
#else
    __sync_fetch_and_add(&v->counter, i);
#endif
}

void atomic_sub(long i, atomic_t *v)
{
#ifdef OS_WIN
    InterlockedExchangeAdd((LONG*)&v->counter, -1*i);
#else
    __sync_fetch_and_sub(&v->counter, i);
#endif
}

void atomic_inc(atomic_t *v)
{
#ifdef OS_WIN
    InterlockedIncrement((LONG*)&v->counter);
#else
    __sync_fetch_and_add(&v->counter, 1);
#endif
}

void atomic_dec(atomic_t *v)
{
#ifdef OS_WIN
    InterlockedDecrement((LONG*)&v->counter);
#else
    __sync_fetch_and_sub(&v->counter, 1);
#endif
}

/* 自旋锁实现 */
void spin_init(spin_lock_t* lock)
{
#ifdef OS_WIN
    InterlockedExchange((long*)&lock->spin, 0);
#elif defined(__GNUC__)
    __sync_and_and_fetch((long*)&lock->spin, 0);
#endif
}

void spin_lock(spin_lock_t* lock)
{
#ifdef OS_WIN
    while(InterlockedExchange((long*)&lock->spin, 1) == 1) { }
#elif defined(__GNUC__)
    while(__sync_fetch_and_or((long*)&lock->spin, 1) == 1) { }
#endif
}

int spin_trylock(spin_lock_t* lock)
{
#ifdef OS_WIN
    return !InterlockedExchange((long*)&lock->spin, 1);
#elif defined(__GNUC__)
    return !__sync_fetch_and_or((long*)&lock->spin, 1);
#endif
}

void spin_unlock(spin_lock_t* lock)
{
#ifdef OS_WIN
    InterlockedExchange((long*)&lock->spin, 0);
#elif defined(__GNUC__)
    __sync_and_and_fetch((long*)&lock->spin, 0);
#endif
}

int spin_is_lock(spin_lock_t* lock)
{
#ifdef OS_WIN
    return InterlockedExchangeAdd((long*)&lock->spin, 0);
#elif defined(__GNUC__)
    return (int)__sync_add_and_fetch((long*)&lock->spin, 0);
#endif
}

/* 线程互斥量实现 */
void mutex_init(mutex_t *lock)
{
#if defined(COMPILER_MSVC) && _MSC_VER <= MSVC6
    InitializeCriticalSection(lock);
#elif defined OS_WIN
    InitializeCriticalSectionAndSpinCount(lock, 3000);
#else
    pthread_mutexattr_t mta;
    pthread_mutexattr_init(&mta);
    pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(lock, &mta);
#endif
}

void mutex_lock(mutex_t *lock)
{
#ifdef OS_WIN
    EnterCriticalSection(lock);
#else
    pthread_mutex_lock(lock);
#endif
}

void mutex_unlock(mutex_t *lock)
{
#ifdef OS_WIN
    LeaveCriticalSection(lock);
#else
    pthread_mutex_unlock(lock);
#endif
}

int mutex_trylock(mutex_t *lock)
{
#if defined OS_POSIX
    return !pthread_mutex_trylock(lock);
#elif (defined OS_WIN) && (OS_WIN_WINNT > 0x400)
    return TryEnterCriticalSection(lock);
#else
    NOT_REACHED();
    return 0;
#endif
}

void mutex_destroy(mutex_t* lock)
{
#ifdef OS_WIN
    DeleteCriticalSection(lock);
#else
    pthread_mutex_destroy(lock);
#endif
}

#ifdef USE_READ_WRITE_LOCK

/* 读写锁实现 */
/* http://stackoverflow.com/questions/1008726/win32-read-write-lock-using-only-critical-sections */

void rwlock_init(rwlock_t *rwlock)
{
#ifdef OS_POSIX
    pthread_rwlock_init(&rwlock->rwlock, NULL);
#elif WINVER >= WINVISTA
    InitializeSRWLock(&rwlock->srwlock);
#else
    mutex_init(&rwlock->write_lock);
    rwlock->reader_count = 0;
    rwlock->no_readers = CreateEvent(NULL, TRUE, TRUE, NULL);
#endif
}

void rwlock_rdlock(rwlock_t *rwlock)
{
#ifdef OS_POSIX
    pthread_rwlock_rdlock(&rwlock->rwlock);
#elif WINVER >= WINVISTA
    AcquireSRWLockShared(&rwlock->srwlock);
#else
    mutex_lock(&rwlock->write_lock);
    if (InterlockedIncrement(&rwlock->reader_count) == 1)
        ResetEvent(rwlock->no_readers);
    mutex_unlock(&rwlock->write_lock);
#endif
}

void rwlock_wrlock(rwlock_t *rwlock)
{
#ifdef OS_POSIX
    pthread_rwlock_wrlock(&rwlock->rwlock);
#elif WINVER >= WINVISTA
    AcquireSRWLockExclusive(&rwlock->srwlock);
#else
    mutex_lock(&rwlock->write_lock);

    if (rwlock->reader_count > 0)
        WaitForSingleObject(rwlock->no_readers, INFINITE);
#endif
}

void rwlock_rdunlock(rwlock_t *rwlock)
{
#ifdef OS_POSIX
    pthread_rwlock_unlock(&rwlock->rwlock);
#elif WINVER >= WINVISTA
    ReleaseSRWLockShared(&rwlock->srwlock);
#else
    if (InterlockedDecrement(&rwlock->reader_count) == 0)
        SetEvent(rwlock->no_readers);
#endif
}

void rwlock_wrunlock(rwlock_t *rwlock)
{
#ifdef OS_POSIX
    pthread_rwlock_unlock(&rwlock->rwlock);
#elif WINVER >= WINVISTA
    ReleaseSRWLockExclusive(&rwlock->srwlock);
#else
    mutex_unlock(&rwlock->write_lock);
#endif
}

void rwlock_destroy(rwlock_t *rwlock)
{
#ifdef OS_POSIX
    pthread_rwlock_destroy(&rwlock->rwlock);
#elif WINVER >= WINVISTA
    // Nothing to do here.
#else
    mutex_destroy(&rwlock->write_lock);
    CloseHandle(rwlock->no_readers);
#endif
}

#endif

#ifdef USE_CONDITION_VARIABLE

/* 条件变量 */
void cond_init(cond_t *cond)
{
#ifdef OS_POSIX
    pthread_cond_init(cond, NULL);
#elif WINVER >= WINVISTA
    InitializeConditionVariable(cond);
#else
    // TODO: unsupported
#endif
}

void cond_wait(cond_t *cond, mutex_t *mutex)
{
#ifdef OS_POSIX
    pthread_cond_wait(cond, mutex);
#elif WINVER >= WINVISTA
    SleepConditionVariableCS(cond, mutex, INFINITE);
#else
    // TODO: unsupported
#endif
}

int cond_wait_time(cond_t *cond, mutex_t *mutex, int milliseconds)
{
#ifdef OS_POSIX
    struct timespec ts;
    int seconds, ms;

    seconds = milliseconds / 1000;    // 秒数
    ms = milliseconds % 1000;        // 毫秒数(<1s)
    ts.tv_sec = seconds;
    ts.tv_nsec = ms * 1000 * 1000;
    return !pthread_cond_timedwait(cond, mutex, &ts);
#elif WINVER >= WINVISTA
    return SleepConditionVariableCS(cond, mutex, milliseconds);
#else
    //TODO: unsupported
#endif
}

void cond_signal(cond_t *cond)
{
#ifdef OS_POSIX
    pthread_cond_signal(cond);
#elif WINVER >= WINVISTA
    WakeConditionVariable(cond);
#else
    //TODO: unsupported
#endif
}

void cond_signal_all(cond_t *cond)
{
#ifdef OS_POSIX
    pthread_cond_broadcast(cond);
#elif WINVER >= WINVISTA
    WakeAllConditionVariable(cond);
#else
    //TODO: unsupported
#endif
}

void cond_destroy(cond_t *cond)
{
#ifdef OS_POSIX
    pthread_cond_destroy(cond);
#elif WINVER >= WINVISTA
    //Nothing to do here
#else
    //TODO: unsupported
#endif
}

#endif

/************************************************************************/
/*                         Thread  多线程                               */
/************************************************************************/

/* 创建线程 */
/* 如果stacksize为0则使用默认的堆栈大小 */
int uthread_create(uthread_t* t, uthread_proc_t proc, void *arg, int stacksize)
{
#ifdef OS_POSIX
    pthread_attr_t attr, *pattr;

    if (stacksize > 0)
    {
        if (pthread_attr_init(&attr))
            return 0;
        if (pthread_attr_setstacksize(&attr, stacksize))
            return 0;
        if (pthread_attr_destroy(&attr))
            return 0;
        pattr = &attr;
    }else
        pattr = NULL;

    if (pthread_create(t, pattr, proc, arg))
        return 0;

    return 1;
#else
    /* 不要使用CreateThread，否则很可能会造成内存泄露 */
    uintptr_t ret = _beginthreadex(NULL, stacksize, proc, arg, 0, NULL);
    if (!ret)
    {
        log_dprintf(LOG_ERROR, "thread create failed: %s", get_last_error_std());
        return 0;
    }

    *t = (HANDLE)ret;
    return 1;
#endif
}

/* 退出线程 */
void uthread_exit(size_t exit_code)
{
#ifdef OS_POSIX
    pthread_exit((void*)exit_code);
#else
    /* 不要使用ExitThread */
    _endthreadex((unsigned)exit_code);
#endif
}

/* 等待线程 */
int uthread_join(uthread_t t, uthread_ret_t *exit_code)
{
#ifdef OS_POSIX
    if (pthread_join(t, exit_code))
        return 0;
#else
    WaitForSingleObject(t, INFINITE);
    if (exit_code && !GetExitCodeThread(t, (LPDWORD)exit_code))
    {
        CloseHandle(t);
        return 0;
    }
    CloseHandle(t);
#endif

    return 1;
}

/* 创建一个线程相关存储（TLS）键 */
int    thread_tls_create(thread_tls_t *tls)
{
#ifdef OS_POSIX
    if (pthread_key_create(tls, NULL))
        return 0;
    return 1;
#else
    DWORD index;
    if ((index = TlsAlloc()) == -1)
        return 0;

    *tls = index;
    return 1;
#endif
}

/* 设置TLS值 */
int thread_tls_set(thread_tls_t tls, void *data)
{
#ifdef OS_POSIX
    if (pthread_setspecific(tls, data))
        return 0;
    return 1;
#else
    if (!TlsSetValue(tls, data))
        return 0;
    return 1;
#endif
}

/* 获取TLS值 */
int thread_tls_get(thread_tls_t tls, void **data)
{
#ifdef OS_POSIX
    /* unable to detect whether the key is legal. */
    *data = pthread_getspecific(tls);
    return 1;
#else
    LPVOID *val = TlsGetValue(tls);
    if (!val && GetLastError() != NO_ERROR)
        return 0;

    *data = val;
    return 1;
#endif
}

/* 释放TLS */
int thread_tls_free(thread_tls_t tls)
{
#ifdef OS_POSIX
    if (pthread_setspecific(tls, NULL)
        || pthread_key_delete(tls))
        return 0;
    return 1;
#else
    if (!TlsFree(tls))
        return 0;
    return 1;
#endif
}

/* 初始化 thread_once_t 结构体 */
void thread_once_init(thread_once_t* once)
{
    once->inited = 0;
    mutex_init(&once->lock);
}

/* 执行线程一次初始化 */
int thread_once(thread_once_t* once, thread_once_func func)
{
    int ret = 1;

    /* Check first for speed */
    if (!once->inited)
    {
        mutex_lock(&once->lock);
        if (!once->inited)
        {
            if (!func())
                ret = 0;
            once->inited = 1;
        }
        mutex_unlock(&once->lock);
    }
    return ret;
}

/************************************************************************/
/*                             Debugging  调试                                */
/************************************************************************/

/* 获取WIN32 API出错后的信息 */
#ifdef OS_WIN
const char* get_last_error_win32()
{
    static char errbuf[128];
    int flag = FORMAT_MESSAGE_ALLOCATE_BUFFER|
        FORMAT_MESSAGE_IGNORE_INSERTS|
        FORMAT_MESSAGE_FROM_SYSTEM;

#ifdef USE_UTF8_STR
    LPWSTR lpBuffer = NULL;
    FormatMessageW(flag, NULL, GetLastError(), LANG_NEUTRAL, (LPWSTR)&lpBuffer, 0, NULL);
    if (!lpBuffer)
        goto not_found;

    if (!UNI2UTF8(lpBuffer, errbuf, sizeof(errbuf)))
        goto not_found;
#else
    LPSTR lpBuffer = NULL;
    FormatMessageA(flag, NULL, GetLastError(), LANG_NEUTRAL, (LPSTR)&lpBuffer, 0, NULL);
    if (!lpBuffer)
        goto not_found;

    if (xstrlcpy(errbuf, lpBuffer, sizeof(errbuf)) >= sizeof(errbuf))
        goto not_found;
#endif

    LocalFree(lpBuffer);
    return errbuf;

not_found:
    xstrlcpy(errbuf, "No corresponding error message.", sizeof(errbuf));

    if (lpBuffer)
        LocalFree(lpBuffer);
    return errbuf;
}
#endif

/* 获取WIN32 API出错后的信息 */
const char* get_last_error_std()
{
    return strerror(errno);
}

const char* get_last_error()
{
#ifdef OS_WIN
    return get_last_error_win32();
#else
    return get_last_error_std();
#endif
}

#ifdef USE_CRASH_HANDLER

void set_crash_handler(crash_handler handler)
{
    g_crash_handler = handler;
}

void set_backtrace_handler(backtrace_handler handler)
{
    g_backtrace_hander = handler;
}

/* 崩溃转储 */
#ifdef OS_WIN

/* 获取堆栈调用记录 */
static void StackBackTraceMsg(const void* const* trace, size_t count, const char* info, int level)
{
    const int kMaxNameLength = 256;
    char msgl[1024], msgs[4096];
    size_t i;

    for (i = 0; i < count; ++i)
    {
        DWORD_PTR frame = (DWORD_PTR)trace[i];
        DWORD64 sym_displacement = 0;
        DWORD line_displacement = 0;
        PSYMBOL_INFO symbol;
        IMAGEHLP_LINE64 line;
        BOOL has_symbol, has_line;

        size_t bufsize = (sizeof(SYMBOL_INFO) +\
            kMaxNameLength * sizeof(wchar_t) +\
            sizeof(ULONG64) - 1) / sizeof(ULONG64);
        ULONG64 *buffer = XNCALLOC(bufsize, ULONG64);

        // Initialize symbol information retrieval structures.
        symbol = (PSYMBOL_INFO)(&buffer[0]);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = kMaxNameLength - 1;
        has_symbol = SymFromAddr(GetCurrentProcess(), frame,
            &sym_displacement, symbol);

        // Attempt to retrieve line number information.
        memset(&line, 0, sizeof(line));
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        has_line = SymGetLineFromAddr64(GetCurrentProcess(), frame,
            &line_displacement, &line);

        // Output the backtrace line.
        if (has_symbol)
            snprintf(msgl, sizeof(msgl), "%d. %s [0x%p+%"PRId64"] (%s : %d)",
                        i+1, symbol->Name, trace[i], sym_displacement,
                        has_line ? line.FileName : "?", line.LineNumber);
        else
            snprintf(msgl, sizeof(msgl), "%d. [0x%p] (No Symbol) ",
                        i+1, trace[i]);

        /* 记录日志 */
        log_dprintf(LOG_DEBUG, "%s", msgl);

        /* 打印到VC输出窗口 */
#ifdef _MSC_VER
        OutputDebugStringA(msgl);
        OutputDebugStringA("\r\n");
#else
        if (is_debug_log_set_to_stderr())
            fprintf(stderr, "%s\n", msgl);
        else
            fprintf(stdout, "%s\n", msgl);
#endif
        /* 完整堆栈 */
        if (i == 0) {
            snprintf(msgs, sizeof(msgs), "%s\r\n", msgl);
        } else {
            xstrlcat(msgs, msgl, sizeof(msgs));
            xstrlcat(msgs, "\r\n", sizeof(msgs));
        }

        xfree(buffer);
    }

    log_flush(DEBUG_LOG);

    if (g_backtrace_hander)
        g_backtrace_hander(level, info, msgs);

    // Only show message box if level is FATAL in release mode
#ifdef NDEBUG
    if (level == LOG_FATAL)
#endif
    {
        MessageBoxA(NULL, msgs, info, MB_OK);
    }
}

static inline
void CrashBackTrace(EXCEPTION_POINTERS *pException, const char* info, int level)
{
    size_t i, count = 0;
    void *trace[62];
    int machine_type;

    STACKFRAME64 stack_frame;
    memset(&stack_frame, 0, sizeof(stack_frame));

#if defined(_WIN64)
    machine_type = IMAGE_FILE_MACHINE_AMD64;
    stack_frame.AddrPC.Offset = pException->ContextRecord->Rip;
    stack_frame.AddrFrame.Offset = pException->ContextRecord->Rbp;
    stack_frame.AddrStack.Offset = pException->ContextRecord->Rsp;
#else
    machine_type = IMAGE_FILE_MACHINE_I386;
    stack_frame.AddrPC.Offset = pException->ContextRecord->Eip;
    stack_frame.AddrFrame.Offset = pException->ContextRecord->Ebp;
    stack_frame.AddrStack.Offset = pException->ContextRecord->Esp;
#endif
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    while (StackWalk64(machine_type,
        GetCurrentProcess(),
        GetCurrentThread(),
        &stack_frame,
        pException->ContextRecord,
        NULL,
        &SymFunctionTableAccess64,
        &SymGetModuleBase64,
        NULL) &&
        count < countof(trace)) {
            trace[count++] = (void*)stack_frame.AddrPC.Offset;
    }

    for (i = count; i < countof(trace); ++i)
        trace[i] = NULL;

    StackBackTraceMsg(trace, count, info, level);
}

// 创建Dump文件并返回其路径
static const char* CreateDumpFile(EXCEPTION_POINTERS *pException) {
    static char dump_file[MAX_PATH];
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    HANDLE hDumpFile;

    snprintf(dump_file, MAX_PATH, "%s%s-%s.dmp", get_temp_dir(),
        g_product_name, timestamp_str(time(NULL)));

#ifdef USE_UTF8_STR
    {
        wchar_t wdump_file[MAX_PATH];
        UTF82UNI(dump_file, wdump_file, MAX_PATH);
        hDumpFile = CreateFileW(wdump_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
#else
    hDumpFile = CreateFileA(dump_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#endif /* USE_UTF8_STR */
    if (hDumpFile == INVALID_HANDLE_VALUE) {
        dump_file[0] = '\0';
        return dump_file;
    }

    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    CloseHandle(hDumpFile);

    return dump_file;
}

// 是否存在崩溃模块对应的PDB文件
static BOOL PDBExists() {
    char pdb[MAX_PATH];
    const char* ext;

    xstrlcpy(pdb, get_module_path(), sizeof(pdb));
    ext = path_find_extension(pdb);
    if (!strcasecmp(ext, ".exe"))
        pdb[ext - pdb] = '\0';
    xstrlcat(pdb, ".pdb", sizeof(pdb));

    return path_file_exists(pdb);
}

static LONG WINAPI CrashDumpHandler(EXCEPTION_POINTERS *pException)
{
    static int handled = 0;
    const char* dump_file;

    if (handled)
        return EXCEPTION_EXECUTE_HANDLER;
    handled = 1;

    // 生成 minidump 转储文件
    dump_file = CreateDumpFile(pException);

    // 提示用户上传转储文件
    // 在服务器上结合PDB文件生成堆栈信息
    if (g_crash_handler)
        g_crash_handler(dump_file);

    // 如果有PDB文件，直接打印堆栈信息
    // 只有内部人员才有Release版本的PDB
    if (PDBExists()) {
        char buf[64];
        memset(buf, 0, sizeof(buf));
        xsnprintf(buf, sizeof(buf), "[Crash] %s crashed!", g_product_name);
        CrashBackTrace(pException, buf, LOG_FATAL);
    }

    // 关闭所有打开的日志
    log_close_all();

    return EXCEPTION_EXECUTE_HANDLER;
}

#else /* POSIX */

void stack_backtrace(int level, const char* info)
{
    void* array[20];
    char msg[1024], msgs[4096];
    size_t i;
    int size;

    memset(msgs, 0, sizeof(msgs));
    log_dprintf(LOG_DEBUG, "%s", info);

    if (is_debug_log_set_to_stderr())
        fprintf(stderr, "%s\n", info);
    else
        fprintf(stdout, "%s\n", info);

    size = backtrace(array, countof(array));
    char **strings = backtrace_symbols(array, size);

    for (i = 0; i < size; i++) {
        xsnprintf(msg, sizeof(msg), "%" PRIuS ". %s", i, strings[i]);

        if (g_backtrace_hander)
            xstrlcat(msgs, msg, sizeof(msgs));

        log_dprintf(LOG_DEBUG, "%s", msg);
        if (is_debug_log_set_to_stderr())
            fprintf(stderr, "%s\n", msg);
        else
            fprintf(stdout, "%s\n", msg);
    }
    free (strings);

    if (g_backtrace_hander)
        g_backtrace_hander(level, info, msgs);
}

void crash_signal_handler(int n, siginfo_t *siginfo, void *act)
{
    char title[128], signam[16];
    switch(siginfo->si_signo)
    {
    case SIGSEGV:
        xstrlcpy(signam, "SIGSEGV", sizeof(signam));
        break;
    case SIGFPE:
        xstrlcpy(signam, "SIGFPE", sizeof(signam));
        break;
    case SIGABRT:
        xstrlcpy(signam, "SIGABRT", sizeof(signam));
        break;
    }

    //FIXME: coredump文件的路径？
    if (g_crash_handler)
        g_crash_handler("");

    memset(title, 0, sizeof(title));
    xsnprintf(title, sizeof(title), "[Crash] killed by signal %s\n", signam);

    stack_backtrace(LOG_FATAL, title);

    log_close_all();
    exit(3);
}

#endif /* #ifdef OS_WIN */

#endif /* #ifdef USE_CRASH_HANDLER */

void backtrace_here(int level, const char *fmt, ...)
{
    va_list args;
    char msg[1024];

    // 打印信息
    va_start(args, fmt);
    xvsnprintf(msg, sizeof(msg), fmt, args);
    log_printf0(DEBUG_LOG, "%s\n", msg);
    va_end(args);

#ifdef OS_WIN
    {
    size_t count;
    void *trace[62];
    count = CaptureStackBackTrace(0, countof(trace), trace, NULL);
    StackBackTraceMsg(trace, count, msg, level);
    }
#if defined(_DEBUG) && defined(COMPILER_MSVC)
    __debugbreak();
#endif
#else /* OS_WIN */
    stack_backtrace(level, msg);
#endif /* OS_WIN */

    if (level == LOG_FATAL) {
        log_close_all();
#ifdef _DEBUG
        // 前面已经显示过堆栈，正常退出即可
        exit(2);
#else
        // 否则使软件崩溃以产生core dump文件
        {
            int *p = NULL;
            *p = 0;
        }
#endif
    }
}

/* 以十六进制的形式打印出一个缓冲区的内容 */
char* hexdump(const void *data, size_t len)
{
    /* 每一个字节需要用两位数加一个空格来表示，最后加'\0' */
    size_t buflen = len * 3 + 1;
    char *buf = (char*)xmalloc(buflen);
    size_t i;

    for (i = 0; i < len; i++)
        snprintf(buf + i*3, buflen - i*3, "%.2X ", ((unsigned char*)data)[i]);
    buf[buflen-1] = '\0';

    return buf;
}


/************************************************************************/
/*                         Log 日志系统                                    */
/************************************************************************/

#define LOG_VALID(id) ((id) >= DEBUG_LOG && (id) <= MAX_LOGS)

static const char* log_severity_names[] = {
    "Fatal",
    "Alert",
    "Critical",
    "Error",
    "Warning",
    "Notice",
    "Info",
    "Debug"
};

static int log_min_severity = LOG_DEBUG;   /* 设置最低记录等级 */
static int debug_log_to_stderr = 0;        /* 调试信息发送到标准错误输出 */

static FILE* log_files[MAX_LOGS+1];
static mutex_t log_locks[MAX_LOGS+1];

static inline
void log_lock(int log_id)
{
    mutex_lock(&log_locks[log_id]);
}

static inline
void log_unlock(int log_id)
{
    mutex_unlock(&log_locks[log_id]);
}

void log_severity(int severity)
{
    log_min_severity = severity;
}

void set_debug_log_to_stderr()
{
    debug_log_to_stderr = 1;
}

int is_debug_log_set_to_stderr()
{
    return debug_log_to_stderr;
}

static inline
FILE* open_log_helper(const char *path, int append, int binary)
{
    const char *mode = append ? (binary ? "ab" : "a") : (binary ? "wb" : "w");

    FILE* fp = xfopen(path, mode);
    if (fp) {
        setvbuf(fp, NULL, _IOLBF, 1024);
#ifdef USE_UTF8_STR
        fwrite(UTF8_BOM, strlen(UTF8_BOM), 1, fp);
#endif
        /* 不跟踪日志文件的关闭情况 */
        /* xfopen已对其递增，递减以保持g_opened_files不变 */
        g_opened_files--;
    }

    return fp;
}

/* 打开用户日志文件 */
int    log_open(const char *path, int append, int binary)
{
    int i;

    CHECK_INIT();

    for (i = 1; i <= MAX_LOGS; i++) {
        log_lock(i);

        if (!log_files[i]) {
            log_files[i] = open_log_helper(path, append, binary);
            log_unlock(i);
            return log_files[i] ? i : LOG_INVALID;
        }

        log_unlock(i);
    }

    return LOG_INVALID;
}

/* 打开调试日志文件 */
int log_dopen(const char *path, int append, int binary)
{
    CHECK_INIT();

    log_lock(DEBUG_LOG);

    if (log_files[DEBUG_LOG]) {
        log_unlock(DEBUG_LOG);
        return DEBUG_LOG;
    }

    log_files[DEBUG_LOG] = open_log_helper(path, append, binary);
    log_unlock(DEBUG_LOG);

    return log_files[DEBUG_LOG] ? DEBUG_LOG : LOG_INVALID;
}

/* 初始化日志模块 */
void log_init()
{
    int i;
    for (i = 0; i < MAX_LOGS+1; i++)
    {
        log_files[i] = NULL;
        mutex_init(&log_locks[i]);
    }

    /* 打开软件全局调试日志 */
#ifdef USE_DEBUG_LOG
    if (!g_disable_debug_log) {
        char exe_name[MAX_PATH], log_path[MAX_PATH];
        xstrlcpy(exe_name, get_execute_name(), MAX_PATH);
        exe_name[path_find_extension(exe_name) - exe_name] = '\0';

        IGNORE_RESULT(create_directory("log"));

        /* 如 Product_20140506143512_1432_debug.log */
        snprintf(log_path, MAX_PATH, "log%c%s_%s_%d_debug.log",
            PATH_SEP_CHAR,exe_name, timestamp_str(time(NULL)), getpid());
        log_dopen(log_path, 0, 0);
        log_dprintf(LOG_INFO, " ==================== Program Started ====================\n\n");
    }
#endif /* USE_DEBUG_LOG */
}

void log_printf0(int log_id, const char *fmt, ...)
{
    FILE *fp;
    va_list args;

    CHECK_INIT();

    if (!LOG_VALID(log_id) ||
        (log_id == DEBUG_LOG && g_disable_debug_log))
        return;

    log_lock(log_id);

    if (!(fp = log_files[log_id]))
    {
        log_unlock(log_id);
        return;
    }

    // 正文信息
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);

#ifdef _DEBUG
    fflush(fp);
#endif

    log_unlock(log_id);
}

/* 写入日志文件 */
/* DEBUG 消息仅在DEBUG模式下才会记录 */
/* FATAL 消息在记录之后会记录堆栈并使程序退出 */
void log_printf(int log_id, int severity, const char *fmt, ...)
{
    FILE *fp;
    time_t t;
    char msgbuf[1024];
    const char *p;
    va_list args;
    size_t len;
    int level;

    CHECK_INIT();

    if (!LOG_VALID(log_id) ||
        (log_id == DEBUG_LOG && g_disable_debug_log))
        return;

    level = xmin(xmax((int)LOG_FATAL, severity), (int)LOG_DEBUG);
    if (level > log_min_severity)
        return;

    log_lock(log_id);

    if (!(fp = log_files[log_id]))
    {
        log_unlock(log_id);
        return;
    }

    // 时间信息
    t = time(NULL);
    memset(msgbuf, '\0', sizeof(msgbuf));
    strftime(msgbuf, sizeof(msgbuf), "%d/%b/%Y %H:%M:%S", localtime(&t));

    // 等级信息
    len = strlen(msgbuf);
    xsnprintf(msgbuf + len, sizeof(msgbuf) - len, " - %s - ", log_severity_names[level]);

    // 正文信息
    len = strlen(msgbuf);
    va_start(args, fmt);
    xvsnprintf(msgbuf + len, sizeof(msgbuf) - len, fmt, args);
    va_end(args);

    // 换行符
    len = strlen(msgbuf);
    p = fmt + strlen(fmt) - 1;
    if (p >= fmt && len < sizeof(msgbuf)-1 && *p != '\n')
        msgbuf[len++] = '\n';

    fwrite(msgbuf, len, 1, log_files[log_id]);

    if (log_id == DEBUG_LOG && debug_log_to_stderr)
        fwrite(msgbuf, len, 1, stderr);

#ifdef _DEBUG
    fflush(log_files[log_id]);
#endif

    log_unlock(log_id);

    /* 如果消息等级为FATAL，则立即记录调用堆栈，并异常退出 */
    if (level == LOG_FATAL
#ifdef _DEBUG
        || level <= LOG_ERROR
#endif
        ) {
        char buf[256];
        va_start(args, fmt);
        xvsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        backtrace_here(level, "%s", buf);
    }
}

void log_flush(int log_id)
{
    log_lock(log_id);

    if (log_files[log_id])
        fflush(log_files[log_id]);

    log_unlock(log_id);
}

/* 关闭日志文件 */
void log_close(int log_id)
{
    if (!LOG_VALID(log_id))
        return;

    log_lock(log_id);

    if (log_files[log_id]) {
        xfclose(log_files[log_id]);
        log_files[log_id] = NULL;
    }

    log_unlock(log_id);
}

void log_close_all()
{
    int i;
    for (i = 1; i <= MAX_LOGS; i++)
        log_close(i);

#ifdef USE_DEBUG_LOG
    if (!g_disable_debug_log) {
        log_printf0(DEBUG_LOG, "\n");
        log_dprintf(LOG_INFO, " ==================== Program Exit ====================\n\n");
        log_close(DEBUG_LOG);
    }
#endif
}

/************************************************************************/
/*              Memory Runtime Check 运行时内存检测                      */
/************************************************************************/

#ifdef DBG_MEM_RT

#define MEMRT_HASH_SIZE        16384                                /* 哈希数组的大小 */
#define MEMRT_HASH(p) (((size_t)(p) >> 8) % MEMRT_HASH_SIZE)        /* 哈希算法 */

static struct MEMRT_OPERATE* memrt_hash_table[MEMRT_HASH_SIZE];     /* 哈希数组链表 */

static mutex_t memrt_lock;
static mutex_t memrt_print_lock;

void memrt_init()
{
    mutex_init(&memrt_lock);
    mutex_init(&memrt_print_lock);
    memset(memrt_hash_table, 0, sizeof(memrt_hash_table));
}

/* 代表一次内存操作 */
struct MEMRT_OPERATE{
    int  method;                /* 操作类型 */
    void* address;              /* 操作地址 */
    size_t size;                /* 内存大小 */

    char file[32];              /* 文件名 */
    char func[32];              /* 函数名 */
    int  line;                  /* 行  号 */

    char desc[64];              /* 如保存类名 */

    memrt_msg_callback msgfunc;

    struct MEMRT_OPERATE *next;
};

static inline
struct MEMRT_OPERATE* memrt_ctor(int mtd, void *addr, size_t sz, const char *file,
        const char* func, int line, const char* desc, memrt_msg_callback pmsgfun)
{
    /* 不能使用xmalloc，debug_backtrace等，否则会造成死循环 */
    struct MEMRT_OPERATE *rtp = (struct MEMRT_OPERATE*)malloc(sizeof(struct MEMRT_OPERATE));
    if (!rtp)
        abort();

    rtp->method = mtd;
    rtp->address = addr;
    rtp->size = sz;
    xstrlcpy(rtp->file, file ? path_find_file_name(file) : "", sizeof(rtp->file));
    xstrlcpy(rtp->func, func ? func : "", sizeof(rtp->func));
    xstrlcpy(rtp->desc, desc ? desc : "", sizeof(rtp->desc));
    rtp->line = line;
    rtp->msgfunc = pmsgfun;
    rtp->next = NULL;

    return rtp;
}

static inline
void memrt_dtor(struct MEMRT_OPERATE *rtp)
{
    /* 不能使用xfree，否则会造成死循环 */
    free(rtp);
}

/* 根据错误类型输出相应字符串 */
static
void memrt_msg_c(int error,
   const char* file, const char* func, int line,
    void* address, size_t size, int method) 
{
    switch(error) {
    case MEMRTE_NOFREE:
        __memrt_printf("[MEMORY] {%s %s %d} Memory not freed at %p size %u method %d.\n",
            file, func, line, address, size, method);
        break;
    case MEMRTE_UNALLOCFREE:
        __memrt_printf("[MEMORY] {%s %s %d} Memory not malloced but freed at %p.\n",
            file, func, line, address);
        break;
    case MEMRTE_MISRELIEF:
        __memrt_printf("[MEMORY] {%s %s %d} Memory not reliefed properly at %p, use delete or delete[] instead.\n",
            file, func, line, address);
        break;
    default:
        NOT_REACHED();
        break;
    }
}

/* 跟踪分配内存操作 */
int __memrt_alloc(int mtd, void* addr, size_t sz, const char* file,
    const char* func, int line, const char* desc, memrt_msg_callback pmsgfun)
{
    size_t index = MEMRT_HASH(addr);
    struct MEMRT_OPERATE* rtp = memrt_ctor(mtd, addr, sz, file, func, line, desc, pmsgfun);

    mutex_lock(&memrt_lock);

    rtp->next = memrt_hash_table[index];
    memrt_hash_table[index] = rtp;

    mutex_unlock(&memrt_lock);
    return MEMRTE_OK;
}

/* 跟踪释放内存操作 */
int __memrt_release(int mtd, void* addr, size_t sz, const char* file,
    const char* func, int line, const char* desc, memrt_msg_callback pmsgfun)
{
    struct MEMRT_OPERATE *ptr, *ptr_last = NULL;
    size_t index = MEMRT_HASH(addr);
    int error = MEMRTE_UNALLOCFREE;

    mutex_lock(&memrt_lock);

    ptr = memrt_hash_table[index];
    while (ptr)
    {
        if (ptr->address == addr)
        {
            if (ptr_last == NULL)
                memrt_hash_table[index] = ptr->next;
            else
                ptr_last->next = ptr->next;

            if ((ptr->method >= MEMRT_MALLOC && ptr->method <= MEMRT_STRDUP && mtd != MEMRT_FREE) ||
                (ptr->method >= MEMRT_C_END && mtd == MEMRT_FREE))
                error = MEMRTE_MISRELIEF;
            else
                error = MEMRTE_OK;

            memrt_dtor(ptr);
            break;
        }

        ptr_last = ptr;
        ptr = ptr->next;
    }

    mutex_unlock(&memrt_lock);

    /* 打印错误信息 */
    if (error != MEMRTE_OK && pmsgfun)
        pmsgfun(error, file, func, line, addr, sz, mtd);

    return error;
}

int memrt_check()
{
    struct MEMRT_OPERATE* ptr_tmp;
    int i, isleaked = 0;

    mutex_lock(&memrt_lock);

    for (i = 0; i < MEMRT_HASH_SIZE; i++)
    {
        struct MEMRT_OPERATE* ptr = memrt_hash_table[i];
        if (ptr == NULL)
            continue;

        isleaked = 1;
        while (ptr)
        {
            /* 打印未释放信息 */
            if (ptr->msgfunc)
                ptr->msgfunc(MEMRTE_NOFREE, 
                  ptr->file, ptr->func, ptr->line,
                  ptr->address, ptr->size, ptr->method);

            ptr_tmp = ptr;
            ptr = ptr->next;
            memrt_dtor(ptr_tmp);
        }

        memrt_hash_table[i] = NULL;
    }

    mutex_unlock(&memrt_lock);
    return isleaked;
}

/* 输出调试信息  */
void __memrt_printf(const char *fmt, ...)
{
    char msg[1024];
    va_list args;

    va_start (args, fmt);
    vsnprintf (msg, 1024, fmt, args);
    va_end (args);

    /* 输出到即时窗口 */
#ifdef COMPILER_MSVC
    OutputDebugStringA(msg);
#else
    fprintf(stderr, "%s", msg);
#endif

    /* 记录系统日志 */
    log_dprintf(LOG_NOTICE, "%s", msg);
}

#endif /* DBG_MEM_RT */

/************************************************************************/
/*                            Version 版本管理                                */
/************************************************************************/

/* 分析一个以点分隔的版本字符串 */
/* 字符串格式依次为以'.'分隔的主版本号、次版本号、修正版本号、其他（编译次数或SVN号等）、后缀描述 */
/* 参数major, minor, revision, build, suffix 均可为NULL；如果suffix不为NULL，则slen必须大于0 */
/* 注：常见的后缀描述有alpha1, beta2, Pro, Free, Ultimate, Stable.etc */
/* 示例: 1.5.8.296 beta1 */
int version_parse(const char* version, int *major, int *minor,
    int *revision, int *build, char *suffix, size_t suffix_outbuf_len)
{
    char *ver, *p, *q;
    int id = 0;

    if (!version)
        return 0;

    /* 初始化 */
    if (major) *major = 0;
    if (minor) *minor = 0;
    if (revision) *revision = 0;
    if (build) *build = 0;
    if (suffix) *suffix = '\0';

    /* 后缀描述 */
    p = strchr((char*)version, ' ');
    if (p && suffix)
        xstrlcpy(suffix, p+1, suffix_outbuf_len);

    /* 版本号 */
    ver = p ? xstrndup(version, p - version) : xstrdup(version);
    q = ver;

    while ((p = strsep(&q, ".")))
    {
        if (id == 0 && major)
            *major = atoi(p);
        else if (id == 1 && minor)
            *minor = atoi(p);
        else if (id == 2 && revision)
            *revision = atoi(p);
        else if (id == 3 && build)
            *build = atoi(p);
        else
            break;

        id++;
    }

    xfree(ver);
    return 1;
}

/* 比较v1和v2两个版本号的公有部分 */
/* 返回值：如果相等返回0，v1比v2旧返回-1，v1比v2新返回1 */
/* 注：不会比较版本后缀信息 */
int version_compare(const char* v1, const char* v2)
{
    int major1, minor1, revision1, build1;
    int major2, minor2, revision2, build2;

    if (!version_parse(v1, &major1, &minor1, &revision1, &build1, NULL, 0)
        || !version_parse(v2, &major2, &minor2, &revision2, &build2, NULL, 0))
        return -2;

    /* 比较主版本号 */
    if (major1 > major2)
        return 1;
    else if (major1 < major2)
        return -1;

    /* 主版本号相同，比较次版本号 */
    if (minor1 > minor2)
        return 1;
    else if (minor1 < minor2)
        return -1;

    /* 次版本号也相同，比较修定版本号 */
    if (revision1 > revision2)
        return 1;
    else if (revision1 < revision2)
        return -1;

    /* 修定版本号还相同，比较编译版本号 */
    if (build1 > build2)
        return 1;
    else if (build1 < build2)
        return -1;

    /* 所有版本号均相同 */
    return 0;
}

/************************************************************************/
/*                             Others 其他                                    */
/************************************************************************/

#define KEY_MAX 1024
#define VAL_MAX 4096

/* 设置环境变量 */
/* 仅影响当前进程的环境变量表，不对系统环境变量产生影响 */
/* 注：键的长度不应超过1k，值的长度不应超过4K */
int set_env(const char* key, const char* val)
{
#ifdef OS_WIN
#ifdef USE_UTF8_STR
    wchar_t wkey[KEY_MAX], wval[VAL_MAX];
    if (!UTF82UNI(key, wkey, KEY_MAX)
        || !UTF82UNI(val, wval, VAL_MAX)
        || !SetEnvironmentVariableW(wkey, wval))
        return 0;
    return 1;
#else /* MBCS */
    return SetEnvironmentVariableA(key, val);
#endif
#else /* POSIX */
    return !setenv(key, val, 1);
#endif
}

/* 获取环境变量 */
/* 成功返回指向该环境变量值的字符串，键不存在返回NULL */
/* 注1：非线程安全，不可重入 */
/* 注2：外界不可以改变返回的字符串 */
const char*    get_env(const char* key)
{
#ifdef OS_WIN
    static char val[VAL_MAX];
#ifdef USE_UTF8_STR
    wchar_t wkey[KEY_MAX], wval[VAL_MAX];
    if (!UTF82UNI(key, wkey, KEY_MAX)
        || !GetEnvironmentVariableW(wkey, wval, VAL_MAX)
        || !UNI2UTF8(wval, val, VAL_MAX))
        return NULL;
    return val;
#else /* MBCS */
    if (!GetEnvironmentVariableA(key, val, VAL_MAX))
        return NULL;
    return val;
#endif
#else /* POSIX */
    return getenv(key);
#endif
}

/* 删除环境变量 */
/* 返回值：键值不存在或删除成功返回1，否则返回0 */
int unset_env(const char* key)
{
#ifdef OS_WIN
#ifdef USE_UTF8_STR
    wchar_t wkey[KEY_MAX];
    if (!UTF82UNI(key, wkey, KEY_MAX)
        || !SetEnvironmentVariableW(wkey, NULL))
        return 0;
    return 1;
#else /* MBCS */
    return SetEnvironmentVariableA(key, NULL);
#endif
#else /* POSIX */
    return !unsetenv(key);
#endif
}

#undef KEY_MAX
#undef VAL_MAX

/* 字符串数字转换 */
#define ATON(t,n,f)    \
    t ato##n(const char* s){\
        t i = 0;\
        if (!sscanf(s, f, &i))\
            return 0;\
        return i;\
    }

ATON(uint, u, "%u")
ATON(size_t, s, "%" PRIuS)
ATON(int64_t, i64, "%" PRId64)
ATON(uint64_t, u64, "%" PRIu64)

#undef ATON

/* 获取可用的CPU数目 */
int number_of_processors()
{
#ifdef OS_WIN
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#else
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/* 计算打印一个数字要占的字节数 */
int num_bits (int64_t number)
{
    int cnt = 1;
    if (number < 0)
        ++cnt;
    while ((number /= 10) != 0)
        ++cnt;
    return cnt;
}

/*
 * vim: et ts=4 sw=4
 */
