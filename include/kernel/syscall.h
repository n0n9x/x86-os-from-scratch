#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>      // 使用标准定义
#include <kernel/idt.h>    // 必须包含，因为需要 registers_t 结构体

// 系统调用分发器
void syscall_handler(registers_t *regs);

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
#define SYS_EXEC 15
#define SYS_PING       16   
#define SYS_NET_SET_IP 17 

#endif