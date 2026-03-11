/*
 * RTL8139 网卡驱动
 * 支持 QEMU 默认的 rtl8139 仿真设备
 *
 * 工作流程：
 *   1. PCI 总线扫描，找到 RTL8139 (VendorID=0x10EC, DeviceID=0x8139)
 *   2. 读取 I/O 基地址 (BAR0)，使能总线主控 (Bus Master)
 *   3. 软复位，分配接收/发送缓冲区（内核堆），写入物理地址
 *   4. 配置接收过滤、打开中断屏蔽
 *   5. 中断处理：读 ISR，接收时调用 net_receive()，发送用轮询
 */

#include <drivers/rtl8139.h>
#include <drivers/io.h>
#include <mm/kheap.h>
#include <mm/vmm.h>
#include <lib/string.h>
#include <drivers/terminal.h>
#include <net/net.h>
#include <mm/pmm.h>

/* outl / inl 封装（io.h 里只有 outb/inb/outw/inw） */
static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %1" ::"a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ── 静态状态 ──────────────────────────────────────────────────── */
static uint16_t rtl_iobase = 0; /* I/O 端口基地址 */
static uint8_t rtl_mac[6];      /* 本机 MAC */

/* 静态分配，避免 kmalloc 跨页问题 */
static uint8_t rx_buf_static[RTL_RX_BUF_SIZE] __attribute__((aligned(4096)));
static uint8_t tx_buf_static[RTL_TX_BUF_COUNT][RTL_TX_BUF_SIZE] __attribute__((aligned(4096)));

static uint8_t *rx_buf = NULL;
static uint8_t *tx_buf[RTL_TX_BUF_COUNT];
static uint16_t rx_offset = 0; /* 当前读取偏移 */

/* 4个发送缓冲区 + 对应的下一个可用索引 */
static uint8_t *tx_buf[RTL_TX_BUF_COUNT];
static int tx_next = 0;

/* ── PCI 读写 ─────────────────────────────────────────────────── */
static uint32_t pci_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    return inl(PCI_CONFIG_DATA);
}

static void pci_write32(uint8_t bus, uint8_t dev, uint8_t func,
                        uint8_t offset, uint32_t val)
{
    uint32_t addr = (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDR, addr);
    outl(PCI_CONFIG_DATA, val);
}

/* ── PCI 扫描 ─────────────────────────────────────────────────── */
/* 返回找到的 I/O BAR0 基地址，0 表示未找到 */
static uint16_t pci_find_rtl8139(uint8_t *irq_out)
{
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t dev = 0; dev < 32; dev++)
        {
            uint32_t id = pci_read32((uint8_t)bus, dev, 0, 0);
            if (id == 0xFFFFFFFF)
                continue;
            uint16_t vendor = id & 0xFFFF;
            uint16_t device = id >> 16;
            if (vendor == RTL8139_VENDOR_ID && device == RTL8139_DEVICE_ID)
            {
                /* 读取 BAR0（I/O 空间） */
                uint32_t bar0 = pci_read32((uint8_t)bus, dev, 0, 0x10);
                uint16_t iobase = (uint16_t)(bar0 & 0xFFFC);

                /* 使能 Bus Master + I/O Space */
                uint32_t cmd = pci_read32((uint8_t)bus, dev, 0, 0x04);
                cmd |= 0x05; /* I/O Enable | Bus Master */
                pci_write32((uint8_t)bus, dev, 0, 0x04, cmd);

                /* 读取 IRQ */
                uint32_t irq_line = pci_read32((uint8_t)bus, dev, 0, 0x3C);
                if (irq_out)
                    *irq_out = (uint8_t)(irq_line & 0xFF);

                terminal_writestring("[RTL8139] Found at bus=");
                terminal_write_dec(bus);
                terminal_writestring(" dev=");
                terminal_write_dec(dev);
                terminal_writestring(" iobase=");
                terminal_write_hex(iobase);
                terminal_writestring("\n");
                return iobase;
            }
        }
    }
    return 0;
}

