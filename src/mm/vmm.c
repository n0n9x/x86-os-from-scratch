#include <mm/vmm.h>
#include <drivers/terminal.h>
#include <mm/pmm.h>
#include <mm/kheap.h>
#include <kernel/idt.h>
#include <lib/string.h>
#include <kernel/gdt.h>

// 全局内核页目录 1024个页目录项
pde_t kernel_page_directory[1024] __attribute__((aligned(4096)));
// 128MB，需要32个页表，每个页表1024个页表项
pte_t kernel_page_tables[32][1024] __attribute__((aligned(4096)));

void vmm_init()
{
    for (int i = 0; i < 1024; i++)
        kernel_page_directory[i] = 0;

    // 映射128MB物理内存到 0xC0000000 以上，高半核设计
    for (int t = 0; t < 32; t++)
    {
        for (int i = 0; i < 1024; i++)
        {
            // 页表赋值，物理地址+属性位
            kernel_page_tables[t][i] = (t * 1024 * PAGE_SIZE + i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        }
        // 页目录赋值，物理地址+属性位
        uint32_t pt_phys = VIRT_TO_PHYS((uint32_t)kernel_page_tables[t]); // 页表的物理地址
        kernel_page_directory[768 + t] = pt_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    // 显存映射保持不变
    map_page(kernel_page_directory, VIDEO_VIRT_ADDR, 0xB8000, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    // 堆的第一页
    uint32_t heap_phys = (uint32_t)pmm_alloc_block(); // 物理地址
    map_page(kernel_page_directory, KHEAP_START, heap_phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);

    // 将页目录表地址写入CR3
    uint32_t pd_phys = VIRT_TO_PHYS((uint32_t)kernel_page_directory); // 页目录表的物理地址
    asm volatile("mov %0, %%cr3" ::"r"(pd_phys));                     // 将页目录表物理地址存入CR3
    // 再次分页确保安全
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0)); // 读取CR0寄存器
    cr0 |= 0x80000000;                         // 开启分页
    asm volatile("mov %0, %%cr0" ::"r"(cr0));  // 写回CR0寄存器

    // 通过重写CR3刷新TLB
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax");

    terminal_initialize();
}

// 将虚拟地址 virt 映射到物理地址 phys
void map_page(pde_t *pdir, uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pd_idx = virt >> 22;           // 高十位 页目录项    查表找页表项
    uint32_t pt_idx = (virt >> 12) & 0x3FF; // 中十位 页表项      查表找页框号

    // 查页目录表，若发现无对应的页表项，则创建页表
    if (!(pdir[pd_idx] & PAGE_PRESENT))
    {
        uint32_t pt_phys = (uint32_t)pmm_alloc_block(); // 获取页表物理地址

        uint32_t pdir_phys = VIRT_TO_PHYS((uint32_t)pdir); // 获取页目录表物理地址
        // 若页表和页目录表的物理地址相同，报错处理
        if (pt_phys == pdir_phys)
        {
            terminal_writestring("CONFLICT!\n");
            for (;;)
                ;
        }
        // 赋值，页表物理地址+属性位
        pdir[pd_idx] = (pt_phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        // 将页表清空（虚拟地址）
        memset((void *)PHYS_TO_VIRT(pt_phys), 0, 4096);
    }
    else // 页表存在，但是需要映射用户页面
    {
        if (flags & PAGE_USER)
            pdir[pd_idx] |= PAGE_USER;
    }

    uint32_t pt_phys = pdir[pd_idx] & 0xFFFFF000;                    // 获取页表物理地址
    uint32_t *page_table = (uint32_t *)PHYS_TO_VIRT(pt_phys);        // 获取页表虚拟地址
    page_table[pt_idx] = (phys & 0xFFFFF000) | flags | PAGE_PRESENT; // 建立映射
    asm volatile("invlpg (%0)" ::"r"(virt) : "memory");              // 刷新TLB
}

void page_fault_handler(registers_t *regs)
{
    uint32_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

    uint8_t present  = regs->err_code & 0x1; // 0=页不存在，1=权限违规
    //uint8_t write    = regs->err_code & 0x2; // 是否是写操作触发
    uint8_t user     = regs->err_code & 0x4; // 0=内核态，1=用户态

    // 1. 内核态缺页：致命错误，只能挂起
    if (!user) {
        terminal_writestring("Kernel Page Fault! addr=");
        terminal_write_hex(fault_addr);
        for (;;);
    }

    // 2. 用户态权限违规（页存在但无权访问）：杀死进程
    if (present) {
        terminal_writestring("[PF] Access violation, killing task.\n");
        current_task->state = TASK_ZOMBIE;
        schedule();
        return;
    }

    // 3. 用户态栈自动增长（访问地址在栈顶以下一定范围内）
    if (fault_addr >= 0xBFF00000 && fault_addr < 0xC0000000) {
        uint32_t page = fault_addr & 0xFFFFF000;
        void *phys = pmm_alloc_block();
        if (phys) {
            pde_t *v_pdir = (pde_t *)PHYS_TO_VIRT((uint32_t)current_task->cr3);
            map_page(v_pdir, page, (uint32_t)phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            return; // 恢复执行
        }
    }

    // 4. 其他用户态缺页：杀死进程
    terminal_writestring("[PF] Segfault, killing task.\n");
    current_task->state = TASK_ZOMBIE;
    schedule();
}
// 为用户进程创建独立的页目录
pde_t *vmm_create_user_directory()
{
    uint32_t phys = (uint32_t)pmm_alloc_block();
    pde_t *v_pdir = (pde_t *)PHYS_TO_VIRT(phys);
    memset(v_pdir, 0, PAGE_SIZE);

    // 重点：直接使用全局定义的 kernel_page_directory
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
    uint32_t gdt_v = (uint32_t)&gdt_entries;  // 获取gdt虚拟地址
    uint32_t pde_idx = gdt_v >> 22;           // gdt虚拟地址的页目录号
    uint32_t pte_idx = (gdt_v >> 12) & 0x3FF; // gdt虚拟地址的页表号

    // 2. 强制获取当前正在使用的二级页表（从当前 CR3 走表）
    uint32_t cur_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3));//读取CR3：当前进程页目录的物理地址
    uint32_t *cur_pdir = (uint32_t *)PHYS_TO_VIRT(cur_cr3);//转为虚拟地址，即当前进程页目录的虚拟地址
    uint32_t cur_pte_phys_base = cur_pdir[pde_idx] & 0xFFFFF000;//存放gdt的页表的物理地址

    // 3. 将存放gdt页面的页表物理地址写入新创建的用户进程的页目录
    v_pdir[pde_idx] = cur_pte_phys_base | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    // 4. 确保这个二级页表里的那一项 PTE 也是 User 权限
    uint32_t *active_pt = (uint32_t *)PHYS_TO_VIRT(cur_pte_phys_base);
    active_pt[pte_idx] |= PAGE_USER;

    return (pde_t *)phys; // 注意：返回物理地址用于加载 CR3
}
