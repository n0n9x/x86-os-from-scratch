#ifndef IO_H
#define IO_H

#include <stdint.h>

// 向port端口发送1字节数据
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}
// 从port端口读取1字节数据
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
// 从port端口读取2字节数据
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
// 向port端口发送2字节数据
static inline void outw(uint16_t port, uint16_t data) {
    asm volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}
// 0x80是空闲端口，强制等待
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif