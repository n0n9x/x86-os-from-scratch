/*
 * 极简网络协议栈
 * 支持：Ethernet / ARP / IPv4 / ICMP (echo request/reply)
 *
 * 设计原则：
 *   - 无动态线程，中断驱动接收，轮询等待 ping 应答
 *   - ARP 缓存 16 条，简单线性查找
 *   - ping 使用内核自旋等待（带超时），适合嵌入 shell 命令
 */

#include <net/net.h>
#include <drivers/rtl8139.h>
#include <lib/string.h>
#include <drivers/terminal.h>
#include <mm/kheap.h>
#include <kernel/timer.h>
#include <drivers/io.h>

/* ── 本机配置 ─────────────────────────────────────────────────── */
static uint32_t local_ip = IP_MAKE(10, 0, 2, 15); // QEMU 默认用户网络ip
static uint32_t gateway = IP_MAKE(10, 0, 2, 2);   // 默认网关
static uint8_t local_mac[6];                      // 网卡地址

/* ── ARP 缓存 ─────────────────────────────────────────────────── */
static arp_entry_t arp_cache[ARP_CACHE_SIZE];
static int arp_cache_count = 0;

/* ── ping 应答同步 ────────────────────────────────────────────── */
static volatile int ping_got_reply = 0;
static volatile uint16_t ping_reply_id = 0;
static volatile uint16_t ping_reply_seq = 0;

/* ════════════════════════════════════════════════════════════════
 *  初始化
 * ════════════════════════════════════════════════════════════════ */
int net_init(void)
{
    memset(arp_cache, 0, sizeof(arp_cache));
    arp_cache_count = 0;
    ping_got_reply = 0;

    int ret = rtl8139_init();
    if (ret != 0)
        return ret;

    rtl8139_get_mac(local_mac);
    return 0;
}

void net_setup(void)
{
    /* 解除 IRQ2（从片级联）和 IRQ11（RTL8139）的屏蔽 */
    uint8_t mask1 = inb(0x21);
    mask1 &= ~(1 << 2);
    outb(0x21, mask1);

    uint8_t mask2 = inb(0xA1);
    mask2 &= ~(1 << 3);
    outb(0xA1, mask2);

    if (net_init() == 0)
    {
        net_set_ip(IP_MAKE(10, 0, 2, 15));
        net_set_gateway(IP_MAKE(10, 0, 2, 2));
        terminal_writestring("[net] IP=10.0.2.15  GW=10.0.2.2\n");
    }
    else
    {
        terminal_writestring("[net] RTL8139 not found, network disabled.\n");
    }
}
void net_set_ip(uint32_t ip) { local_ip = ip; }
void net_set_gateway(uint32_t gw) { gateway = gw; }
uint32_t net_get_ip(void) { return local_ip; }

/* ════════════════════════════════════════════════════════════════
 *  校验和（Internet Checksum，RFC1071）16位累加反码求和
 * ════════════════════════════════════════════════════════════════ */
uint16_t net_checksum(const void *data, uint32_t len) // 待计算的数据包 数据总字节数
{
    const uint16_t *p = (const uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1)
    {
        sum += *p++;
        len -= 2;
    }
    if (len == 1)
        sum += *(const uint8_t *)p;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(~sum);
}

/* ════════════════════════════════════════════════════════════════
 *  ARP
 * ════════════════════════════════════════════════════════════════ */

/* 在缓存中查找 IP 对应的 MAC */
static arp_entry_t *arp_cache_lookup(uint32_t ip)
{
    for (int i = 0; i < arp_cache_count; i++)
        if (arp_cache[i].valid && arp_cache[i].ip == ip)
            return &arp_cache[i];
    return NULL;
}

/* 添加/更新缓存条目 */
static void arp_cache_update(uint32_t ip, const uint8_t mac[6])
{
    /* 先找已有条目 */
    arp_entry_t *e = arp_cache_lookup(ip);
    if (!e)
    {
        if (arp_cache_count < ARP_CACHE_SIZE)
            e = &arp_cache[arp_cache_count++];
        else
            e = &arp_cache[0]; /* 缓存满了，覆盖第一条（LRU 太复杂，先这样） */
    }
    e->ip = ip;
    e->valid = 1;
    memcpy(e->mac, mac, 6);
}

