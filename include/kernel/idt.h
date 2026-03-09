#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// 对应汇编中 pusha 后的栈结构
struct registers {
    uint32_t ds;                                     // 数据段选择子
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha 压入的寄存器
    uint32_t int_no, err_code;                       // 我们手动压入的中断号和错误码
    uint32_t eip, cs, eflags, useresp, ss;           // CPU 自动压入的
};

typedef struct registers registers_t;

void init_idt();

#endif
