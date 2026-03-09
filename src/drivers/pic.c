#include <drivers/pic.h>

// PIC 8259A PIC芯片
void pic_remap()
{
    // 保存当前的屏蔽位
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    // 开始初始化 ICW1
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    // 设置偏移量 ICW2 (IRQ 0-7 -> 32-39, IRQ 8-15 -> 40-47)
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    // 级联设置 ICW3
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    // 设置环境模式 ICW4
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // 恢复屏蔽位
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}