/* ── 初始化 ───────────────────────────────────────────────────── */
int rtl8139_init(void)
{
    uint8_t irq = 0;
    rtl_iobase = pci_find_rtl8139(&irq);
    if (rtl_iobase == 0)
    {
        terminal_writestring("[RTL8139] Not found!\n");
        return -1;
    }

    /* 1. 打开电源 */
    outb(rtl_iobase + RTL_CONFIG1, 0x00);

    /* 2. 软复位 */
    outb(rtl_iobase + RTL_CMD, RTL_CMD_RST);
    while (inb(rtl_iobase + RTL_CMD) & RTL_CMD_RST)
    {
    }

    /* 3. 读取 MAC 地址 */
    for (int i = 0; i < 6; i++)
        rtl_mac[i] = inb(rtl_iobase + RTL_MAC0 + i);

    terminal_writestring("[RTL8139] MAC: ");
    for (int i = 0; i < 6; i++)
    {
        terminal_write_hex(rtl_mac[i]);
        if (i < 5)
            terminal_putchar(':');
    }
    terminal_putchar('\n');

    /* 4. 使用静态缓冲区，物理地址连续有保证 */
    rx_buf = rx_buf_static;
    memset(rx_buf, 0, RTL_RX_BUF_SIZE);

    for (int i = 0; i < RTL_TX_BUF_COUNT; i++)
    {
        tx_buf[i] = tx_buf_static[i];
        memset(tx_buf[i], 0, RTL_TX_BUF_SIZE);
    }

    /* 5. 写入物理地址 */
    uint32_t rx_phys = VIRT_TO_PHYS((uint32_t)rx_buf);
    outl(rtl_iobase + RTL_RBSTART, rx_phys);

    /* 6. 配置接收 */
    outl(rtl_iobase + RTL_RCR, RTL_RCR_VAL);

    /* 7. 配置发送 */
    outl(rtl_iobase + RTL_TCR, (6 << 8));

    /* 8. 开启接收+发送 */
    outb(rtl_iobase + RTL_CMD, RTL_CMD_RE | RTL_CMD_TE);

    /* 9. ★ 初始化读指针为 0，CAPR 写 0xFFF0（硬件要求偏移-16） */
    rx_offset = 0;
    outw(rtl_iobase + RTL_CAPR, 0xFFF0);

    /* 10. 设置中断屏蔽 */
    outw(rtl_iobase + RTL_IMR, RTL_ISR_ROK | RTL_ISR_TOK |
                                   RTL_ISR_RER | RTL_ISR_TER);

    terminal_writestring("[RTL8139] Init OK, IRQ=");
    terminal_write_dec(irq);
    terminal_putchar('\n');
    return 0;
}

/* ── 发送以太网帧 ─────────────────────────────────────────────── */
int rtl8139_send(const uint8_t *data, uint16_t len)
{
    if (!rtl_iobase || len > RTL_TX_BUF_SIZE)
        return -1;

    /* 等待当前发送描述符的 OWN 位为1（DMA 已完成，CPU 可写） */
    uint16_t tsd_reg = RTL_TSD0 + tx_next * 4;
    uint16_t tsad_reg = RTL_TSAD0 + tx_next * 4;

    /* 轮询等待 OWN */
    uint32_t timeout = 100000;
    while (!(inl(rtl_iobase + tsd_reg) & RTL_TSD_OWN) && --timeout)
        ;
    if (timeout == 0)
        return -1;

    /* 复制数据到发送缓冲区 */
    memcpy(tx_buf[tx_next], data, len);
    if (len < 60)
    {
        memset(tx_buf[tx_next] + len, 0, 60 - len);
        len = 60;
    }

    /* 写入发送起始地址（物理地址） */
    uint32_t tx_phys = VIRT_TO_PHYS((uint32_t)tx_buf[tx_next]);
    outl(rtl_iobase + tsad_reg, tx_phys);

    /* 触发发送：写入长度，清OWN位，硬件开始DMA */
    outl(rtl_iobase + tsd_reg, (uint32_t)len & 0x1FFF);

    tx_next = (tx_next + 1) % RTL_TX_BUF_COUNT;
    return 0;
}

/* ── 中断处理 ─────────────────────────────────────────────────── */
void rtl8139_handler(void)
{
    if (!rtl_iobase)
        return;

    uint16_t isr = inw(rtl_iobase + RTL_ISR);
    /* 清除中断状态（写1清零） */
    outw(rtl_iobase + RTL_ISR, isr);

    /* 接收成功 */
    if (isr & RTL_ISR_ROK)
    {
        while (!(inb(rtl_iobase + RTL_CMD) & 0x01))
        {
            /* 直接用 CAPR+16 作为读取位置 */
            uint16_t capr = inw(rtl_iobase + RTL_CAPR);
            uint16_t cur = (uint16_t)((capr + 16) & 0x1FFF); /* % 8192 */

            uint16_t *hdr = (uint16_t *)(rx_buf + cur);
            uint16_t rx_status = hdr[0];
            uint16_t rx_len = hdr[1];

            if (rx_len == 0 || rx_len > 1518 + 4)
                break;

            uint8_t *frame = (uint8_t *)(rx_buf + cur + 4);
            uint16_t frame_len = rx_len - 4;
            net_receive(frame, frame_len);

            /* 更新 CAPR */
            uint16_t next = (uint16_t)((cur + rx_len + 4 + 3) & ~3) & 0x1FFF;
            outw(rtl_iobase + RTL_CAPR, (uint16_t)(next - 16));
        }
    }

    if (isr & RTL_ISR_RER)
    {
        terminal_writestring("[RTL8139] RX error\n");
    }
    if (isr & RTL_ISR_TER)
    {
        terminal_writestring("[RTL8139] TX error\n");
    }
}

/* ── 获取 MAC ─────────────────────────────────────────────────── */
void rtl8139_get_mac(uint8_t mac[6])
{
    for (int i = 0; i < 6; i++)
        mac[i] = rtl_mac[i];
}