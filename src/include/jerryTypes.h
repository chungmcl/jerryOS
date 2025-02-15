// Whoever came up with the "_t" convention hurts me
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;

typedef unsigned long long uintptr;
typedef unsigned long long size;

#define bool  _Bool
#define true  1
#define false 0

#define NULL 0

#define INT8_MAX         127
#define INT16_MAX        32767
#define INT32_MAX        2147483647
#define INT64_MAX        9223372036854775807LL

#define UINT_MAX        0xffffffff      /* max value for an unsigned int */
#define INT_MAX         2147483647      /* max value for an int */
#define INT_MIN         (-2147483647-1) /* min value for an int */

#define READ32(_reg) (*(volatile u32 *)&(_reg))
#define READ64(_reg) (*(volatile u64 *)&(_reg))
#define WRITE32(_reg, _val)                  \
do {                                         \
	register u32 __myval__ = (_val);      \
	*(volatile u32*)&(_reg) = __myval__; \
} while (0)

#define dsb() asm volatile("dsb sy" ::: "memory")
#define isb() asm volatile("isb" ::: "memory")
