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
#include <crtdbg.h> /* _CrtSetReportMode */
#include <ShellAPI.h> /* ShellExecuteEx */
#include <shlwapi.h> /* path, str.etc */
#include <process.h> /* _beginthreadex.etc */

#ifdef _MSC_VER
#pragma comment(lib, "shlwapi.lib") /* path, str.etc */
#pragma comment(lib, "User32.lib") /* MessageBox.etc */
#pragma comment(lib, "kernel32.lib") /* SRW Lock.etc */
#pragma comment(lib, "shell32.lib")
#endif

/* MBCS <-> Unicode */
#define MBCS2UNI(m, w, s) MultiByteToWideChar(CP_ACP, 0, m, -1, w, s)
#define UNI2MBCS(w, m, s) WideCharToMultiByte(CP_ACP, 0, w, -1, m, s, NULL, NULL)

/* UTF-8 <-> Unicode */
#define UTF82UNI(m, w, s) MultiByteToWideChar(CP_UTF8, 0, m, -1, w, s)
#define UNI2UTF8(w, m, s) WideCharToMultiByte(CP_UTF8, 0, w, -1, m, s, NULL, NULL)

static LONG WINAPI CrashDumpHandler(EXCEPTION_POINTERS *pException);

#else /* OS_POSIX */

#include <sys/types.h>		/* mkdir */
#include <sys/stat.h>		/* stat */
#include <sys/time.h>		/* gettimeofday */
#include <sys/wait.h>		/* waitpid */
#include <sys/resource.h>	/* setrlimit */
#include <fcntl.h>			/* copy_file */
#include <utime.h>			/* utime */
#include <signal.h>			/* SIGCHLD.etc */
#include <stddef.h>         /* ptrdiff_t type */

#ifdef OS_MACOSX
#include <sys/mount.h>      /* statfs */
#include <libproc.h>        /* proc_pidpath */
#else
#include <sys/statfs.h>		/* statfs */
#endif

#ifndef OS_ANDROID
#include <execinfo.h>		/* backtrace */
#endif

#ifdef _DEBUG
void crash_signal_handler(int n, siginfo_t *siginfo, void *act);
#endif

#endif /* OS_WIN */

/* stat���ļ�(>2GB) */
#if defined(COMPILER_MSVC)
#define STAT_STRUCT struct _stati64
#define stat _stati64
#else
#define STAT_STRUCT struct stat
#endif

/* ������� */
static char g_product_name[256];

/* �����ѳ�ʼ����־ */
static int g_cutil_inited;

/* �Ѵ򿪵��ļ��� */
static int g_opened_files;

/* �ⲿ���������� */
static crash_handler g_crash_handler;

/* �ж��źŴ����� */
typedef void (*interrupt_handler_func)(int);
static interrupt_handler_func g_interrupt_handler;

/* ����Ĭ�ϵ��жϴ����� */
int set_default_interrupt_handler();

/************************************************************************/
/*                         ��ʼ��/����ʹ�ñ���                          */
/************************************************************************/

void cutil_init()
{
    int ret ALLOW_UNUSED;
	g_cutil_inited = 1;

	/* ����ǰ����Ŀ¼Ϊ��ִ���ļ�����Ŀ¼ */
    ret = set_current_dir(get_execute_dir());
	ASSERT(ret);

	/* ��ʼ����־ģ�� */
	log_init();

	/* ����ʱ�ڴ���� */
#ifdef DBG_MEM_RT
	memrt_init();
#endif

	/* ��ʼ��������� */
	xstrlcpy(g_product_name, "AppUtils", sizeof(g_product_name));

	/* �жϴ����� */
	ret = set_default_interrupt_handler();
	ASSERT(ret);

	/* ����������� */
#ifdef USE_CRASH_HANDLER
	set_default_crash_handler();
#endif

#if defined(OS_POSIX)
	/* ���ֽ�/���ַ���ת�� */
	/* �����̵�locale����Ϊϵͳ��locale����������ʱĬ��Ϊ"C" */
	/* GLIBCʵ��Ϊ���β�ѯLC_ALL,LC_CTYPE,LANG�������� */
	setlocale(LC_CTYPE, "");

#ifndef OS_ANDROID
	/* ���ϵͳĬ�ϱ��� */
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
	/* ɾ����ʱĿ¼ */
	if (path_is_directory(get_temp_dir()))
		IGNORE_RESULT(delete_directories(get_temp_dir(), 0, 0));

	/* ����ļ�ʹ����� */
	if (g_opened_files)
		log_dprintf(LOG_NOTICE, "%d files still open!", g_opened_files);

	/* ����ڴ�ʹ����� */
#ifdef DBG_MEM_RT
	if (g_xalloc_count)
		log_dprintf(LOG_NOTICE, "Detected %d memory leaks!", g_xalloc_count);
	memrt_check();
#endif

	/* �ر���־ģ�� */
	log_close_all();
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
 * ���ó����յ��ж��źţ���Ctrl^C��ʱ�Ĵ�����
 * ��δ���ô˴��������������յ������źŵ�Ĭ����Ϊ����ֹ����
 * ע������յ��ж���Ϣ��������ֹ��ǰ���̣������ڴ���������ʽ�˳��������exit)
 * ��������int����������Ϣ���ͣ���CTRL_C_EVENT��SIGINT����ʵ�֣���һ����Ժ���
 * ����ֵ��ע��ɹ�����1��ʧ�ܷ���0
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
	error = sigaction(SIGINT, &act, NULL);	//Interrupt from keyboard
	error += sigaction(SIGQUIT, &act, NULL); //Termination signal
	error += sigaction(SIGTERM, &act, NULL);	//Quit from keyboard
	if (error)
		return 0;
	g_interrupt_handler = handler;
	return 1;
#endif
}

/* Ĭ�ϵ��ж��źŴ����� */
/* �����ն������ȫ�˳� */
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

/* ����Ĭ�ϵ��жϴ����� */
/* ������cutil_exit()�������ǰ���� */
int set_default_interrupt_handler()
{
	return set_interrupt_handler(default_interrupt_handler);
}

#ifdef USE_CRASH_HANDLER

/* ����Ĭ�ϵı��������� */
/* Windows������minidump�ļ������������pdb����ջ��ʾ�ڵ����Ի����� */
/* Linux������coredump�ļ����������debug���ӡ��ջ����׼�������(��ʹ��-rdynamic����ѡ��) */
void set_default_crash_handler()
{
#ifdef OS_WIN
	/* ���ص��Է��� */
	/* Release�汾�����pdb�ļ�Ҳ���Դ�ӡ��ջ */
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
	if (!SymInitialize(GetCurrentProcess(), NULL, TRUE))
		log_dprintf(LOG_WARNING, "Initialize debug symbol failed! %s", get_last_error_win32());

	//_CrtSetReportMode(_CRT_ASSERT, 0);	/* disable default assert dialog */

	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CrashDumpHandler);
#else /* POSIX */
	struct rlimit lmt;
	lmt.rlim_cur = RLIM_INFINITY;
	lmt.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &lmt);

#ifdef _DEBUG
	struct sigaction act;  
	sigemptyset(&act.sa_mask);     
	act.sa_flags = SA_SIGINFO;      
	act.sa_sigaction = crash_signal_handler;  
	sigaction(SIGSEGV, &act, NULL); //Invalid memory segment access
	sigaction(SIGFPE, &act, NULL);  //Floating point error
	sigaction(SIGABRT, &act, NULL);	//abort(), C++ exception.etc

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
#endif
#endif /* OS_WIN */
}

#endif

/************************************************************************/
/*                         Memory �ڴ����                               */
/************************************************************************/

#ifdef DBG_MEM

int g_xalloc_count = 0;

#ifdef DBG_MEM_RT
static void memrt_msg_c(int error, struct MEMRT_OPERATE *rtp);
#endif

/* �ͷ��ڴ� */
void free_d(void *ptr, const char* file, const char* func, int line)
{
	if (!ptr)
	{
		/* FATAL��־��Ϣ���¼���ö�ջ��exit */
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

/* �����ڴ� */
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

	/* ���ָ��Ϊ�գ���Ч��malloc */
	if (!ptr)
		return malloc_d(size, file, func, line);

	/* �����СΪ0����Ч��free */
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

/* �����ַ��� */
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
/*                         String �ַ�������                             */
/************************************************************************/

/* ���ַ�����ΪСд�������ַ������ı䷵��1�����򷵻�0 */
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

/* ���ַ�����Ϊ��д�������ַ������ı䷵��1�����򷵻�0 */
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

/* ��ȡһ���ַ�����Сд�濽�� */
char *strdup_lower (const char *s)
{
	char *copy, *p;

	copy = xstrdup(s);
	for (p = copy; *p; p++)
		*p = tolower (*p);

	return copy;
}

/* ��ȡһ���ַ����Ĵ�д�濽�� */
char *strdup_upper (const char *s)
{
	char *copy, *p;
	
	copy = xstrdup (s);
	for (p = copy; *p; p++)
		*p = toupper (*p);

	return copy;
}

/* ����һ���ַ������ִ� */
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

/* ��ȫ��vsnprintf */
/* �����÷���asnprintf */
int xvsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
#ifdef OS_WIN
#if _MSC_VER <= MSVC6
	/* �ú����ڻ�����������������Ҳ�᷵��-1 
	 * ��VC6�У���������errno��ֵ��
	 * �ڸ߰汾VC�У��Ὣerrno��ΪERANGE������ʧ������ΪEINVAL */
	int i = _vsnprintf(buffer, size, format, args);
	if (size > 0)
		buffer[size-1] = '\0';
	return i;
#else
	/* �߰汾VC֧��ģ��C99��׼��Ϊ */
	int length = _vsprintf_p(buffer, size, format, args);
	if (length < 0) {
		if (size > 0)
			buffer[0] = 0;
		return _vscprintf_p(format, args);
	}
	return length;
#endif
#else
	/* ע���е�POSIXʵ���ڻ�����������ʱҲ�᷵��-1�����Ὣerrno��ΪEOVERFLOW */
	return vsnprintf(buffer, size, format, args);
#endif
}

/* ����ֲ��snprintf */
int xsnprintf(char* buffer, size_t size, const char* format, ...)
{
	int result;
	va_list arguments;
	
	va_start(arguments, format);
	result = xvsnprintf(buffer, size, format, arguments);
	va_end(arguments);
	
	return result;
}

#ifndef _GNU_SOURCE

/* ����ֲ��asprintf */
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
		n = vsnprintf (str, size, fmt, args);
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
			if (errno != 0				/* VC6��������errno */
#ifdef OS_WIN
				&& errno != ERANGE		/* VC�߰汾��ΪERANGE */
#else
				&& errno != EOVERFLOW	/* ĳЩPOSIXʵ����ΪEOVERFLOW */
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

		if (size > 32 * 1024 * 1024)
		{
			ASSERT(!"Unable to asprintf the requested string due to size.");
			return 0;
		}

		str = (char*)xrealloc (str, size);
	}
}

#endif /* _GNU_SOURCE */

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
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
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

	return(dlen + (s - src));	/* count does not include NUL */
}

#ifndef __GLIBC__

/* ����һ���ַ�����ǰn���ֽ� */
char *strndup(const char* s, size_t n)
{
	size_t slen, len;
	char *p;

	ASSERT(s);

	slen = strlen(s);
    len = xmin(slen, n);
	p = (char*)malloc(len+1);	/* ��ʹ��xmalloc�����ظ������ڴ�������� */
	if (!p)
		log_dprintf(LOG_FATAL, "strndup malloc failed");

	strncpy(p, s, len);
	p[len] = '\0';

	return p;
}

/* 
 * �ָ��ַ���
 * �ú�����ı�Դ�ַ����������Դ���ո�����
 * ȡ�� GLIBC 2.16.0
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

/* �޿������������Ҫ�Լ�ʵ��strcasecmp, strncasecmp */
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

/* �����ִ�Сд�Ƚ������ַ��� */
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

/* ���ַ���s��ǰslen���ֽ��в���find */
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

/*
 * �����ִ�Сд�����ַ����Ƿ����ָ���ִ�
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

/* ͨ���򵥵ؽ�ÿ���ֽ���00101010�����ʵ�� */
void* memfrob(void *mem, size_t length)
{
	char *p = (char*)mem;

	while (length-- > 0)
		*p++ ^= 42;

	return mem;
}

#endif /* !__GLIBC__ */

/*
 * �����ִ�Сд�����ַ�����ǰ���ٸ��ֽ��Ƿ����ָ���ִ�
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
/*                         Filesystem �ļ�ϵͳ���	                    */
/************************************************************************/

/* �ж�·�����Ƿ���Ч */
/* ��·����Ч������·�����ַ����ĳ��ȣ����򷵻�0 */
static inline 
size_t path_valid(const char* path, int absolute)
{
	size_t len;

	/* ��·���������󶼶���ΪMAX_PATH��С�����·�������Ȳ��ܴ���MAX_PATH -1 */
	if (!path || !(len = strlen(path)) || len + 1 > MAX_PATH)
		return 0;

	if (absolute) {
		if (len < MIN_PATH)
			return 0;

#ifdef OS_WIN
		if (xisalpha(path[0]) && !(path[1] == ':' && path[2] == '\\') &&		/* ���ش��� */
			!(path[0] == '\\' && path[1] == '\\'))		/* ���繲�� */
			return 0;
#else
		if (path[0] != '/')
			return 0;
#endif
	}

	return len;
}

size_t strnlen(register const char *s, register size_t maxlen)
{
	const char *end= (const char *)memchr(s, '\0', maxlen);
	return end ? (size_t) (end - s) : maxlen;
}

/* �ж�path��ָ��·���Ƿ��Ǿ���·�� */
int	is_absolute_path(const char* path)
{
	if (!path || strnlen(path, MIN_PATH) != MIN_PATH)
		return 0;

#ifdef OS_WIN
	if ((xisalpha(path[0]) && path[1] == ':' && path[2] == '\\') ||	/* ���ش��� */
		(path[0] == '\\' && path[1] == '\\'))						/* ���繲�� */
		return 1;
#else
	if (path[0] == '/')
		return 1;
#endif

	return 0;
}

/* �ж�·���Ƿ��Ǹ�·�� */
int is_root_path(const char* path)
{
	return is_absolute_path(path) && (path[MIN_PATH] == '\0');
}

