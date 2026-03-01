#include<lib/stdio.h>

void printf_simple(char *s) {
    asm volatile("int $0x80" :: "a"(0), "b"(s));
}

// 简单的 scanf 实现，只演示读取字符串
int scanf_simple(char *buf, uint32_t max_len) {
    // 调用 sys_read, fd=0 (标准输入), count=max_len
    uint32_t len = syscall_2(SYS_READ, 0, (uint32_t)buf, max_len);
    
    // 字符串化处理
    if (len > 0) {
        if (buf[len-1] == '\n') {
            buf[len-1] = '\0';
        } else if (len < max_len) {
            buf[len] = '\0';
        }
    }
    return len;
}