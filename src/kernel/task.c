#include <kernel/task.h>
#include <mm/kheap.h>
#include <drivers/terminal.h>
#include <kernel/gdt.h>
#include <mm/pmm.h>

task_t *current_task = NULL;
task_t *ready_queue = NULL;
task_t *wait_queue = NULL;
task_t *disk_wait_queue = NULL; // 定义磁盘队列

uint32_t next_pid = 1;
uint32_t DEFAULT_TIME_SLICE = 10;

task_t *create_task(void (*entry)())
{
    task_t *t = (task_t *)kmalloc(sizeof(task_t));
    t->id = next_pid++;

    // 关键：克隆内核地址空间，确保进程能看到内核 3GB 以上的代码
    t->cr3 = vmm_create_user_directory();

    uint32_t stack_mem = (uint32_t)kmalloc(4096);
    uint32_t *stack = (uint32_t *)(stack_mem + 4096);

    t->ustack = 0;

    *(--stack) = (uint32_t)kernel_task_exit;

    // --- 1. 硬件中断返回现场 (iret 消耗) ---
    *(--stack) = 0x202;           // EFLAGS (开中断)
    *(--stack) = 0x08;            // CS
    *(--stack) = (uint32_t)entry; // EIP (任务入口)

    // --- 2. 中断 stub 现场 (irq_common_stub 结尾消耗) ---
    *(--stack) = 0;  // Error Code
    *(--stack) = 32; // Interrupt Number

    // pusha 压入 8 个寄存器 (顺序: eax, ecx, edx, ebx, esp, ebp, esi, edi)
    for (int i = 0; i < 8; i++)
    {
        *(--stack) = 0;
    }

    *(--stack) = 0x10; // DS/ES/FS/GS (对应 pop eax)

    extern void irq_exit();
    *(--stack) = (uint32_t)irq_exit;

    // --- 4. switch_to_task 内部 pop 消耗 ---
    // 严格对应 switch.s：pop edi, esi, ebx, ebp
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;

    t->esp = (uint32_t)stack;
    t->state = TASK_READY;
    t->next = NULL;

    // 加入就绪队列
    task_add_to_queue(&ready_queue, t);

    return t;
}

extern void user_task_exit();
extern void first_return_from_kernel();
task_t *create_user_task(void (*entry)())
{
    task_t *t = (task_t *)kmalloc(sizeof(task_t));
    t->id = next_pid++; 

    // 1. 关键：首先创建页目录并获取 cr3
    // 这样接下来的 map_page 才有合法的页目录指针可以使用
    t->cr3 = (pde_t *)vmm_create_user_directory();

    // 2. 准备内核栈 (内核空间分配)
    uint32_t kstack_mem = (uint32_t)kmalloc(4096);
    uint32_t *kstack = (uint32_t *)(kstack_mem + 4096);

    // 3. 准备用户栈 (3GB 以下虚拟空间)
    void *ustack_phys = pmm_alloc_block(); 
    uint32_t ustack_virt_top = 0xBFFFF000; // 用户栈虚拟地址顶端

    // 映射物理页到该进程页表的 0xBFFFF000 处
    // 权限：用户态(PAGE_USER)、可写(PAGE_WRITE)、存在(PAGE_PRESENT)
    map_page((pde_t*)PHYS_TO_VIRT(t->cr3), ustack_virt_top - 4096, (uint32_t)ustack_phys, 0x07);

    // --- [B] 准备用户栈内容 ---
    // 核心：此时不能直接操作 0xBFFFF000，因为当前 cr3 还没切换。
    // 我们必须通过内核映射后的虚拟地址来初始化这块物理内存。
    uint32_t *u_ptr_kernel_view = (uint32_t *)PHYS_TO_VIRT((uint32_t)ustack_phys + 4096);
    *(--u_ptr_kernel_view) = (uint32_t)user_task_exit; 

    // --- [A] 压入 IRET 帧 (5个参数) ---
    // 注意：用户态 ESP 填入的是用户空间的虚拟地址，且已经减去了压入返回地址的 4 字节
    *(--kstack) = 0x23;                          // [4] SS (用户数据段)
    *(--kstack) = ustack_virt_top - sizeof(uint32_t); // [3] ESP
    *(--kstack) = 0x202;                         // [2] EFLAGS (IF=1)
    *(--kstack) = 0x1B;                          // [1] CS (用户代码段)
    *(--kstack) = (uint32_t)entry;               // [0] EIP

    // --- 第二部分：数据段 (对应你的 first_return_from_kernel 弹出) ---
    *(--kstack) = 0x23; // DS (赋给 ds, es, fs, gs)

    // --- 第三部分：任务切换上下文 (匹配 switch_to_task) ---
    extern void first_return_from_kernel();
    *(--kstack) = (uint32_t)first_return_from_kernel;

    // 严格对应汇编中的 pop edi, esi, ebx, ebp
    *(--kstack) = 0; // edi
    *(--kstack) = 0; // esi
    *(--kstack) = 0; // ebx
    *(--kstack) = 0; // ebp

    // --- 4. 完善 PCB 信息 ---
    t->esp = (uint32_t)kstack;
    t->kstack = kstack_mem + 4096;
    t->ustack = ustack_virt_top;  // 记录用户栈虚拟顶端
    
    t->heap_start = 0x400000;     // 用户堆起始地址
    t->heap_end = 0x400000;
    
    t->state = TASK_READY;
    t->next = NULL;

    // 加入就绪队列
    task_add_to_queue(&ready_queue, t);

    return t;
}

