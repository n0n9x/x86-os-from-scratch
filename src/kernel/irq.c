#include <kernel/idt.h>
#include <drivers/terminal.h>
#include <drivers/keyboard.h>
#include <drivers/io.h>
#include <kernel/task.h> 
#include<drivers/ide.h>


extern void pic_send_eoi(uint8_t irq);
extern void keyboard_handler_main();
extern void ide_handler();
void irq_handler(registers_t *regs)
{
    
    // 处理时钟中断 (IRQ0 -> 32)
    if (regs->int_no == 32)
    {
        pic_send_eoi(0);
        task_tick();
        return;
    }

    // 处理键盘中断 (IRQ1 -> 33)
    if (regs->int_no == 33) {
        // 调用键盘驱动的核心处理函数
        keyboard_handler_main(); 
        pic_send_eoi(1);
        return;
    }
    // 处理硬盘中断 (Primary IDE 通道 IRQ14 -> 46)
    if (regs->int_no == 46)
    {
        ide_handler();
        pic_send_eoi(14); // 告知 PIC IRQ14 中断已结束
        return;
    }
}

