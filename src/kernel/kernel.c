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
extern uint32_t _kernel_end; // 内核结束地址
//内核初始化函数
void kernel_init(){
    init_gdt();
    init_idt();
    pic_remap();
    
    uint32_t total_mem = 0x8000000;
    pmm_init(total_mem);
    pmm_init_region(0x200000, total_mem - 0x200000);
    
    vmm_init();
    task_init();
    ide_init();
    ide_identify();
    fat_init();
    terminal_writestring("kernel init success!\n");
}

void kernel_main(void)
{
    kernel_init();


    if (load_elf("/shell.elf") != 0) {
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