/* ����·�����ļ�������ײ�Ŀ¼�� 
 * ��1: C:\myfolder\a.txt -> a.txt ; /var/a.out -> a.out 
 * ��2: C:\myfolder\dir -> dir ; /var/temp -> temp 
 * ��3: C:\myfolder\dir\ -> dir\ ; /var/temp/ -> temp/
 * ��4: C:\ -> C:\ ; / -> /
 */
/* ע�������linuxϵͳ������locale����UTF8��������ܲ���ȷ */
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
		return path;			/* ���ַ��� */

	p = path + len - 1;

	while (*p == PATH_SEP_CHAR && p != path)
		--p;
	if (p == path)
		return p;				/*  ȫ�ָ��� */

	while (*p != PATH_SEP_CHAR && p != path)
		--p;

	if (p == path)				/* ���·�� */
		return p;
	else
		return p+1;
#endif
}

/* ����·������չ�� */
/* ��1��C:\\1.txt -> .txt */
/* ��2��C:\\abc -> "" */
/* ��4��C:\\abc. -> . */
/* ��5: NULL -> NULL */
/* ��6��"" -> "" */
/* ע�������linuxϵͳ������locale����UTF8��������ܲ���ȷ */
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

	while (*p != PATH_SEP_CHAR && *p != '.' && p != path)
		-- p;

	if (*p == '.')
		return p;
	
	return pe;
#endif
}

/* ����·����ָĿ¼/�ļ����ϼ�Ŀ¼·��(��������·���ָ���) */
/* ��1: C:\a.txt -> C:\ */
/* ��2: /usr/include/ -> /usr/ */
/* ����ֵ: �ɹ�����1�����󷵻�0����outbufΪ"" */
/* ע�������linuxϵͳ������locale����UTF8��������ܲ���ȷ */
int path_find_directory(const char *path, char* outbuf, size_t outlen)
{
	size_t plen, nlen;

	if (!path)
		return 0;

	plen = strlen(path);

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
		const char* p;

		p = path + plen - 1;
		while (*p == PATH_SEP_CHAR && p != path)
			--p;
		
		while (*p != PATH_SEP_CHAR && p != path)
			--p;

		if (p == path)
			return 0;
		
		nlen = p - path + 1 + 1;
		if (outlen < nlen)
			return 0;
		xstrlcpy(outbuf, path, nlen);
	
		return 1;
	}
#endif
}

