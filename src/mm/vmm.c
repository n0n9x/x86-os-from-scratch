#include <mm/vmm.h>
#include <drivers/terminal.h>
#include <mm/pmm.h>
#include <mm/kheap.h>
#include <kernel/idt.h>
#include <lib/string.h>
#include <kernel/gdt.h>

// 全局内核页目录（虚拟地址）
pde_t kernel_page_directory[1024] __attribute__((aligned(4096)));
// 第一个页表（用于映射内核前4MB）
pte_t kernel_page_tables[32][1024] __attribute__((aligned(4096)));

void vmm_init()
{
    for (int i = 0; i < 1024; i++)
        kernel_page_directory[i] = 0;

    // 映射128MB物理内存到 0xC0000000 以上
    for (int t = 0; t < 32; t++)
    {
        for (int i = 0; i < 1024; i++)
        {
            kernel_page_tables[t][i] = (t * 1024 * PAGE_SIZE + i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        }
        uint32_t pt_phys = VIRT_TO_PHYS((uint32_t)kernel_page_tables[t]);
        kernel_page_directory[768 + t] = pt_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    // 显存映射保持不变
    map_page(kernel_page_directory, VIDEO_VIRT_ADDR, 0xB8000, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    // 堆的第一页（现在已经在上面的循环里映射过了，但map_page会覆盖，没问题）
    uint32_t heap_phys = (uint32_t)pmm_alloc_block();
    map_page(kernel_page_directory, KHEAP_START, heap_phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    // 开启分页
    uint32_t pd_phys = VIRT_TO_PHYS((uint32_t)kernel_page_directory);
    asm volatile("mov %0, %%cr3" ::"r"(pd_phys));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" ::"r"(cr0));

    // 不需要清零 PDE[0] 了，因为我们没有做身份映射
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax");

    terminal_initialize();
}

// 将虚拟地址 virt 映射到物理地址 phys
void map_page(pde_t *pdir, uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(pdir[pd_idx] & PAGE_PRESENT))
    {
        uint32_t pt_phys = (uint32_t)pmm_alloc_block();

        uint32_t pdir_phys = VIRT_TO_PHYS((uint32_t)pdir);
        if (pt_phys == pdir_phys)
        {
            terminal_writestring("CONFLICT!\n");
            for (;;)
                ;
        }

        pdir[pd_idx] = (pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        memset((void *)PHYS_TO_VIRT(pt_phys), 0, 4096);
    }
    else
    {
        if (flags & PAGE_USER)
            pdir[pd_idx] |= PAGE_USER;
    }

    uint32_t pt_phys = pdir[pd_idx] & 0xFFFFF000;
    uint32_t *page_table = (uint32_t *)PHYS_TO_VIRT(pt_phys);
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags | PAGE_PRESENT;
    asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

void page_fault_handler(registers_t *regs) {
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
    
    terminal_writestring("PF addr=");
    terminal_write_hex(fault_addr);
    terminal_writestring(" err=");
    terminal_write_hex(regs->err_code);
    terminal_writestring("\n");
    for(;;);
}

// 在 vmm.c 中添加此函数
pde_t *vmm_create_user_directory()
{
    uint32_t phys = (uint32_t)pmm_alloc_block();
    pde_t *v_pdir = (pde_t *)PHYS_TO_VIRT(phys);
    memset(v_pdir, 0, PAGE_SIZE);

    // 重点：直接使用你全局定义的 kernel_page_directory
    // 它是所有进程内核空间的“母本”
    extern pde_t kernel_page_directory[];

    for (int i = 768; i < 1024; i++)
    {
        if (kernel_page_directory[i] & 0x1)
        {
            // 确保 PDE 允许用户访问
            v_pdir[i] = kernel_page_directory[i] | PAGE_USER;
        }
    }

    // 1. 找到 GDT 虚拟地址对应的 PDE 和 PTE 索引
    uint32_t gdt_v = (uint32_t)&gdt_entries;
    uint32_t pde_idx = gdt_v >> 22;
    uint32_t pte_idx = (gdt_v >> 12) & 0x3FF;

    // 2. 强制获取当前正在使用的二级页表（从当前 CR3 走表）
    uint32_t cur_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3));
    uint32_t *cur_pdir = (uint32_t *)PHYS_TO_VIRT(cur_cr3);
    uint32_t cur_pte_phys_base = cur_pdir[pde_idx] & 0xFFFFF000;

    // 3. 把这个“活的”二级页表物理页，塞进你新创建的页目录里
    v_pdir[pde_idx] = cur_pte_phys_base | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    // 4. 确保这个二级页表里的那一项 PTE 也是 User 权限
    uint32_t *active_pt = (uint32_t *)PHYS_TO_VIRT(cur_pte_phys_base);
    active_pt[pte_idx] |= PAGE_USER;

    return (pde_t *)phys; // 注意：返回物理地址用于加载 CR3
}
