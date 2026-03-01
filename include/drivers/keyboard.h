#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// 声明外部数组，告诉编译器这两个表在别处定义
extern unsigned char kbd_us[128];
extern unsigned char kbd_us_shift[128];


char kbd_get_char();
void keyboard_handler_main();


#endif