/* ·����ָ�ļ�/Ŀ¼�Ƿ���� */
int path_file_exists(const char* path)
{
	if (!path_valid(path, 0))
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

/* ·���Ƿ�����ЧĿ¼ */
int path_is_directory(const char* path)
{
	if (!path_valid(path, 0))
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

/* ·���Ƿ����ļ��������Ҳ���Ŀ¼�� */
int path_is_file(const char* path)
{
	if (!path_valid(path, 0))
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
 * ��ȡ�ļ�/Ŀ¼����ڵ�ǰ����Ŀ¼�ľ���·��
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

	alen = strlen(buf);
	if (alen >= len)
		return 0;
	
	memcpy(buf, abuf, alen + 1);
	return 1;
#endif
}

/*
* ��ȡsrcָ��dst�����·��
* eg. src="/root/a/b/1.txt", dst="/2.txt", ret="../../../2.txt"
* eg. src="2.txt", dst="root/a/b/1.txt", ret="root/a/b/1.txt"
* ע1��src��dst����ͬʱ����Ի��������
* ע2�����ͬ�Ǿ������Ӷ�������windows�£�src��dst������ͬһ�̷���
* ע3��src�Ƿ���·���ָ�����β�������ͬ�����·������:
* eg. src="C:\\a", dst="C:\\a\\b\\1.txt", ret="a\\b\\1.txt"
* eg. src="C:\\a\\", dst="C:\\a\\b\\1.txt", ret="b\\1.txt"
 */
int relative_path(const char* src, const char* dst, char sep, char* outbuf, size_t slen)
{
  char sepbuf[3];
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
      if (*b == sep)
        start = (b - src) + 1;
    }
  src += start;
  dst += start;

  /* Count the directory components in B. */
  basedirs = 0;
  for (b = src; *b; b++)
    {
      if (*b == sep)
        ++basedirs;
    }

  sepbuf[0] = '.';
  sepbuf[1] = '.';
  sepbuf[2] = sep;

  if (slen < 3 * basedirs + strlen (dst) + 1)
	  return 0;
  memset(outbuf, 0, slen);

  /* Construct LINK as explained above. */
  for (i = 0; i < basedirs; i++)
    memcpy (outbuf + 3 * i, sepbuf, 3);
  
  strcpy (outbuf + 3 * i, dst);

  return 1;
}

/* ��ȡ���õ��ļ�·�� */
int unique_file(const char* path, char *buf, size_t len)
{
	const char* ext;
	size_t plen, elen;
	int i;

	plen = path_valid(path, 0);
	if (!plen)
		return 0;

	//�����ʼ·�������ڣ�ֱ�ӷ���
	if (!path_file_exists(path))
	{
		if (xstrlcpy(buf, path, len) >= len)
			return 0;

		return 1;
	}

	/* ��ȡ�ļ���չ�� */
	ext = path_find_extension(path);
	elen = strlen(ext);

	for (i = 1; ;i++)
	{
		if (len < plen + num_bits(i) + 4 + elen)	/* " ()"��'\0' */
			return 0;

		/* ʹ��strncpy �Ժ�����չ�� */
		strncpy(buf, path, plen - elen);
		sprintf(buf + plen - elen, " (%d)%s", i, ext);

		if (!path_file_exists(buf))
			return 1;
	}

	NOT_REACHED();
	return 0;
}

/* ��ȡ���õ�Ŀ¼·�� */
/* ���ص�·��������·���ָ�����β */
int unique_dir(const char* path, char *buf, size_t len)
{
	int has_slash, i;

	size_t plen = path_valid(path, 0); 
	if (!plen)
		return 0;

	/* �����ʼĿ¼�����ڣ�ֱ�ӷ��� */
	if (!path_file_exists(path))
	{
		if (xstrlcpy(buf, path, len) >= len)
			return 0;
	
		return 1;
	}

	/* ֱ��·�����" (N)"���� */
	has_slash = (path[plen-1] == PATH_SEP_CHAR ? 1 : 0);
	if (has_slash)
		plen --;

	for (i = 1; ;i++)
	{
		/* ��·�����ĳ��ȣ������� " (x)"��'\0' */
		size_t explen = plen + num_bits(i) + 4 + (has_slash ? 1 : 0);
		if (len < explen)				
			return 0;

		xstrlcpy(buf, path, len);
		sprintf(buf + plen, " (%d)", i);
		if (has_slash)
		{
			buf[explen-2] = PATH_SEP_CHAR;
			buf[explen-1] = '\0';
		}	
		if (!path_file_exists(buf))
			return 1;
	}

	NOT_REACHED();
	return 0;
}

#define U PATH_UNIX			/* Unix�����ã� / and \0 */
#define W PATH_WINDOWS		/* Windows�����ã� \|/<>?:*" */
#define C 4					/* �����ַ����� 0-31 */

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

/* ��·���еķǷ��ַ��滻Ϊ�ո�� */
/* ������滻Ϊ��������Ŀո񽫱��ϲ�Ϊһ�� */
/* reserved��ָ�����ַ���������(ͨ��Ϊ·���ָ���) */
void path_illegal_blankspace(char *path, int platform, int reserve_separator)
{
	char *p, *q;
	for (p = q = path; *p; p++) {
		if (PATH_CHAR_ILLEGAL(*p, platform) && 
			(!reserve_separator || *p != PATH_SEP_CHAR)) {
			if (*q != ' ')
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
 * ��·���еķǷ��ַ��滻Ϊ%HH����ʽ�����ⲿ�ͷ�
 * ע��path����ΪUTF-8����
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
			(!reserve_separator || *p != PATH_SEP_CHAR))
			++quoted;

	if (!quoted)
		return xstrdup(path);

	outlen = (pe - path) + (2 * quoted) + 1;
	out = (char*)xmalloc(outlen);
	q = out;

	for (p = path; p < pe; p++) {
		if (PATH_CHAR_ILLEGAL(*p, platform) &&
			(!reserve_separator || *p != PATH_SEP_CHAR)) {
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

/* ����Ŀ¼ */
struct walk_dir_context {
	/* ������·������������·���ָ�����β */
	char	basedir[MAX_PATH];

#ifdef OS_WIN
	HANDLE				hFind;
#ifdef USE_UTF8_STR
	WIN32_FIND_DATAW	wfd;
	char				filename[MAX_PATH];
#else
	WIN32_FIND_DATAA	wfd;
#endif
#else
	DIR*				dir;
	struct dirent*		pentry;
#endif
};

/* ��Ŀ¼����ȡ��һ�� */
struct walk_dir_context* walk_dir_begin(const char *dir)
{
	struct walk_dir_context* ctx = NULL;
	const char* p;
	size_t len;

#ifdef OS_WIN
	char buf[MAX_PATH + 3];   /* strlen("*.*") */
#endif

	len = path_valid(dir, 0);
	if (!len)
		return NULL;

	ctx = XCALLOC(struct walk_dir_context);
	strcpy(ctx->basedir, dir);

	p = dir + len - 1;
	if (*p != PATH_SEP_CHAR) {
		if (len + 1 == MAX_PATH)
			return NULL;

		ctx->basedir[len] = PATH_SEP_CHAR;
	}

#ifdef OS_WIN
	snprintf(buf, MAX_PATH, "%s*.*", ctx->basedir);
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

/* ����Ŀ¼�е���һ�� */
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

/* �����Ƿ���.(��ǰĿ¼) */
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

/* �����Ƿ���..(�ϼ�Ŀ¼) */
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

/* �����Ƿ���Ŀ¼ */
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

/* �����Ƿ����ļ� */
int walk_entry_is_file(struct walk_dir_context *ctx)
{
	if (!ctx)
		return 0;

    return !walk_entry_is_dir(ctx);
}

/* ��ͨ�ļ��Ķ����ǣ���Widnowsϵͳ�ϲ��������ļ���ϵͳ�ļ��ȣ�
 * ��Linuxϵͳ�ϲ��ǿ��豸���ַ��豸��FIFO�� */
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

/* ��ȡ����·���� */
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

/* ��ȡ����·���� */
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

/* �������� */
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
 * ��������Ŀ¼���ɹ�����1
 * ע�⣺Windows�´���Ŀ¼ʱĿ¼�����ߵĿո�ᱻ�Զ�ɾ����
 * �ļ�����ĩβ��.Ҳ�ᱻ�Զ�ɾ��
 */
int create_directory(const char *dir)
{
	if (!path_valid(dir, 0))
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
	return mkdir(dir, 0755) == 0;
#endif
}

/*
 * �ݹ鴴��Ŀ¼
 * ע�⣺1��·���������Ǿ���·����
 * 2�����·������"\"��β�������Ĳ��ִ���ΪĿ¼
 * ע��linux��dir����ΪUTF-8����
 */
int create_directories(const char* dir)
{
#ifdef OS_WIN
	wchar_t wdir[MAX_PATH];
	wchar_t *pb, *p, *pe;

	size_t len = path_valid(dir, 1);
	if (!len)
		return 0;
	else if (len == MIN_PATH)			/* "C:\", "/" */
		return 1;

#ifdef USE_UTF8_STR
	if (!UTF82UNI(dir, wdir, MAX_PATH))
		return 0;
#else
	if (!MBCS2UNI(dir, wdir, MAX_PATH))
		return 0;
#endif

	pb = wdir;
	pe = wdir + len;

	p = pb + MIN_PATH;
	while((p = wcschr(p, PATH_SEP_WCHAR)))
	{
		*p = L'\0';

		if (PathFileExistsW(pb))
		{
			if (!PathIsDirectoryW(pb))
				return 0;
		}
		else if (!CreateDirectoryW(pb, NULL))
			return 0;

		*p = PATH_SEP_WCHAR;

		if (++p >= pe)
			break;
	}

	return 1;

#else //Linux
	char *pb, *p, *pe;
	char buf[MAX_PATH+1];

	size_t len = path_valid(dir, 1);
	if (!len)
		return 0;
	else if (len == MIN_PATH)			/* "C:\", "/" */
		return 1;

	strcpy(buf, dir);

	pb = buf;							/* path begin */
	pe = buf + len;						/* path end */

	p = pb + MIN_PATH;							/* "a\b\c\d.txt */
	while((p = strchr(p, PATH_SEP_CHAR)))
	{
		*p = '\0';

		if (path_is_file(pb) || (!path_is_directory(pb) && !create_directory(pb)))
			return 0;

		*p = PATH_SEP_CHAR;

		if (++p >= pe)
			break;								/* ��"/"��β */
	}

	return 1;
#endif
}

/* ɾ��һ����Ŀ¼ */
int delete_directory(const char *dir)
{	
	if (!path_valid(dir, 0))
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

#define TRAV_RETURN_0 do{	\
	walk_dir_end(ctx);		\
	return 0;				\
	}while(0)

/* �ݹ�ɾ��Ŀ¼�µ����� */
static int _delete_directories(const char *dir, delete_dir_cb func, void *arg)
{
	struct walk_dir_context* ctx = NULL;
	char buf[MAX_PATH];
	int succ;

	ctx = walk_dir_begin(dir);
	if (ctx) {
		do {
			if (walk_entry_is_dot(ctx)|| walk_entry_is_dotdot(ctx))
				continue;
			else if (likely(walk_entry_path(ctx, buf, MAX_PATH))) {
				if (!walk_entry_is_dir(ctx)) {		//�ļ�
					succ = delete_file(buf);
					if ((func && !func(buf, 0, succ, arg)))
						TRAV_RETURN_0;
				} else {							//Ŀ¼
					succ = _delete_directories(buf, func, arg);
					if ((func && !func(buf, 1, succ, arg)))
						TRAV_RETURN_0;
				}
			} else
				TRAV_RETURN_0;
		}while(walk_dir_next(ctx));

		walk_dir_end(ctx);
	} else
		return 0;

	if (!delete_directory(dir))
		return 0;

	return 1;
}

int delete_directories(const char *dir, delete_dir_cb func, void *arg)
{
	if (!path_valid(dir, 0))
		return 0;

	return _delete_directories(dir, func, arg);
}

/* �ж�Ŀ¼�Ƿ��ǿ�Ŀ¼ */
int is_empty_dir(const char* dir)
{
	struct walk_dir_context* ctx = NULL;

	if (!path_is_directory(dir))
		return 0;

	ctx = walk_dir_begin(dir);
	if (ctx) {
		do {
			if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
				continue;
			else 
				TRAV_RETURN_0;
		} while (walk_dir_next(ctx));

		walk_dir_end(ctx);
	} else
		return 0;

	return 1;
}

static int _delete_empty_directories(const char* dir)
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
				if (walk_entry_is_dir(ctx)) {		//Ŀ¼
					if (!_delete_empty_directories(buf))
						empty = 0;
				} else								//�ļ�
					empty = 0;
			} else
				TRAV_RETURN_0;
		} while(walk_dir_next(ctx));

		walk_dir_end(ctx);
	} else
		return 0;

	return empty && delete_directory(dir);
}

/*
 * ɾ��Ŀ¼�µ����п�Ŀ¼
 * ��ɾ���ļ���������Ŀ¼����
 * ����ֵ������Ŀ¼��û���κ��ļ�ʱ����1�����򶼷���0
 */
int delete_empty_directories(const char* dir)
{
	if (!path_valid(dir, 0))
		return 0;

	/* ����������Ŀ¼���������ļ�ʱ����1 */
	/* ��ʱ_delete_empty_directoriesҲ��ɾ������Ŀ¼ */
	if (_delete_empty_directories(dir))
	{
		IGNORE_RESULT(create_directory(dir)); //FIXME: handle failure
		return 1;
	}

	return 0;
}

/* �ݹ鿽��Ŀ¼
 * curdir��ָ����Ҫ������Ŀ¼��srcdir��dstdir�ǲ���ģ�����ָ�������Ŀ¼���ұ��붼��·���ָ�����β
 * �ص���������action��0��ʾ�����ļ���1��ʾ����Ŀ¼��2��ʾ����Ŀ¼
 */
static int _copy_directories(char *curdir, const char *srcdir, const char *dstdir,
							copy_dir_cb func, void *arg)
{
	struct walk_dir_context* ctx = NULL;
	char spath[MAX_PATH+1];				//������·��
	char dpath[MAX_PATH+1];				//������·��;
	size_t srclen = strlen(srcdir);
	size_t dstlen = strlen(dstdir);
	int succ;

	ctx = walk_dir_begin(curdir);
	if (!ctx)
		return 0;

	//����Ŀ¼
	do {
		if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
			continue;
		else if (likely(walk_entry_path(ctx, spath, MAX_PATH))) {
			char *partial = spath + srclen;		    //�ļ������·��
			size_t	partlen = strlen(partial);		//���·������

			if (dstlen + partlen + 1 > sizeof(dpath))
				TRAV_RETURN_0;

			snprintf(dpath, sizeof(dpath), "%s%s", dstdir, partial);

			if (!walk_entry_is_dir(ctx)) {		    //�ļ�
				//�������ļ�
				succ = copy_file(spath, dpath, 1);
				if (func && !func(spath, dpath, 0, succ, arg))
					TRAV_RETURN_0;
			} else {							    //Ŀ¼
				//������Ŀ¼
				succ = create_directory(dpath);
				if (func && !func(spath, dpath, 2, succ, arg))
					TRAV_RETURN_0;

				//�ݹ鿽��Ŀ¼
				succ = _copy_directories(spath, srcdir, dstdir, func, arg);
				if (func && !func(spath, dpath, 1, succ, arg))
					TRAV_RETURN_0;
			}
		} else
			TRAV_RETURN_0;

	} while (walk_dir_next(ctx));

	walk_dir_end(ctx);

	return 1;
}

/* �ݹ鸴��Ŀ¼(��ԴĿ¼���Ƶ�Ŀ��Ŀ¼��)
 * �罫/aĿ¼���Ƶ�/bĿ¼�£����γɵ�Ŀ¼��/b/a/

 * ע��1��ʧ���������
 * 1��Ŀ��Ŀ¼�²�����ͬԭĿ¼����ͬ��Ŀ¼������/b/a/֮ǰ�ʹ��ڣ���ô��ʧ��
 * 2�����Ŀ��Ŀ¼����ͬԴĿ¼����ͬ���ֵ��ļ�������/b/a��һ���ļ����Ǿ�Ҳ��ʧ��
 *
 * ע��2 ����������¸����ǲ�����ģ�
 * 1��ԴĿ¼��Ŀ��Ŀ�겻����ͬ���ܲ��ܰ��Լ��������Լ������ -_-||
 * 2��Ŀ��Ŀ¼��ԴĿ¼����Ŀ¼�����罫/usr���Ƶ�/usr/include/��ȥ�������������ѭ��
 * 3��ԭĿ¼�Ѿ���Ŀ��Ŀ¼�£��罫/usr/include/���Ƶ�/usr/�£���������ø���
 *
 * ע��3 ��Windows�£����ֱ�ӽ�C:\���Ƶ�D:\���ᴴ��D:\C_DRIVE\...
 * ��Linux�£��ǲ����ܰ�/���Ƶ���������·���ģ���ΪΥ���˹���2

 * ע��4 Linux��·�����������ִ�Сд�ģ���a��A��ͬʱ������һ��Ŀ¼��
 * ����Windows�²���ʱ��������ͬһĿ¼�´����Ѵ����ļ�/Ŀ¼���Ĵ�Сд��ͬ�汾
 * �����Linux��ΪNTFS/FAT�ļ�ϵͳ�����˽���Сд��ͬ���ļ�/Ŀ¼�����л���Windows���ǲ���Ԥ���
 * Ϊ�˱�д����ֲ��Ӧ�ó��򣬲�Ӧ���Դ�Сд�����ֲ�ͬ���ļ���Ŀ¼��
 */
int copy_directories(const char *src, const char *dst, 
					copy_dir_cb func, void *arg)
{
	char sdir[MAX_PATH+1], ddir[MAX_PATH+1];
	char dbuf[MAX_PATH+1], lastdir[256];	/* Linux֧�ֵ���ļ���/Ŀ¼��Ϊ255 */
	char *p;
	int ret;	

	size_t slen = path_valid(src, 0);
	size_t dlen = path_valid(dst, 0);

	if (!slen || !dlen)
		return 0;

	if (!path_is_directory(src) || !path_is_directory(dst))
		return 0;

	/* ȷ��Ŀ¼�Էָ�����β */
	strcpy(sdir, src);
	if (sdir[slen-1] != PATH_SEP_CHAR) {
		sdir[slen++] = PATH_SEP_CHAR;
		sdir[slen] = '\0';
	}

	strcpy(ddir, dst);
	if (ddir[dlen-1] != PATH_SEP_CHAR) {
		ddir[dlen++] = PATH_SEP_CHAR;
		ddir[dlen] = '\0';
	}

	/* ������1 */
#if defined OS_WIN
	if (!strcasecmp(sdir, ddir))
		goto copy_dirs_error;
#else
	if (!strcmp(sdir, ddir))
		goto copy_dirs_error;
#endif

	/* ������2 */
#ifdef OS_WIN
	p = strcasestr(ddir, sdir);
#else
	p = strstr(ddir, sdir);
#endif

	if (p == ddir)
		goto copy_dirs_error;

	/* ������3 */
#ifdef OS_WIN
	p = strcasestr(sdir, ddir);
#else
	p = strstr(sdir, ddir);
#endif

	if (p == sdir)
	{
		char *q = sdir + dlen;
		char *end = sdir + slen;
		int slashes = 0;

		for (;q < end; q++)
			if (*q == PATH_SEP_CHAR)
				slashes++;

		if (slashes == 1)
			goto copy_dirs_error;
	}

	/* ��Ŀ��Ŀ¼�´���ͬ���ļ��У���Ҫ�ҳ�ԴĿ¼�����һ��Ŀ¼�� */
	/* path_find_file_name����ֵ��/��β,��Ҫע��ԴĿ¼��C:\����� */
	p = (char*)path_find_file_name(sdir);
	if (!p)
		goto copy_dirs_error;

	memset(lastdir, 0, sizeof(lastdir));
#ifdef OS_WIN
	if (strlen(p) == MIN_PATH)	/* C:\ */
	{
		lastdir[0] = p[0];
		strcpy(lastdir+1, "_DRIVE\\");
	}
	else
		xstrlcpy(lastdir, p, sizeof(lastdir));
#else
	ASSERT(strlen(p) != MIN_PATH);
	xstrlcpy(lastdir, p, sizeof(lastdir));
#endif

	memset(dbuf, 0, sizeof(dbuf));
	strcpy(dbuf, ddir);
	xstrlcat(dbuf, lastdir, sizeof(dbuf));

	/* ���Ŀ��Ŀ¼���Ѿ����ڴ�Ŀ¼���ļ� */
	dbuf[strlen(dbuf)-1] = '\0';
	if (path_file_exists(dbuf)
	   ||!create_directory(dbuf))
		goto copy_dirs_error;

	/* �ݹ鸴�� */
	dbuf[strlen(dbuf)] = PATH_SEP_CHAR;
	ret = _copy_directories(sdir, sdir, dbuf, func, arg);

	return ret;

copy_dirs_error:

	return 0;
}

/* 
 *  ���Ҳ�����Ŀ¼�µ�ÿ���ļ� 
 * ��������recursively���Ƿ�ݹ�����ļ���regular_only���Ƿ��������ͨ�ļ�
 * ������ֵ��1��ʾ�ɹ�����ÿ�����ֵ��ļ���0��ʾ�����������������0
 */
int foreach_file(const char* dir, foreach_file_func_t func, int recursively, int regular_only, void *arg)
{
	struct walk_dir_context* ctx = NULL;
	char buf[MAX_PATH+1];

	ctx = walk_dir_begin(dir);
	if (ctx) {
		do {
			if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
				continue;
			else if (likely(walk_entry_path(ctx, buf, MAX_PATH))) {
				if (walk_entry_is_dir(ctx)){				//Ŀ¼
					if (recursively){
						if (!foreach_file(buf, func, recursively, regular_only, arg))
							TRAV_RETURN_0;
					}
				} else {										//�ļ�
					if (!regular_only || walk_entry_is_regular(ctx)){
						if (!(*func)(buf, arg))
							TRAV_RETURN_0;
					}
				}
			} else
				TRAV_RETURN_0;
		}while(walk_dir_next(ctx));

		walk_dir_end(ctx);
	} else
		return 0;

	return 1;
}

/* 
 *  ���Ҳ�����Ŀ¼�µ�ÿ��Ŀ¼
 * ������ֵ��1��ʾ�ɹ�����ÿ�����ֵ�Ŀ¼��0��ʾ�����������������0
 */
int foreach_dir(const char* dir, foreach_dir_func_t func, void *arg)
{
	struct walk_dir_context* ctx = NULL;
	char buf[MAX_PATH+1];

	ctx = walk_dir_begin(dir);
	if (ctx) {
		do {
			if (walk_entry_is_dot(ctx) || walk_entry_is_dotdot(ctx))
				continue;
			else if (walk_entry_is_dir(ctx)) {				//Ŀ¼
				if (walk_entry_path(ctx, buf, MAX_PATH)) {
					if (!(*func)(buf, arg))
						TRAV_RETURN_0;	
				} else
					TRAV_RETURN_0;
			}
		} while (walk_dir_next(ctx));

		walk_dir_end(ctx);
	} else
		return 0;

	return 1;
}

/* ɾ���ļ� */
int delete_file(const char *path)
{
	if (!path_valid(path, 0))
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

/* ɾ���ļ������ϵĿ�Ŀ¼ֱ��ָ��Ŀ¼
 * ɾ���ļ������鿴�ļ����ڵ�Ŀ¼�Ƿ��ǿ�Ŀ¼������ǿ�Ŀ¼�Ҳ���top_dir������ɾ����Ŀ¼
 * ����������Ѱ�������ǿ�Ŀ¼���Ѵﵽtop_dirΪֹ
 * path��top_dir����ͬʱ�Ǿ���·���������·����top_dir�Ƿ���·���ָ�����β��Ӱ��
 * ע�����κ�����£��Ѿ�ȷ������ɾ����Ŀ¼(/��C:\)��������һ�����ѣ�
 */
int	delete_file_empty_updir(const char* path, const char* top_dir)
{
	char pbuf[MAX_PATH], *p;
	size_t top_len;

	/* path������top_dirĿ¼�µ��ļ� */
	if (!path_valid(path, 0) 
		|| !path_valid(top_dir, 0) 
		|| strstr(path, top_dir) != path)
		return 0;

	/* ɾ��ָ���ļ�
	 * ����ļ�ɾ��ʧ�ܣ�����·���������ޣ������ļ�����ʹ�ã�����...
	 * ���Լ���ɾ��Ŀ¼Ҳ���п��ܻ�ʧ�ܣ����ֱ�ӷ���ʧ��ֵ
	 */
	if (!delete_file(path))
		return 0;

	/* ����·���������� */
	xstrlcpy(pbuf, path, MAX_PATH);

	/* ��ֹĿ¼��·�����ȣ�������β��·���ָ��� */
	top_len = strlen(top_dir);
	if (top_dir[top_len-1] != PATH_SEP_CHAR)
		top_len++;

	/* ��ɾ���ϲ��Ŀ¼ */
	while((p = strrchr(pbuf, PATH_SEP_CHAR)))
	{
		*(p+1) = '\0';
		if (is_root_path(pbuf)					//Oops! ��Ҫɾ�����ļ�ϵͳ��
			|| (size_t)(p - pbuf) < top_len		//���ﶥ��Ŀ¼
			|| !is_empty_dir(pbuf))				//�����ǿ�Ŀ¼
			return 1;

		if (!delete_directory(pbuf))
			return 0;

		*p = '\0';
	}

	return 1;
}

/* �����ļ� */
int copy_file(const char *exists, const char *newfile, int overwritten)
{
	if (!path_valid(exists, 0) || !path_valid(newfile, 0))
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
 * �ƶ��ļ� (�����ƶ�Ŀ¼)
 * �����·��λ��ͬһ�ļ�ϵͳ�У�������ļ����������������أ�
 * ���򣬽�Դ�ļ����������ļ�λ�ã���ɾ��Դ�ļ���
 * ע�⣺1�����ļ������ڵ�Ŀ¼������ڡ�
 * 2�����Դ�ļ���Ŀ���ļ���ͬ�����ɹ�����
 * TODO:���exists��newfile��ָ��ͬһ���ļ���Ӳ������δ���
 */
int move_file(const char* exists, const char* newfile, int overwritten)
{
	if (!path_valid(exists, 0) || !path_valid(newfile, 0))
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
		/* λ�ڲ�ͬ�ļ�ϵͳ�� */
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

/* ��ȡ�ļ����� */
int64_t file_size(const char* path)
{
#ifdef OS_WIN
	HANDLE handle;
	DWORD length;
#ifdef USE_UTF8_STR
	wchar_t wpath[MAX_PATH];
	if (!path_valid(path, 0))
		return -1;
	if (!UTF82UNI(path, wpath, MAX_PATH))
		return -1;
	handle = CreateFileW(wpath, FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
#else /* MBCS */
	handle = CreateFileA(path, FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
#endif
	if (handle == INVALID_HANDLE_VALUE)
		return -1;
	if (!GetFileSize(handle, &length))
	{
		CloseHandle(handle);
		return -1;
	}
	CloseHandle(handle);
	return length;
#else /* POSIX */
	struct stat st;
	if (!path_valid(path, 0))
		return -1;
	if (stat(path, &st))
		return -1;

	return st.st_size;
#endif
}

/* ��ȡ�ļ���Ŀ¼�����ļ�ϵͳ��������Ĵ�С */
int get_file_block_size(const char *path)
{
#ifdef OS_WIN
		DWORD SectorsPerCluster; 
		DWORD BytesPerSector; 
		DWORD NumberOfFreeClusters;
		DWORD TotalNumberOfClusters;
		char drive[4];

		/* ����Ƿ��Ǿ���·�� */
		if (!path_valid(path, 1))
			return 0;

		strncpy(drive, path, 3);
		drive[3] = '\0';

		/* ��ȡ���̷�����/���С */
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

/* ��ȡ�ļ�ʵ��ռ�ô��̿ռ�Ĵ�С */
/* ����file�����Ǿ���·�� */
/* ��Linux�£�Ŀ¼�ļ���Ҳ��ռ�ô��̿飬��Windows�²�ռ�ô��̿ռ� */
/* ���ļ�������Ϊ0������ռ�ô��̿ռ� */
int64_t get_file_disk_usage(const char *path)
{
#ifdef OS_WIN
	int blocksize;
	int64_t fsize;

	blocksize = get_file_block_size(path);
	if (blocksize <= 0)
		return 0;

	/* Ŀ¼��ռ�ռ� */
	fsize = file_size(path);
	if (fsize <= 0)
		return 0;

	return (fsize + blocksize) & (~(blocksize - 1));
#else
	struct stat st;
	if (stat(path, &st))
		return 0;

	return st.st_blocks * 512;
#endif
}

/* �����ļ�ʵ�ʴ�С�ͷ������С��ȡʵ��ռ�ô��̿ռ� */
int64_t compute_file_disk_usage(int64_t real_size, int block_size)
{
	return (real_size + block_size) & (~(block_size - 1));
}

/* ��ȡ�ɶ���ǿ���ļ���С */ 
/* ��3.51GB, 2.2MB, INT64_MAX ��߿ɴ� 8EB */
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
	
#define reach_size_unit(fmt, unit)	\
	else if (size >= unit)	\
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

/* �ر��ļ� */
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

/* ��ȡ�ļ�
 * ��ȡ�ļ�ֱ����
 * 1��separator>=0 ? ����separatorΪֹ��
 * 2��maxcount>=0 ? ���maxcount���ֽ�Ϊֹ
 * 3���ļ�ĩβ��
 * ����ֵ��1����ɹ�0����ʧ�ܣ�����ɹ���ȡ�������ݽ������*outbuf���ҳ���Ϊ*outlen
 * ����������ָ��������������ص����ݽ������ָ������κ�����¾���'\0'��β��
 */
int xfread(FILE* fp, int separator, size_t max_count,
			char** outbuf, size_t* outlen)
{
    char *content;
	size_t n = 0, bufsize = 1024;
    int ch;

	if (!fp) {
		ASSERT(!"xfread fp is null");
		return 0;
	}

	content = (char*)xmalloc(bufsize);

	while ((ch = getc(fp)) != EOF) {
		if (n == bufsize - 1) {	 /* -1 to reserver space for lastly '\0' */
			if (n > SIZE_T_MAX /2) {
				/* file too large */
				xfree(content);
				return 0;
			} else {
				/* use a larger buffer */
				bufsize <<= 1;
				content = (char*)xrealloc(content, bufsize);
			}
		}

		content[n++] = ch;

		if (ch == separator || 
			--max_count == 0)
			break;
	}
	
	content[n] = '\0';

	*outbuf = content;
	*outlen = n;

	return 1;
}

/* ���ļ������ڴ� */
/* ����ļ������ڻ����1GB��������NULL */
/* ���max_size�����㣬������ļ���СС��ָ��ֵʱ�ż��� */
/* ����ļ��ĳ���Ϊ0����contentΪNULL����length����0 */
/* ��������ڴ�ɹ�������������'\0'��β */
struct file_mem* read_file_mem(const char* file, size_t max_size)
{
	struct file_mem *fm;
	FILE *fp;
	int64_t length;
	size_t readed;

	if (!path_valid(file, 0))
		return NULL;

	/* ��ȡ�ļ���С */
	length = file_size(file);
	if (length < 0 || length > (1<<30) || 
		(max_size > 0 && length > (int64_t)max_size))
		return NULL;

	/* �򿪶�ȡ�ļ� */
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

	/* �����ڴ沢���� */
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

/* �ͷŶ�ȡ���ڴ��е��ļ� */
void free_file_mem(struct file_mem *fm)
{
	if (fm)
	{
		if (fm->content)
			xfree(fm->content);	
		xfree(fm);
	}
}

/* ���ڴ��е�����д���ļ� */
/* ����append:����ļ��Ѵ��ڣ��Ƿ�׷�����ݵ��ļ����������Դ�ļ����ݣ� */
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

/* ���ڴ��е�����д���ļ� */
/* �ú�������ֱ�Ӵ��Ѵ��ڵ��ļ�����д�룬��Ϊ�⽫��ض�֮ǰ���ļ� */
/* �������ļ����ڵ��ļ�ϵͳ�д���һ��ʱ�ļ�����д����ʱ�ļ��ɹ�����������Ϊָ���ļ� */
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

/* 
 * ���ļ��ж���������ָ���Ϊ��β��һ������
 * ����ָ�����'\n'�����Ǵ��ļ��ж�ȡһ��(���µ�get_line����)
 * �������������*lineptr��ΪNULL,�Ҷ������ݵĳ���С��*n,������д����ԭ�������У�
 * ���򽫶�̬����һ���ڴ����ڴ�Ŷ������ݣ����*lineptrΪNULL�������Ƕ�̬�����ڴ棻
 * ��ע�⡿������Ļ�������������󽫲��ᱻ�ͷ�(��ͬ��GLIBC��getline)�������ζ��
 * *lineptr�������������alloca���ڴ档�������Լ��ٶ�̬����/�ͷ��ڴ�Ĵ�����
 * ����ֵ�����سɹ�������ֽ����������ָ���������������β��'\0'���������ֵΪ0��������
 *        �ļ�Ϊ�ջ��ļ�̫��
 */
size_t get_delim(char **lineptr, size_t *n, int delim, FILE *fp)
{
	char *origptr, *mallptr;
	size_t count, nm = 1024;
	int ch;

	origptr = *lineptr;
	mallptr = NULL;
	count = 0;

	while ((ch = getc(fp)) != EOF)
	{
		if (origptr && count < *n - 1)	// -1 to reserve space for '\0'
			origptr[count++] = ch;
		else if (origptr && !mallptr)	// count == n - 1
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

		//�ָ���
		if (ch == delim)
			break;
	}

	//������
	if (mallptr)
		mallptr[count] = '\0';
	else if (origptr && *n > 0)
		origptr[count] = '\0';

	//�������
	if (mallptr)
		*lineptr = mallptr;
	*n = count;

	return count;
}

/*
 * ���ļ��ж���һ��
 * ������GLIBC��getline������C++Ҳʵ����ȫ�ֵ�getline
 * �����foreach_line�����ܺõ�ڹ�������ʹ�ô˺���
 */
size_t get_line(char **lineptr, size_t *n, FILE *fp)
{
	return get_delim(lineptr, n, '\n', fp);
}

/* ���ļ��ж�ȡÿ���ָ�������в��� */
int	foreach_block(FILE* fp, foreach_block_cb func, int delim, void *arg)
{
	char buf[1024], *block;
	size_t len, nblock;
	int ret;

	block = buf;
	len = sizeof(buf);
	nblock = 0;

	while (get_delim(&block, &len, delim, fp) > 0)
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

/* ���ļ����ж���ÿһ�в����в��� */
int	foreach_line(FILE* fp, foreach_line_cb func, void *arg)
{
	return foreach_block(fp, func, '\n', arg);
}

/* ��ȡ�ļ�ϵͳ��ʹ��״̬ */
/* ����path�����Ǿ���·�� */
int get_fs_usage(const char* path, struct fs_usage *fsp)
{
	if (!path_valid(path, 1) || !fsp)
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

/* ��ȡ��ǰ�����ļ���·��(argv[0]) */
/* ʧ�ܷ��ؿ��ַ��� */
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

/* ��ȡ���̵��ļ���(������չ��) */
/* ʧ�ܷ��ؿ��ַ��� */
const char* get_execute_name()
{
	const char *path, *slash_end;

	path = get_execute_path();
	if (!path[0])
		return path;

	slash_end = strrchr(path, PATH_SEP_CHAR);
	if (slash_end)
		return slash_end+1;

	return path + strlen(path);
}

/* ����Ŀ¼ */
/* ��ȡ������ʼĿ¼����ִ���ļ�����Ŀ¼��,��·���ָ�����β */
/* ����ֵ���ɹ���ȡ����ָ��·���ַ�����ʧ�ܷ��ؿ��ַ��� */
const char* get_execute_dir()
{
	static char dir[MAX_PATH];
	static int init = 0;

	if (!init)
	{
		const char* path = get_execute_path();
		if (path[0])
		{
			const char *slash_end = strrchr(path, PATH_SEP_CHAR);
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

// /* ��ȡ��ǰ����ģ���·�� */
// /* ����ִ���ļ���̬���ӿ��·�� */
// const char* get_module_path()
// {
// 	static char path[MAX_PATH];
// 
// #ifdef OS_WIN
// 	HMODULE instance = NULL;
// 	GetModuleHandleExA(
// 		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
// 		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
// 		(char*)get_module_path, 
// 		&instance);
// 
// #ifdef USE_UTF8_STR
// 	{
// 		wchar_t wpath[MAX_PATH];
// 		if (!GetModuleFileNameW(instance, wpath, MAX_PATH) ||
// 			!UNI2UTF8(wpath, path, MAX_PATH))
// 			path[0] = '\0';
// 	}
// #else
// 	if (!GetModuleFileNameA(instance, path, MAX_PATH))
// 		path[0] = '\0';
// #endif
// #else
// 	NOT_IMPLEMENTED();
// 	path[0] = '\0';
// #endif
// 	return path;
// }

/* ��ȡ���̵�ǰ����Ŀ¼ */
/* ����ֵ���ɹ���ȡ����ָ��·���ַ�����ʧ�ܷ��ؿ��ַ��� */
/* ע����Ϊ��ǰ����Ŀ¼���ܸı䣬��˲���ʹ��һ�γ�ʼ�� */
const char* get_current_dir()
{
	static char path[MAX_PATH];
	size_t len;

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

	len = strlen(path);
	if (len > 0 && len < MAX_PATH + 1)
	{
		if (path[len-1] != PATH_SEP_CHAR)
			path[len] = PATH_SEP_CHAR;
	}
	else
		path[0] = '\0';

	return path;
}

/* ���ý��̵�ǰ�Ĺ���Ŀ¼ */
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
/* ��ȡWindows������Ŀ¼��·��, path�ĳ���Ӧ����ΪMAX_PATH */
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

/* ��ȡ��ǰ�û�����Ŀ¼����·���ָ�����β */
/* ����ֵ����ȡ�ɹ�����·���ַ�����ʧ�ܷ��ؿ��ַ��� */
const char*get_home_dir()
{
	static char path[MAX_PATH];
	const char* val;
	size_t len;

	val = get_env("HOME");
	if ((len = path_valid(val, 1))) {
		xstrlcpy(path, val, MAX_PATH);
		goto success;
	}

#ifdef OS_WIN
	{
		wchar_t wpath[MAX_PATH];
		wchar_t* last_slash;
		if (!SHGetSpecialFolderPathW(NULL, wpath, CSIDL_DESKTOP, 0))
			goto failed;

		last_slash = wcsrchr(wpath, '\\');
		if (!last_slash || (len = last_slash - wpath) <= MIN_PATH - 1)
			goto failed;

		wpath[++len] = '\0';

#ifdef USE_UTF8_STR
		if (!UNI2UTF8(wpath, path, MAX_PATH))
#else /* MBCS */
		if (!UNI2MBCS(wpath, path, MAX_PATH))
#endif /* USE_UTF8_STR */
			goto failed;

		return path;
	}

failed:
    path[0] = '\0';
    return path;

#endif /* OS_WIN */

success:
	if (path[len-1] != PATH_SEP_CHAR)
		path[len] = PATH_SEP_CHAR;

	return path;
}

/* ��ȡ����Ļ����ļ�Ŀ¼ */
/* Windows��λ��C:\Users\xxx\AppData\Local\PACKAGE_NAME\ */
/* Linux��λ��~/.PACKAGE_NAME��ͬ get_config_dir() */
const char* get_app_data_dir()
{
	static char path[MAX_PATH];

#ifdef OS_WIN
	if (!GetShellSpecialFolder(CSIDL_APPDATA, path))
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

/* ��ȡӦ�ó�����ʱĿ¼ */
/* ��Windows�·����� C:\Windows\Temp\SafeSite\ */
/* ��Linux�·����� /tmp/SafeSite/ */
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
		len = strlen(path);
		if (path[len-1] != PATH_SEP_CHAR)
			path[len++] = PATH_SEP_CHAR;
#else /* OS_POSIX */
		strcpy(path, "/tmp/");
		len = 5;
#endif /* OS_WIN */

		if (len + strlen(g_product_name) < MAX_PATH - 2)
		{
			strcpy(path + len, g_product_name);
			strcat(path, PATH_SEP_STR);
			if (path_is_file(path))
				IGNORE_RESULT(delete_file(path));
			if (!path_is_directory(path) && 
				!create_directory(path))
				path[0] = '\0';
		}
		else
			path[0] = '\0';

		init = 1;
	}

	return path;
}

/* ��ȡָ��Ŀ¼�¿��õ���ʱ�ļ�·�� */
/* ���ָ������ʱĿ¼�����ڣ���������Ŀ¼ */
/* ���prefixΪNULL��Ϊ���ַ�������ʹ��"temp"��Ϊǰ׺ */
/* ע1������������ĳ���Ӧ����ΪMAX_PATH */
/* ע2���˺�������֤�ڵ���ʱ����ʱ�ļ������� */
int get_temp_file_under(const char* tmpdir, const char *prefix, 
	char *outbuf, size_t outlen)
{
	const char *prefix_use = "temp";
	if (prefix && prefix[0])
		prefix_use = prefix;

	if (!tmpdir || !outbuf || outlen < MAX_PATH)
		return 0;

	if (path_is_file(tmpdir) 
		||!create_directories(tmpdir))
		return 0;

#ifdef OS_WIN
#ifdef USE_UTF8_STR
	{
		wchar_t wtmpdir[MAX_PATH], wtmpfile[MAX_PATH], wprefix[128];
		if (!UTF82UNI(tmpdir, wtmpdir, MAX_PATH)
			|| !UTF82UNI(prefix_use, wprefix, sizeof(wprefix))
			|| !GetTempFileNameW(wtmpdir, wprefix, 0, wtmpfile)
			|| !UNI2UTF8(wtmpfile, outbuf, outlen))
			return 0;
		return 1;
	}
#else /* MBCS */
	if (!GetTempFileNameA(tmpdir, prefix_use, 0, tmpfile))
		return 0;
	return 1;
#endif
#else /* POSIX */
	{
		char *tempname = tempnam(tmpdir, prefix_use);
		if (!tempname)
			return 0;
		xstrlcpy(outbuf, tempname, outlen);
		free(tempname);
	}
	return 1;
#endif
}

/* ��Ĭ����ʱĿ¼�»�ȡ��ʱ�ļ�·�� */
/* ���̰߳�ȫ��ʧ�ܷ��ؿ��ַ��� */
const char* get_temp_file(const char* prefix)
{
	static char tmpfile[MAX_PATH];

	if (!get_temp_file_under(get_temp_dir(), prefix, tmpfile, MAX_PATH))
		tmpfile[0] = '\0';

	return tmpfile;
}

/************************************************************************/
/*                         Time & Timer ʱ��Ͷ�ʱ��                     */
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

/* ����һ�� HH:MM:SS ��ʽ��ʱ���ַ������������� */
const char *time_str (time_t t)
{
  return fmttime(t, "%H:%M:%S");
}

/* ����һ�� YY-MM-DD ��ʽ�������ַ������������� */
const char *date_str (time_t t)
{
	return fmttime(t, "%Y-%m-%d");
}

/* ����һ�� YY-MM-DD HH:MM:SS ��ʽ��ʱ�������ַ������������� */
const char *datetime_str (time_t t)
{
  return fmttime(t, "%Y-%m-%d %H:%M:%S");
}

/* ����һ�� YYMMDDHHMMSS ��ʽ��ʱ��� */
const char* timestamp_str(time_t t)
{
	return fmttime(t, "%Y%m%d%H%M%S");
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
 * ����ʱ�䵥λ�ĵ���������ʽ 
 * time_span_readable �����ݴ˵�λ������ʻ����ַ��� 
 * pluralΪ��Ϊ0��ʾ���ø�����ʽ
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

/* ����һ���ɶ���ǿ��ʱ���(һ����Ϊ30��)����2Сʱ15�֣�3��� */
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

	/* ʱ�䵥λ(������ʽ) */
	if (tu_single_init)
		units_single = &tu_locale_single;
	else
		units_single = &tu_en_single;

	/* ʱ�䵥λ(������ʽ) */
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

#define REACH_TIME_UNIT(unit)			\
	else if (diff >= secs_per_##unit){	\
	n##unit = diff / secs_per_##unit;	\
	diff %= secs_per_##unit;			\
	unit##_s = n##unit > 1 ? units_plural->unit : units_single->unit;\
	if (pos + num_bits(n##unit) + strlen(unit##_s) + 1 > outlen) break;	\
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

/* ˯��ָ������ */
void ssleep(int seconds)
{
#ifdef OS_POSIX
	sleep(seconds);
#else
	Sleep(seconds*1000);
#endif
}

/* ˯��ָ�������� */
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

/* ΢�뼶��ʱ�� */
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

/* �ӿ�ʼ��ֹͣ������΢���� */
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

/* �ӿ�ʼ��ֹͣ�����ĺ����� */
double time_meter_elapsed_ms(time_meter_t *meter)
{
	return time_meter_elapsed_us(meter) / 1000;
}

/* �ӿ�ʼ��ֹͣ���������� */
double time_meter_elapsed_s(time_meter_t *meter)
{
	return time_meter_elapsed_us(meter) / 1000000;
}

/* �ӿ�ʼ�����ھ�����΢���� */
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

/* ��ȡ����ʱ���α����� */
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
/*                         Charset �ַ������ຯ��	                    */
/************************************************************************/

/* UTF-7, UTF-8, UTF-16, UTF-32 ����ת�� */
#include "deps/ConvertUTF-inl.h"

#if defined(OS_POSIX) && !defined(OS_ANDROID)
#include <langinfo.h>
#endif

//7λ��׼ASCII��
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

//�Ƿ���UTF-8����
//
//128��US-ASCII�ַ�ֻ��һ���ֽڱ��루Unicode��Χ��U+0000��U+007F����
//���и��ӷ��ŵ������ġ�ϣ���ġ��������ĸ�����������ϣ�����ġ��������ġ��������ļ�������ĸ����Ҫ�����ֽڱ��롣
//��������������ƽ�棨BMP���е��ַ���������˴󲿷ֳ����֣�ʹ�������ֽڱ��롣
//��������ʹ�õ�Unicode ����ƽ����ַ�ʹ�����ֽڱ��롣
//
//UCS-4����	�� UTF-8�ֽ��� ��Ӧ��ϵ���£�
//U+00000000 �C U+0000007F [1] 0xxxxxxx 
//U+00000080 �C U+000007FF [2] 110xxxxx 10xxxxxx 
//U+00000800 �C U+0000FFFF [3] 1110xxxx 10xxxxxx 10xxxxxx
//U+00010000 �C U+001FFFFF [4] 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//U+00200000 �C U+03FFFFFF [5] 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
//U+04000000 �C U+7FFFFFFF [6] 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
int is_utf8(const void* pBuffer, size_t size)
{
	int IsUTF8 = 1;
	unsigned char* start = (unsigned char*)pBuffer;
	unsigned char* end = (unsigned char*)pBuffer + size;
	while (start < end)
	{
		if (*start < 0x80)				/* 1�ֽ� */
		{
			start++;
		}
		else if (*start < (0xC0))
		{
			IsUTF8 = 0;
			break;
		}
		else if (*start < (0xE0))		/* 2�ֽ� */
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
		else if (*start < (0xF0))		/* 3�ֽ� */
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
		else if (*start < (0xF8))		/* 4�ֽ� */
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

//�Ƿ���GB2312����
int is_gb2312(const void* pBuffer, size_t size)
{
	int IsGB2312 = 1;
	unsigned char* start = (unsigned char*)pBuffer;
	unsigned char* end = (unsigned char*)pBuffer + size;

	while (start < end)
	{
		if (*start < 0x80)			//0x00~0x7F ASCII��
			start++;
		else if (*start < 0xA1)		//0x80~0xA0 ����
		{
			IsGB2312 = 0;
			break;
		}
		else if (*start < 0xAA)		//0xA1~0xA9 �ַ�����
		{
			if (start >= end -1)
				break;

			//���ֽڷ�Χ 0xA1~0xFE
			if (start[1] < 0xA1 || start[1] > 0xFE)
			{
				IsGB2312 = 0;
				break;
			}

			start += 2;
		}
		else if (*start < 0xB0)		//0xAA~0xAF ����
		{
			IsGB2312 = 0;
			break;
		}
		else if (*start < 0xF8)		//0xB0~0xF7 �Ƕ�GB2312���ֱ������
		{
			if (start >= end -1)
				break;

			//���ֽڷ�Χ 0xA1~0xFE(�޳�0x7F)
			if (start[1] < 0xA1 || start[1] > 0xFE)
			{
				IsGB2312 = 0;
				break;
			}

			start += 2;
		}
		else						//0xFF ����
		{
			IsGB2312 = 0;
			break;
		}
	}

	return IsGB2312;
}

//�Ƿ���GBK����
int is_gbk(const void* pBuffer, size_t size)
{
	int IsGBK = 1;
	unsigned char* start = (unsigned char*)pBuffer;
	unsigned char* end = (unsigned char*)pBuffer + size;

	while (start < end)
	{
		if (*start < 0x80)			//0x00~0x7F ASCII��
			start++;
		else if (*start < 0x81)		//0x80 ����
		{
			IsGBK = 0;
			break;
		}
		else if (*start < 0xA1)		//0x81~0xA0 �ַ���ǰ�Ĳ����ú���
		{
			if (start >= end -1)
				break;

			//���ֽڷ�Χ 0x40~0xFE (�޳�0x7F)
			if (start[1] < 0x40 || start[1] > 0xFE || start[1] == 0x7F)
			{
				IsGBK = 0;
				break;
			}

			start += 2;
		}
		else if (*start < 0xAA)		//0xA1~0xA9 �ַ�����
		{
			if (start >= end -1)
				break;

			if (*start < 0xA8)		//0xA1~0xA7 ��GB2312һ��
			{
				//���ֽڷ�Χ 0xA1~0xFE
				if (start[1] < 0xA1 || start[1] > 0xFE)
				{
					IsGBK = 0;
					break;
				}
			}
			else					//0xA8~0xA9 ������GB2312����
			{
				//���ֽڷ�Χ 0x40~0xEF(�޳�0x7F) (���Ǻܾ�ȷ)
				if (start[1] < 0x40 || start[1] > 0xEF || start[1] == 0x7F)
				{
					IsGBK = 0;
					break;
				}
			}

			start += 2;
		}
		else if (*start < 0xB0)		//0xAA~0xAF ��GB2312���ֱ�ǰ����ӵ���Ƨ��
		{
			if (start >= end -1)
				break;

			//���ֽڷ�Χ 0x40~0xA0(�޳�0x7F)
			if (start[1] < 0x40 || start[1] > 0xA0 || start[1] == 0x7F)
			{
				IsGBK = 0;
				break;
			}

			start += 2;
		}
		else if (*start < 0xF8)		//0xB0~0xF7 �Ƕ�GB2312���ֱ������
		{
			if (start >= end -1)
				break;

			//���ֽڷ�Χ 0x40~0xFE(�޳�0x7F)
			if (start[1] < 0x40 || start[1] > 0xFE || start[1] == 0x7F)
			{
				IsGBK = 0;
				break;
			}

			start += 2;
		}
		else if (*start < 0xFF)		//0xF8~0xFE ��GB2312���ֱ������ӵ���Ƨ��
		{
			if (start >= end -1)
				break;

			//���ֽڷ�Χ 0x40~0xA0(�޳�0x7F)
			if (start[1] < 0x40 || start[1] > 0xA0 || start[1] == 0x7F)
			{
				IsGBK = 0;
				break;
			}

			start += 2;
		}
		else						//0xFF ����
		{
			IsGBK = 0;
			break;
		}
	}

	return IsGBK;
}

#define MATCH_CHARSET_RETURN(csfunc, csname)\
	if (is_##csfunc(str, size)){		\
	xstrlcpy(outbuf, csname, outlen);	\
	return 1;}

/* ��ȡ�ַ�����׼ȷ�ַ���(ASCII,UTF-8,GB2312,GBK,GB18030) */
/* ���can_asciiΪ0���򷵻ذ���ASCII�ַ�������С�ַ��� */
int get_charset(const void* str, size_t size, char *outbuf, size_t outlen, int can_ascii)
{
	if (can_ascii){
		MATCH_CHARSET_RETURN(ascii, "ASCII");
	}

	/* ÿ�������ַ�ռ3���ֽڵ�UTF-8�ַ��������ױ�����ΪGBϵ�б��룬��֮��Ȼ */
	/* ��˾����ȼ�����Ƿ���UTF-8�ַ��� */
	MATCH_CHARSET_RETURN(utf8, "UTF-8");
	MATCH_CHARSET_RETURN(gb2312, "GB2312");
	MATCH_CHARSET_RETURN(gbk, "GBK");

	return 0;
}

#undef MATCH_CHARSET_RETURN

/* ����ļ��ַ�����������Ϣ */
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

/* ����ļ�ÿһ�е��ַ��� */
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

	/* �Ѽ������ */
	fci->count++;

	/* ��������� */
	if (fci->max_line > 0 && nline + 2 > fci->max_line)
		return 0;

	return 1;
}

/* 
 * @fname: ̽���ļ����ַ��� 
 * @param: outbuf: ����̽�⵽���ַ�������
 * @param: outlen: ���뻺�������ȣ�Ӧ����ΪMAX_CHARSET��
 * @param: probability: �Ǵ��ַ����Ŀ�����(0~1.0, 0.95���Ͽ�ȷ��)���ɴ�NULL
 * @param: max_line: ���̽������� (0��ʾ��������ļ�)
 * @return: 1: �ɹ����أ�0: �޷���ȡָ���ļ����ַ���
 */
int get_file_charset(const char* file, char *outbuf, size_t outlen,	
					double* probability, int max_line)
{
	FILE *fp;
	struct file_charset_info* fci;

	if (!path_valid(file, 0) || !outbuf)
		return 0;

	fp = xfopen(file, "rb");
	if (!fp)
		return 0;

	/* �ļ��Ƿ���BOMͷ */
	if (read_file_bom(fp, outbuf, outlen))
	{
		xfclose(fp);
		*probability = 1.0;
		return 1;
	}

	fci = XALLOCA(struct file_charset_info);
	memset(fci, 0, sizeof(struct file_charset_info));

	/* ��������� */
	fci->max_line = max_line > 0 ? max_line : 0;

	/* ̽��ÿһ�� */
	IGNORE_RESULT(foreach_line(fp, get_line_charset, fci));
	xfclose(fp);

	/* ����Ч���� */
	if (!fci->count)
		return 0;

	/* ȷ���ַ��� */
	{
		/* ÿ���ַ����Ŀ����� */
		double prob_ascii = (double)fci->ascii / (double)fci->count;
		double prob_utf8 =	(double)fci->utf8 / (double)fci->count;
		double prob_gb2312 = (double)fci->gb2312 / (double)fci->count;
		double prob_gbk =	(double)fci->gbk	 / (double)fci->count;

		double prob_gbx = prob_gb2312 + prob_gbk;	/* GBϵ����ֵ */

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
			/* �ַ���δ֪ */
			return 0;
		}

		/* �Ѿ���ȡ���ļ��ַ��� */
		cslen = strlen(charset);
		if (outlen > cslen)
		{
			/* �ɹ����� */
			if (probability)
				*probability = prob;
			strcpy(outbuf, charset);
			return 1;
		}
		else
		{
			/* ���������̫С */
			return 0;
		}
	}

	NOT_REACHED();
	return 0;
}

#define FOUND_FILE_BOM(s) {			\
	cslen = strlen(s);				\
	if (outlen < cslen + 1){		\
	  fseek(fp, -1*len, SEEK_CUR);	\
	  return 0;						\
	}								\
	strcpy(outbuf, s);				\
	outbuf[cslen] = '\0';			\
	return 1;						\
	}

/*
 * @fname: ��ȡ�ļ���BOMͷ
 * @param: fp: �մ򿪵��ļ���
 * @outbuf: ���ض�ȡ����BOMͷ
 * @return: �ɹ�����1��outbuf�����������ַ���
 *			���򷵻�0�������ļ�����λ��ԭ����λ��
 */
int read_file_bom(FILE *fp, char *outbuf, size_t outlen)
{
	unsigned char buf[10];
	size_t cslen, len = 0;
	int utf16le = 0, utf16be = 0;

	if (!fp || !outbuf)
		return 0;

	while(!feof(fp)){
		fread((char*)buf + len, 1, 1, fp);
		len++;

		if (len == 2)
		{
			if (buf[0] == 255 && buf[1] == 254)										//FF FE
				utf16le = 1;
			else if (buf[0] == 254 && buf[1] == 255)								//FE FF
				utf16be = 1;
		}
		else if (len == 3)
		{
			if (buf[0] == 239 && buf[1] == 187 && buf[2] == 191)					//EF BB BF
				FOUND_FILE_BOM("UTF-8")
		}
		else if (len == 4)
		{
			if (buf[0] == 255 && buf[1] == 254 && buf[2] == 0 && buf[3] == 0)		//FF FE 00 00
				FOUND_FILE_BOM("UTF-32LE")
			else if (buf[0] == 0 && buf[1] == 0 && buf[2] == 254 && buf[3] == 255)	//00 00 FE FF
				FOUND_FILE_BOM("UTF-32BE")
			else if (buf[0] == 132 && buf[1] == 49 && buf[2] == 149 && buf[3] == 51)
				FOUND_FILE_BOM("GB18030")
			else
				goto exit;
		}
	}

exit:
	if (utf16le)
		FOUND_FILE_BOM("UTF-16LE")
	else if (utf16be)
		FOUND_FILE_BOM("UTF-16BE")
	else
	{
		fseek(fp, (seek_off_t)-1*len, SEEK_CUR);
		return 0;
	}
}

#undef FOUND_FILE_BOM

/* ���ñ�����ļ�BOMͷ */
#define UTF8_BOM		"\xEF\xBB\xBF"
#define UTF16LE_BOM		"\xFF\xFE"
#define UTF16BE_BOM		"\xFE\xFF"
#define UTF32LE_BOM		"\xFF\xFE\x00\x00"
#define UTF32BE_BOM		"\x00\x00\xFE\xFF"
#define GB18030_BOM		"\x84\x31\x95\x33"

#define ELSEIF_FILE_BOM(c, b)				\
	else if (!strcmp(charset, c)){			\
		xstrlcpy((char*)bom, b, sizeof(bom));\
		len = strlen(b);					\
	}

/*
 * д���ļ���BOMͷ
 * @param: charset: �ļ��ı����ַ���
 * @return: �ɹ�д���ַ�������Ӧ��BOMͷ����1;
 *			����ַ���û����ӦBOMͷ����0;
 */
int write_file_bom(FILE *fp, const char* charset)
{
	byte bom[5];
	size_t len = 0;

	if (!fp || !charset)
		return 0;

	if (0);
	ELSEIF_FILE_BOM("UTF-8", UTF8_BOM)
	ELSEIF_FILE_BOM("UTF-16LE", UTF16LE_BOM)
	ELSEIF_FILE_BOM("UTF-16BE", UTF16BE_BOM)
	ELSEIF_FILE_BOM("UTF-32LE", UTF32LE_BOM)
	ELSEIF_FILE_BOM("UTF-32BE", UTF32BE_BOM)
	ELSEIF_FILE_BOM("GB18030", GB18030_BOM)

	if (!len)
		return 0;

	/* д��BOMͷ */;
	return fwrite(bom, len, 1, fp) == 1;
}

#undef ELSEIF_FILE_BOM


/* ��ʾ��ASCII�ַ��Ķ��ֽڴ��ĵ�һ���ֽ�������0xC0��0xFD�ķ�Χ�� */
/* ���ֽڴ��������ֽڶ���0x80��0xBF��Χ�� */
#define UTF8_ASCII(ch)  (((byte)ch)<=0x7F)
#define UTF8_FIRST(ch) ((((byte)ch)>=0xC0) && (((byte)ch)<=0xFD))
#define UTF8_OTHER(ch) ((((byte)ch)>=0x80) && (((byte)ch)<=0xBF))

int utf8_len(const char* utf8)
{
	byte *p = (byte*)utf8;
	int count = 0;

	if (!utf8)
		return -1;

	while (*p){
		if (UTF8_ASCII(*p) || (UTF8_FIRST(*p)))
			count++;
		else if (!UTF8_OTHER(*p))
			return -1;

		p++;
	}

	return count;
}

/* ���ո���������ֽ�����ȡUTF-8�ַ�����
 * �˺������������Ա������ַ������������ַ������ֽ���
 * outbuf��ΪNULL�������ΪNULL���䳤�ȱ������max_byte (�����'\0')���ҽ�ȡ����ִ��ᱻ���Ƶ�outbuf��
 * ��������UTF-8�ַ�����Ч�����أ�1 */
int utf8_trim(const char* utf8, char* outbuf, size_t max_byte)
{
	size_t slen = strlen(utf8);
	const byte *begin = (byte*)utf8;
	const byte *p = begin + max_byte;

	if (!utf8)
		return -1;

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
			return -1;

		p--;
	}

	/* �ҵ���ȡλ�� */
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

	return -1;
}

/* ��дUTF-8�ַ�����ָ����󳤶ȣ�������ǰ�������ַ����м���...��ʾʡ�� 
 * ���� "c is a wonderful language" ��дΪ20���ֽڣ��������3���ַ�
 * ���Ϊ "c is a wonderf...age"
 * ע��1��"..."�ĳ��ȣ�3���ֽڣ������� max_byte ������
 * 2�����last_reserved_wordsΪ0����"..."�������������
 *	���last_reserved_words���ڵ���max_byte-3����"..."����������ǰ
 * 3���ܶ��ֽڱ����Ӱ�죬���շ����ַ����ĳ��ȿ���С�� max_byte�������
 * 4��outbuf����Ϊ�������С������� max_byte (���'\0')
 * ����ֵ���ɹ�����1��ʧ�ܷ���-1
 */
#define ABBR_DOTS_LEN	3	/* strlen("...") */

int utf8_abbr(const char* utf8, char* outbuf, size_t max_byte, 
	size_t last_reserved_words)
{
	const char* p, *plast;
	size_t origlen, word_count;
    size_t suffix_count, left_room;
    int prefix_count;

	if (!utf8 || !outbuf)
		return -1;

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

	/* �ҵ����ַ�����"..."������ַ���Դ�ַ����е�λ�� */
	p = plast = utf8 + origlen - 1;
	suffix_count = 0;
	word_count = 0;

	while(1) {
		++suffix_count;
		if (UTF8_ASCII(*p) || (UTF8_FIRST(*p))) {
			if (suffix_count > max_byte) {
				suffix_count -= plast - p;	/* ��λ���ϴ�δ������Χ���ַ��� */
				p = plast;
				break;
			}
			if (++word_count == last_reserved_words)
				break;
			plast = p;
		} else if (!UTF8_OTHER(*p))
			return -1;

		--p;
	}

	left_room = max_byte - suffix_count;
	if (left_room <= ABBR_DOTS_LEN) {
		strncpy(outbuf, "...", left_room);
		outbuf[left_room] = '\0';
		strcat(outbuf, p);
		return 1;
	}

	//�ҵ����ַ�����"..."֮ǰ���ַ���Ŀ
	prefix_count = utf8_trim(utf8, NULL, left_room - ABBR_DOTS_LEN);
	if (prefix_count == -1)
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

/* �����ֽ��ַ���ת��Ϊ���ַ��� */
wchar_t* mbcs_to_wcs(const char* src)
{
#ifdef OS_WIN
	wchar_t *buf;

	int outlen = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
	if (outlen <= 0)
		return NULL;

	buf = xcalloc(outlen, sizeof(wchar_t));
	if (!MultiByteToWideChar(CP_ACP, 0, src, strlen(src), buf, outlen))
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

/* �����ַ���ת��Ϊ���ֽ��ַ��� */
char* wcs_to_mbcs(const wchar_t* src)
{
#ifdef OS_WIN
	BOOL bUsed;
	char *buf;

	int outlen = WideCharToMultiByte(CP_ACP, 0,  src, -1, NULL, 0, NULL, &bUsed);
	if (outlen <= 0)
		return NULL;

	buf = xcalloc(1, outlen);
	if (!WideCharToMultiByte(CP_ACP, 0, src, wcslen(src), buf, outlen, NULL, &bUsed))
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

/* ��UTF-8������ַ���ת��ΪUTF-16�ַ��������ⲿ�ͷ� */
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

/* ��UTF-16�ַ���ת��ΪUTF-8������ַ��������ⲿ�ͷ� */
/* ע����UTF-8������ÿ���ַ�һ�����ռ3���ֽڣ������п������ռ6���ֽ� */
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
				uBeg = '\0';
				return uBegOrig;
			}

			/* �޷�����ַ���������'\0' */
			cr = targetExhausted;
		}

		xfree(uBegOrig);
		nBytes ++;

	}while(cr == targetExhausted && nBytes <= 6);

	return NULL;
}

/* ��UTF-8������ַ���ת��ΪUTF-32�ַ��������ⲿ�ͷ� */
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

/* ��UTF-32�ַ���ת��ΪUTF-8������ַ��������ⲿ�ͷ� */
/* ע����UTF-8������ÿ���ַ�һ�����ռ3���ֽڣ������п������ռ6���ֽ� */
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
				uBeg = '\0';
				return uBegOrig;
			}

			/* �޷�����ַ���������'\0' */
			cr = targetExhausted;
		}

		xfree(uBegOrig);
		nBytes ++;

	}while(cr == targetExhausted && nBytes <= 6);

	return NULL;
}

/* ��UTF-16������ַ���ת��ΪUTF-32�ַ��������ⲿ�ͷ� */
UTF32* utf16_to_utf32(const UTF16* src, int strict)
{
	ConversionResult cr;
	UTF32* u32, *u32Orig;
	const UTF16 *u16 = src;
	size_t nLen = utf16_len(src);

	u32 = (UTF32*)xcalloc((nLen+1), sizeof(UTF32));
	u32Orig = u32;
	
	cr = ConvertUTF16toUTF32(&u16, u16+nLen, &u32, u32+nLen,
		strict ? strictConversion : lenientConversion);
	if (cr == conversionOK)
	{
		*u32 = L'\0';
		return u32Orig;
	}
	
	xfree(u32Orig);
	return NULL;
}

/* ��UTF-32�ַ���ת��ΪUTF-16������ַ��������ⲿ�ͷ� */
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

/*��UTF-8�����ַ���ת��ΪUTF-7�ַ�������Ҫ�ⲿ�ͷţ�strict������*/
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

/*��UTF-7�����ַ���ת��ΪUTF-8�ַ�������Ҫ�ⲿ�ͷţ�strict������*/
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

/* ��UTF-8������ַ���ת��Ϊϵͳ���ֽڱ��� */
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

/* ��ϵͳ���ֽڱ�����ַ���ת��ΪUTF-8���� */
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

ssize_t utf16_len(const UTF16* u16)
{
	const UTF16* p = u16;

	if (!u16)
		return -1;

	while (*p) p++;
	return p - u16;
}

ssize_t utf32_len(const UTF32* u32)
{
	const UTF32* p = u32;

	if (!u32)
		return -1;

	while (*p) p++;
	return p - u32;
}

#ifndef OS_ANDROID
/* ��ȡϵͳĬ���ַ���(���ֽڱ��������ַ���) */
const char *get_locale()
{
#ifdef OS_POSIX
	CHECK_INIT();
	return nl_langinfo(CODESET);
#else
	static char lcs[20];
	snprintf(lcs, sizeof(lcs), "CP%u", GetACP());
	return lcs;
#endif
}
#endif

/* ��ȡϵͳĬ������ */
/* ����ֵ�� zh_CN��en_US */
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
	case 2052:	//���ģ��й���
		strcpy(lang, "zh_CN");
		break;
	case 1028:	//���ģ�̨��)
	case 3076:	//���ģ����)
		strcpy(lang, "zh_TW");
		break;
	case 1033:	//Ӣ�� (����)
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

/* ʹ��libiconv��ת���ַ�����
 * �ɹ�����ת����Ŀ�����ĳ���
 * ʧ�ܷ���-1������errno�������¼������:
 * 1����Ч���������У�����errnoΪEILSEQ
 * 2�����������������У�����errnoΪEINVAL
 * 3�������������С������errnoΪE2BIG
 */
static inline
size_t iconv_convert(const char* from_charset, const char* to_charset, 
	const char* inbuf, size_t inlen, 
	char* outbuf, size_t outlen,
	int strict)
{
	iconv_t cd;
	const char** pin = &inbuf;  
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
 * �ַ�����ת��
 * ���outbuf��ΪNULL��outlen�ĳ����㹻����ת������ַ�
 * ��ת������ַ��������outbuf�У�����������һ���ڴ沢ͨ��outbuf����
 * outlenΪת������ֽڳ���
 * ����ֵ���ɹ�����1��ʧ�ܷ���0
 * ע����Ȼinbufһ�㲻��'\0'��β�����ᱣ֤�����'\0'��β���Է�����
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

	/* ԭ�ַ�����Ŀ���ַ�����ͬ */
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

	if ((dlen == (size_t)-1 && errno == E2BIG)			//ת��ʧ�ܣ�ԭ�������������̫С
		|| (dlen != (size_t)-1 && tlen < dlen +1))		//ת���ɹ������޷����ɸ��ӵ�'\0'
	{
		tlen += inlen;

		if (tbuf == *outbuf)
			tbuf = (char*)xmalloc(tlen);				//�״��ڲ�����
		else
			tbuf = (char*)xrealloc(tbuf, tlen);			//���·����С

		goto retry_convert;
	}
	else if (dlen == (size_t)-1)	//EILSEQ, EINVAL
	{
		if (tbuf != *outbuf)
			xfree(tbuf);

		return 0;
	}

	tbuf[dlen] = '\0';

	//�����������
	*outlen = dlen;
	
	if (tbuf != *outbuf)
		*outbuf = tbuf;

	return 1;
}

#endif

/************************************************************************/
/*                         Process  �����                              */
/************************************************************************/

/* �����µĽ��� */
/* ����executable����Ҫִ�еĿ�ִ���ļ�·�� */
/* ����param�����ݸ���ִ���ļ��Ĳ���ָ�����飬���һ��Ԫ�ر���ΪNULL */
/* ����ֵ�������ɹ����ؽ��̱�ʶ��(process_t)��ʧ�ܷ���INVALID_PROCESS */
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

		/* CreateProcess���޸�exe_param��������˲���ֱ��ʹ�� */
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
		/* �ӽ��� */
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

		/* �����ź���Ӧ���� */
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGILL, SIG_DFL);
		signal(SIGABRT, SIG_DFL);
		signal(SIGFPE, SIG_DFL);
		signal(SIGBUS, SIG_DFL);
		signal(SIGSEGV, SIG_DFL);
		signal(SIGSYS, SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/* ��ʼִ�� */
		execl("/bin/sh", "sh", "-c", cmd_with_param, NULL);

		/* ���ִ�гɹ����������е����� */
		_exit(127);
	}
	
	return pid;
#endif
}

/* �ȴ����̽�����������Ϊ INFINITE �����޵ȴ� */
int	process_wait(process_t process, int milliseconds, int *exit_code)
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
	const int wait_per_cycle = 10; /* ÿ�εȴ�10���� */
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
	
	/* �ӽ��̱��ź�����ֹ */
	if (WIFSIGNALED(status)) {
		if (exit_code)
			*exit_code = -1;
		return 1;
	}

	/* �ӽ������˳� */
	if (WIFEXITED(status)) {
		if (exit_code)
			*exit_code = WEXITSTATUS(status);
		return 1;
	}

	/* �ӽ�����δ��ֹ */
	return 0;

#endif
}

/* ɱ������ */
int process_kill(process_t process, int exit_code ALLOW_UNUSED, int wait)
{
#ifdef OS_WIN
	if (!TerminateProcess(process, exit_code))
		return 0;

	//�ȴ���������첽IO����
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

	//�ȴ���������첽IO����
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
					/* ��һ������Ҳ�ڵȴ���ͬ��pid */
					exited = 1;
					break;
				}
			}

			msleep(sleep_ms);
			if (sleep_ms < kMaxSleepMs)
				sleep_ms *= 2;
		}

		/* ɱ��������SIGKILL�ź���ɱһ�� */
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
	size_t bufsize = 1024;
	size_t len, readed = 0;

	FILE* f = popen(command, "rb");
	if (!f)
		return NULL;

	buf = (char*)xmalloc(bufsize);

	while((len = fread(buf + readed, 1, bufsize - readed - 1, f))) {
		readed += len;
		if (readed == bufsize - 1) {
			bufsize <<= 1;
			buf = (char*)xrealloc(buf, bufsize);
		}
	}

	if (feof(f))
		buf[readed] = '\0';
	else {
		xfree(buf);
		buf = NULL;
	}

	pclose(f);
	return buf;
}

#ifdef OS_WIN
/* 
 * �����ӽ����Լ���֮���ӵĹܵ� 
 * ����ͨ���ܵ���ȡ�ӽ������(r)��д��ܵ���Ϊ�ӽ��̱�׼����(w)
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
 * TODO: linux, mac ֧��
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

	//sei.fMask = SEE_MASK_NOASYNC;
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

	if (!ShellExecuteEx(&sei)) {
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
/*                         Sync �߳���ͬ��                               */
/************************************************************************/

//sync_fetch_and_* ����Ȼ�󷵻�֮ǰ��ֵ
//sync_*_and_fetch ����Ȼ�󷵻��µ�ֵ
//http://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
//InterlockedExchangeAdd ���ز���֮ǰ��ֵ
//InterlockedIncrement ���ز���֮���ֵ

/* ԭ�ӱ������� */
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

/* ������ʵ�� */
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

/* �̻߳�����ʵ�� */
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

/* ��д��ʵ�� */
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
	//Nothing to do here.
#else
	mutex_destroy(&rwlock->write_lock);
	CloseHandle(rwlock->no_readers);
#endif
}

#endif

#ifdef USE_CONDITION_VARIABLE

/* �������� */
void cond_init(cond_t *cond)
{
#ifdef OS_POSIX
	pthread_cond_init(cond, NULL);
#elif WINVER >= WINVISTA
	InitializeConditionVariable(cond);
#else
	//TODO: unsupported
#endif
}

void cond_wait(cond_t *cond, mutex_t *mutex)
{
#ifdef OS_POSIX
	pthread_cond_wait(cond, mutex);
#elif WINVER >= WINVISTA
	SleepConditionVariableCS(cond, mutex, INFINITE);
#else
	//TODO: unsupported
#endif
}

int cond_wait_time(cond_t *cond, mutex_t *mutex, int milliseconds)
{
#ifdef OS_POSIX
	struct timespec ts;
	int seconds, ms;

	seconds = milliseconds / 1000;	//����
	ms = milliseconds % 1000;		//������(<1s)
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
/*                         Thread  ���߳�                               */
/************************************************************************/

/* �����߳� */
/* ���stacksizeΪ0��ʹ��Ĭ�ϵĶ�ջ��С */
int thread_create1(thread_t* t, thread_proc_t proc, void *arg, int stacksize)
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
	/* ��Ҫʹ��CreateThread������ܿ��ܻ�����ڴ�й¶ */
	uint ret = _beginthreadex(NULL, stacksize, proc, arg, 0, NULL);
	if (!ret)
	{
		log_dprintf(LOG_ERROR, "thread create failed: %s", get_last_error_std());
		return 0;
	}

	*t = (HANDLE)ret;
	return 1;
#endif
}

/* �˳��߳� */
void thread_exit(size_t exit_code)
{
#ifdef OS_POSIX
	pthread_exit((void*)exit_code);
#else
	/* ��Ҫʹ��ExitThread */
	_endthreadex((unsigned)exit_code);
#endif
}

/* �ȴ��߳� */
int thread_join(thread_t t, thread_ret_t *exit_code)
{
#ifdef OS_POSIX
	if (pthread_join(t, exit_code))
		return 0;
#else
	WaitForSingleObject(t, INFINITE);
	if (exit_code && !GetExitCodeThread(t, exit_code))
	{
		CloseHandle(t);
		return 0;
	}
	CloseHandle(t);
#endif

	return 1;
}

/* ����һ���߳���ش洢��TLS���� */
int	thread_tls_create(thread_tls_t *tls)
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

/* ����TLSֵ */
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

/* ��ȡTLSֵ */
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

/* �ͷ�TLS */
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

/* ��ʼ�� thread_once_t �ṹ�� */
void thread_once_init(thread_once_t* once)
{
	once->inited = 0;
	mutex_init(&once->lock);
}

/* ִ���߳�һ�γ�ʼ�� */
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
/*							 Debugging  ����					            */
/************************************************************************/

#define ERRBUF_LEN	128

/* ��ȡWIN32 API��������Ϣ */
#ifdef OS_WIN
const char* get_last_error_win32()
{
	static char errbuf[ERRBUF_LEN];

	LPSTR lpBuffer;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), LANG_NEUTRAL, (LPSTR)&lpBuffer, 0, NULL);

	if (lpBuffer)
		xstrlcpy(errbuf, lpBuffer, ERRBUF_LEN);
	else
		xstrlcpy(errbuf, "No corresponding error message.", ERRBUF_LEN);

	LocalFree (lpBuffer);
	return errbuf;
}
#endif

/* ��ȡWIN32 API��������Ϣ */
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

enum BackTraceFlag{
	BackTraceLogging = 1,
	BackTraceStdout = 2,
	BackTraceStderr = 4,
#ifdef OS_WIN
	BackTraceDialog = 8,
#ifdef _MSC_VER
	BackTraceOutput = 16
#endif
#endif
};

/* ����ת�� */
#ifdef OS_WIN

/* ��ȡ��ջ���ü�¼ */
static void StackBackTraceMsg(const void* const* trace, size_t count, const char* info, int flag) 
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

		/* ��¼��־ */
		if (flag & BackTraceLogging)
			log_dprintf(LOG_DEBUG, "%s", msgl);

		/* ��ӡ����׼��� */
		if (flag & BackTraceStdout)
			fprintf(stdout, "%s\n", msgl);

		/* ��ӡ����׼������� */
		if (flag & BackTraceStderr)
			fprintf(stderr, "%s\n", msgl);

		/* ��ӡ��VC������� */
#ifdef _MSC_VER
		if (flag & BackTraceOutput)
		{
			OutputDebugStringA(msgl);
			OutputDebugStringA("\r\n");
		}
#endif

		/* �Ի�����Ϣ */
		if (flag & BackTraceDialog)
		{
			if (i == 0)
				snprintf(msgs, sizeof(msgs), "%s\r\n", msgl);
			else
			{
				xstrlcat(msgs, msgl, sizeof(msgs));
				xstrlcat(msgs, "\r\n", sizeof(msgs));
			}
		}

		xfree(buffer);
	}

	log_flush(DEBUG_LOG);

	if (flag & BackTraceDialog)
		MessageBoxA(NULL, msgs, info, MB_OK);
}

