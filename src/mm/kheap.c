#include <mm/kheap.h>
#include <mm/vmm.h>
#include <mm/pmm.h>

static header_t *heap_first = (header_t *)KHEAP_START;
static bool heap_initialized = false;

void kheap_init()
{
    // 这一步会触发 Page Fault
    // 只有当 map_page 成功返回后，下面的赋值才会真正完成
    heap_first->size = 0;
    heap_first->is_free = true;
    heap_first->next = NULL;
    heap_initialized = true;
    heap_first->prev = NULL;
}

void *kmalloc(size_t size)
{
    if (!heap_initialized)
        kheap_init();
    size = (size + 7) & ~7; // 8字节对齐
    header_t *curr = heap_first;

    //首次适应法
    while (curr)
    {
        //寻找合适的块（空闲、空间足够大）
        if (curr->is_free && curr->size >= size && curr->size > 0)
        {
            // 如果剩余空间足够大，切分出一个新块
            if (curr->size >= size + sizeof(header_t) + 8)
            {
                header_t *split_node = (header_t *)((uint32_t)curr + sizeof(header_t) + size);
                split_node->size = curr->size - size - sizeof(header_t);
                split_node->is_free = true;
                split_node->next = curr->next;
                split_node->prev = curr; // 设置反向指针

                if (curr->next)
                    curr->next->prev = split_node;
                curr->next = split_node;
                curr->size = size;
            }
            curr->is_free = false;
            return (void *)(curr + 1); // curr是管理头的起始地址，curr+1是紧跟在管理头后面的地址
        }
        if (!curr->next)
            break;
        curr = curr->next;
    }

    // 2. 没有合适的块，进行堆末尾扩展 (注意维护 prev)
    header_t *new_node;
    if (curr == heap_first && curr->size == 0)
    {
        new_node = curr;
    }
    else
    {
        new_node = (header_t *)((uint32_t)curr + sizeof(header_t) + curr->size);
        curr->next = new_node;
        new_node->prev = curr;
    }

    new_node->size = size;
    new_node->is_free = false;
    new_node->next = NULL;
    return (void *)(new_node + 1);
}

void kfree(void *ptr)
{
    if (!ptr)
        return;
    header_t *node = (header_t *)ptr - 1;
    node->is_free = true;

    // 1. 向后看：如果后面是空的，合并它
    if (node->next && node->next->is_free)
    {
        node->size += sizeof(header_t) + node->next->size;
        node->next = node->next->next;
        if (node->next)
            node->next->prev = node;
    }

    // 2. 向前看：如果前面是空的，合并到前面去 (彻底解决测试一 FAIL)
    if (node->prev && node->prev->is_free)
    {
        header_t *p = node->prev;
        p->size += sizeof(header_t) + node->size;
        p->next = node->next;
        if (node->next)
            node->next->prev = p;
    }
}
