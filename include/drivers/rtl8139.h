#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

/* ── PCI 搜索 ─────────────────────────────────────────────────── */
#define PCI_CONFIG_ADDR   0xCF8
#define PCI_CONFIG_DATA   0xCFC

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

/* ── RTL8139 寄存器偏移 ────────────────────────────────────────── */
#define RTL_MAC0          0x00   /* MAC 地址 (6字节) */
#define RTL_MAR0          0x08   /* 多播过滤 */
#define RTL_TSD0          0x10   /* 发送状态 描述符0-3 */
#define RTL_TSAD0         0x20   /* 发送起始地址 描述符0-3 */
#define RTL_RBSTART       0x30   /* 接收缓冲区起始地址 */
#define RTL_CMD           0x37   /* 命令寄存器 */
#define RTL_CAPR          0x38   /* 当前地址(读取接收包位置) */
#define RTL_IMR           0x3C   /* 中断屏蔽寄存器 */
#define RTL_ISR           0x3E   /* 中断状态寄存器 */
#define RTL_TCR           0x40   /* 发送配置寄存器 */
#define RTL_RCR           0x44   /* 接收配置寄存器 */
#define RTL_MPC           0x4C   /* 漏包计数 */
#define RTL_CONFIG1       0x52   /* 配置寄存器1 */

/* ── 命令位 ───────────────────────────────────────────────────── */
#define RTL_CMD_RST       0x10   /* 软复位 */
#define RTL_CMD_RE        0x08   /* 接收使能 */
#define RTL_CMD_TE        0x04   /* 发送使能 */

/* ── 中断位 ───────────────────────────────────────────────────── */
#define RTL_ISR_ROK       0x0001 /* 接收成功 */
#define RTL_ISR_RER       0x0002 /* 接收错误 */
#define RTL_ISR_TOK       0x0004 /* 发送成功 */
#define RTL_ISR_TER       0x0008 /* 发送错误 */

/* ── 接收配置 ─────────────────────────────────────────────────── */
/* AAP=接收所有包, APM=接收物理匹配包, AM=多播, AB=广播, WRAP=环绕 */
#define RTL_RCR_VAL  (0xF | (1<<7))  /* AAP|APM|AM|AB + WRAP */

/* ── 发送状态位 ───────────────────────────────────────────────── */
#define RTL_TSD_OWN   (1<<13)  /* 0=DMA传输中，1=CPU可写 */
#define RTL_TSD_TOK   (1<<15)  /* 发送成功 */

/* ── 接收缓冲区 ───────────────────────────────────────────────── */
#define RTL_RX_BUF_SIZE  (8192 + 16 + 1500)  /* 8K + 环绕保护 */
#define RTL_TX_BUF_COUNT 4                    /* 4个发送描述符 */
#define RTL_TX_BUF_SIZE  1536                 /* 每个发送缓冲区 */

/* ── 公共接口 ─────────────────────────────────────────────────── */
int  rtl8139_init(void);          /* 初始化，返回0成功 */
int  rtl8139_send(const uint8_t *data, uint16_t len); /* 发送以太网帧 */
void rtl8139_handler(void);       /* IRQ中断处理 */
void rtl8139_get_mac(uint8_t mac[6]);

#endif /* RTL8139_H */