static inline
void CrashBackTrace(EXCEPTION_POINTERS *pException, const char* info, int flags)
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

	StackBackTraceMsg(trace, count, info, flags);
}

static LONG WINAPI CrashDumpHandler(EXCEPTION_POINTERS *pException)
{
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	HANDLE hDumpFile;
	char dump_file[MAX_PATH];
	char pdb[MAX_PATH];
	const char* ext;
	static int handled = 0;

	log_dprintf(LOG_ALERT, "CrashDumpHandler Called!");

	if (handled)
		return EXCEPTION_EXECUTE_HANDLER;
	handled = 1;

	//////////////////////////////////////////////////////////////////////////
	//�����ڴ�ת���ļ�

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
		MessageBoxA(NULL, dump_file, "Could not create dump file!", MB_OK);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// ����dump�ļ�
	dumpInfo.ExceptionPointers = pException;
	dumpInfo.ThreadId = GetCurrentThreadId();
	dumpInfo.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
	CloseHandle(hDumpFile);

	// ��ʾ�û��ϴ�ת���ļ����ڷ������Ͻ��PDB�ļ����ɶ�ջ��Ϣ
	if (g_crash_handler)
		g_crash_handler(dump_file);

	// �����PDB�ļ���ֱ�Ӵ�ӡ��ջ��Ϣ
	xstrlcpy(pdb, get_module_path(), sizeof(pdb));
	ext = path_find_extension(pdb);
	if (!strcasecmp(ext, ".exe"))
		pdb[ext - pdb] = '\0';
	xstrlcat(pdb, ".pdb", sizeof(pdb));

	if (path_file_exists(pdb)) {
        int flag = BackTraceLogging|BackTraceDialog;
#ifdef _MSC_VER
        flag |= BackTraceOutput;
#endif
        CrashBackTrace(pException, g_product_name, flag);
	}

	log_close_all();

	return EXCEPTION_EXECUTE_HANDLER;
}
#else /* POSIX */

