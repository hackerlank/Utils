/************************************************************************
 * file: platform-arch.h
 * desc: 平台相关宏定义
 * detail: 系统平台架构预编译宏
 * author: liwei (www.leewei.org)
 * mailto: ari.feng@qq.com
 * create: 2013/4/25 11:10:44
 * modify: 2013/4/25 11:10:44
 * version:  1.0
 ************************************************************************/

#ifndef UTILS_PLATFORM_ARCH_H
#define UTILS_PLATFORM_ARCH_H

#include "compiler.h"

/* 操作系统类型 */
#if defined(__APPLE__)
#include <TargetConditionals.h>
#define OS_MACOSX 1
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define OS_IOS 1
#endif  // defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#elif defined(ANDROID)
#define OS_ANDROID 1
#elif defined(__linux__)
#define OS_LINUX 1
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#elif defined(_WIN32)
#define OS_WIN 1
#define TOOLKIT_VIEWS 1
#elif defined(__FreeBSD__)
#define OS_FREEBSD 1
#define TOOLKIT_GTK
#elif defined(__OpenBSD__)
#define OS_OPENBSD 1
#define TOOLKIT_GTK
#elif defined(__sun)
#define OS_SOLARIS 1
#define TOOLKIT_GTK
#else
#error Please add support for your platform.
#endif

// For access to standard BSD features, use OS_BSD instead of a
// more specific macro.
#if defined(OS_FREEBSD) || defined(OS_OPENBSD)
#define OS_BSD 1
#endif

// For access to standard POSIXish features, use OS_POSIX instead of a
// more specific macro.
#if defined(OS_MACOSX) || defined(OS_LINUX) || defined(OS_FREEBSD) ||     \
    defined(OS_OPENBSD) || defined(OS_SOLARIS) || defined(OS_ANDROID) ||  \
    defined(OS_NACL)
#define OS_POSIX 1
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX) && !defined(OS_ANDROID) && \
    !defined(OS_NACL)
#define USE_X11 1  // Use X for graphics.
#endif

/* 处理器类型 */
#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_ARMEL 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__pnacl__)
#define ARCH_CPU_32_BITS 1
#elif defined(__MIPSEL__)
#define ARCH_CPU_MIPS_FAMILY 1
#define ARCH_CPU_MIPSEL 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#else
#error Please add support for your processor architecture.
#endif

/* Windows 操作系统版本 (WINVER) */
#ifdef OS_WIN
#define WIN2000        0x0500                /* Windows 2000 */
#define WINXP        0x0501                /* Windows XP */
#define WINSRV2003    0x0502                /* Windows server 2003 */
#define WINVISTA    0x0600                /* Windows vista */
#define WIN7        0x0601                /* Windows 7 */
#define WIN8        0x0602                /* Windows 8 */
#endif

/* 应用程序位数 */
#ifndef OS_LINUX                        /* linux already defined */
#ifdef ARCH_CPU_64_BITS
#define __WORDSIZE    64
#else
#define __WORDSIZE    32
#endif
#endif

/* Linux大文件支持 */
/* Large File Support (LFS) */
#if (defined OS_LINUX && __WORDSIZE == 32)
#define _FILE_OFFSET_BITS 64    //fopen,fseeko
#define __USE_FILE_OFFSET64        //stat,statfs
#endif

/* 处理被中断的系统调用 */
#ifndef HANDLE_FAILURE
#ifdef OS_LINUX
#define HANDLE_FAILURE(x) ({ \
    typeof(x) eintr_wrapper_result; \
    do { \
    eintr_wrapper_result = (x); \
    } while (eintr_wrapper_result == -1 && errno == EINTR); \
    eintr_wrapper_result; \
})
#else 
#define HANDLE_FAILURE(x) (x)
#endif
#endif /* HANDLE_FAILURE */

/* wchar_t 类型 */
#if defined(OS_WIN)
#define WCHAR_T_IS_UTF16
#elif defined(OS_POSIX) && defined(COMPILER_GCC) && \
    defined(__WCHAR_MAX__) && \
    (__WCHAR_MAX__ == 0x7fffffff || __WCHAR_MAX__ == 0xffffffff)
#define WCHAR_T_IS_UTF32
#elif defined(OS_POSIX) && defined(COMPILER_GCC) && \
    defined(__WCHAR_MAX__) && \
    (__WCHAR_MAX__ == 0x7fff || __WCHAR_MAX__ == 0xffff)
#define WCHAR_T_IS_UTF16
#else
#error Please add support for wchar_t.
#endif

#if defined(__ARMEL__) && !defined(OS_IOS)
#define WCHAR_T_IS_UNSIGNED 1
#elif defined(__MIPSEL__)
#define WCHAR_T_IS_UNSIGNED 0
#endif

#define DISABLE_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);             \
    void operator=(const TypeName&)

#endif // platform-arch_H
