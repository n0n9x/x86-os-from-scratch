#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/idt.h>

// 页表条目属性
#define PAGE_PRESENT  0x1   // 存在位
#define PAGE_WRITE    0x2   // 读写位 (1=可写)
#define PAGE_USER     0x4   // 用户位 (1=用户态可访问)

// 内核虚拟地址空间的起始偏移量（3GB）
#define KERNEL_VIRT_OFFSET 0xC0000000
// 地址转换宏
#define PHYS_TO_VIRT(addr) ((uintptr_t)(addr) + KERNEL_VIRT_OFFSET)
#define VIRT_TO_PHYS(addr) ((uintptr_t)(addr) - KERNEL_VIRT_OFFSET)

// 索引提取宏
#define PDE_INDEX(addr) ((addr) >> 22)
#define PTE_INDEX(addr) (((addr) >> 12) & 0x3FF)

typedef uint32_t pde_t;
typedef uint32_t pte_t;

void vmm_init();
void map_page(pde_t *pdir, uint32_t virt, uint32_t phys, uint32_t flags);
void page_fault_handler(registers_t *regs);

pde_t* vmm_create_user_directory();

#endif