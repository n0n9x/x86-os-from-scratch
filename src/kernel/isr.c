#include <kernel/idt.h>
#include <drivers/terminal.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <kernel/syscall.h>

// 所有的 ISR 汇编原型声明
extern void isr0();  // 0: 除法错误
extern void isr1();  // 1: 调试异常
extern void isr2();  // 2: NMI 中断（不可屏蔽）
extern void isr3();  // 3: 断点异常
extern void isr4();  // 4: 溢出异常
extern void isr5();  // 5: 边界检查异常
extern void isr6();  // 6: 无效操作码
extern void isr7();  // 7: 设备不可用
extern void isr8();  // 8: 双重故障（带错误码）
extern void isr9();  // 9: 协处理器段超越
extern void isr10(); // 10: 无效 TSS（带错误码）
extern void isr11(); // 11: 段不存在（带错误码）
extern void isr12(); // 12: 栈段故障（带错误码）
extern void isr13(); // 13: 通用保护故障（带错误码）
extern void isr14(); // 14: 页故障（带错误码）
extern void isr15(); // 15: 保留
extern void isr16(); // 16: 浮点异常
extern void isr17(); // 17: 对齐检查异常
extern void isr18(); // 18: 机器检查异常
extern void isr19(); // 19: SIMD 浮点异常
extern void isr20(); // 20: 虚拟化异常
extern void isr21(); // 21: 控制保护异常（带错误码）
extern void isr22(); // 22: 保留
extern void isr23(); // 23: 保留
extern void isr24(); // 24: 保留
extern void isr25(); // 25: 保留
extern void isr26(); // 26: 保留
extern void isr27(); // 27: 保留
extern void isr28(); // 28: 保留
extern void isr29(); // 29: 保留
extern void isr30(); // 30: 保留
extern void isr31(); // 31: 保留

void isr_handler(registers_t *regs)
{
    if (regs->int_no == 6)
    {
        terminal_writestring("\n--- EXCEPTION: INVALID OPCODE (0x06) ---\n");

        // 打印报错时的指令地址 (EIP)
        terminal_writestring("EIP: ");
        terminal_write_hex(regs->eip); // 你需要一个打印 16 进制的函数

        terminal_writestring("\nCS: ");
        terminal_write_hex(regs->cs);

        // 如果是在用户态崩溃，CS 通常是 0x1B
        if (regs->cs == 0x1B)
        {
            terminal_writestring(" [User Mode]");
        }
        else
        {
            terminal_writestring(" [Kernel Mode]");
        }

        terminal_writestring("\n---------------------------------------\n");

        // 报错后挂起，防止屏幕被后续日志冲掉
        for (;;)
            ;
    }
    // 128是系统调用
    if (regs->int_no == 128)
    {
        // 跳转到系统调用分发函数
        syscall_handler(regs);
    }
    if (regs->int_no == 13)
    {
        terminal_writestring("\nGPF! err=");
        terminal_write_hex(regs->err_code);
        terminal_writestring(" eip=");
        terminal_write_hex(regs->eip);
        terminal_writestring(" cs=");
        terminal_write_hex(regs->cs);
        terminal_writestring("\n");
        for (;;)
            ;
    }
    // 14 号是 Page Fault
    if (regs->int_no == 14)
    {
        page_fault_handler(regs);
        return;
    }

    // 处理其他致命异常
    if (regs->int_no < 32)
    {
        terminal_writestring("\nUnhandled Exception: ");
        terminal_write_hex(regs->int_no);
        terminal_writestring(". Halted.\n");
        for (;;)
            ;
    }
}
