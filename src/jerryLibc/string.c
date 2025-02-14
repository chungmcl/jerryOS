#include "string.h"

size strlen(const char* s) {
	const char* p = s;
	while (*p) {
		p += 1;
	}
	return p - s;
}

char* strchr(const char *s, int c) {
  const char* last = NULL;

  while (*s) {
    if (*s == (char)c) {
      last = s;
    }
    s += 1;
  }

  return (char *)(c == '\0' ? s : last);
}

void* memchr(const void* s, int c, size n) {
  const unsigned char* p = s;
  unsigned char uc = (unsigned char)c;

  while (n--) {
    if (*p == uc) {
      return (void*)p;
    }
    p += 1;
  }
  return NULL;
}

int memcmp(const void* s1, const void* s2, size n) {
  const unsigned char* p1 = (const unsigned char*)s1;
  const unsigned char* p2 = (const unsigned char*)s2;

  while (n--) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1 += 1;
    p2 += 1;
  }
  return 0;
}
