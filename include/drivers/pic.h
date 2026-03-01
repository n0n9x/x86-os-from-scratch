#ifndef PIC_H
#define PIC_H

#include <stdint.h>
#include <drivers/io.h> // 必须包含 io.h 以使用 outb 函数

// PIC 主片和从片的控制端口
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// PIC 命令
#define PIC_EOI         0x20 // End of Interrupt (中断结束信号)

/**
 * 重映射 PIC 中断向量
 * 将 IRQ 0-15 映射到 IDT 的 32-47 号中断
 */
void pic_remap();

/**
 * 发送中断结束信号 (EOI)
 * @param irq 中断号 (0-15)
 */
void pic_send_eoi(uint8_t irq);

#endif