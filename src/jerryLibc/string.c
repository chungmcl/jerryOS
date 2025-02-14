#include "string.h"

size strlen(const char* s) {
	const char* p = s;
	while (*p) {
		p++;
	}
	return p - s;
}

char* strchr(const char *s, int c) {
  const char* last = NULL;

  while (*s) {
    if (*s == (char)c) {
      last = s;
    }
    s++;
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
    p++;
  }
  return NULL;
}