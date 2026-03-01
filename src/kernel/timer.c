#include <stdint.h>
#include <drivers/io.h>
// 设置 PIT 频率 每秒frequency次时钟中断
void init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);             // 命令字节
    outb(0x40, divisor & 0xFF);   // 低8位
    outb(0x40, (divisor >> 8) & 0xFF); // 高8位
}