#ifdef _DEBUG
void stack_backtrace(int flag)
{
	void* array[20];
	char msg[1024];
	size_t i;
    int size;
    
    size = backtrace(array, countof(array));
	char **strings = backtrace_symbols(array, size);

	for (i = 0; i < size; i++) {
		IGNORE_RESULT(xsnprintf(msg, sizeof(msg), "%" PRIuS ". %s", i, strings[i]));

		if (flag & BackTraceLogging)
			log_dprintf(LOG_DEBUG, "%s", msg);
		if (flag & BackTraceStderr)
			fprintf(stderr, "%s\n", msg);
		if (flag & BackTraceStdout)
			fprintf(stdout, "%s\n", msg);
	}
	free (strings);
}

void crash_signal_handler(int n, siginfo_t *siginfo, void *act)
{
	char signam[16];
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

	//FIXME: coredump�ļ���·����
	if (g_crash_handler)
		g_crash_handler("");

	log_dprintf(LOG_CRIT, "%s received. exit now...\n", signam);
	if (!is_debug_log_set_to_stderr())
		fprintf(stderr, "%s received. exit now...\n", signam);

	int flag = BackTraceLogging;
	if (!is_debug_log_set_to_stderr())
		flag |= BackTraceStderr;
	stack_backtrace(flag);

	log_close_all();
	exit(3);
}
#endif

