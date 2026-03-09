; src/apps/ustart.s
; 用户程序通用入口，NASM 语法
; 栈布局（由 elf.c 的 load_elf_with_args 布置）：
;   [esp+0] = argc
;   [esp+4] = argv[0]
;   [esp+8] = argv[1]
;   ...

section .text
global _start
extern _start_main

_start:
    xor   ebp, ebp          ; 清空帧指针

    mov   eax, [esp]        ; eax = argc
    lea   ebx, [esp + 4]    ; ebx = argv（指向 argv[0] 的指针）

    push  ebx               ; 压入 argv
    push  eax               ; 压入 argc
    call  _start_main        ; void _start_main(int argc, char **argv)
    add   esp, 8

    ; 如果意外返回，强制 exit(0)
    mov   eax, 2            ; SYS_EXIT = 2
    xor   ebx, ebx
    int   0x80