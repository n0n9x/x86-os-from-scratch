#ifndef IDE_H
#define IDE_H

#include <stdint.h>

// IDE Primary 总线 IO 端口基地址
#define IDE_PRIMARY_BASE 0x1F0

// 寄存器偏移
#define IDE_REG_DATA       0x00    // 数据寄存器 (读写数据)
#define IDE_REG_ERR        0x01    // 错误寄存器
#define IDE_REG_SEC_COUNT  0x02    // 扇区计数寄存器
#define IDE_REG_LBA_LOW    0x03    // LBA 地址 0-7 位
#define IDE_REG_LBA_MID    0x04    // LBA 地址 8-15 位
#define IDE_REG_LBA_HIGH   0x05    // LBA 地址 16-23 位
#define IDE_REG_DEVICE     0x06    // 设备/LBA 高 4 位选择
#define IDE_REG_STATUS     0x07    // 状态寄存器 (读) / 命令寄存器 (写)

// IDE 命令
#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_IDENTIFY 0xEC

// 状态位
#define IDE_STATUS_BSY  0x80    // 忙
#define IDE_STATUS_DRQ  0x08    // 数据请求 (准备好传输数据)
#define IDE_STATUS_DF   0x20    // 驱动错误
#define IDE_STATUS_ERR  0x01    // 错误发生

void ide_init();
void ide_read_sector(uint32_t lba, uint8_t *buf);
void ide_write_sector(uint32_t lba, uint8_t *buf);
void ide_handler(); // 中断处理函数
void ide_identify();

#endif