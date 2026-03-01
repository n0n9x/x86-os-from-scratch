#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <mm/vmm.h>

// 进程状态
typedef enum
{
    TASK_RUNNING, // 运行
    TASK_READY,   // 就绪
    TASK_ZOMBIE,  // 死亡
    TASK_WAIT,    // 阻塞
} task_state_t;

// 进程切换的上下文
typedef struct context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip; // 这里的 eip 实际上是 switch_to_task 的返回地址 (ret)
} context_t;

// PCB
typedef struct task
{
    uint32_t esp; // 切换时保存的栈指针
    uint32_t id;
    uint32_t kstack; // 内核栈起始地址
    uint32_t ustack; // 用户栈起始地址
    pde_t *cr3;      // 进程页目录的物理地址 (CR3 内容)
    task_state_t state;
    uint32_t heap_start; // 堆的起始虚拟地址
    uint32_t heap_end;   // 当前堆的结束虚拟地址 (brk 指针)
    int32_t ticks_left;  // 当前任务剩余的时间片
    struct task *next;
} task_t;

extern task_t *current_task;
extern task_t *ready_queue;
extern task_t *wait_queue;      // 键盘使用的通用阻塞队列
extern task_t *disk_wait_queue; // 【新增】磁盘专用的阻塞队列

extern uint32_t DEFAULT_TIME_SLICE;

void task_init();
void schedule();
task_t *create_task(void (*entry)(void));  // 内核态进程
task_t *create_user_task(void (*entry)()); // 用户态进程
task_t *create_user_task_from_elf(uint32_t entry, uint32_t cr3_phys);
void kernel_task_exit();
void task_tick();
void task_add_to_queue(task_t **queue, task_t *task);
void task_wakeup(task_t **queue);

void set_kernel_stack(uint32_t stack);

#endif