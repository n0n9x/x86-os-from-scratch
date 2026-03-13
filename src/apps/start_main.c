
// 普通elf程序调用入口

#include <lib/mylibc.h>

//声明main函数
int main(int argc, char **argv);

void _start_main(int argc, char **argv) {
    exit(main(argc, argv));
}

