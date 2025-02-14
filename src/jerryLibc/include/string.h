#ifndef JERRYLIBC_STRING_H
#define JERRYLIBC_STRING_H

#include "jerryTypes.h"

size strlen(const char* s);
char* strchr(const char* s, int c);
void* memchr(const void* s, int c, size n);
int memcmp(const void* s1, const void* s2, size n);

#endif  // JERRYLIBC_STRING_H