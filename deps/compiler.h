/************************************************************************
 * file: compiler-specific.h
 * desc: 编译器相关宏定义
 * detail: GCC vs MSVC
 * author: liwei (www.leewei.org)
 * mailto: ari.feng@qq.com
 * create: 2013/4/25 11:07:07
 * modify: 2013/4/25 11:07:07
 * version:  1.0
 ************************************************************************/

#ifndef UTILS_COMPILER_SPECIFIC_H
#define UTILS_COMPILER_SPECIFIC_H

/* 编译器类型 */
#if defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#endif

/* VC编译器 */
#if defined(COMPILER_MSVC)

#define MSVC5    1100        /* VC 5.0 */
#define MSVC6    1200        /* VC 6.0 */
#define MSVC7    1300        /* VC .NET 2002 */
#define MSVC71    1310        /* VC .NET 2003 */
#define MSVC8    1400        /* VC 2005 */
#define MSVC9    1500        /* VC 2008 */
#define MSVC10    1600        /* VC 2010 */
#define MSVC11    1700        /* VC 2012 */
#define MSVC12    1800       /* VC 2013 */

#if _MSC_VER < MSVC12
#define restrict
#endif

#pragma warning(disable: 4996)            /* The POSIX name for this item is deprecated */
#pragma warning (disable: 4786)            /* identifier was truncated to '255' characters */

/*
 * Macros for suppressing and disabling warnings on MSVC.
 */
#define MSVC_SUPPRESS_WARNING(n) __pragma(warning(suppress:n))

/*
 * MSVC_PUSH_DISABLE_WARNING pushes |n| onto a stack of warnings to be disabled.
 * The warning remains disabled until popped by MSVC_POP_WARNING.
 */
#define MSVC_PUSH_DISABLE_WARNING(n) __pragma(warning(push)) \
    __pragma(warning(disable:n))

/*
 * MSVC_PUSH_WARNING_LEVEL pushes |n| as the global warning level.  The level
 * remains in effect until popped by MSVC_POP_WARNING().  Use 0 to disable all
 * warnings.
 */
#define MSVC_PUSH_WARNING_LEVEL(n) __pragma(warning(push, n))

/* Pop effects of innermost MSVC_PUSH_* macro. */
#define MSVC_POP_WARNING() __pragma(warning(pop))

#define MSVC_DISABLE_OPTIMIZE() __pragma(optimize("", off))
#define MSVC_ENABLE_OPTIMIZE() __pragma(optimize("", on))

/*
 * Compiler warning C4355: 'this': used in base member initializer list:
 * http://msdn.microsoft.com/en-us/library/3c594ae3(VS.80).aspx
 */
#define ALLOW_THIS_IN_INITIALIZER_LIST(code) MSVC_PUSH_DISABLE_WARNING(4355) \
    code \
    MSVC_POP_WARNING()

/*
 * Allows exporting a class that inherits from a non-exported base class.
 * This uses suppress instead of push/pop because the delimiter after the
 * declaration (either "," or "{") has to be placed before the pop macro.
 *
 * Example usage:
 * class EXPORT_API Foo : NON_EXPORTED_BASE(public Bar) {
 */
#define NON_EXPORTED_BASE(code) MSVC_SUPPRESS_WARNING(4275) \
    code

#else  /* !COMPILER_MSVC */

#define MSVC_SUPPRESS_WARNING(n)
#define MSVC_PUSH_DISABLE_WARNING(n)
#define MSVC_PUSH_WARNING_LEVEL(n)
#define MSVC_POP_WARNING()
#define MSVC_DISABLE_OPTIMIZE()
#define MSVC_ENABLE_OPTIMIZE()
#define ALLOW_THIS_IN_INITIALIZER_LIST(code) code
#define NON_EXPORTED_BASE(code) code

#endif  // COMPILER_MSVC

/* GCC Specific */
#if defined(COMPILER_GCC)
#define ALLOW_UNUSED        __attribute__((unused))                    /* int x ALLOW_UNUSED = ...; */
#define DEPRECATED            __attribute__((deprecated))    
#define ALWAYS_INLINE        __attribute__((always_inline))
#define PRINTF_FMT(a, b) __attribute__((format(printf, a, b)))
#define SCANF_FORMAT(a, b)    __attribute__((format(scanf, a, b)))
#define ATTR_PURE            __attribute__((pure))
#define ATTR_CONST            __attribute__((__const__))
#define LINK_ERROR(msg)        __attribute__((__error__(msg)))
#define COMPILE_WARN(msg)    __attribute__((warning(msg)))
#define COMPILE_ERROR(msg)    __attribute__((error(msg)))
#define WARN_UNUSED_RESULT    __attribute__((warn_unused_result))
inline static void ignore_result_helper(int __attribute__((unused)) dummy, ...) {}
#define IGNORE_RESULT(x) ignore_result_helper(0, (x))
#else /* !COMPILER_GCC */
#define ALLOW_UNUSED
#define DEPRECATED
#define ALWAYS_INLINE __forceinline
#define PRINTF_FMT(a, b)
#define SCANF_FORMAT(a, b)
#define ATTR_PURE            
#define ATTR_CONST        
#define LINK_ERROR(msg)        
#define COMPILE_WARN(msg)    
#define COMPILE_ERROR(msg)
#define WARN_UNUSED_RESULT
#define IGNORE_RESULT(x) (x)
#endif /* COMPILER_GCC */

#define WUR    WARN_UNUSED_RESULT

/* 禁用函数内联优化 */
/* NOINLINE void DoStuff() { ... } */
#if defined(COMPILER_GCC)
#define NOINLINE __attribute__((noinline))
#elif defined(COMPILER_MSVC)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif

/*
 * 指定类、结构体等内存对齐
 * class ALIGNAS(16) MyClass { ... }
 * ALIGNAS(16) int array[4];
 */
#if defined(COMPILER_MSVC)
#define ALIGNAS(byte_alignment) __declspec(align(byte_alignment))
#elif defined(COMPILER_GCC)
#define ALIGNAS(byte_alignment) __attribute__((aligned(byte_alignment)))
#endif

/*
 * Return the byte alignment of the given type (available at compile time).  Use
 * sizeof(type) prior to checking __alignof to workaround Visual C++ bug:
 * http://goo.gl/isH0C
 * Use like:
 *   ALIGNOF(int32)  // this would be 4
 */
#if defined(COMPILER_MSVC)
#define ALIGNOF(type) (sizeof(type) - sizeof(type) + __alignof(type))
#elif defined(COMPILER_GCC)
#define ALIGNOF(type) __alignof__(type)
#endif

/*
 * Annotate a virtual method indicating it must be overriding a virtual
 * method in the parent class.
 * Use like:
 *   virtual void foo() OVERRIDE;
 */
#ifndef OVERRIDE
#if defined(COMPILER_MSVC)
#define OVERRIDE override
#elif defined(__clang__)
#define OVERRIDE override
#else
#define OVERRIDE
#endif
#endif

/* 分支预测优化 */
#if defined(COMPILER_GCC)
#define likely(x)            __builtin_expect(!!(x), 1)    
#define unlikely(x)            __builtin_expect(!!(x), 0)
#else
#define likely(x)    (x)
#define unlikely(x) (x)    
#endif

/* 可变参数架构体复制 */
#if defined(COMPILER_GCC)
#define VA_COPY(a, b) (va_copy(a, b))
#elif defined(COMPILER_MSVC)
#define VA_COPY(a, b) (a = b)
#endif

#endif // UTILS_COMPILER_SPECIFIC_H