#endif /* OS_WIN */

#endif // #ifdef USE_CRASH_HANDLER

/* ��¼��ǰ��ջ��Ϣ�������˳� */
void fatal_exit(const char *fmt, ...)
{
	va_list args;
	char msg[1024];

	//��ӡ��Ϣ
	va_start(args, fmt);
	IGNORE_RESULT(xvsnprintf(msg, sizeof(msg), fmt, args));
	log_printf0(DEBUG_LOG, "%s\n", msg);
	fprintf(stderr, "%s\n", msg);
    va_end(args);

// #ifdef _DEBUG
// 	/* ����ģʽ�£�ֱ�Ӵ�ӡ���ö�ջ */
// #ifdef OS_WIN
// 	{
// 	size_t count;
// 	void *trace[62];
// 	count = CaptureStackBackTrace(0, countof(trace), trace, NULL);
// 	StackBackTraceMsg(trace, count, msg, BackTraceLogging|BackTraceDialog);
// 	}
// #else
// 	stack_backtrace(BackTraceLogging|BackTraceStderr);
// #endif /* OS_WIN */
// 
// 	log_close_all();
// 
// #ifdef COMPILER_MSVC
// 	__debugbreak();
// #endif

	exit(2);

//#else
//	//����ʹ��������Բ���core dump�ļ�
//	{
//	int *p = NULL;
//	*p = 0;
//	}
//#endif /* _DEBUG */
}

