[GLOBAL _start]
_start:
    xor ebp, ebp
    extern shell_main
    call shell_main
    mov eax, 2
    int 0x80