/* 发送 ARP 请求 */
static void arp_send_request(uint32_t target_ip)
{
    /* 以太网帧 + ARP 包 */
    uint8_t frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    memset(frame, 0, sizeof(frame));

    eth_header_t *eth = (eth_header_t *)frame;                          // 以太网帧
    arp_packet_t *arp = (arp_packet_t *)(frame + sizeof(eth_header_t)); // arp数据包

    /* 以太网头：广播 */
    memset(eth->dst, 0xFF, 6);
    memcpy(eth->src, local_mac, 6);
    eth->type = htons(ETHERTYPE_ARP); // x86是小端，网络传输是大端

    /* ARP 字段 */
    arp->htype = htons(1);            // 硬件类型
    arp->ptype = htons(ETHERTYPE_IP); // 协议类型 ipv4
    arp->hlen = 6;                    // mac长度
    arp->plen = 4;                    // ip长度
    arp->oper = htons(ARP_REQUEST);   // arp请求
    memcpy(arp->sha, local_mac, 6);   // 本机mac
    arp->spa = local_ip;              // 本机ip
    memset(arp->tha, 0, 6);
    arp->tpa = target_ip; // 目标ip

    rtl8139_send(frame, (uint16_t)sizeof(frame));
}

/* 处理收到的 ARP 包 */
void arp_handle(const uint8_t *payload, uint16_t len)
{
    // payload 以太网帧中剥离出来的 ARP 数据部分 len 长度
    if (len < sizeof(arp_packet_t))
        return;
    const arp_packet_t *arp = (const arp_packet_t *)payload;

    // 收录缓存
    arp_cache_update(arp->spa, arp->sha);

    /* 如果是询问我的 IP，回复 */
    if (ntohs(arp->oper) == ARP_REQUEST && arp->tpa == local_ip)
    {
        uint8_t frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
        eth_header_t *eth = (eth_header_t *)frame;
        arp_packet_t *reply = (arp_packet_t *)(frame + sizeof(eth_header_t));

        memcpy(eth->dst, arp->sha, 6);
        memcpy(eth->src, local_mac, 6);
        eth->type = htons(ETHERTYPE_ARP);

        reply->htype = htons(1);
        reply->ptype = htons(ETHERTYPE_IP);
        reply->hlen = 6;
        reply->plen = 4;
        reply->oper = htons(ARP_REPLY);
        memcpy(reply->sha, local_mac, 6);
        reply->spa = local_ip;
        memcpy(reply->tha, arp->sha, 6);
        reply->tpa = arp->spa;

        rtl8139_send(frame, (uint16_t)sizeof(frame));
    }
}

/* ARP 解析：发请求，轮询等待应答（最多等 500ms） */
int arp_resolve(uint32_t ip, uint8_t mac_out[6])
{
    /* 先查缓存 */
    arp_entry_t *e = arp_cache_lookup(ip);
    if (e)
    {
        memcpy(mac_out, e->mac, 6);
        return 0;
    }

    /* 发 ARP request */
    arp_send_request(ip);
    asm volatile("sti"); /* 开中断，让网卡 IRQ 能进来 */
    /* 轮询等待（最多约 500ms，以 timer tick 为单位） */
    extern uint32_t get_ticks(void);
    uint32_t start = get_ticks();
    while (get_ticks() - start < 500)
    {
        e = arp_cache_lookup(ip);
        if (e)
        {
            memcpy(mac_out, e->mac, 6);
            return 0;
        }
        asm volatile("pause");
    }
    asm volatile("cli"); /* 超时也要恢复 */
    return -1;           /* 超时 */
}

/* ════════════════════════════════════════════════════════════════
 *  IP 层
 * ════════════════════════════════════════════════════════════════ */
static uint16_t ip_id_counter = 1;