task_t *create_user_task_from_elf(uint32_t entry, uint32_t cr3_phys) {
    task_t *t = (task_t *)kmalloc(sizeof(task_t));
    t->id = next_pid++;
    t->cr3 = (pde_t *)(uintptr_t)cr3_phys;

    uint32_t kstack_base = (uint32_t)kmalloc(4096);
    uint32_t *kstack = (uint32_t *)(kstack_base + 4096);

    *(--kstack) = 0x23;
    *(--kstack) = 0xBFFFF000;
    *(--kstack) = 0x202;
    *(--kstack) = 0x1B;
    *(--kstack) = entry;
    *(--kstack) = 0x23;
    *(--kstack) = (uint32_t)first_return_from_kernel;
    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;
    *(--kstack) = 0;

    t->esp = (uint32_t)kstack;
    t->kstack = kstack_base + 4096;
    t->ustack = 0xBFFFF000;
    t->state = TASK_READY;
    t->next = NULL;
    task_add_to_queue(&ready_queue, t);
    return t;
}

void task_tick()
{
    if (!current_task)
        return;

    // 递减当前任务的时间片
    current_task->ticks_left--;

    // 如果时间片用完，触发调度
    if (current_task->ticks_left <= 0)
    {
        current_task->ticks_left = DEFAULT_TIME_SLICE; // 重置时间片
        schedule();
    }
}

void task_init()
{
    // 1. 封装当前 kernel_main 为任务 0
    current_task = (task_t *)kmalloc(sizeof(task_t));
    current_task->id = 0;
    current_task->state = TASK_RUNNING;

    // 关键：初始任务使用当前的内核页目录物理地址
    uint32_t active_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(active_cr3));
    current_task->cr3 = (pde_t *)active_cr3; // 获取 boot.s 真正加载的那个页表

    current_task->ticks_left = DEFAULT_TIME_SLICE;
    current_task->next = NULL;

    // 注意：这里不要让 ready_queue = current_task!
    ready_queue = NULL;
}

void kernel_task_exit()
{
    // 强制关中断，防止在销毁任务时被时钟中断打断导致不一致
    // asm volatile("cli");

    terminal_writestring("\n[Kernel Task] Gracefully Exited.\n");

    current_task->state = TASK_ZOMBIE;

    // 手动调用调度，不再返回
    schedule();

    while (1)
    {
        asm volatile("hlt");
    }
}

// 辅助函数：简化队列添加逻辑
void task_add_to_queue(task_t **queue, task_t *task)
{
    if (!task)
        return;

    task->next = NULL; // 任何入队操作第一步：清空指针

    if (*queue == NULL)
    {
        *queue = task;
    }
    else
    {
        task_t *temp = *queue;
        // 防御：如果要加的任务已经在队头，直接返回
        if (temp == task)
            return;

        while (temp->next)
        {
            // 防御：如果要加的任务已经在队中，直接返回
            if (temp->next == task)
                return;
            temp = temp->next;
        }
        temp->next = task;
    }
}

void task_wakeup(task_t **queue)
{
    if (queue == NULL || *queue == NULL)
        return;

    task_t *task = *queue;
    *queue = NULL; // 立即清空原等待队列头，防止重入或逻辑干扰

    while (task)
    {
        task_t *next_node = task->next; // 1. 先保存好真正的下一个待唤醒任务

        task->next = NULL;        // 2. 核心：强制断开该任务的所有旧连接！
        task->state = TASK_READY; // 3. 修改状态

        task_add_to_queue(&ready_queue, task); // 4. 安全地放入就绪队列

        task = next_node; // 5. 处理下一个
    }
}

void schedule()
{
    if (!ready_queue)
    {
        if (current_task->state == TASK_WAIT)
        {
            // 临时处理：如果没别的跑，强制唤醒自己继续跑，避免系统挂起
            current_task->state = TASK_RUNNING;
        }
        return;
    }

    task_t *prev = current_task; // 当前正在运行的任务
    task_t *nxt = ready_queue;   // 准备运行的任务

    // 1. 从就绪队列头部取出新任务
    ready_queue = nxt->next;
    nxt->next = NULL;

    // 2. 如果当前任务还在运行（即不是因为退出而被切走），将其放回就绪队列末尾
    if (prev->state == TASK_RUNNING)
    {
        prev->state = TASK_READY;
        // 根本性修复：统一使用入队函数，确保 prev->next = NULL 被执行
        task_add_to_queue(&ready_queue, prev);
    }
    else if (prev->state == TASK_ZOMBIE)
    {
        // 这里可以进行简单的资源回收，比如 kfree(prev->kstack)
        // 但最安全的是交给一个专门的 idle 进程去回收，现在可以先不管它
    }
    else if (prev->state == TASK_WAIT)
    {
        // 如果是主动阻塞，加入等待队列
        task_add_to_queue(&wait_queue, prev);
    }

    // 3. 切换当前任务指针
    current_task = nxt;
    current_task->state = TASK_RUNNING;

    // 将待执行进程的内核栈栈顶写入TSS
    extern void set_kernel_stack(uint32_t stack);
    tss_entry.esp0 = nxt->kstack;
    set_kernel_stack(nxt->kstack);

    extern void switch_to_task(uint32_t *old_esp_ptr, uint32_t new_esp, uint32_t new_cr3);
    switch_to_task(&(prev->esp), nxt->esp, (uint32_t)nxt->cr3);
}