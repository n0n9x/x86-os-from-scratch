#include <stdint.h>


// IDT 条目结构体
struct idt_entry_struct {
    uint16_t base_lo;             // 处理程序地址低 16 位
    uint16_t sel;                 // 段选择子（通常是内核代码段 0x08）
    uint8_t  always0;             // 必须为 0
    uint8_t  flags;               // 标志位（存在位、特权级、类型）
    uint16_t base_hi;             // 处理程序地址高 16 位
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

// IDT 指针结构体（用于 lidt 指令）
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 汇编实现的函数，用于加载 IDT
extern void idt_flush(uint32_t);

// num中断号 base中断处理程序地址 sel段选择子 flags标志位
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // 0x8E 表示：段存在，特权级0，32位中断门
    idt_entries[num].flags   = flags;
}

//各个中断处理程序
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();    // 8: 双重故障（CPU 自动压入错误码）
extern void isr9();    // 9: 协处理器段超越
extern void isr10();   // 10: 无效 TSS（CPU 自动压入错误码）
extern void isr11();   // 11: 段不存在（CPU 自动压入错误码）
extern void isr12();   // 12: 栈段故障（CPU 自动压入错误码）
extern void isr13();   // 13: 通用保护故障（CPU 自动压入错误码）
extern void isr14();   // 14: 页故障（CPU 自动压入错误码，核心异常）

// 保留/其他异常中断处理程序（15-31）
extern void isr15();   // 15: 保留
extern void isr16();   // 16: 浮点异常
extern void isr17();   // 17: 对齐检查异常
extern void isr18();   // 18: 机器检查异常
extern void isr19();   // 19: SIMD 浮点异常
extern void isr20();   // 20: 虚拟化异常
extern void isr21();   // 21: 控制保护异常（CPU 自动压入错误码）
extern void isr22();   // 22: 保留
extern void isr23();   // 23: 保留
extern void isr24();   // 24: 保留
extern void isr25();   // 25: 保留
extern void isr26();   // 26: 保留
extern void isr27();   // 27: 保留
extern void isr28();   // 28: 保留
extern void isr29();   // 29: 保留
extern void isr30();   // 30: 保留
extern void isr31();   // 31: 保留
extern void isr128();  //128：系统调用

extern void irq0();
extern void irq1();
extern void irq11();
extern void irq14();


void init_idt() {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 清零所有条目
    for(int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);  // 0: 除法错误
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);  // 1: 调试异常
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);  // 2: NMI 中断（不可屏蔽）
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);  // 3: 断点异常
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);  // 4: 溢出异常
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);  // 5: 边界检查异常
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);  // 6: 无效操作码
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);  // 7: 设备不可用

    // 带错误码的异常中断（8-14）
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);  // 8: 双重故障（带错误码）
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);  // 9: 协处理器段超越
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);  // 10: 无效 TSS（带错误码）
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);  // 11: 段不存在（带错误码）
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);  // 12: 栈段故障（带错误码）
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);  // 13: 通用保护故障（带错误码）
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);  // 14: 页故障（带错误码，特别重要）
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);  // 15: 保留

    // 其他异常中断（16-31）
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);  // 16: 浮点异常
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);  // 17: 对齐检查异常
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);  // 18: 机器检查异常
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);  // 19: SIMD 浮点异常
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);  // 20: 虚拟化异常
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);  // 21: 控制保护异常（带错误码）
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);  // 22: 保留
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);  // 23: 保留
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);  // 24: 保留
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);  // 25: 保留
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);  // 26: 保留
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);  // 27: 保留
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);  // 28: 保留
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);  // 29: 保留
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);  // 30: 保留
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);  // 31: 保留
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);//128:系统调用

    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    

    idt_flush((uint32_t)&idt_ptr);
}