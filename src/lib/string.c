#include<lib/string.h>

// 内存拷贝：将 src 指向的 n 字节拷贝到 dest
void *memcpy(void *dest, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// 内存设置：将 dest 指向的前 n 字节设置为固定值 c
void *memset(void *dest, int c, uint32_t n) {
    uint8_t *d = (uint8_t *)dest;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = (uint8_t)c;
    }
    return dest;
}

// 内存比较：比较两个内存区域的前 n 字节
int memcmp(const void *s1, const void *s2, uint32_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for (uint32_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

// 获取字符串长度
uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (s[len]) len++;
    return len;
}

// 字符串比较
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(uint8_t *)s1 - *(uint8_t *)s2;
}

// 字符串拷贝
char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

// 限长字符串拷贝（mkdir 和 create_file 中用到）
char *strncpy(char *dest, const char *src, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

// 查找字符最后一次出现的位置（路径解析关键函数）
char *strrchr(const char *s, int c) {

    char *last = 0;
    do {
        if (*s == (char)c) last = (char *)s;
    } while (*s++);
    return last;
}

int strncmp(const char *s1, const char *s2, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return (uint8_t)s1[i] - (uint8_t)s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

