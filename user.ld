/* user.ld */
ENTRY(_start) /* 假设你的 shell.c 入口是 main */

SECTIONS
{
    . = 0x08048000; /* 用户态程序起始虚拟地址 */

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}