#include <lib/unistd_.h>

// 基础系统调用封装，保持现有汇编风格
void syscall_0(int num, uint32_t arg1) {
    asm volatile(
        "int $0x80"
        :
        : "a"(num), "b"(arg1)
    );
}

uint32_t syscall_1(int num, uint32_t arg1) {
    uint32_t ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)          // 执行完后，把 eax 存入 ret
        : "a"(num), "b"(arg1) // eax = 调用号, ebx = 参数 1
        : "memory"
    );
    return ret;
}

uint32_t syscall_2(int num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    uint32_t ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}
