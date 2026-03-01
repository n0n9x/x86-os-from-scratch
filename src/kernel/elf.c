#include <kernel/elf.h>
#include <fs/fat.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <kernel/task.h>
#include <lib/string.h>
#include <mm/kheap.h>
#include <drivers/terminal.h>
#include <kernel/gdt.h>

// 辅助：读取和写入 CR3
static inline uint32_t get_cr3()
{
    uint32_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

static inline void set_cr3(uint32_t val)
{
    asm volatile("mov %0, %%cr3" : : "r"(val));
}

int load_elf(const char *path)
{
    fat_dirent_t dirent;
    if (fat_path_to_dirent(path, &dirent, NULL, NULL) != 0)
        return -1;

    uint8_t *elf_buf = (uint8_t *)kmalloc(dirent.file_size);
    if (fat_read_file(path, elf_buf, dirent.file_size) != 0)
        return -1;


    elf_header_t *header = (elf_header_t *)elf_buf;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC)
        return -2;

    // 1. 分配用户页目录
    uint32_t new_cr3_phys = (uint32_t)pmm_alloc_block();
    pde_t *v_pdir = (pde_t *)PHYS_TO_VIRT(new_cr3_phys);
    memset(v_pdir, 0, 4096);

    // 2. 加载ELF段
    elf_phdr_t *phdr = (elf_phdr_t *)(elf_buf + header->e_phoff);
    for (int i = 0; i < header->e_phnum; i++)
    {
        if (phdr[i].p_type != 1)
            continue;

        uint32_t start = phdr[i].p_vaddr & 0xFFFFF000;
        uint32_t end = phdr[i].p_vaddr + phdr[i].p_memsz;

        for (uint32_t vaddr = start; vaddr < end; vaddr += 4096)
        {
            uint32_t paddr = (uint32_t)pmm_alloc_block();
            map_page(v_pdir, vaddr, paddr, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            memset((void *)PHYS_TO_VIRT(paddr), 0, 4096);
        }

        if (phdr[i].p_filesz > 0)
        {
            uint32_t src_off = phdr[i].p_offset;
            uint32_t dst_vaddr = phdr[i].p_vaddr;
            uint32_t remaining = phdr[i].p_filesz;

            while (remaining > 0)
            {
                uint32_t pt_phys = v_pdir[dst_vaddr >> 22] & 0xFFFFF000;
                uint32_t *pt = (uint32_t *)PHYS_TO_VIRT(pt_phys);
                uint32_t page_phys = pt[(dst_vaddr >> 12) & 0x3FF] & 0xFFFFF000;
                uint32_t page_off = dst_vaddr & 0xFFF;
                uint32_t chunk = 4096 - page_off;

                if (chunk > remaining)
                    chunk = remaining;
                memcpy((void *)(PHYS_TO_VIRT(page_phys) + page_off),
                       elf_buf + src_off, chunk);

                dst_vaddr += chunk;
                src_off += chunk;
                remaining -= chunk;
            }
        }
    }
    // 3. 用户栈
    uint32_t ustack_phys1 = (uint32_t)pmm_alloc_block();
    uint32_t ustack_phys2 = (uint32_t)pmm_alloc_block();
    map_page(v_pdir, 0xBFFFE000, ustack_phys1, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    map_page(v_pdir, 0xBFFFF000, ustack_phys2, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    memset((void *)PHYS_TO_VIRT(ustack_phys1), 0, 4096);
    memset((void *)PHYS_TO_VIRT(ustack_phys2), 0, 4096);


    // 4. 同步内核PDE
    extern pde_t kernel_page_directory[];
    for (int i = 768; i < 1024; i++)
    {
        if (kernel_page_directory[i] & PAGE_PRESENT)
        {
            v_pdir[i] = kernel_page_directory[i];
        }
    }

    // 5. 创建任务
    create_user_task_from_elf(header->e_entry, new_cr3_phys);

    // 6. 创建任务后再同步一次（kmalloc可能触发新映射）
    for (int i = 768; i < 1024; i++)
    {
        if (kernel_page_directory[i] & PAGE_PRESENT)
        {
            v_pdir[i] = kernel_page_directory[i];
        }
    }

    return 0;
}