/* ��ʮ�����Ƶ���ʽ��ӡ��һ�������������� */
char* hexdump(const void *data, size_t len)
{
	/* ÿһ���ֽ���Ҫ����λ����һ���ո�����ʾ������'\0' */
	size_t buflen = len * 3 + 1;
	char *buf = (char*)xmalloc(buflen);
	size_t i;

	for (i = 0; i < len; i++)
		snprintf(buf + i*3, buflen - i*3, "%.2X ", ((unsigned char*)data)[i]);
	buf[buflen-1] = '\0';

	return buf;
}


/************************************************************************/
/*                         Log ��־ϵͳ		                            */
/************************************************************************/

#define LOG_VALID(id) ((id) >= DEBUG_LOG && (id) <= MAX_LOGS)

static const char* log_severity_names[] = {
	"Debug",
	"Info",
	"Notice",
	"Warning",
	"Error",
	"Critical",
	"Alert",
	"Fatal"};

static int log_min_severity = 0;		/* ������ͼ�¼�ȼ� */
static int debug_log_to_stderr = 0;		/* ������Ϣ���͵���׼������� */

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
	CHECK_INIT();
	log_min_severity = severity;
}

void set_debug_log_to_stderr(int enable)
{
	CHECK_INIT();
	debug_log_to_stderr = enable;
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
		/* ��������־�ļ��Ĺر���� */
		/* xfopen�Ѷ���������ݼ��Ա���g_opened_files���� */
		g_opened_files--;
	}

	return fp;
}

