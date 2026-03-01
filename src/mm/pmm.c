#include <mm/pmm.h>
#include <lib/string.h>


static uint8_t bitmap[BITMAP_SIZE];
static uint32_t total_blocks;

// 初始化位图，将所有内存设为已用，后续根据内存图释放可用部分
void pmm_init(uint32_t mem_size) {
    total_blocks = mem_size / PAGE_SIZE;
    memset(bitmap, 0xFF, BITMAP_SIZE); // 默认全占用（安全起见）
}

// 标记[base,base+size]的块为可用
void pmm_init_region(uint32_t base, uint32_t size) {
    uint32_t align = base / PAGE_SIZE;
    uint32_t blocks = size / PAGE_SIZE;
    for (; blocks > 0; blocks--) {
        uint32_t bit = align % 8;
        uint32_t byte = align / 8;
        bitmap[byte] &= ~(1 << bit); // 设为 0 表示可用
        align++;
    }
}

// 分配一个物理页框
void* pmm_alloc_block() {
    for (uint32_t i = 32; i < BITMAP_SIZE; i++) { 
        if (bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!(bitmap[i] & (1 << j))) {
                    bitmap[i] |= (1 << j);
                    return (void*)((i * 8 + j) * PAGE_SIZE);
                }
            }
        }
    }
    return NULL; // 内存耗尽
}

// 释放物理页框
void pmm_free_block(void* addr) {
    uint32_t block = (uint32_t)addr / PAGE_SIZE;
    bitmap[block / 8] &= ~(1 << (block % 8));
}