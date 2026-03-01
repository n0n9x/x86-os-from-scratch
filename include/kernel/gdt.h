#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include <kernel/task.h>

// 定义一个 GDT 条目结构体。按照 Intel 手册要求，必须是 packed（紧凑型）
struct gdt_entry_struct {
    uint16_t limit_low;    // 段限长低 16 位
    uint16_t base_low;     // 段基址低 16 位
    uint8_t  base_middle;  // 段基址中间 8 位
    uint8_t  access;       // 访问标志（权限控制就在这里）
    uint8_t  granularity;  // 粒度
    uint8_t  base_high;    // 段基址高 8 位
} __attribute__((packed));
typedef struct gdt_entry_struct gdt_entry_t;

// 定义 GDT 指针结构体，用于告诉 CPU GDT 在哪
struct gdt_ptr_struct {
    uint16_t limit;        // GDT 大小
    uint32_t base;         // GDT 基地址
} __attribute__((packed));
typedef struct gdt_ptr_struct gdt_ptr_t;

// x86 TSS 结构体定义
struct tss_entry_struct {
    uint32_t prev_tss;   // 如果使用硬件切换则指向前一个 TSS
    uint32_t esp0;       // 重要：内核态栈指针
    uint32_t ss0;        // 重要：内核态栈段选择子 (通常是 0x10)
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));
typedef struct tss_entry_struct tss_entry_t;

extern gdt_entry_t gdt_entries[6];
extern gdt_ptr_t   gdt_ptr;
extern tss_entry_t tss_entry;

void write_tss(int32_t num, uint16_t ss0, uint32_t esp0);
void init_gdt();
void set_kernel_stack(uint32_t stack);

#endif