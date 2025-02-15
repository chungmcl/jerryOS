#include "string.h"

size strlen(const char* s) {
	const char* p = s;
	while (*p) {
		p += 1;
	}
	return p - s;
}

char* strchr(const char *s, s32 c) {
  const char* last = NULL;

  while (*s) {
    if (*s == (char)c) {
      last = s;
    }
    s += 1;
  }

  return (char *)(c == '\0' ? s : last);
}

s32 strncmp(const char* s1, const char* s2, size n) {
  while (n--) {
    if (*s1 != *s2 || *s1 == '\0') {
      return (unsigned char)*s1 - (unsigned char)*s2;
    }
    s1 += 1;
    s2 += 1;
  }
  return 0;
}

bool strStartsWith(const char* input, const char* prefix) {
  return strncmp(prefix, input, strlen(prefix)) == 0;
}

void* memchr(const void* s, s32 c, size n) {
  const u8* p = s;
  unsigned char uc = (unsigned char)c;

  while (n--) {
    if (*p == uc) {
      return (void*)p;
    }
    p += 1;
  }
  return NULL;
}

s32 memcmp(const void* s1, const void* s2, size n) {
  const u8* p1 = (const u8*)s1;
  const u8* p2 = (const u8*)s2;

  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1 += 1;
    p2 += 1;
  }
  return 0;
}
