#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 堆的内存范围定义 */
#define KHEAP_START 0xC0400000
#define KHEAP_MAX   0xE0000000

/* 内存块头部结构 */
typedef struct header {
    uint32_t size;         // 当前数据区的大小 (不含 header 本身)
    bool is_free;          // 该块是否处于空闲状态
    struct header *prev;
    struct header *next;   // 指向链表中下一个内存块
} header_t;

/* 导出函数供其他文件使用 */
void kheap_init();         // 初始化堆起始标记
void* kmalloc(size_t size); // 分配内存
void kfree(void* ptr);     // 释放内存并尝试合并

#endif