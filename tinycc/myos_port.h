/*
 * myos_port.h - TCC 移植适配层
 * 放在 tinycc/ 目录下
 * 通过 -include myos_port.h 强制在每个编译单元最前面包含
 *
 * 作用：拦截 TCC 对标准库头文件的 include，替换成 mylibc
 */

#ifndef MYOS_PORT_H
#define MYOS_PORT_H

/* ── 1. 包含我们自己的 libc ─────────────────────────────────────
 * 标准头文件由 fake_inc/ 目录下的空文件拦截（通过 -I./fake_inc 优先搜索）
 * 所有实际内容都在 mylibc.h 里
 */
#include "../include/lib/mylibc.h"

/* ── 2. 补充 mylibc 没有但 TCC 需要的类型 ───────────────────── */

/* bool */
#ifndef __bool_true_false_are_defined
typedef int bool;
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif

/* va_list —— mylibc.h 里已经有，但以防万一 */
#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)
#define va_arg(ap, type)   __builtin_va_arg(ap, type)
#define va_copy(dst, src)  __builtin_va_copy(dst, src)
#endif

/* intptr_t / uintptr_t / ptrdiff_t 由 stdint.h 提供，不重复定义 */

/* ── 3. 文件系统相关（TCC 用来查找头文件和输出目标文件） ──────── */

/* stat 结构体，TCC 用来获取文件大小 */
struct stat {
    unsigned int st_mode;
    unsigned int st_size;
    unsigned int st_mtime;
};
#define S_ISREG(m)  1
#define S_ISDIR(m)  0
#define S_IFREG     0x8000
#define S_IFDIR     0x4000
#define O_RDONLY    0
#define O_WRONLY    1
#define O_CREAT     0x40
#define O_TRUNC     0x200
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

static inline int stat(const char *path, struct stat *st) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    st->st_size  = (unsigned int)ftell(f);
    st->st_mode  = S_IFREG;
    st->st_mtime = 0;
    fclose(f);
    return 0;
}

static inline int access(const char *path, int mode) {
    (void)mode;
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    fclose(f);
    return 0;
}

/* open/read/write/close —— TCC 内部直接用 POSIX 接口读文件 */
typedef int ssize_t_port;

static inline int open(const char *path, int flags, ...) {
    (void)flags;
    /* 把文件指针转成 int 句柄（简单做法：用 FILE* 当 fd）
     * TCC 的 open 只在少数路径用，主要还是走 fopen */
    FILE *f;
    if (flags & O_WRONLY)
        f = fopen(path, "w");
    else
        f = fopen(path, "r");
    if (!f) return -1;
    return (int)(uintptr_t)f;  /* 把 FILE* 当 fd 返回 */
}

static inline int close(int fd) {
    if (fd < 0) return -1;
    return fclose((FILE*)(uintptr_t)fd);
}

static inline int read(int fd, void *buf, size_t count) {
    if (fd < 0) return -1;
    return (int)fread(buf, 1, count, (FILE*)(uintptr_t)fd);
}

static inline int write(int fd, const void *buf, size_t count) {
    if (fd < 0) return -1;
    return (int)fwrite(buf, 1, count, (FILE*)(uintptr_t)fd);
}

static inline long lseek(int fd, long offset, int whence) {
    if (fd < 0) return -1;
    fseek((FILE*)(uintptr_t)fd, offset, whence);
    return ftell((FILE*)(uintptr_t)fd);
}

/* getcwd / chdir —— TCC 处理相对 include 路径时用 */
static inline char *getcwd(char *buf, size_t size) {
    /* 我们的 OS 没有工作目录，固定返回根目录 */
    if (buf && size > 1) { buf[0] = '/'; buf[1] = '\0'; }
    return buf;
}

static inline int chdir(const char *path) {
    (void)path;
    return 0;
}

/* getenv —— TCC 用来找头文件路径，返回 NULL 表示没有环境变量 */
static inline char *getenv(const char *name) {
    (void)name;
    return NULL;
}

/* ── 4. 错误处理 ──────────────────────────────────────────────── */
extern int errno;
#define ENOENT  2
#define EACCES  13
#define EINVAL  22
#define ENOMEM  12
#define EEXIST  17

/* perror */
static inline void perror(const char *s) {
    if (s) printf("%s: error %d\n", s, errno);
}

/* ── 5. 其他杂项 ──────────────────────────────────────────────── */

/* TCC 用 longjmp 做错误恢复 */
/* 我们用最简版实现：直接 exit */
typedef struct { int _dummy; } jmp_buf[1];
static inline int  setjmp(jmp_buf env)               { (void)env; return 0; }
static inline void longjmp(jmp_buf env, int val)      { (void)env; (void)val; exit(1); }

/* assert */
#ifndef assert
#define assert(x) do { if (!(x)) { \
    printf("assert failed: %s line %d\n", __FILE__, __LINE__); \
    exit(1); } } while(0)
#endif

/* signal —— TCC 有时会注册信号处理，我们忽略 */
#define SIGABRT 6
#define SIGSEGV 11
typedef void (*sighandler_t)(int);
static inline sighandler_t signal(int sig, sighandler_t handler) {
    (void)sig; (void)handler; return (sighandler_t)0;
}
static inline void abort(void) { exit(1); }

