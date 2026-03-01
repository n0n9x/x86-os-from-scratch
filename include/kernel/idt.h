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

// 内存地址,内容,来源,对应结构体成员
// [ESP + 60],SS,硬件 (特权级切换时),ss
// [ESP + 56],ESP,硬件 (特权级切换时),useresp
// [ESP + 52],EFLAGS,硬件,eflags
// [ESP + 48],CS,硬件,cs
// [ESP + 44],EIP,硬件,eip
// [ESP + 40],Error Code,硬件或宏 (push 0),err_code
// [ESP + 36],Int No,宏 (push %1),int_no
// [ESP + 32],EAX,pusha,eax
// [ESP + 28],ECX,pusha,ecx
// [ESP + 24],EDX,pusha,edx
// [ESP + 20],EBX,pusha,ebx
// [ESP + 16],Old ESP,pusha,esp (被忽略)
// [ESP + 12],EBP,pusha,ebp
// [ESP + 8],ESI,pusha,esi
// [ESP + 4],EDI,pusha,edi
// [ESP + 0],DS,push eax (旧的 ds),ds