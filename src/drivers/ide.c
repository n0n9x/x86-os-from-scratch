#include <drivers/ide.h>
#include <drivers/io.h>
#include <kernel/task.h>
#include <drivers/terminal.h>

// 用于存储硬盘信息的结构
static struct {
    uint32_t sectors;   // 总扇区数，一个扇区512B
    char model[41];     // 型号字符串
} ide_info;

// 提供一个公共函数获取总扇区数
uint32_t ide_get_sectors() {
    return ide_info.sectors;
}

// 检查磁盘是否忙碌
static void ide_wait_ready() {
    while (inb(IDE_PRIMARY_BASE + IDE_REG_STATUS) & IDE_STATUS_BSY);
}

void ide_init() {
    // 以后可以在这里探测硬盘是否存在
}

// 读取一个扇区的框架
void ide_read_sector(uint32_t lba, uint8_t *buf) {
    ide_wait_ready();

    // 1. 设置扇区数量和 LBA 地址
    outb(IDE_PRIMARY_BASE + IDE_REG_SEC_COUNT, 1);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_LOW,  (uint8_t)lba);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_HIGH, (uint8_t)(lba >> 16));
    
    // 模式选择: LBA模式 + Master盘 + LBA最高4位
    outb(IDE_PRIMARY_BASE + IDE_REG_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));

    // 2. 发送读命令
    outb(IDE_PRIMARY_BASE + IDE_REG_STATUS, IDE_CMD_READ);

    //3.阻塞当前进程
    current_task->state = TASK_WAIT; 
    task_add_to_queue(&disk_wait_queue, current_task);
    schedule(); 
    while (!(inb(IDE_PRIMARY_BASE + IDE_REG_STATUS) & IDE_STATUS_DRQ));
    // 4. 被唤醒后，从数据端口读取 512 字节 (256个16位字)
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(IDE_PRIMARY_BASE + IDE_REG_DATA);
        ((uint16_t*)buf)[i] = data;
    }
}

// 写入一个扇区 (LBA模式)
void ide_write_sector(uint32_t lba, uint8_t *buf) {
    ide_wait_ready();

    outb(IDE_PRIMARY_BASE + IDE_REG_SEC_COUNT, 1);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_LOW,  (uint8_t)lba);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(IDE_PRIMARY_BASE + IDE_REG_DEVICE,   0xE0 | ((lba >> 24) & 0x0F));
    
    // 发送命令
    outb(IDE_PRIMARY_BASE + IDE_REG_STATUS, IDE_CMD_WRITE);

    // 必须等待硬件准备好接收数据
    while (!(inb(IDE_PRIMARY_BASE + IDE_REG_STATUS) & IDE_STATUS_DRQ));

    // 发送数据
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < 256; i++) {
        outw(IDE_PRIMARY_BASE + IDE_REG_DATA, ptr[i]);
    }

    // 等待写操作落盘完成
    ide_wait_ready();

    // 如果你有进程调度，在这里等待中断
    current_task->state = TASK_WAIT;
    task_add_to_queue(&disk_wait_queue, current_task);
    schedule();
}

// 硬盘中断处理程序 (IRQ 14)
void ide_handler() {
    // 1. 读取状态寄存器以应答中断（否则硬盘不发下一次中断）
    inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);

    // 2. 唤醒所有等待磁盘的进程
    // 这里你应该实现一个更精准的 task_wakeup(&disk_wait_queue)
    task_wakeup(&disk_wait_queue);
}

void ide_identify() {
    // 1. 选择主盘 (Master)
    outb(IDE_PRIMARY_BASE + IDE_REG_DEVICE, 0xA0);
    
    
    // 2. 发送命令前先清空参数寄存器（良好的习惯）
    outb(IDE_PRIMARY_BASE + IDE_REG_SEC_COUNT, 0);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_LOW, 0);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_MID, 0);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_HIGH, 0);

    // 3. 发送 IDENTIFY 命令
    outb(IDE_PRIMARY_BASE + IDE_REG_STATUS, IDE_CMD_IDENTIFY);

    // 4. 检查驱动器是否存在
    uint8_t status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);
    if (status == 0) {
        terminal_writestring("No IDE drive found on Primary Master");
        return;
    }

    // 5. 等待 BSY 清除并检查是否有错误发生
    while (inb(IDE_PRIMARY_BASE + IDE_REG_STATUS) & IDE_STATUS_BSY);
    
    // 再次检查状态，确保没有 ERR 位
    status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);
    if (status & IDE_STATUS_ERR) {
        //terminal_writestring("IDE Identify Error.\n");
        return;
    }

    // 等待数据请求就绪 (DRQ)
    while (!(inb(IDE_PRIMARY_BASE + IDE_REG_STATUS) & IDE_STATUS_DRQ));

    // 6. 读取 256 个字 (512 字节)
    uint16_t data[256] = {0};
    for (int i = 0; i < 256; i++) {
        data[i] = inw(IDE_PRIMARY_BASE + IDE_REG_DATA);
    }

    // 7. 解析 LBA28 总扇区数
    ide_info.sectors = (uint32_t)data[60] | ((uint32_t)data[61] << 16);

    // 8. 解析型号并处理字节序 (可选)
    for (int i = 0; i < 20; i++) {
        ide_info.model[i * 2] = (char)(data[27 + i] >> 8);   // 高字节
        ide_info.model[i * 2 + 1] = (char)(data[27 + i]);    // 低字节
    }
    ide_info.model[40] = '\0';
}

