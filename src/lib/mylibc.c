#include <lib/mylibc.h>

// ═══════════════════════════════════════════════════════════════
// errno
// ═══════════════════════════════════════════════════════════════
int errno = 0;

// ═══════════════════════════════════════════════════════════════
// 标准流对象
// ═══════════════════════════════════════════════════════════════
static FILE _stdout_file = { .is_stdout = 1, .valid = 1 };
static FILE _stderr_file = { .is_stdout = 1, .valid = 1 }; // stderr 也输出到屏幕
static FILE _stdin_file  = { .is_stdin  = 1, .valid = 1 };

FILE *stdout = &_stdout_file;
FILE *stderr = &_stderr_file;
FILE *stdin  = &_stdin_file;

// ═══════════════════════════════════════════════════════════════
// 进程控制
// ═══════════════════════════════════════════════════════════════
void exit(int code) {
    syscall_0(SYS_EXIT, (uint32_t)code);
    for(;;); // 不会到达，消除编译器警告
}

void *sbrk(int increment) {
    void *ret = (void *)syscall_1(SYS_SBRK, (uint32_t)increment);
    return ret;
}

// ═══════════════════════════════════════════════════════════════
// 内存管理
// 策略：直接调用内核 sbrk，维护一个极简 free list
// ═══════════════════════════════════════════════════════════════

void *malloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 7) & ~7;  // 8字节对齐
    void *ptr = sbrk((int)size);
    if (ptr == (void*)-1) { errno = ENOMEM; return NULL; }
    return ptr;
}

void free(void *ptr) {
    (void)ptr;  // 不释放
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }
    void *new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    // 保守复制：我们不知道原块大小，复制 size 字节
    // 用 memmove 避免重叠（虽然 bump allocator 不会重叠）
    memmove(new_ptr, ptr, size);
    return new_ptr;
}

// ═══════════════════════════════════════════════════════════════
// 内存操作
// ═══════════════════════════════════════════════════════════════
void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t*)dst;
    for (size_t i = 0; i < n; i++) d[i] = (uint8_t)c;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *p = (const uint8_t*)a;
    const uint8_t *q = (const uint8_t*)b;
    for (size_t i = 0; i < n; i++)
        if (p[i] != q[i]) return p[i] - q[i];
    return 0;
}

// ═══════════════════════════════════════════════════════════════
// 字符串
// ═══════════════════════════════════════════════════════════════
size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

char *strcat(char *dst, const char *src) {
    char *d = dst;
    while (*d) d++;
    while ((*d++ = *src++));
    return dst;
}

char *strncat(char *dst, const char *src, size_t n) {
    char *d = dst;
    while (*d) d++;
    while (n-- && (*d++ = *src++));
    *d = '\0';
    return dst;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (a[i] == '\0') return 0;
    }
    return 0;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return (c == '\0') ? (char*)s : NULL;
}

char *strrchr(const char *s, int c) {
    char *last = NULL;
    do {
        if (*s == (char)c) last = (char*)s;
    } while (*s++);
    return last;
}

char *strstr(const char *haystack, const char *needle) {
    size_t nl = strlen(needle);
    if (!nl) return (char*)haystack;
    while (*haystack) {
        if (strncmp(haystack, needle, nl) == 0)
            return (char*)haystack;
        haystack++;
    }
    return NULL;
}

char *strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

// ─── 数值转换 ───────────────────────────────────────────────────

long strtol(const char *s, char **endptr, int base) {
    while (isspace((unsigned char)*s)) s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') { base = 8; s++; }
        else base = 10;
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    long val = 0;
    const char *start = s;
    while (*s) {
        int digit;
        if (*s >= '0' && *s <= '9') digit = *s - '0';
        else if (*s >= 'a' && *s <= 'z') digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z') digit = *s - 'A' + 10;
        else break;
        if (digit >= base) break;
        val = val * base + digit;
        s++;
    }
    (void)start;
    if (endptr) *endptr = (char*)s;
    return neg ? -val : val;
}

unsigned long strtoul(const char *s, char **endptr, int base) {
    return (unsigned long)strtol(s, endptr, base);
}

