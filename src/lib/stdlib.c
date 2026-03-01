#include<lib/stdlib.h>

void* sbrk_simple(int increment) {
    // 像 printf_simple 调 syscall_0 一样
    // 这里调用 syscall_1 并返回地址
    return (void*)syscall_1(SYS_SBRK, (uint32_t)increment);
}
// 用户态封装
void user_task_exit() {
    // 触发系统调用，假设 SYS_EXIT 是你的调用号
    syscall_0(SYS_EXIT, 0);
}

int disk_read(uint32_t lba, uint8_t *buf) {
    // 调用 syscall_2，传入 LBA 和缓冲区指针
    return (int)syscall_2(SYS_DISK_READ, lba, (uint32_t)buf, 0);
}

int disk_write(uint32_t lba, uint8_t *buf) {
    return (int)syscall_2(SYS_DISK_WRITE, lba, (uint32_t)buf, 0);
}

int fcreate(const char *name) {
    // 假设调用号为 10 或你定义的 SYS_FCREATE
    return (int)syscall_1(SYS_FCREATE, (uint32_t)name); 
}

int open(const char *name) {
    return (int)syscall_1(SYS_OPEN, (uint32_t)name);
}

int fread(const char *name, uint8_t *buf, uint32_t len) {
    // ebx=name, ecx=buf, edx=len
    return (int)syscall_2(SYS_FREAD, (uint32_t)name, (uint32_t)buf, len);
}

int fwrite(const char *name, uint8_t *buf, uint32_t len) {
    // 使用 syscall_2，ebx=文件名, ecx=缓冲区, edx=长度
    return (int)syscall_2(SYS_FWRITE, (uint32_t)name, (uint32_t)buf, len);
}

int fdelete(const char *filename) {
    // ebx 传文件名
    return (int)syscall_1(SYS_FDELETE, (uint32_t)filename);
}

int fappend(const char *filename, uint8_t *buf, uint32_t len) {
    // ebx=文件名, ecx=缓冲区, edx=长度
    return (int)syscall_2(SYS_FAPPEND, (uint32_t)filename, (uint32_t)buf, len);
}

void flist(const char *path) {
    syscall_1(SYS_FLIST, (uint32_t)path);
}

int mkdir(const char *path) {
    return (int)syscall_1(SYS_MKDIR, (uint32_t)path);
}

void shutdown() {
    syscall_0(SYS_SHUTDOWN, 0);
}