int ip_send(uint32_t dst_ip, uint8_t proto,
            const uint8_t *payload, uint16_t payload_len)
{
    uint16_t ip_total = sizeof(ip_header_t) + payload_len;
    uint16_t frame_len = sizeof(eth_header_t) + ip_total;

    uint8_t *frame = (uint8_t *)kmalloc(frame_len);
    if (!frame)
        return -1;
    memset(frame, 0, frame_len);

    /* ─ 以太网头 ─ */
    eth_header_t *eth = (eth_header_t *)frame;
    memcpy(eth->src, local_mac, 6);
    eth->type = htons(ETHERTYPE_IP);

    /* 决定下一跳 IP */
    uint32_t nexthop = dst_ip;
    /* 简单子网判断：同一个 /24 网段直接发，否则发网关 */
    if ((dst_ip & 0x00FFFFFF) != (local_ip & 0x00FFFFFF))
        nexthop = gateway;

    /* ARP 解析下一跳 MAC */
    if (arp_resolve(nexthop, eth->dst) != 0)
    {
        kfree(frame);
        return -1;
    }

    /* ─ IP 头 ─ */
    ip_header_t *ip = (ip_header_t *)(frame + sizeof(eth_header_t));
    ip->ver_ihl = 0x45; // 版本4, 长度20字节
    ip->tos = 0;
    ip->total_len = htons(ip_total); // 填入总长度
    ip->id = htons(ip_id_counter++); // 填入id号
    ip->frag_off = 0;
    ip->ttl = 64;                                         // 生存时间，防止死循环
    ip->proto = proto;                                    // 参数传递的上层协议号 (1-ICMP, 17-UDP)
    ip->src_ip = local_ip;                                // 源ip地址
    ip->dst_ip = dst_ip;                                  // 目的ip地址
    ip->checksum = net_checksum(ip, sizeof(ip_header_t)); // 计算校验和

    /* ─ 载荷 ─ */
    memcpy(frame + sizeof(eth_header_t) + sizeof(ip_header_t), payload, payload_len);

    int ret = rtl8139_send(frame, frame_len);
    kfree(frame);
    return ret;
}

/* ════════════════════════════════════════════════════════════════
 *  ICMP
 * ════════════════════════════════════════════════════════════════ */

/* 发送 ICMP Echo Request /ICMP回显请求*/
int icmp_send_echo(uint32_t dst_ip, uint16_t id, uint16_t seq)
{
    /* ICMP 头 + 16字节数据 */
    const uint16_t data_len = 16;                         // icmp的数据载荷
    uint16_t icmp_len = sizeof(icmp_header_t) + data_len; // 整个 ICMP 报文的总长度
    uint8_t buf[sizeof(icmp_header_t) + 16];              // 数据缓冲区

    icmp_header_t *icmp = (icmp_header_t *)buf;
    icmp->type = ICMP_ECHO_REQUEST; // 回显请求
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = htons(id);   // 标识符
    icmp->seq = htons(seq); // 序列号

    /* 填充载荷：简单的 0xAB 重复 */
    uint8_t *data = buf + sizeof(icmp_header_t);
    for (int i = 0; i < data_len; i++)
        data[i] = 0xAB;

    icmp->checksum = net_checksum(buf, icmp_len);

    return ip_send(dst_ip, IP_PROTO_ICMP, buf, icmp_len);
}

/* 处理收到的 ICMP 包 */
static void icmp_handle(const ip_header_t *ip, const uint8_t *payload, uint16_t len)
{
    // ip ip数据包头部 payload icmp原始数据
    if (len < sizeof(icmp_header_t))
        return;
    const icmp_header_t *icmp = (const icmp_header_t *)payload;

    if (icmp->type == ICMP_ECHO_REQUEST)
    {
        /* 回复 Echo Reply */
        uint8_t *reply = (uint8_t *)kmalloc(len);
        if (!reply)
            return;
        memcpy(reply, payload, len);

        icmp_header_t *r = (icmp_header_t *)reply;
        r->type = ICMP_ECHO_REPLY;
        r->checksum = 0;
        r->checksum = net_checksum(reply, len);

        ip_send(ip->src_ip, IP_PROTO_ICMP, reply, len);
        kfree(reply);
    }
    else if (icmp->type == ICMP_ECHO_REPLY)
    {
        /* 通知等待中的 ping */
        ping_reply_id = ntohs(icmp->id);
        ping_reply_seq = ntohs(icmp->seq);
        ping_got_reply = 1;
    }
}

/* ════════════════════════════════════════════════════════════════
 *  IP 接收分发
 * ════════════════════════════════════════════════════════════════ */
