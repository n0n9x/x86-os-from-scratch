; 符合 Multiboot 标准的引导程序
; 这样我们可以直接用 GRUB 或 QEMU 加载它

; 定义常量
MBALIGN  equ  1 << 0            ; 内存对齐
MEMINFO  equ  1 << 1            ; 获取内存信息
FLAGS    equ  MBALIGN | MEMINFO ; 标志位
MAGIC    equ  0x1BADB002        ; 魔法数字，引导程序识别我们的凭证
CHECKSUM equ -(MAGIC + FLAGS)   ; 校验和

; 多引导头部分
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; --- 核心修改：定义临时页表 ---
section .data
align 4096
boot_page_directory:
    ; 第 0 项：身份映射 (0..4MB -> 0..4MB)
    dd (boot_page_table1 - 0xC0000000 + 0x003) 
    times (768 - 1) dd 0
    ; 第 768 项：内核映射 (3GB..3GB+4MB -> 0..4MB)
    dd (boot_page_table1 - 0xC0000000 + 0x003)
    times (1024 - 768 - 1) dd 0

boot_page_table1:
    %assign i 0
    %rep 1024
        dd (i << 12) | 0x003
        %assign i i+1
    %endrep

; 定义栈空间（内核运行需要栈）
section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB 栈空间
global stack_top   
stack_top:

; 内核入口点
section .text
global _start
_start:
    ; 1. 加载临时页目录的物理地址
    mov ecx, (boot_page_directory - 0xC0000000)
    mov cr3, ecx

    ; 2. 开启分页 (CR0.PG = 1)
    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    ; 3. 此时 EIP 还在低地址，我们需要一个长跳转进入高地址执行
    lea ecx, [.higher_half]
    jmp ecx

.higher_half:
    ; 4. 现在已经在 0xC0000000 以上运行了！设置高地址栈
    mov esp, stack_top

    ; 5. 调用 C
    extern kernel_main
    call kernel_main

    cli
.hang: hlt
    jmp .hang

global gdt_flush    ; 导出函数给 C 语言调用
gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]
    jmp 0x08:.next    ; 必不可少！刷新 CS 缓存
.next:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

global idt_flush
idt_flush:
    mov eax, [esp+4]  ; 获取 IDT 指针参数
    lidt [eax]        ; 加载 IDT
    ret

global tss_flush
tss_flush:
    mov ax, 0x28      ; TSS 在 GDT 中的索引是 5，(5 << 3) | 3 = 0x2B
                      ; 注意：TSS 的选择子也需要设置 RPL 为 3
    ltr ax            ; Load Task Register
    ret



; -----------------------------------ISR

; 宏：用于没有错误码的异常
%macro ISR_NOERRCODE 1
  [global isr%1]        ; 显式使用方括号确保全局导出
  isr%1:
    cli
    push dword 0
    push dword %1
    jmp isr_common_stub
%endmacro

; 宏：用于有错误码的异常
%macro ISR_ERRCODE 1
  [global isr%1]
  isr%1:
    cli
    push dword %1
    jmp isr_common_stub
%endmacro

; 定义前 32 个异常
ISR_NOERRCODE 0   ; 0: 除以零
ISR_NOERRCODE 1   ; 1: 调试
ISR_NOERRCODE 2   ; 2: 非屏蔽中断
ISR_NOERRCODE 3   ; 3: 断点
ISR_NOERRCODE 4   ; 4: 溢出
ISR_NOERRCODE 5   ; 5: 边界范围超出
ISR_NOERRCODE 6   ; 6: 无效指令
ISR_NOERRCODE 7   ; 7: 设备不可用
ISR_ERRCODE   8   ; 8: 双重错误 (有错误码)
ISR_NOERRCODE 9   ; 9: 协处理器段超出
ISR_ERRCODE   10  ; 10: 无效 TSS
ISR_ERRCODE   11  ; 11: 段不存在
ISR_ERRCODE   12  ; 12: 栈段故障 (有错误码)
ISR_ERRCODE   13  ; 13: 通用保护故障 (有错误码)
ISR_ERRCODE   14  ; 14: 页故障 (有错误码，核心异常)
ISR_NOERRCODE 15  ; 15: 保留
ISR_NOERRCODE 16  ; 16: 浮点运算异常
ISR_NOERRCODE 17  ; 17: 对齐检查异常
ISR_NOERRCODE 18  ; 18: 机器检查异常
ISR_NOERRCODE 19  ; 19: SIMD 浮点运算异常
ISR_NOERRCODE 20  ; 20: 虚拟化异常
ISR_ERRCODE   21  ; 21: 控制保护异常 (有错误码)
ISR_NOERRCODE 22  ; 22: 保留
ISR_NOERRCODE 23  ; 23: 保留
ISR_NOERRCODE 24  ; 24: 保留
ISR_NOERRCODE 25  ; 25: 保留
ISR_NOERRCODE 26  ; 26: 保留
ISR_NOERRCODE 27  ; 27: 保留
ISR_NOERRCODE 28  ; 28: 保留
ISR_NOERRCODE 29  ; 29: 保留
ISR_NOERRCODE 30  ; 30: 保留
ISR_NOERRCODE 31  ; 31: 保留
ISR_NOERRCODE 128

; 统一的中断处理入口
extern isr_handler
isr_common_stub:
    pusha
    mov ax, ds
    push eax ;保存旧数据
    mov ax, 0x10 ;进入内核数据段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp          ; 把当前栈顶地址作为参数传给 C 函数
    call isr_handler
    add esp, 4        ; 清理压入的参数

    pop eax ;装回原来的旧段
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret ; 中断返回（弹出 CS, EIP, EFLAGS等）


; -----------------------------------IRQ

%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push dword 0
    push dword %2
    jmp irq_common_stub
%endmacro

IRQ 0, 32    ; 时钟 (Timer)
IRQ 1, 33    ; 键盘 (Keyboard)
IRQ 14, 46   ; 硬盘 (IDE Primary Channel)

extern irq_handler
extern schedule
irq_common_stub:
    pusha                   ; 压入 8 个通用寄存器 (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    mov ax, ds
    push eax                ; 保存原 DS 段选择子

    ; 加载内核数据段选择子 (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                ; 将当前栈指针作为 registers_t *regs 参数传给 C
    call irq_handler        ; 调用 C 语言中断处理逻辑
    add esp, 4              ; 清理参数栈

    ; 显式导出退出点，供 create_task 伪造现场使用
global irq_exit
irq_exit:
    pop eax                 ; 弹出原 DS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                    ; 弹出 8 个通用寄存器
    add esp, 8              ; 跳过 int_no 和 err_code
    iret                    ; 彻底恢复硬件现场 (EIP, CS, EFLAGS)