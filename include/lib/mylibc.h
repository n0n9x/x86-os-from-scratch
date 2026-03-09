#ifndef MYLIBC_H
#define MYLIBC_H

// 依赖系统调用号和封装函数
#include <lib/unistd_.h>

// ── 基本类型 ────────────────────────────────────────────────────
// 用编译器内建宏定义，避免与 stddef.h / stdint.h 里的 typedef 冲突
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int ssize_t;
#endif

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef long off_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// ── FILE 结构 ───────────────────────────────────────────────────
#define MYLIBC_FILE_MAX 16

typedef struct _FILE
{
    char path[128];
    int mode; // 0=r 1=w 3=a
    uint32_t pos;
    uint32_t size;
    int valid;
    int is_stdout;
    int is_stdin;
    uint8_t *data; // 文件内容缓冲区，fopen 时一次性读入
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

// ── 文件 I/O ────────────────────────────────────────────────────
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *f);
size_t fread(void *buf, size_t size, size_t nmemb, FILE *f);
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *f);
int fseek(FILE *f, off_t offset, int whence);
long ftell(FILE *f);
int feof(FILE *f);
int fgetc(FILE *f);
int fputc(int c, FILE *f);
char *fgets(char *buf, int n, FILE *f);
int fputs(const char *s, FILE *f);
int fprintf(FILE *f, const char *fmt, ...);
int remove(const char *path);

// ── 标准输入输出 ────────────────────────────────────────────────
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t n, const char *fmt, ...);
int puts(const char *s);
int putchar(int c);
int getchar(void);
char *gets(char *buf);
int scanf(const char *fmt, ...);

// ── 内存管理 ────────────────────────────────────────────────────
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

// ── 字符串 ──────────────────────────────────────────────────────
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t n);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);
long strtol(const char *s, char **endptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
double strtod(const char *s, char **endptr);
int atoi(const char *s);
long atol(const char *s);

// ── 内存操作 ────────────────────────────────────────────────────
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);

// ── 字符分类 ────────────────────────────────────────────────────
int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int isprint(int c);
int isxdigit(int c);
int toupper(int c);
int tolower(int c);

// ── 进程控制 ────────────────────────────────────────────────────
void exit(int code);
void *sbrk(int increment);

// ── 错误处理 ────────────────────────────────────────────────────
extern int errno;
#define ENOENT 2
#define ENOMEM 12
#define EINVAL 22
#define EEXIST 17
#define ENOSPC 28
char *strerror(int err);

// ── 可变参数 ────────────────────────────────────────────────────
typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
int vfprintf(FILE *f, const char *fmt, va_list ap);

#endif