/* qsort —— mylibc 里如果没有，补一个简单版 */
#ifndef _QSORT_DEFINED
#define _QSORT_DEFINED
static inline void qsort(void *base, size_t nmemb, size_t size,
                          int (*compar)(const void *, const void *)) {
    /* 插入排序，够 TCC 用 */
    char *b = (char *)base;
    for (size_t i = 1; i < nmemb; i++) {
        char tmp[256]; /* size 不会超过这个 */
        memcpy(tmp, b + i * size, size);
        size_t j = i;
        while (j > 0 && compar(b + (j-1)*size, tmp) > 0) {
            memcpy(b + j*size, b + (j-1)*size, size);
            j--;
        }
        memcpy(b + j * size, tmp, size);
    }
}
#endif

/* ── 6. TCC 编译目标配置 ─────────────────────────────────────── */
/* 只保留 i386 目标，禁用其他架构 */
#define TCC_TARGET_I386
#undef  TCC_TARGET_X86_64
#undef  TCC_TARGET_ARM
#undef  TCC_TARGET_ARM64
#undef  TCC_TARGET_RISCV64
#undef  TCC_TARGET_C67

/* 禁用我们不需要的功能 */
#define CONFIG_TCC_STATIC       /* 静态编译，不用 dlopen */
#define CONFIG_TCC_SEMLOCK  0   /* 不需要线程锁 */

/* 告诉 TCC 头文件在哪里（运行时通过 -I 参数覆盖也可以） */
#define CONFIG_SYSROOT          ""
#define CONFIG_TCC_SYSINCLUDEPATHS "/include"

/* ── 7. sys/time.h 里的 timeval ───────────────────────────────── */
struct timeval { long tv_sec; long tv_usec; };
static inline int gettimeofday(struct timeval *tv, void *tz) {
    (void)tz;
    if (tv) { tv->tv_sec = 0; tv->tv_usec = 0; }
    return 0;
}

/* ── 8. mmap 相关（tccrun.c 用到，我们用 malloc 模拟）──────────── */
#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4
#define MAP_PRIVATE 2
#define MAP_ANON    0x20
#define MAP_FAILED  ((void*)-1)

static inline void *mmap(void *addr, size_t len, int prot, int flags, int fd, long off) {
    (void)addr; (void)prot; (void)flags; (void)fd; (void)off;
    return malloc(len);
}
static inline int munmap(void *addr, size_t len) {
    (void)len; free(addr); return 0;
}
static inline int mprotect(void *addr, size_t len, int prot) {
    (void)addr; (void)len; (void)prot; return 0;
}

/* ── 9. environ 和 execv（tccrun.c 用到）────────────────────────── */
static char *_empty_env[] = { NULL };
static char **environ = _empty_env;
static inline int execv(const char *path, char **argv) {
    (void)path; (void)argv; return -1;
}

/* ── 10. fflush / fdopen / freopen ───────────────────────────── */
static inline int fflush(FILE *f) { (void)f; return 0; }
static inline FILE *fdopen(int fd, const char *mode) {
    (void)mode; return (FILE*)(uintptr_t)fd;
}
static inline FILE *freopen(const char *path, const char *mode, FILE *f) {
    (void)f;
    return fopen(path, mode);
}

/* ── 11. 缺失的字符串和数学函数 ──────────────────────────────── */
static inline int strpbrk_idx(const char *s, const char *accept) {
    while (*s) {
        const char *a = accept;
        while (*a) { if (*s == *a) return 1; a++; }
        s++;
    }
    return 0;
}
static inline char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        const char *a = accept;
        while (*a) { if (*s == *a) return (char*)s; a++; }
        s++;
    }
    return NULL;
}
static inline char *realpath(const char *path, char *resolved) {
    if (!resolved) resolved = (char*)malloc(256);
    if (!resolved) return NULL;
    /* 我们的 OS 没有复杂路径，直接复制 */
    strncpy(resolved, path, 255);
    resolved[255] = '\0';
    return resolved;
}
static inline int unlink(const char *path) {
    /* SYS_FDELETE = 10 */
    return (int)syscall_1(10, (uint32_t)path);
}
static inline int execvp(const char *path, char **argv) {
    (void)path; (void)argv; return -1;
}

/* 浮点数解析 */
static inline float       strtof(const char *s, char **e)  { return (float)strtod(s, e); }
static inline long double strtold(const char *s, char **e) { return (long double)strtod(s, e); }
static inline long long   strtoll(const char *s, char **e, int base) {
    return (long long)strtol(s, e, base);
}
static inline unsigned long long strtoull(const char *s, char **e, int base) {
    return (unsigned long long)strtoul(s, e, base);
}
static inline long double ldexpl(long double x, int exp) {
    /* 简单实现：x * 2^exp */
    if (exp >= 0) { while (exp--) x *= 2.0L; }
    else          { while (exp++) x /= 2.0L; }
    return x;
}

/* _start_main 定义在单独的 tcc_entry.c 里，不放在这里 */

#endif /* MYOS_PORT_H */