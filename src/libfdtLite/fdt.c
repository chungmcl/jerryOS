// SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause)
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 */
#include "libfdt_env.h"

#include "fdt.h"
#include "libfdt.h"

#include "libfdt_internal.h"

static int check_off_(u32 hdrsize, u32 totalsize, u32 off)
{
	return (off >= hdrsize) && (off <= totalsize);
}

static int check_block_(u32 hdrsize, u32 totalsize,
  u32 base, u32 size)
{
if (!check_off_(hdrsize, totalsize, base))
return 0; /* block start out of bounds */
if ((base + size) < base)
return 0; /* overflow */
if (!check_off_(hdrsize, totalsize, base + size))
return 0; /* block end out of bounds */
return 1;
}

size fdt_header_size_(u32 version)
{
	if (version <= 1)
		return FDT_V1_SIZE;
	else if (version <= 2)
		return FDT_V2_SIZE;
	else if (version <= 3)
		return FDT_V3_SIZE;
	else if (version <= 16)
		return FDT_V16_SIZE;
	else
		return FDT_V17_SIZE;
}

size fdt_header_size(const void *fdt)
{
	return can_assume(LATEST) ? FDT_V17_SIZE :
		fdt_header_size_(fdt_version(fdt));
}

// Returns 0 upon success
int fdt_check_header(const void *fdt)
{
	size hdrsize;

	/* The device tree must be at an 8-byte aligned address */
	if ((uintptr)fdt & 7)
		return -FDT_ERR_ALIGNMENT;

	if (fdt_magic(fdt) != FDT_MAGIC)
		return -FDT_ERR_BADMAGIC;
	if (!can_assume(LATEST)) {
		if ((fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
		    || (fdt_last_comp_version(fdt) >
			FDT_LAST_SUPPORTED_VERSION))
			return -FDT_ERR_BADVERSION;
		if (fdt_version(fdt) < fdt_last_comp_version(fdt))
			return -FDT_ERR_BADVERSION;
	}
	hdrsize = fdt_header_size(fdt);
	if (!can_assume(VALID_DTB)) {
		if ((fdt_totalsize(fdt) < hdrsize)
		    || (fdt_totalsize(fdt) > INT_MAX))
			return -FDT_ERR_TRUNCATED;

		/* Bounds check memrsv block */
		if (!check_off_(hdrsize, fdt_totalsize(fdt),
				fdt_off_mem_rsvmap(fdt)))
			return -FDT_ERR_TRUNCATED;

		/* Bounds check structure block */
		if (!can_assume(LATEST) && fdt_version(fdt) < 17) {
			if (!check_off_(hdrsize, fdt_totalsize(fdt),
					fdt_off_dt_struct(fdt)))
				return -FDT_ERR_TRUNCATED;
		} else {
			if (!check_block_(hdrsize, fdt_totalsize(fdt),
					  fdt_off_dt_struct(fdt),
					  fdt_size_dt_struct(fdt)))
				return -FDT_ERR_TRUNCATED;
		}

		/* Bounds check strings block */
		if (!check_block_(hdrsize, fdt_totalsize(fdt),
				  fdt_off_dt_strings(fdt),
				  fdt_size_dt_strings(fdt)))
			return -FDT_ERR_TRUNCATED;
	}

	return 0;
}

const void *fdt_offset_ptr(const void *fdt, int offset, unsigned int len)
{
	unsigned int uoffset = offset;
	unsigned int absoffset = offset + fdt_off_dt_struct(fdt);

	if (offset < 0)
		return NULL;

	if (!can_assume(VALID_INPUT))
		if ((absoffset < uoffset)
		    || ((absoffset + len) < absoffset)
		    || (absoffset + len) > fdt_totalsize(fdt))
			return NULL;

	if (can_assume(LATEST) || fdt_version(fdt) >= 0x11)
		if (((uoffset + len) < uoffset)
		    || ((offset + len) > fdt_size_dt_struct(fdt)))
			return NULL;

	return fdt_offset_ptr_(fdt, offset);
}

u32 fdt_next_tag(const void *fdt, int startoffset, int *nextoffset)
{
	const fdt32_t *tagp, *lenp;
	u32 tag, len, sum;
	int offset = startoffset;
	const char *p;

	*nextoffset = -FDT_ERR_TRUNCATED;
	tagp = fdt_offset_ptr(fdt, offset, FDT_TAGSIZE);
	if (!can_assume(VALID_DTB) && !tagp)
		return FDT_END; /* premature end */
	tag = fdt32_to_cpu(*tagp);
	offset += FDT_TAGSIZE;

	*nextoffset = -FDT_ERR_BADSTRUCTURE;
	switch (tag) {
	case FDT_BEGIN_NODE:
		/* skip name */
		do {
			p = fdt_offset_ptr(fdt, offset++, 1);
		} while (p && (*p != '\0'));
		if (!can_assume(VALID_DTB) && !p)
			return FDT_END; /* premature end */
		break;

	case FDT_PROP:
		lenp = fdt_offset_ptr(fdt, offset, sizeof(*lenp));
		if (!can_assume(VALID_DTB) && !lenp)
			return FDT_END; /* premature end */

		len = fdt32_to_cpu(*lenp);
		sum = len + offset;
		if (!can_assume(VALID_DTB) &&
		    (INT_MAX <= sum || sum < (u32) offset))
			return FDT_END; /* premature end */

		/* skip-name offset, length and value */
		offset += sizeof(struct fdt_property) - FDT_TAGSIZE + len;

		if (!can_assume(LATEST) &&
		    fdt_version(fdt) < 0x10 && len >= 8 &&
		    ((offset - len) % 8) != 0)
			offset += 4;
		break;

	case FDT_END:
	case FDT_END_NODE:
	case FDT_NOP:
		break;

	default:
		return FDT_END;
	}

	if (!fdt_offset_ptr(fdt, startoffset, offset - startoffset))
		return FDT_END; /* premature end */

	*nextoffset = FDT_TAGALIGN(offset);
	return tag;
}

int fdt_check_node_offset_(const void *fdt, int offset)
{
	if (!can_assume(VALID_INPUT)
	    && ((offset < 0) || (offset % FDT_TAGSIZE)))
		return -FDT_ERR_BADOFFSET;

	if (fdt_next_tag(fdt, offset, &offset) != FDT_BEGIN_NODE)
		return -FDT_ERR_BADOFFSET;

	return offset;
}

int fdt_next_node(const void *fdt, int offset, int *depth)
{
	int nextoffset = 0;
	u32 tag;

	if (offset >= 0)
		if ((nextoffset = fdt_check_node_offset_(fdt, offset)) < 0)
			return nextoffset;

	do {
		offset = nextoffset;
		tag = fdt_next_tag(fdt, offset, &nextoffset);

		switch (tag) {
		case FDT_PROP:
		case FDT_NOP:
			break;

		case FDT_BEGIN_NODE:
			if (depth)
				(*depth)++;
			break;

		case FDT_END_NODE:
			if (depth && ((--(*depth)) < 0))
				return nextoffset;
			break;

		case FDT_END:
			if ((nextoffset >= 0)
			    || ((nextoffset == -FDT_ERR_TRUNCATED) && !depth))
				return -FDT_ERR_NOTFOUND;
			else
				return nextoffset;
		}
	} while (tag != FDT_BEGIN_NODE);

	return offset;
}