static void ip_handle(const uint8_t *payload, uint16_t len)
{
    // payload 完整的ip数据包
    if (len < sizeof(ip_header_t))
        return;
    const ip_header_t *ip = (const ip_header_t *)payload;

    //计算包头长度
    uint8_t ihl = (ip->ver_ihl & 0x0F) * 4;
    if (ihl < 20 || ihl > len)
        return;

    /* 只处理发给本机的包 */
    if (ip->dst_ip != local_ip)
        return;

    const uint8_t *upper = payload + ihl;//数据起始地址
    uint16_t upper_len = (uint16_t)(ntohs(ip->total_len) - ihl);

    //分发协议
    switch (ip->proto)
    {
    case IP_PROTO_ICMP:
        icmp_handle(ip, upper, upper_len);
        break;
    default:
        break;
    }
}

/* ════════════════════════════════════════════════════════════════
 *  以太网帧接收分发（由 RTL8139 中断调用）
 * ════════════════════════════════════════════════════════════════ */
void net_receive(const uint8_t *frame, uint16_t len)
{
    // frame 原始以太网帧
    if (len < sizeof(eth_header_t))
        return;
    const eth_header_t *eth = (const eth_header_t *)frame;

    const uint8_t *payload = frame + sizeof(eth_header_t);
    uint16_t payload_len = (uint16_t)(len - sizeof(eth_header_t));

    //协议分发
    switch (ntohs(eth->type))
    {
    case ETHERTYPE_ARP:
        arp_handle(payload, payload_len);
        break;
    case ETHERTYPE_IP:
        ip_handle(payload, payload_len);
        break;
    default:
        break;
    }
}

/* ════════════════════════════════════════════════════════════════
 *  ping 高层接口
 * ════════════════════════════════════════════════════════════════ */

/* IP 转字符串（简单版，写入静态 buf） */
static void print_ip(uint32_t ip)
{
    terminal_write_dec(ip & 0xFF);
    terminal_putchar('.');
    terminal_write_dec((ip >> 8) & 0xFF);
    terminal_putchar('.');
    terminal_write_dec((ip >> 16) & 0xFF);
    terminal_putchar('.');
    terminal_write_dec((ip >> 24) & 0xFF);
}

int ping(uint32_t dst_ip, int count, uint32_t timeout_ms)
{
    //dst_ip 目标ip地址     count 探测包数目    timeout_ms 超时时间
    extern uint32_t get_ticks(void);

    terminal_writestring("PING ");
    print_ip(dst_ip);
    terminal_writestring(" 16 bytes of data.\n");

    int sent = 0, received = 0;
    static uint16_t ping_id = 0x1234;

    for (int i = 0; i < count; i++)
    {
        uint16_t seq = (uint16_t)(i + 1);
        ping_got_reply = 0;

        uint32_t t0 = get_ticks();
        if (icmp_send_echo(dst_ip, ping_id, seq) != 0)
        {
            terminal_writestring("ping: send failed (ARP timeout?)\n");
            sent++;
            continue;
        }
        sent++;

        /* 等待应答 */
        asm volatile("sti"); 
        uint32_t deadline = get_ticks() + timeout_ms;
        while (!ping_got_reply && get_ticks() < deadline)
            asm volatile("pause");
        asm volatile("cli"); 

        uint32_t t1 = get_ticks();
        uint32_t rtt = t1 - t0;

        if (ping_got_reply && ping_reply_id == ping_id && ping_reply_seq == seq)
        {
            received++;
            terminal_writestring("16 bytes from ");
            print_ip(dst_ip);
            terminal_writestring(": icmp_seq=");
            terminal_write_dec(seq);
            terminal_writestring(" ttl=64 time=");
            terminal_write_dec(rtt);
            terminal_writestring(" ms\n");
        }
        else
        {
            terminal_writestring("Request timeout for icmp_seq ");
            terminal_write_dec(seq);
            terminal_putchar('\n');
        }

        /* 间隔约 1 秒 */
        asm volatile("sti"); /* ★ */
        uint32_t wait_end = get_ticks() + 1000;
        while (get_ticks() < wait_end)
            asm volatile("pause");
        asm volatile("cli"); /* ★ */
    }

    /* 统计 */
    terminal_writestring("--- ");
    print_ip(dst_ip);
    terminal_writestring(" ping statistics ---\n");
    terminal_write_dec((uint32_t)sent);
    terminal_writestring(" packets transmitted, ");
    terminal_write_dec((uint32_t)received);
    terminal_writestring(" received, ");
    uint32_t loss = (sent > 0) ? (uint32_t)((sent - received) * 100 / sent) : 0;
    terminal_write_dec(loss);
    terminal_writestring("% packet loss\n");

    return (received > 0) ? 0 : -1;
}