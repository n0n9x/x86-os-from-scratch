#ifndef NET_H
#define NET_H

#include <stdint.h>

/* ════════════════════════════════════════════════
 *  基础类型 & 字节序转换
 * ════════════════════════════════════════════════ */
// 大小端转换
static inline uint16_t htons(uint16_t x) {
    return (uint16_t)((x >> 8) | (x << 8));
}
static inline uint16_t ntohs(uint16_t x) { return htons(x); }
static inline uint32_t htonl(uint32_t x) {
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >>  8) |
           ((x & 0x0000FF00) <<  8) |
           ((x & 0x000000FF) << 24);
}
static inline uint32_t ntohl(uint32_t x) { return htonl(x); }

/* ════════════════════════════════════════════════
 *  MAC / IP 辅助宏
 * ════════════════════════════════════════════════ */
#define MAC_BROADCAST  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
#define IP_MAKE(a,b,c,d) ((uint32_t)(a)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))

/* ════════════════════════════════════════════════
 *  以太网帧头  (14字节)
 * ════════════════════════════════════════════════ */
#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IP   0x0800

typedef struct __attribute__((packed)) {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;      /* 网络字节序 */
} eth_header_t;

/* ════════════════════════════════════════════════
 *  ARP 包  (28字节，仅IPv4/以太网)
 * ════════════════════════════════════════════════ */
#define ARP_REQUEST 1
#define ARP_REPLY   2

typedef struct __attribute__((packed)) {
    uint16_t htype;     /* 硬件类型：1=以太网 */
    uint16_t ptype;     /* 协议类型：0x0800=IP */
    uint8_t  hlen;      /* 硬件地址长度：6 */
    uint8_t  plen;      /* 协议地址长度：4 */
    uint16_t oper;      /* 操作：1=请求，2=应答 */
    uint8_t  sha[6];    /* 发送方 MAC */
    uint32_t spa;       /* 发送方 IP（小端序存储） */
    uint8_t  tha[6];    /* 目标 MAC */
    uint32_t tpa;       /* 目标 IP */
} arp_packet_t;

/* ════════════════════════════════════════════════
 *  IP 头  (20字节，不含选项)
 * ════════════════════════════════════════════════ */
#define IP_PROTO_ICMP 1

typedef struct __attribute__((packed)) {
    uint8_t  ver_ihl;   /* 版本(4) | 头长度(4)，单位4字节 */
    uint8_t  tos;
    uint16_t total_len; /* 网络字节序 */
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ip_header_t;

/* ════════════════════════════════════════════════
 *  ICMP 头  (8字节 echo/reply)
 * ════════════════════════════════════════════════ */
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY   0

typedef struct __attribute__((packed)) {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} icmp_header_t;

/* ════════════════════════════════════════════════
 *  ARP 缓存
 * ════════════════════════════════════════════════ */
#define ARP_CACHE_SIZE 16

typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    uint8_t  valid;
} arp_entry_t;

/* ════════════════════════════════════════════════
 *  公共接口
 * ════════════════════════════════════════════════ */

/* 网络子系统初始化 */
int  net_init(void);
void net_setup(void);

/* 本机 IP/MAC 配置（简单静态配置） */
void net_set_ip(uint32_t ip);
void net_set_gateway(uint32_t gw);
uint32_t net_get_ip(void);

/* 接收数据包分发（由 RTL8139 中断调用） */
void net_receive(const uint8_t *frame, uint16_t len);

/* ARP */
int  arp_resolve(uint32_t ip, uint8_t mac_out[6]); /* 返回0成功 */
void arp_handle(const uint8_t *payload, uint16_t len);

/* IP 层发送 */
int  ip_send(uint32_t dst_ip, uint8_t proto,
             const uint8_t *payload, uint16_t payload_len);

/* ICMP ping：发送1个echo request，返回0成功 */
int  icmp_send_echo(uint32_t dst_ip, uint16_t id, uint16_t seq);

/* ping 高层接口：发count个包，超时timeout_ms毫秒 */
int  ping(uint32_t dst_ip, int count, uint32_t timeout_ms);

/* 校验和（通用） */
uint16_t net_checksum(const void *data, uint32_t len);

#endif /* NET_H */