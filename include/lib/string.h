#ifndef _STRING_H
#define _STRING_H
#include <stdint.h>

void *memcpy(void *dest, const void *src, uint32_t n);
void *memset(void *dest, int c, uint32_t n);
int memcmp(const void *s1, const void *s2, uint32_t n);
uint32_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, uint32_t n);
char *strrchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, uint32_t n);
char *strcat(char *dest, const char *src);

#endif