#ifndef STDIO_H
#define STDIO_H

#include <lib/unistd_.h>

void printf_simple(char *s);
int scanf_simple(char *buf, uint32_t max_len);

#endif