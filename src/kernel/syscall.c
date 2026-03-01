#include <kernel/syscall.h>
#include <drivers/terminal.h>
#include <kernel/task.h>
#include <mm/kheap.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <drivers/ide.h>
#include <fs/fat.h>
#include <drivers/io.h>

void sys_print(char *s) {
    terminal_writestring(s);
}

void sys_exit(int exit_code)
{
    (void)exit_code;
    // 1. 简单打印一下信息用于调试
    terminal_writestring("\n[Task Exited]\n");

    // 2. 标记当前任务为“死亡”状态
    current_task->state = TASK_ZOMBIE;

    schedule();
    while (1)
        ;
}

void *sys_sbrk(int increment)
{
    task_t *task = current_task; //
    uint32_t old_brk = task->heap_end;
    uint32_t new_brk = old_brk + increment;

    // 1. 安全检查：堆空间不允许跨越到内核空间 (3GB 边界)
    if (new_brk >= KERNEL_VIRT_OFFSET)
    {
        return (void *)-1;
    }

    // 2. 如果是申请内存 (increment > 0)
    if (increment > 0)
    {
        // 计算需要映射的页范围
        // 确保从 old_brk 所在的页开始，映射到 new_brk 覆盖的所有页
        uint32_t start_page = old_brk & 0xFFFFF000;
        uint32_t end_page = (new_brk + 4095) & 0xFFFFF000;

        // 【核心修改】：在高半核模式下，cr3 是物理地址，必须转为虚拟地址才能由内核读写
        // 使用你 vmm.h 中定义的 PHYS_TO_VIRT 宏
        pde_t *v_pdir = (pde_t *)PHYS_TO_VIRT(task->cr3);

        for (uint32_t page = start_page; page < end_page; page += 4096)
        {
            // 只有当该虚拟页尚未被映射时，才分配物理页
            // (此处建议实现一个 get_page 检查，防止重复分配导致内存泄漏)

            void *phys = pmm_alloc_block(); // 分配物理内存页
            if (!phys)
            {
                // 如果物理内存耗尽，返回失败
                return (void *)-1;
            }

            // 执行映射
            // 必须传入 PAGE_USER，否则用户态进程在 Ring 3 访问会触发页错误
            // map_page 内部应确保 PDE 和 PTE 都开启了 USER 位
            map_page(v_pdir, page, (uint32_t)phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        }
    }

    // 3. 更新任务结构体中的堆结束地址
    task->heap_end = new_brk;

    // 返回分配前的旧堆顶指针给用户
    return (void *)old_brk;
}

static int sys_disk_read(uint32_t lba, uint8_t *user_buf)
{
    if (!user_buf)
        return -1;
    ide_read_sector(lba, user_buf);
    return 0;
}

static int sys_disk_write(uint32_t lba, uint8_t *user_buf)
{
    if (!user_buf)
        return -1;
    ide_write_sector(lba, user_buf);
    return 0;
}

static int sys_fcreate(const char *filename)
{
    if (!filename)
        return -1;
    return fat_create_file(filename);
}

static int sys_open(const char *filename)
{
    fat_dirent_t dirent;
    if (fat_path_to_dirent(filename, &dirent, NULL, NULL) == 0)
    {
        // 简化版：这里本该返回 FD，目前我们直接返回起始簇号作为简易“句柄”
        return dirent.first_cluster_low;
    }
    return -1;
}

static int sys_fread(const char *filename, uint8_t *buf, uint32_t len)
{
    return fat_read_file(filename, buf, len);
}

static int sys_fwrite(const char *filename, uint8_t *buf, uint32_t len)
{
    return fat_write_file(filename, buf, len);
}

static int sys_fdelete(const char *filename)
{
    return fat_delete_file(filename);
}

static int sys_fappend(const char *filename, uint8_t *buf, uint32_t len)
{
    return fat_append_file(filename, buf, len);
}

int sys_read(int fd, char *buf, uint32_t count)
{
    extern char kbd_get_char();
    extern int kbd_is_empty();
    if (fd != 0)
        return -1; // 仅支持标准输入 (stdin)
    // 1. 核心阻塞逻辑：只要缓冲区为空，就睡一会儿
    while (kbd_is_empty())
    {
        current_task->state = TASK_WAIT; // 标记任务为阻塞状态
        schedule();                      // 发起调度，由于状态不是 RUNNING，它会被放入 wait_queue

        // 当任务被唤醒后，会从这里继续执行，重新检查 kbd_is_empty()
    }

    // 2. 走到这里说明一定有字符可以读取了
    uint32_t i = 0;
    while (i < count)
    {
        char c = kbd_get_char();
        if (c == 0)
            break;

        buf[i++] = c;
        if (c == '\n')
            break;
    }
    return i;
}

static void sys_flist(const char *path) {
    fat_list_files(path);
}

static int sys_mkdir(const char *path)
{
    if (!path) return -1;
    return fat_mkdir(path); // 调用 fat.c 中的实现
}

void sys_shutdown() {
    outw(0x604, 0x2000); 
}

static void *syscalls[] = {
    [SYS_PRINT] = (void *)sys_print,
    [SYS_SBRK] = (void *)sys_sbrk,
    [SYS_EXIT] = (void *)sys_exit,
    [SYS_READ] = (void *)sys_read,
    [SYS_DISK_READ] = (void *)sys_disk_read,
    [SYS_DISK_WRITE] = (void *)sys_disk_write,
    [SYS_OPEN] = (void *)sys_open,
    [SYS_FREAD] = (void *)sys_fread,
    [SYS_FWRITE] = (void *)sys_fwrite,
    [SYS_FCREATE] = (void *)sys_fcreate,
    [SYS_FDELETE] = (void *)sys_fdelete,
    [SYS_FAPPEND] = (void *)sys_fappend,
    [SYS_FLIST] = (void *)sys_flist,
    [SYS_MKDIR]   = (void *)sys_mkdir,
    [SYS_SHUTDOWN]=(void* )sys_shutdown,
};

// 3. 分发器
#define SYSCALL_COUNT (sizeof(syscalls) / sizeof(void *))
void syscall_handler(registers_t *regs)
{
    // 调用号存在 eax 中
    uint32_t num = regs->eax;

    if (num >= SYSCALL_COUNT)
        return;

    void *handler = syscalls[num];

    if (num == SYS_DISK_READ || num == SYS_DISK_WRITE)
    {
        int (*func)(uint32_t, uint8_t *) = (int (*)(uint32_t, uint8_t *))handler;
        // ebx 传 LBA，ecx 传缓冲区地址
        regs->eax = func(regs->ebx, (uint8_t *)regs->ecx);
    }
    else if (num == SYS_PRINT)
    {
        void (*func)(char *) = (void (*)(char *))handler;
        func((char *)regs->ebx);
    }
    else if (regs->eax == SYS_SBRK)
    {                                              // 定义 SYS_SBRK 为某个数字
        regs->eax = (uint32_t)sys_sbrk(regs->ebx); // ebx 作为参数
    }
    else if (num == SYS_EXIT)
    {
        void (*func)(int) = (void (*)(int))handler;
        func((int)regs->ebx); // ebx 存储的是退出状态码
    }
    else if (num == SYS_READ)
    {
        int (*func)(int, char *, uint32_t) = (int (*)(int, char *, uint32_t))handler;
        regs->eax = func(regs->ebx, (char *)regs->ecx, regs->edx);
    }
    else if (num == SYS_OPEN)
    {
        int (*func)(const char *) = (int (*)(const char *))handler;
        regs->eax = func((const char *)regs->ebx); // ebx 传文件名指针
    }
    else if (num == SYS_FREAD)
    {
        int (*func)(const char *, uint8_t *, uint32_t) = (int (*)(const char *, uint8_t *, uint32_t))handler;
        // ebx 传文件名，ecx 传缓冲区地址
        regs->eax = func((const char *)regs->ebx, (uint8_t *)regs->ecx, regs->edx);
    }
    else if (num == SYS_FWRITE)
    {
        int (*func)(const char *, uint8_t *, uint32_t) = (int (*)(const char *, uint8_t *, uint32_t))handler;
        // ebx=文件名, ecx=数据, edx=长度
        regs->eax = func((char *)regs->ebx, (uint8_t *)regs->ecx, regs->edx);
    }
    else if (num == SYS_FCREATE)
    {
        int (*func)(const char *) = (int (*)(const char *))handler;
        regs->eax = func((const char *)regs->ebx);
    }
    else if (num == SYS_FDELETE)
    {
        int (*func)(const char *) = (int (*)(const char *))handler;
        regs->eax = func((const char *)regs->ebx);
    }
    else if (num == SYS_FAPPEND)
    {
        int (*func)(const char *, uint8_t *, uint32_t) = (int (*)(const char *, uint8_t *, uint32_t))handler;
        regs->eax = func((const char *)regs->ebx, (uint8_t *)regs->ecx, regs->edx);
    }
    else if (num == SYS_FLIST)
    {
        void (*func)(const char *) = (void (*)(const char *))handler;
        func((const char *)regs->ebx);
    }else if (num == SYS_MKDIR)
    {
        int (*func)(const char *) = (int (*)(const char *))handler;
        // ebx 存储的是用户传进来的路径字符串指针
        regs->eax = func((const char *)regs->ebx);
    }else if (num == SYS_SHUTDOWN)
    {
        sys_shutdown();
    }
}

