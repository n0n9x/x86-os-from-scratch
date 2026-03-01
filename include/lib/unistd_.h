#ifndef UNISTD__H
#define UNISTD__H

#include <stdint.h>
#include <stddef.h>

// 系统调用号统一管理
#define SYS_PRINT 0
#define SYS_SBRK 1
#define SYS_EXIT 2
#define SYS_READ 3
#define SYS_DISK_READ  4
#define SYS_DISK_WRITE 5
#define SYS_OPEN 6
#define SYS_FREAD 7
#define SYS_FWRITE 8
#define SYS_FCREATE 9
#define SYS_FDELETE 10
#define SYS_FAPPEND 11
#define SYS_FLIST 12
#define SYS_MKDIR 13
#define SYS_SHUTDOWN 14

uint32_t syscall_1(int num, uint32_t arg1);
void syscall_0(int num, uint32_t arg1);
uint32_t syscall_2(int num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif