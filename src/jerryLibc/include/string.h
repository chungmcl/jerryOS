#ifndef JERRYLIBC_STRING_H
#define JERRYLIBC_STRING_H

#include "jerryOS.h"

size strlen(const char* s);
char* strchr(const char* s, int c);
i32 strncmp(const char* s1, const char* s2, size n);
bool strStartsWith(const char* input, const char* prefix);

void* memset(void* s, u8 c, size n);
void* memchr(const void* s, int c, size n);
i32 memcmp(const void* s1, const void* s2, size n);

#endif  // JERRYLIBC_STRING_H