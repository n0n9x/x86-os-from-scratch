#include <drivers/terminal.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <mm/kheap.h>
#include <kernel/task.h>
#include <kernel/timer.h>
#include <drivers/ide.h>
#include <fs/fat.h>
#include <kernel/elf.h>

extern void init_gdt();
extern void pic_remap();
extern void init_idt();
extern uint32_t _kernel_end; // 内核结束地址(虚拟地址)
// 内核初始化函数
void kernel_init()
{
    //初始化gdt、idt、重映射pic
    init_gdt();
    init_idt();
    pic_remap();
    //初始化物理内存管理
    uint32_t total_mem = 0x8000000; // 128MB物理内存
    pmm_init(total_mem);
    uint32_t kernel_end_vaddr = (uint32_t)&_kernel_end;                // 内核结束地址/虚拟地址
    uint32_t kernel_end_paddr = kernel_end_vaddr - 0xC0000000;         // 内核结束地址/物理地址
    uint32_t free_mem_start = (kernel_end_paddr + 0xFFF) & 0xFFFFF000; // 对齐到4KB，从完整的页开始
    pmm_init_region(free_mem_start, total_mem - free_mem_start);
    //初始化虚拟内存管理
    vmm_init();
    //初始化进程
    task_init();
    //磁盘操作初始化
    ide_init();
    ide_identify();
    //文件系统初始化
    fat_init();
    terminal_writestring("kernel init success!\n");
}

void kernel_main(void)
{
    kernel_init();

    if (load_elf("/shell.elf") != 0)
    {
        // 如果加载失败，可以在这里打印错误或进入内核 shell 备份
        terminal_writestring("Error: Failed to load /shell.elf from disk.\n");
    }

    init_timer(1000);
    asm volatile("sti");
    while (1)
    {
        asm volatile("hlt");
    }
}