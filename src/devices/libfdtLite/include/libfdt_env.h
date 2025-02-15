/* SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause) */
#ifndef LIBFDT_ENV_H
#define LIBFDT_ENV_H
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 * Copyright 2012 Kim Phillips, Freescale Semiconductor.
 */

#include "jerryTypes.h"
#include "string.h"

#ifdef __CHECKER__
#define FDT_FORCE __attribute__((force))
#define FDT_BITWISE __attribute__((bitwise))
#else
#define FDT_FORCE
#define FDT_BITWISE
#endif

typedef u16 FDT_BITWISE fdt16_t;
typedef u32 FDT_BITWISE fdt32_t;
typedef u64 FDT_BITWISE fdt64_t;

#define EXTRACT_BYTE(x, n)	((unsigned long long)((u8 *)&x)[n])
#define CPU_TO_FDT16(x) ((EXTRACT_BYTE(x, 0) << 8) | EXTRACT_BYTE(x, 1))
#define CPU_TO_FDT32(x) ((EXTRACT_BYTE(x, 0) << 24) | (EXTRACT_BYTE(x, 1) << 16) | \
			 (EXTRACT_BYTE(x, 2) << 8) | EXTRACT_BYTE(x, 3))
#define CPU_TO_FDT64(x) ((EXTRACT_BYTE(x, 0) << 56) | (EXTRACT_BYTE(x, 1) << 48) | \
			 (EXTRACT_BYTE(x, 2) << 40) | (EXTRACT_BYTE(x, 3) << 32) | \
			 (EXTRACT_BYTE(x, 4) << 24) | (EXTRACT_BYTE(x, 5) << 16) | \
			 (EXTRACT_BYTE(x, 6) << 8) | EXTRACT_BYTE(x, 7))

static inline u16 fdt16_to_cpu(fdt16_t x)
{
	return (FDT_FORCE u16)CPU_TO_FDT16(x);
}
static inline fdt16_t cpu_to_fdt16(u16 x)
{
	return (FDT_FORCE fdt16_t)CPU_TO_FDT16(x);
}

static inline u32 fdt32_to_cpu(fdt32_t x)
{
	return (FDT_FORCE u32)CPU_TO_FDT32(x);
}
static inline fdt32_t cpu_to_fdt32(u32 x)
{
	return (FDT_FORCE fdt32_t)CPU_TO_FDT32(x);
}

static inline u64 fdt64_to_cpu(fdt64_t x)
{
	return (FDT_FORCE u64)CPU_TO_FDT64(x);
}
static inline fdt64_t cpu_to_fdt64(u64 x)
{
	return (FDT_FORCE fdt64_t)CPU_TO_FDT64(x);
}
#undef CPU_TO_FDT64
#undef CPU_TO_FDT32
#undef CPU_TO_FDT16
#undef EXTRACT_BYTE

#endif /* LIBFDT_ENV_H */