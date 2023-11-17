#pragma once

#if defined(EN_SIM)
#include <Windows.h>
#include <intrin.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "types.h"

//// 기본 macro 정의 ////

#ifdef FALSE
#undef FALSE
#undef TRUE
#endif

#if defined(__cplusplus)
#define FORCE_C		extern "C"
#else
#define FORCE_C
#endif

#define NOT(x)						(!(x))
#if defined(EN_SIM)
#define BRK_IF(cond, print)			do{ if (cond){								\
										if(print){SIM_Print("BRK: %s(%d) %s\n", __FILE__, __LINE__, #cond);} \
										__debugbreak(); }}while(0)
#else
#define BRK_IF(cond, print)			if (cond){while(1);}
#endif

#define ASSERT(cond)				BRK_IF(NOT(cond), true)
#define UNUSED(x)					(void)(x)

#define IF_THEN(cond, check)		ASSERT(NOT(exp) || (check))

#define STATIC_ASSERT(exp, str)		static_assert(exp, str);

#define BIT(shift)					(1 <<(shift))
#define BIT_SET(dst, mask)			((dst) |= (mask))
#define BIT_CLR(dst, mask)			((dst) &= ~(mask))
#define BIT_TST(dst, mask)			((dst) & (mask))
//#define BIT_COUNT(val32)			__popcnt(val32)

#define BIT_SCAN_LSB(bmVal)			(__builtin_ctz(bmVal))
#define BIT_SCAN_MSB(bmVal)			(__builtin_clz(bmVal))

#define MEMSET_OBJ(obj, val)		memset((void*)&(obj), val, sizeof(obj))
#define MEMSET_ARRAY(arr, val)		memset((void*)(arr), val, sizeof(arr))
#define MEMSET_PTR(ptr, val)		memset((void*)(ptr), val, sizeof(*ptr))

#define DIV_UP(nNum, nAlign)		(((nNum) + (nAlign) - 1) / (nAlign))
#define DIV_DN(nNum, nAlign)		((nNum) / (nAlign))

#define ALIGN_UP(nNum, nAlign)		(DIV_UP(nNum, nAlign) * (nAlign))
#define ALIGN_DN(nNum, nAlign)		(DIV_DN(nNum, nAlign) * (nAlign))

#define MIN(x, y)					((x) > (y)) ? (y) : (x)
#define MAX(x, y)					((x) > (y)) ? (x) : (y)

#if (0)	// if branch prediction works well, the gab isn't big.
#define likely(x)					__builtin_expect(!!(x), 1)
#define unlikely(x)					__builtin_expect(!!(x), 0)
#else
#define likely(x)					(x)
#define unlikely(x)					(x)
#endif

#define SECT_AT(x)	__attribute__((section(#x)))
