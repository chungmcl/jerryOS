#include "libfdt_env.h"
#include "fdt.h"
#include "libfdt.h"
#include "libfdt_internal.h"

static int fdt_string_eq_(const void *fdt, int stroffset,
			  const char *s, int len)
{
	int slen;
	const char *p = fdt_get_string(fdt, stroffset, &slen);

	return p && (slen == len) && (memcmp(p, s, len) == 0);
}

const char *fdt_string(const void *fdt, int stroffset)
{
	return fdt_get_string(fdt, stroffset, NULL);
}

int fdt_check_prop_offset_(const void *fdt, int offset)
{
	if (!can_assume(VALID_INPUT)
	    && ((offset < 0) || (offset % FDT_TAGSIZE)))
		return -FDT_ERR_BADOFFSET;

	if (fdt_next_tag(fdt, offset, &offset) != FDT_PROP)
		return -FDT_ERR_BADOFFSET;

	return offset;
}

static const struct fdt_property *fdt_get_property_by_offset_(const void *fdt,
  int offset,
  int *lenp)
{
  int err;
  const struct fdt_property *prop;

  if (!can_assume(VALID_INPUT) &&
      (err = fdt_check_prop_offset_(fdt, offset)) < 0) {
    if (lenp)
      *lenp = err;
    return NULL;
  }

  prop = fdt_offset_ptr_(fdt, offset);

  if (lenp)
    *lenp = fdt32_ld_(&prop->len);

  return prop;
}

const struct fdt_property *fdt_get_property_by_offset(const void *fdt,
int offset,
int *lenp)
{
  /* Prior to version 16, properties may need realignment
  * and this API does not work. fdt_getprop_*() will, however. */

  if (!can_assume(LATEST) && fdt_version(fdt) < 0x10) {
    if (lenp)
      *lenp = -FDT_ERR_BADVERSION;
    return NULL;
  }

  return fdt_get_property_by_offset_(fdt, offset, lenp);
}

static const struct fdt_property *fdt_get_property_namelen_(const void *fdt,
						            int offset,
						            const char *name,
						            int namelen,
							    int *lenp,
							    int *poffset)
{
	for (offset = fdt_first_property_offset(fdt, offset);
	     (offset >= 0);
	     (offset = fdt_next_property_offset(fdt, offset))) {
		const struct fdt_property *prop;

		prop = fdt_get_property_by_offset_(fdt, offset, lenp);
		if (!can_assume(LIBFDT_FLAWLESS) && !prop) {
			offset = -FDT_ERR_INTERNAL;
			break;
		}
		if (fdt_string_eq_(fdt, fdt32_ld_(&prop->nameoff),
				   name, namelen)) {
			if (poffset)
				*poffset = offset;
			return prop;
		}
	}

	if (lenp)
		*lenp = offset;
	return NULL;
}


const void *fdt_getprop_namelen(const void *fdt, int nodeoffset,
				const char *name, int namelen, int *lenp)
{
	int poffset;
	const struct fdt_property *prop;

	prop = fdt_get_property_namelen_(fdt, nodeoffset, name, namelen, lenp,
					 &poffset);
	if (!prop)
		return NULL;

	/* Handle realignment */
	if (!can_assume(LATEST) && fdt_version(fdt) < 0x10 &&
	    (poffset + sizeof(*prop)) % 8 && fdt32_ld_(&prop->len) >= 8)
		return prop->data + 4;
	return prop->data;
}

const void *fdt_getprop(const void *fdt, int nodeoffset,
	const char *name, int *lenp)
{
	return fdt_getprop_namelen(fdt, nodeoffset, name, strlen(name), lenp);
}

static int nextprop_(const void *fdt, int offset)
{
	u32 tag;
	int nextoffset;

	do {
		tag = fdt_next_tag(fdt, offset, &nextoffset);

		switch (tag) {
		case FDT_END:
			if (nextoffset >= 0)
				return -FDT_ERR_BADSTRUCTURE;
			else
				return nextoffset;

		case FDT_PROP:
			return offset;
		}
		offset = nextoffset;
	} while (tag == FDT_NOP);

	return -FDT_ERR_NOTFOUND;
}

int fdt_next_property_offset(const void *fdt, int offset)
{
	if ((offset = fdt_check_prop_offset_(fdt, offset)) < 0)
		return offset;

	return nextprop_(fdt, offset);
}

