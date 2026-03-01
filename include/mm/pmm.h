#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096  // 标准 4KB 页

// 假设我们管理 128MB 物理内存（可以根据实际 RAM 大小调整）
#define MAX_MEM_SIZE 0x8000000 
#define BITMAP_SIZE  (MAX_MEM_SIZE / PAGE_SIZE / 8)

void pmm_init(uint32_t mem_size);
void* pmm_alloc_block();
void pmm_free_block(void* addr);
void pmm_init_region(uint32_t base, uint32_t size);

#endif