/* ���û���־�ļ� */
int	log_open(const char *path, int append, int binary)
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

/* �򿪵�����־�ļ� */
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

/* ��ʼ����־ģ�� */
void log_init()
{
	int i;
	for (i = 0; i < MAX_LOGS+1; i++)
	{
		log_files[i] = NULL;
		mutex_init(&log_locks[i]);
	}

	/* �����ȫ�ֵ�����־ */
#ifdef USE_DEBUG_LOG
	{
		char exe_name[MAX_PATH], log_path[MAX_PATH];
		xstrlcpy(exe_name, get_execute_name(), MAX_PATH);
		exe_name[path_find_extension(exe_name) - exe_name] = '\0';

		IGNORE_RESULT(create_directory("log"));

		/* �� Product_20140506143512_1432_debug.log */
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

	if (!LOG_VALID(log_id))
		return;

	log_lock(log_id);

	if (!(fp = log_files[log_id]))
	{
		log_unlock(log_id);
		return;
	}

	//������Ϣ
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

#ifdef _DEBUG
	fflush(fp);
#endif

	log_unlock(log_id);
}

/* д����־�ļ� */
/* DEBUG ��Ϣ����DEBUGģʽ�²Ż��¼ */
/* FATAL ��Ϣ�ڼ�¼֮����¼��ջ��ʹ�����˳� */
void log_printf(int log_id, int severity, const char *fmt, ...)
{
	FILE *fp;
	time_t t;
	char tmbuf[32];
	const char *p;
	va_list args;
	int level;

	CHECK_INIT();

	if (!LOG_VALID(log_id))
		return;

	level = xmin(xmax((int)LOG_DEBUG, severity), (int)LOG_FATAL);
	if (level < log_min_severity)
		return;

	log_lock(log_id);

	if (!(fp = log_files[log_id]))
	{
		log_unlock(log_id);
		return;
	}

	//ʱ����Ϣ
	t = time(NULL);
	memset(tmbuf, 0, sizeof(tmbuf));
	strftime(tmbuf, sizeof(tmbuf), "%d/%b/%Y %H:%M:%S", localtime(&t));
	
	fprintf (fp, "%s ", tmbuf);

	if (log_id == DEBUG_LOG && debug_log_to_stderr)
		fprintf(stderr, "%s ", tmbuf);

	//�ȼ���Ϣ
	fprintf(fp, "[%s] ", log_severity_names[level]);

	if (log_id == DEBUG_LOG && debug_log_to_stderr)
		fprintf(stderr, "[%s] ", log_severity_names[level]);

	//������Ϣ
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);

	if (log_id == DEBUG_LOG && debug_log_to_stderr) {
      	va_list argsd;
        va_start(argsd, fmt);
        vfprintf(stderr, fmt, argsd);
        va_end(argsd);
    }

	//���з�
	p = fmt + strlen(fmt) - 1;
	if (*p != '\n') {
		fputc('\n', fp);
		if (log_id == DEBUG_LOG && debug_log_to_stderr)
			fputc('\n', stderr);
	}

#ifdef _DEBUG
	fflush(log_files[log_id]);
#endif

	log_unlock(log_id);

	/* �����Ϣ�ȼ�ΪFATAL����������¼���ö�ջ�����쳣�˳� */
	if (level == LOG_FATAL)
		fatal_exit("logging level fatal");
}

void log_flush(int log_id)
{
	log_lock(log_id);

	if (log_files[log_id])
		fflush(log_files[log_id]);

	log_unlock(log_id);
}

/* �ر���־�ļ� */
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
	log_printf0(DEBUG_LOG, "\n");
	log_dprintf(LOG_INFO, " ==================== Program Exit ====================\n\n");
	log_close(DEBUG_LOG);
#endif
}

/************************************************************************/
/*              Memory Runtime Check ����ʱ�ڴ���                      */
/************************************************************************/

#ifdef DBG_MEM_RT

#define MEMRT_HASH_SIZE		16384									/* ��ϣ����Ĵ�С */
#define MEMRT_HASH(p) (((size_t)(p) >> 8) % MEMRT_HASH_SIZE)		/* ��ϣ�㷨 */

static struct MEMRT_OPERATE* memrt_hash_table[MEMRT_HASH_SIZE];		/* ��ϣ�������� */

static mutex_t memrt_lock;
static mutex_t memrt_print_lock;

void memrt_init()
{
	mutex_init(&memrt_lock);
	mutex_init(&memrt_print_lock);
	memset(memrt_hash_table, 0, sizeof(memrt_hash_table));
}

static inline 
struct MEMRT_OPERATE* memrt_ctor(int mtd, void *addr, size_t sz, const char *file, 
		const char* func, int line, const char* desc, memrt_msg_callback pmsgfun)
{
	/* ��Ҫʹ��xmalloc��fatal_exit�ȣ�����������ѭ�� */
	struct MEMRT_OPERATE *rtp = (struct MEMRT_OPERATE*)malloc(sizeof(struct MEMRT_OPERATE));
	if (!rtp) 
		abort();

	rtp->method = mtd;
	rtp->address = addr;
	rtp->size = sz;
	rtp->file = file ? strdup(path_find_file_name(file)) : NULL;
	rtp->func = func ? strdup(func) : NULL;
	rtp->line = line;
	rtp->desc = desc ? strdup(desc) : NULL;
	rtp->msgfunc = pmsgfun;
	rtp->next = NULL;

	return rtp;
}

static inline 
void memrt_dtor(struct MEMRT_OPERATE *rtp)
{
	/* ��Ҫʹ��xfree������������ѭ�� */
	if (rtp->file)
		free(rtp->file);
	if (rtp->func)
		free(rtp->func);
	if (rtp->desc)
		free(rtp->desc);
	free(rtp);
}

/* ���ݴ������������Ӧ�ַ��� */
static
void memrt_msg_c(int error, struct MEMRT_OPERATE *rtp)
{
	switch(error)
	{
	case MEMRTE_NOFREE:
		__memrt_printf("[MEMORY] {%s %s %d} Memory not freed at %p size %u method %d.\n",
			rtp->file, rtp->func, rtp->line, rtp->address ,rtp->size, rtp->method);
		break;
	case MEMRTE_UNALLOCFREE:
		__memrt_printf("[MEMORY] {%s %s %d} Memory not malloced but freed at %p.\n", 
			rtp->file, rtp->func, rtp->line, rtp->address);
		break;
	case MEMRTE_MISRELIEF:
		__memrt_printf("[MEMORY] {%s %s %d} Memory not reliefed properly at %p, use delete or delete[] instead.\n", 
			rtp->file, rtp->func, rtp->line, rtp->address);
		break;
	default:
		NOT_REACHED();
		break;
	}
}

/* ���ٷ����ڴ���� */
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

/* �����ͷ��ڴ���� */
int __memrt_release(int mtd, void* addr, size_t sz, const char* file, 
	const char* func, int line, const char* desc, memrt_msg_callback pmsgfun)
{
	struct MEMRT_OPERATE *rtp, *ptr, *ptr_last = NULL;
	size_t index = MEMRT_HASH(addr);
	int ret = MEMRTE_UNALLOCFREE;

	rtp = memrt_ctor(mtd, addr, sz, file, func, line, desc, pmsgfun);

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

			if ((ptr->method >= MEMRT_MALLOC && ptr->method <= MEMRT_STRDUP && rtp->method != MEMRT_FREE) ||
				(ptr->method >= MEMRT_C_END && rtp->method == MEMRT_FREE))
				ret = MEMRTE_MISRELIEF;
			else
				ret = MEMRTE_OK;

			memrt_dtor(ptr);
			break;
		}

		ptr_last = ptr;
		ptr = ptr->next;
	}

	mutex_unlock(&memrt_lock);

	/* ��ӡ������Ϣ */
	if (ret != MEMRTE_OK && rtp->msgfunc)
		rtp->msgfunc(ret, rtp);

	memrt_dtor(rtp);
	return ret;
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
			/* ��ӡδ�ͷ���Ϣ */
			if (ptr->msgfunc)
				ptr->msgfunc(MEMRTE_NOFREE, ptr);

			ptr_tmp = ptr;
			ptr = ptr->next;
			memrt_dtor(ptr_tmp);
		}

		memrt_hash_table[i] = NULL;
	}

	mutex_unlock(&memrt_lock);
	return isleaked;
}

/* ���������Ϣ  */
void __memrt_printf(const char *fmt, ...)
{
	char msg[1024];
	va_list args;

	va_start (args, fmt);
	vsnprintf (msg, 1024, fmt, args);
	va_end (args);

	/* �������ʱ���� */
#ifdef COMPILER_MSVC
	OutputDebugStringA(msg);
#else
	fprintf(stderr, "%s", msg);
#endif

	/* ��¼ϵͳ��־ */
	log_dprintf(LOG_NOTICE, "%s", msg);
}

#endif /* DBG_MEM_RT */

/************************************************************************/
/*						    Version �汾����		                        */
/************************************************************************/

/* ����һ���Ե�ָ��İ汾�ַ��� */
/* �ַ�����ʽ����Ϊ��'.'�ָ������汾�š��ΰ汾�š������汾�š����������������SVN�ŵȣ�����׺���� */
/* ����major, minor, revision, build, suffix ����ΪNULL�����suffix��ΪNULL����slen�������0 */
/* ע�������ĺ�׺������alpha1, beta2, Pro, Free, Ultimate, Stable.etc */
/* ʾ��: 1.5.8.296 beta1 */
int version_parse(const char* version, int *major, int *minor,					
	int *revision, int *build, char *suffix, size_t suffix_outbuf_len)
{
	char *ver, *p, *q;
	int id = 0;

	if (!version)
		return 0;

	/* ��ʼ�� */
	if (major) *major = 0;
	if (minor) *minor = 0;
	if (revision) *revision = 0;
	if (build) *build = 0;
	if (suffix) *suffix = '\0';

	/* ��׺���� */
	p = strchr((char*)version, ' ');
	if (p && suffix)
		xstrlcpy(suffix, p+1, suffix_outbuf_len);
	
	/* �汾�� */
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

/* �Ƚ�v1��v2�����汾�ŵĹ��в��� */
/* ����ֵ�������ȷ���0��v1��v2�ɷ���-1��v1��v2�·���1 */
/* ע������Ƚϰ汾��׺��Ϣ */
int version_compare(const char* v1, const char* v2)
{
	int major1, minor1, revision1, build1;
	int major2, minor2, revision2, build2;

	if (!version_parse(v1, &major1, &minor1, &revision1, &build1, NULL, 0)
	    || !version_parse(v2, &major2, &minor2, &revision2, &build2, NULL, 0))
        return -2;

	/* �Ƚ����汾�� */
	if (major1 > major2)
		return 1;
	else if (major1 < major2)
		return -1;

	/* ���汾����ͬ���Ƚϴΰ汾�� */
	if (minor1 > minor2)
		return 1;
	else if (minor1 < minor2)
		return -1;

	/* �ΰ汾��Ҳ��ͬ���Ƚ��޶��汾�� */
	if (revision1 > revision2)
		return 1;
	else if (revision1 < revision2)
		return -1;

	/* �޶��汾�Ż���ͬ���Ƚϱ���汾�� */
	if (build1 > build2)
		return 1;
	else if (build1 < build2)
		return -1;

	/* ���а汾�ž���ͬ */
	return 0;
}

/************************************************************************/
/*							 Others ����		                            */
/************************************************************************/

#define KEY_MAX 1024
#define VAL_MAX 4096

/* ���û������� */
/* ��Ӱ�쵱ǰ���̵Ļ�������������ϵͳ������������Ӱ�� */
/* ע�����ĳ��Ȳ�Ӧ����1k��ֵ�ĳ��Ȳ�Ӧ����4K */
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

/* ��ȡ�������� */
/* �ɹ�����ָ��û�������ֵ���ַ������������ڷ���NULL */
/* ע1�����̰߳�ȫ���������� */
/* ע2����粻���Ըı䷵�ص��ַ��� */
const char*	get_env(const char* key)
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

/* ɾ���������� */
/* ����ֵ����ֵ�����ڻ�ɾ���ɹ�����1�����򷵻�0 */
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

/* �ַ�������ת�� */
#define ATON(t,n,f)	\
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

/* ��ָ��ת����ʮ�����Ƶ��ַ��� */
int ptr_to_str(void *ptr, char* buf, int len)
{
	if (snprintf(buf, len, "%p", ptr) < 0)
		return 0;

	return 1;
}

/* ������ָ���ʮ�����Ƶ��ַ���ת��Ϊָ��ֵ */
void* str_to_ptr(const char *str)
{
	void *ptr;

	if (strlen(str) / 2 != sizeof(ptr))
		return NULL;

	sscanf(str, "%p", &ptr);

	return ptr;
}

/* ��ȡ���õ�CPU��Ŀ */
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

/* �����ӡһ������Ҫռ���ֽ��� */
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