const char *fdt_get_string(const void *fdt, int stroffset, int *lenp)
{
	s32 totalsize;
	u32 absoffset;
	size len;
	int err;
	const char *s, *n;

	if (can_assume(VALID_INPUT)) {
		s = (const char *)fdt + fdt_off_dt_strings(fdt) + stroffset;

		if (lenp)
			*lenp = strlen(s);
		return s;
	}
	totalsize = fdt_ro_probe_(fdt);
	err = totalsize;
	if (totalsize < 0)
		goto fail;

	err = -FDT_ERR_BADOFFSET;
	absoffset = stroffset + fdt_off_dt_strings(fdt);
	if (absoffset >= (unsigned)totalsize)
		goto fail;
	len = totalsize - absoffset;

	if (fdt_magic(fdt) == FDT_MAGIC) {
		if (stroffset < 0)
			goto fail;
		if (can_assume(LATEST) || fdt_version(fdt) >= 17) {
			if ((unsigned)stroffset >= fdt_size_dt_strings(fdt))
				goto fail;
			if ((fdt_size_dt_strings(fdt) - stroffset) < len)
				len = fdt_size_dt_strings(fdt) - stroffset;
		}
	} else if (fdt_magic(fdt) == FDT_SW_MAGIC) {
		unsigned int sw_stroffset = -stroffset;

		if ((stroffset >= 0) ||
		    (sw_stroffset > fdt_size_dt_strings(fdt)))
			goto fail;
		if (sw_stroffset < len)
			len = sw_stroffset;
	} else {
		err = -FDT_ERR_INTERNAL;
		goto fail;
	}

	s = (const char *)fdt + absoffset;
	n = memchr(s, '\0', len);
	if (!n) {
		/* missing terminating NULL */
		err = -FDT_ERR_TRUNCATED;
		goto fail;
	}

	if (lenp)
		*lenp = n - s;
	return s;

fail:
	if (lenp)
		*lenp = err;
	return NULL;
}

int fdt_first_property_offset(const void *fdt, int nodeoffset)
{
	int offset;

	if ((offset = fdt_check_node_offset_(fdt, nodeoffset)) < 0)
		return offset;

	return nextprop_(fdt, offset);
}

/*
 * Minimal sanity check for a read-only tree. fdt_ro_probe_() checks
 * that the given buffer contains what appears to be a flattened
 * device tree with sane information in its header.
 */
s32 fdt_ro_probe_(const void *fdt)
{
	u32 totalsize = fdt_totalsize(fdt);

	if (can_assume(VALID_DTB))
		return totalsize;

	/* The device tree must be at an 8-byte aligned address */
	if ((uintptr)fdt & 7)
		return -FDT_ERR_ALIGNMENT;

	if (fdt_magic(fdt) == FDT_MAGIC) {
		/* Complete tree */
		if (!can_assume(LATEST)) {
			if (fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
				return -FDT_ERR_BADVERSION;
			if (fdt_last_comp_version(fdt) >
					FDT_LAST_SUPPORTED_VERSION)
				return -FDT_ERR_BADVERSION;
		}
	} else if (fdt_magic(fdt) == FDT_SW_MAGIC) {
		/* Unfinished sequential-write blob */
		if (!can_assume(VALID_INPUT) && fdt_size_dt_struct(fdt) == 0)
			return -FDT_ERR_BADSTATE;
	} else {
		return -FDT_ERR_BADMAGIC;
	}

	if (totalsize < INT32_MAX)
		return totalsize;
	else
		return -FDT_ERR_TRUNCATED;
}

const char *fdt_get_name(const void *fdt, int nodeoffset, int *len)
{
	const struct fdt_node_header *nh = fdt_offset_ptr_(fdt, nodeoffset);
	const char *nameptr;
	int err;

	if (((err = fdt_ro_probe_(fdt)) < 0)
	    || ((err = fdt_check_node_offset_(fdt, nodeoffset)) < 0))
			goto fail;

	nameptr = nh->name;

	if (!can_assume(LATEST) && fdt_version(fdt) < 0x10) {
		/*
		 * For old FDT versions, match the naming conventions of V16:
		 * give only the leaf name (after all /). The actual tree
		 * contents are loosely checked.
		 */
		const char *leaf;
		leaf = strchr(nameptr, '/');
		if (leaf == NULL) {
			err = -FDT_ERR_BADSTRUCTURE;
			goto fail;
		}
		nameptr = leaf+1;
	}

	if (len)
		*len = strlen(nameptr);

	return nameptr;

 fail:
	if (len)
		*len = err;
	return NULL;
}