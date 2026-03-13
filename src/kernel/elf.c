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
    return (load_elf_with_args(path, NULL, 0) != NULL) ? 0 : -1;
}

struct task *load_elf_with_args(const char *path, char **argv, int argc)
{
    fat_dirent_t dirent;
    if (fat_path_to_dirent(path, &dirent, NULL, NULL) != 0)
        return NULL;

    uint8_t *elf_buf = (uint8_t *)kmalloc(dirent.file_size);
    if (fat_read_file(path, elf_buf, dirent.file_size) < 0)
        return NULL;

    elf_header_t *header = (elf_header_t *)elf_buf;
    if (*(uint32_t *)header->e_ident != ELF_MAGIC)
        return NULL;

    // 1. 分配用户页目录（返回物理地址）
    uint32_t new_cr3_phys = (uint32_t)vmm_create_user_directory();
    pde_t *v_pdir = (pde_t *)PHYS_TO_VIRT(new_cr3_phys);

    // 2. 加载 ELF 段
    elf_phdr_t *phdr = (elf_phdr_t *)(elf_buf + header->e_phoff);
    for (int i = 0; i < header->e_phnum; i++)
    {
        if (phdr[i].p_type != 1)
            continue;

        uint32_t start = phdr[i].p_vaddr & 0xFFFFF000;
        uint32_t end   = phdr[i].p_vaddr + phdr[i].p_memsz;

        for (uint32_t vaddr = start; vaddr < end; vaddr += 4096)
        {
            uint32_t paddr = (uint32_t)pmm_alloc_block();
            map_page(v_pdir, vaddr, paddr, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
            memset((void *)PHYS_TO_VIRT(paddr), 0, 4096);
        }

        if (phdr[i].p_filesz > 0)
        {
            uint32_t src_off   = phdr[i].p_offset;
            uint32_t dst_vaddr = phdr[i].p_vaddr;
            uint32_t remaining = phdr[i].p_filesz;

            while (remaining > 0)
            {
                uint32_t pt_phys   = v_pdir[dst_vaddr >> 22] & 0xFFFFF000;
                uint32_t *pt       = (uint32_t *)PHYS_TO_VIRT(pt_phys);
                uint32_t page_phys = pt[(dst_vaddr >> 12) & 0x3FF] & 0xFFFFF000;
                uint32_t page_off  = dst_vaddr & 0xFFF;
                uint32_t chunk     = 4096 - page_off;
                if (chunk > remaining) chunk = remaining;
                memcpy((void *)(PHYS_TO_VIRT(page_phys) + page_off),
                       elf_buf + src_off, chunk);
                dst_vaddr += chunk;
                src_off   += chunk;
                remaining -= chunk;
            }
        }
    }

    // 3. 用户栈：分配两页
    uint32_t ustack_phys1 = (uint32_t)pmm_alloc_block();
    uint32_t ustack_phys2 = (uint32_t)pmm_alloc_block();
    map_page(v_pdir, 0xBFFFE000, ustack_phys1, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    map_page(v_pdir, 0xBFFFF000, ustack_phys2, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    memset((void *)PHYS_TO_VIRT(ustack_phys1), 0, 4096);
    memset((void *)PHYS_TO_VIRT(ustack_phys2), 0, 4096);

    // 4. 在用户栈上布置 argc/argv
    uint8_t *stack_page = (uint8_t *)PHYS_TO_VIRT(ustack_phys1);

    int real_argc = argc;
    if (real_argc <= 0 || argv == NULL)
        real_argc = 1;

    uint32_t str_off = 4096;
    uint32_t ptr_arr[32];
    uint32_t str_base_uva = 0xBFFFE000;

    for (int i = real_argc - 1; i >= 0; i--)
    {
        const char *s = (i == 0 && (argv == NULL || argc <= 0)) ? path : argv[i];
        int len = 0;
        while (s[len]) len++;
        len++;
        str_off -= len;
        memcpy(stack_page + str_off, s, len);
        ptr_arr[i] = str_base_uva + str_off;
    }

    uint32_t *sp = (uint32_t *)stack_page;
    sp[0] = (uint32_t)real_argc;
    for (int i = 0; i < real_argc; i++)
        sp[1 + i] = ptr_arr[i];
    sp[1 + real_argc] = 0;

    // 5. 创建任务，parent 由 sys_exec 设置
    task_t *t = create_user_task_from_elf(header->e_entry, new_cr3_phys, NULL);

    kfree(elf_buf);
    return t;
}