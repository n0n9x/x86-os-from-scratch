#include <lib/mylibc.h>

int main(int argc, char **argv);

void _start_main(int argc, char **argv) {
    exit(main(argc, argv));
}

void __attribute__((naked)) _start(void) {
    asm volatile(
        "xor %ebp, %ebp\n"
        "mov (%esp), %eax\n"
        "lea 4(%esp), %ebx\n"
        "push %ebx\n"
        "push %eax\n"
        "call _start_main\n"
        "mov $2, %eax\n"
        "xor %ebx, %ebx\n"
        "int $0x80\n"
    );
}
