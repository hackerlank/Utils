/************************************************************************
 * file: min-max.h
 * desc: 最大值/最小值宏
 * detail: GCC编译器可避免对参数二次求值的副作用
 * author: liwei (www.leewei.org)
 * mailto: ari.feng@qq.com
 * create: 2013/4/25 12:09:11
 * modify: 2013/4/25 12:09:11
 * version:  1.0
 * copyright: Pansafe Software (Shanghai) Co,.Ltd (C) 2011 - 2013
 ************************************************************************/

#ifndef UTILS_MIN_MAX_H
#define UTILS_MIN_MAX_H

#include "compiler.h"

#if defined(COMPILER_GCC)
#define xmin(x, y) ({                \
    typeof(x) _min1 = (x);            \
    typeof(y) _min2 = (y);            \
    (void) (&_min1 == &_min2);        \
    _min1 < _min2 ? _min1 : _min2; })

#define xmax(x, y) ({                \
    typeof(x) _max1 = (x);            \
    typeof(y) _max2 = (y);            \
    (void) (&_max1 == &_max2);        \
    _max1 > _max2 ? _max1 : _max2; })

#define xmin3(x, y, z) ({            \
    typeof(x) _min1 = (x);            \
    typeof(y) _min2 = (y);            \
    typeof(z) _min3 = (z);            \
    (void) (&_min1 == &_min2);        \
    (void) (&_min1 == &_min3);        \
    _min1 < _min2 ? (_min1 < _min3 ? _min1 : _min3) : \
    (_min2 < _min3 ? _min2 : _min3); })

#define xmax3(x, y, z) ({            \
    typeof(x) _max1 = (x);            \
    typeof(y) _max2 = (y);            \
    typeof(z) _max3 = (z);            \
    (void) (&_max1 == &_max2);        \
    (void) (&_max1 == &_max3);        \
    _max1 > _max2 ? (_max1 > _max3 ? _max1 : _max3) : \
    (_max2 > _max3 ? _max2 : _max3); })

#else /* !COMPILER_GCC */

#define xmin(x, y) ((x) > (y) ? (y) : (x))
#define xmax(x, y) ((x) > (y) ? (x) : (y))
#define xmin3(x, y, z) xmin(xmin(x, y), z)
#define xmax3(x, y, z) xmax(xmax(x, y), z)

#endif /* COMPILER_GCC */

#endif // UTILS_MIN_MAX_H