// 极简 strtod：只处理整数部分和小数部分，不处理科学计数法
// TCC 移植时如果用到浮点可以扩展
double strtod(const char *s, char **endptr) {
    while (isspace((unsigned char)*s)) s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    double val = 0.0;
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s++ - '0'); }
    if (*s == '.') {
        s++;
        double frac = 0.1;
        while (*s >= '0' && *s <= '9') {
            val += (*s++ - '0') * frac;
            frac *= 0.1;
        }
    }
    if (endptr) *endptr = (char*)s;
    return neg ? -val : val;
}

int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }

// ═══════════════════════════════════════════════════════════════
// 字符分类
// ═══════════════════════════════════════════════════════════════
int isalpha(int c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z'); }
int isdigit(int c) { return c>='0'&&c<='9'; }
int isalnum(int c) { return isalpha(c)||isdigit(c); }
int isspace(int c) { return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v'; }
int isupper(int c) { return c>='A'&&c<='Z'; }
int islower(int c) { return c>='a'&&c<='z'; }
int isprint(int c) { return c>=0x20&&c<0x7F; }
int isxdigit(int c){ return isdigit(c)||(c>='a'&&c<='f')||(c>='A'&&c<='F'); }
int toupper(int c) { return islower(c)?c-32:c; }
int tolower(int c) { return isupper(c)?c+32:c; }

// ═══════════════════════════════════════════════════════════════
// 错误字符串
// ═══════════════════════════════════════════════════════════════
char *strerror(int err) {
    switch(err) {
        case 0:      return "Success";
        case ENOENT: return "No such file or directory";
        case ENOMEM: return "Out of memory";
        case EINVAL: return "Invalid argument";
        case EEXIST: return "File exists";
        case ENOSPC: return "No space left on device";
        default:     return "Unknown error";
    }
}

// ═══════════════════════════════════════════════════════════════
// printf 格式化引擎（vsnprintf 核心）
// ═══════════════════════════════════════════════════════════════

// 将整数写入 buf，返回写入字节数
static int fmt_uint(char *buf, size_t buf_size, size_t *pos,
                    unsigned long val, int base, int upper, int width, int zero_pad)
{
    char tmp[32];
    int  len = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    if (val == 0) {
        tmp[len++] = '0';
    } else {
        while (val) {
            tmp[len++] = digits[val % base];
            val /= base;
        }
    }

    // 补零或空格
    char pad = zero_pad ? '0' : ' ';
    while (width > len) {
        if (*pos < buf_size - 1) buf[(*pos)++] = pad;
        width--;
    }

    // 反转
    for (int i = len - 1; i >= 0; i--) {
        if (*pos < buf_size - 1) buf[(*pos)++] = tmp[i];
    }
    return len;
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    size_t pos = 0;

#define PUTC(c) do { if (pos < n-1) buf[pos++] = (c); } while(0)

    while (*fmt) {
        if (*fmt != '%') { PUTC(*fmt++); continue; }
        fmt++; // 跳过 '%'

        // 解析标志（'-' 和 '+' 和空格，'0' 只在宽度前有效）
        int zero_pad = 0, left_align = 0;
        int flag_done = 0;
        while (!flag_done) {
            switch (*fmt) {
                case '-': left_align = 1; fmt++; break;
                case '0': zero_pad = 1;   fmt++; break;
                default:  flag_done = 1;  break;
            }
        }
        if (left_align) zero_pad = 0; // '-' 优先于 '0'

        // 解析宽度
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') width = width*10 + (*fmt++ - '0');

        // 解析长度修饰符
        int is_long = 0;
        if (*fmt == 'l') { is_long = 1; fmt++; }

        switch (*fmt++) {
        case 'd': case 'i': {
            long val = is_long ? va_arg(ap, long) : (long)va_arg(ap, int);
            if (val < 0) { PUTC('-'); val = -val; }
            fmt_uint(buf, n, &pos, (unsigned long)val, 10, 0, width, zero_pad);
            break;
        }
        case 'u': {
            unsigned long val = is_long ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
            fmt_uint(buf, n, &pos, val, 10, 0, width, zero_pad);
            break;
        }
        case 'x': {
            unsigned long val = is_long ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
            fmt_uint(buf, n, &pos, val, 16, 0, width, zero_pad);
            break;
        }
        case 'X': {
            unsigned long val = is_long ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
            fmt_uint(buf, n, &pos, val, 16, 1, width, zero_pad);
            break;
        }
        case 'o': {
            unsigned long val = is_long ? va_arg(ap, unsigned long)
                                        : (unsigned long)va_arg(ap, unsigned int);
            fmt_uint(buf, n, &pos, val, 8, 0, width, zero_pad);
            break;
        }
        case 'p': {
            unsigned long val = (unsigned long)va_arg(ap, void*);
            PUTC('0'); PUTC('x');
            fmt_uint(buf, n, &pos, val, 16, 0, width, zero_pad);
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            int slen = 0;
            const char *tmp = s;
            while (*tmp++) slen++;
            if (!left_align) {
                for (int i = slen; i < width; i++) PUTC(' ');
            }
            while (*s) PUTC(*s++);
            if (left_align) {
                for (int i = slen; i < width; i++) PUTC(' ');
            }
            break;
        }
        case 'c':
            PUTC((char)va_arg(ap, int));
            break;
        case '%':
            PUTC('%');
            break;
        default:
            PUTC('?');
            break;
        }
    }
    buf[pos] = '\0';
    return (int)pos;

#undef PUTC
}

int vfprintf(FILE *f, const char *fmt, va_list ap) {
    char buf[1024];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    fwrite(buf, 1, n, f);
    return n;
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, 65536, fmt, ap);
    va_end(ap);
    return r;
}

// ═══════════════════════════════════════════════════════════════
// 文件 I/O
// ═══════════════════════════════════════════════════════════════

// 文件描述符池
static FILE _file_pool[MYLIBC_FILE_MAX];
static int  _pool_init = 0;

static void pool_init(void) {
    if (_pool_init) return;
    memset(_file_pool, 0, sizeof(_file_pool));
    _pool_init = 1;
}

static FILE *alloc_file(void) {
    pool_init();
    for (int i = 0; i < MYLIBC_FILE_MAX; i++) {
        if (!_file_pool[i].valid) {
            memset(&_file_pool[i], 0, sizeof(FILE));
            _file_pool[i].valid = 1;
            return &_file_pool[i];
        }
    }
    return NULL;
}

FILE *fopen(const char *path, const char *mode) {
    if (!path || !mode) return NULL;

    // 解析模式
    int m = 0;
    if (mode[0] == 'r') m = 0;
    else if (mode[0] == 'w') m = 1;
    else if (mode[0] == 'a') m = 3;
    else return NULL;

    FILE *f = alloc_file();
    if (!f) return NULL;

    strncpy(f->path, path, 127);
    f->path[127] = '\0';
    f->mode = m;
    f->pos  = 0;

    if (m == 0) {
        // 只读：查找文件，获取大小
        // 通过 fread 系统调用读一次来确认文件存在
        uint8_t tmp[1];
        int ret = syscall_2(SYS_FREAD, (uint32_t)path, (uint32_t)tmp, 0);
        if (ret < 0) { f->valid = 0; errno = ENOENT; return NULL; }
        // 获取文件大小：读 0 字节，返回值里我们拿不到 size
        // 简化方案：先读全部到临时缓冲区获取大小（只用于 feof 判断）
        // 实际 size 在首次 fread 时动态更新
        f->size = 0; // 后续 fread 时更新
    } else if (m == 1) {
        // 写模式：如果文件不存在则创建，存在则截断（用空内容覆盖）
        syscall_1(SYS_FDELETE, (uint32_t)path);
        syscall_1(SYS_FCREATE, (uint32_t)path); // 忽略已存在的错误
        // 截断为空
        uint8_t empty = 0;
        syscall_2(SYS_FWRITE, (uint32_t)path, (uint32_t)&empty, 0);
    } else if (m == 3) {
        // 追加：文件不存在则创建
        syscall_1(SYS_FCREATE, (uint32_t)path);
    }

    return f;
}

int fclose(FILE *f) {
    if (!f || f == stdout || f == stderr || f == stdin) return 0;
    f->valid = 0;
    return 0;
}

// 核心读：从 pos 位置读取 size*nmemb 字节
size_t fread(void *buf, size_t size, size_t nmemb, FILE *f) {
    if (!f || !buf) return 0;
    if (f->is_stdin) {
        return (size_t)syscall_2(SYS_READ, 0, (uint32_t)buf, size * nmemb);
    }

    size_t total = size * nmemb;
    if (total == 0) return 0;

    // 先获取文件大小
    if (f->size == 0) {
        int sz = syscall_2(SYS_FREAD, (uint32_t)f->path, (uint32_t)buf, 0);
        if (sz > 0) f->size = (uint32_t)sz;
    }

    if (f->pos >= f->size) return 0;

    size_t avail = f->size - f->pos;
    size_t copy  = (total < avail) ? total : avail;

    // 只分配 pos+copy 大小的缓冲区
    uint8_t *tmp = (uint8_t*)malloc(f->pos + copy);
    if (!tmp) return 0;

    int got = syscall_2(SYS_FREAD, (uint32_t)f->path, (uint32_t)tmp, f->pos + copy);
    if (got <= 0) { free(tmp); return 0; }

    memcpy(buf, tmp + f->pos, copy);
    f->pos += copy;
    free(tmp);
    return copy / size;
}

// 核心写
size_t fwrite(const void *buf, size_t size, size_t nmemb, FILE *f) {
    if (!f) return 0;
    size_t total = size * nmemb;
    if (total == 0) return 0;

    if (f->is_stdout) {
        // 输出到屏幕（通过 SYS_PRINT 系统调用）
        // SYS_PRINT 需要 null-terminated 字符串，所以拷贝一份加 \0
        char *tmp = (char*)malloc(total + 1);
        if (!tmp) return 0;
        memcpy(tmp, buf, total);
        tmp[total] = '\0';
        syscall_0(SYS_PRINT, (uint32_t)tmp);
        free(tmp);
        return nmemb;
    }

    int ret;
    if (f->mode == 3) {
        // 追加模式
        ret = syscall_2(SYS_FAPPEND, (uint32_t)f->path, (uint32_t)buf, total);
    } else {
        // 写模式：写到 pos 处（通过先读后写实现随机写，简化版只支持顺序写）
        ret = syscall_3(SYS_FWRITE, (uint32_t)f->path, (uint32_t)buf, total, f->pos);
        if (ret >= 0) f->pos += total;
    }
    return (ret >= 0) ? nmemb : 0;
}

int fseek(FILE *f, off_t offset, int whence) {
    if (!f) return -1;
    if (f->is_stdout || f->is_stdin) return -1;

    // SEEK_END 需要知道文件大小
    if (whence == SEEK_END && f->size == 0) {
        uint8_t tmp[1];
        int sz = syscall_2(SYS_FREAD, (uint32_t)f->path, (uint32_t)tmp, 0);
        if (sz > 0) f->size = (uint32_t)sz;
    }

    uint32_t new_pos;
    switch(whence) {
        case SEEK_SET: new_pos = (uint32_t)offset; break;
        case SEEK_CUR: new_pos = f->pos + (uint32_t)offset; break;
        case SEEK_END: new_pos = f->size + (uint32_t)offset; break;
        default: return -1;
    }
    f->pos = new_pos;
    return 0;
}

long ftell(FILE *f) {
    if (!f) return -1;
    return (long)f->pos;
}

int feof(FILE *f) {
    if (!f) return 1;
    return f->pos >= f->size;
}

int fgetc(FILE *f) {
    unsigned char c;
    if (fread(&c, 1, 1, f) != 1) return EOF;
    return (int)c;
}

int fputc(int c, FILE *f) {
    unsigned char ch = (unsigned char)c;
    if (fwrite(&ch, 1, 1, f) != 1) return EOF;
    return c;
}

char *fgets(char *buf, int n, FILE *f) {
    if (!buf || n <= 0) return NULL;
    int i = 0;
    while (i < n - 1) {
        int c = fgetc(f);
        if (c == EOF) break;
        buf[i++] = (char)c;
        if (c == '\n') break;
    }
    if (i == 0) return NULL;
    buf[i] = '\0';
    return buf;
}

int fputs(const char *s, FILE *f) {
    size_t n = strlen(s);
    return (fwrite(s, 1, n, f) == n) ? (int)n : EOF;
}

int fprintf(FILE *f, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

int remove(const char *path) {
    return (int)syscall_1(SYS_FDELETE, (uint32_t)path);
}

int rename(const char *old_path, const char *new_path) {
    // 简化实现：读出内容，写到新文件，删旧文件
    (void)old_path; (void)new_path;
    // 后续实现
    return -1;
}

// ═══════════════════════════════════════════════════════════════
// 标准输入输出
// ═══════════════════════════════════════════════════════════════
int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[1024];
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    syscall_0(SYS_PRINT, (uint32_t)buf);
    return n;
}

int puts(const char *s) {
    printf("%s\n", s);
    return 0;
}

int putchar(int c) {
    char buf[2] = { (char)c, '\0' };
    syscall_0(SYS_PRINT, (uint32_t)buf);
    return c;
}

int getchar(void) {
    char buf[2] = {0};
    syscall_2(SYS_READ, 0, (uint32_t)buf, 1);
    return (int)buf[0];
}

char *gets(char *buf) {
    // 读到换行符为止
    int i = 0;
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        buf[i++] = (char)c;
    buf[i] = '\0';
    return buf;
}

int scanf(const char *fmt, ...) {
    // 读一行输入
    char line[256];
    int len = (int)syscall_2(SYS_READ, 0, (uint32_t)line, 255);
    if (len <= 0) return 0;
    if (line[len-1] == '\n') line[len-1] = '\0';
    else line[len] = '\0';

    va_list ap;
    va_start(ap, fmt);

    const char *p = line;  // 当前读取位置
    int matched = 0;

    while (*fmt) {
        // 跳过格式串里的空白
        if (isspace((unsigned char)*fmt)) { fmt++; continue; }

        if (*fmt != '%') {
            // 普通字符：匹配输入
            if (*p == *fmt) { p++; fmt++; }
            else break;
            continue;
        }

        fmt++; // 跳过 '%'

        // 长度修饰符
        int is_long = 0;
        if (*fmt == 'l') { is_long = 1; fmt++; }

        // 跳过输入里的空白（%c 除外）
        if (*fmt != 'c') {
            while (isspace((unsigned char)*p)) p++;
        }

        switch (*fmt++) {
        case 'd': case 'i': {
            if (!*p) goto done;
            char *end;
            long val = strtol(p, &end, 10);
            if (end == p) goto done;
            p = end;
            if (is_long) *va_arg(ap, long*) = val;
            else         *va_arg(ap, int*)  = (int)val;
            matched++;
            break;
        }
        case 'u': {
            if (!*p) goto done;
            char *end;
            unsigned long val = strtoul(p, &end, 10);
            if (end == p) goto done;
            p = end;
            if (is_long) *va_arg(ap, unsigned long*) = val;
            else         *va_arg(ap, unsigned int*)  = (unsigned int)val;
            matched++;
            break;
        }
        case 'x': case 'X': {
            if (!*p) goto done;
            char *end;
            unsigned long val = strtoul(p, &end, 16);
            if (end == p) goto done;
            p = end;
            if (is_long) *va_arg(ap, unsigned long*) = val;
            else         *va_arg(ap, unsigned int*)  = (unsigned int)val;
            matched++;
            break;
        }
        case 's': {
            if (!*p) goto done;
            char *dst = va_arg(ap, char*);
            while (*p && !isspace((unsigned char)*p)) *dst++ = *p++;
            *dst = '\0';
            matched++;
            break;
        }
        case 'c': {
            char *dst = va_arg(ap, char*);
            *dst = *p ? *p++ : '\0';
            matched++;
            break;
        }
        default:
            goto done;
        }
    }

done:
    va_end(ap);
    return matched;
}