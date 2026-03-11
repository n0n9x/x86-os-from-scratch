#include <drivers/terminal.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <mm/kheap.h>
#include <kernel/task.h>
#include <kernel/timer.h>
#include <drivers/ide.h>
#include <fs/fat.h>
#include <kernel/elf.h>
#include <net/net.h>      

extern void init_gdt();
extern void pic_remap();
extern void init_idt();
extern uint32_t _kernel_end;

void kernel_init()
{
    init_gdt();
    init_idt();
    pic_remap();

    uint32_t total_mem = 0x8000000; // 128MB
    pmm_init(total_mem);
    uint32_t kernel_end_vaddr = (uint32_t)&_kernel_end;
    uint32_t kernel_end_paddr = kernel_end_vaddr - 0xC0000000;
    uint32_t free_mem_start = (kernel_end_paddr + 0xFFF) & 0xFFFFF000;
    pmm_init_region(free_mem_start, total_mem - free_mem_start);

    vmm_init();
    task_init();

    ide_init();
    ide_identify();
    fat_init();

    //初始化网络子系统
    net_setup();

    terminal_writestring("kernel init success!\n");
}

void kernel_main(void)
{
    kernel_init();

    if (load_elf("/shell.elf") != 0)
    {
        terminal_writestring("Error: Failed to load /shell.elf from disk.\n");
    }

    init_timer(1000);
    asm volatile("sti");
    while (1)
    {
        asm volatile("hlt");
    }
}