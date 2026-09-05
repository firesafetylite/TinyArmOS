#ifndef TINYGPT_NATIVE_STRING_H
#define TINYGPT_NATIVE_STRING_H
#include <stddef.h>
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
char *strchr(const char *, int);
size_t strlen(const char *);
#endif
