/* 程序的入口点，对应 boot.s 中的 _start 标签 */
ENTRY(_start)

SECTIONS
{
    /* 1. 设置内核运行的虚拟起始地址 (3GB + 1MB) */
    . = 0xC0100000;

    /* 2. 定义代码段 */
    /* AT(...) 告诉链接器，这部分的物理加载地址 (LMA) 是虚拟地址减去 0xC0000000 */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.multiboot)   /* 必须放在最前面，方便引导程序识别 */
        *(.text)
    }

    /* 3. 只读数据 */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    /* 4. 已初始化的数据 */
    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    /* 5. 未初始化的数据 (BSS) */
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }

    /* 定义内核结束的虚拟地址符号 */
    _kernel_end = .;
}
