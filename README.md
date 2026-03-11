# MyOS — 自制 x86 32位操作系统

一个从零开始实现的 x86 32位保护模式操作系统，支持多进程、FAT16 文件系统、ELF 加载、网络通信，并成功移植了 TinyCC 编译器，实现了在 OS 内部编写、编译、运行 C 程序的完整闭环。

---

## 架构概览

```
+---------------------------+
|        用户程序 (Ring 3)   |
|  shell.elf  tcc.elf  *.elf |
+---------------------------+
|        mylibc (用户态 libc) |
|  printf / scanf / malloc  |
|  fopen / fread / fwrite   |
+---------------------------+
|       系统调用 (int 0x80)  |
|    18 个系统调用接口        |
+---------------------------+
|          内核 (Ring 0)     |
|  进程调度  ELF加载器        |
|  内存管理  FAT16文件系统    |
|  IDE驱动  键盘驱动          |
|  RTL8139驱动  网络协议栈    |
+---------------------------+
|        硬件 / QEMU         |
+---------------------------+
```

- **内存模型**：高半核设计，内核映射到虚拟地址 `0xC0000000` 以上，用户态运行在低 3GB
- **特权级**：内核 Ring 0，用户程序 Ring 3，通过 TSS 实现特权级切换

---

## 技术栈

| 组件 | 技术 |
|------|------|
| 语言 | C + x86 汇编 (AT&T 语法) |
| 交叉编译器 | i686-elf-gcc |
| 链接器 | GNU ld，自定义链接脚本 |
| 汇编器 | NASM / GAS |
| 模拟器 | QEMU |
| 构建系统 | GNU Make |
| 磁盘格式 | FAT16 |
| 可执行格式 | ELF32 |

---

## 已实现功能

### 内存管理
- 物理内存管理（PMM）：bitmap 页帧分配器，4KB 粒度
- 虚拟内存管理（VMM）：页目录/页表，`map_page` / `unmap_page`
- 内核堆：`kmalloc` / `kfree`
- 用户态堆：`sbrk` 系统调用，缺页时按需映射物理页

### 中断与系统调用
- IDT 初始化，8259A PIC 重映射
- PIT 时钟中断（调度驱动）
- 页错误处理（`#PF`）
- `int 0x80` 系统调用，共 18 个：

| 编号 | 名称 | 功能 |
|------|------|------|
| 0 | SYS_PRINT | 终端输出 |
| 1 | SYS_SBRK | 扩展用户堆 |
| 2 | SYS_EXIT | 进程退出 |
| 3 | SYS_READ | 键盘读取 |
| 4 | SYS_DISK_READ | 磁盘读扇区 |
| 5 | SYS_DISK_WRITE | 磁盘写扇区 |
| 6 | SYS_OPEN | 打开文件 |
| 7 | SYS_FREAD | 读文件 |
| 8 | SYS_FWRITE | 写文件（带 offset） |
| 9 | SYS_FCREATE | 创建文件 |
| 10 | SYS_FDELETE | 删除文件 |
| 11 | SYS_FAPPEND | 追加文件 |
| 12 | SYS_FLIST | 列目录 |
| 13 | SYS_MKDIR | 创建目录 |
| 14 | SYS_SHUTDOWN | 关机 |
| 15 | SYS_EXEC | 加载并执行 ELF |
| 16 | SYS_PING | 发送 ICMP ping |
| 17 | SYS_NET_SET_IP | 设置本机 IP / 网关 |

### 进程管理
- 多任务时间片轮转调度（Round Robin）
- 上下文切换（汇编实现 `switch_to_task`，保存/恢复寄存器）
- TSS（Task State Segment）内核栈切换
- 父子进程机制：`SYS_EXEC` 阻塞父进程，子进程退出时自动唤醒
- 进程状态：RUNNING / READY / WAITING / WAIT / ZOMBIE
- 键盘阻塞队列：无输入时挂起进程，中断到来时唤醒

### 驱动
- **键盘驱动**：扫描码转 ASCII，行缓冲，退格，Shift，Ctrl+C（`0x03`）
- **IDE 磁盘驱动**：PIO 轮询模式，扇区读写
- **终端驱动**：VGA 文本模式，滚屏，退格擦除
- **RTL8139 网卡驱动**：PCI 总线扫描，DMA 收发，IRQ11 中断处理

### 文件系统（FAT16）
- 挂载：解析 BPB，计算根目录/数据区偏移
- 路径解析：支持多级目录（`/dir/subdir/file`）
- 文件操作：创建、删除、读取、顺序写、带 offset 随机写、追加
- 目录操作：创建目录，列出目录内容

### 网络（TCP/IP 协议栈）
从零实现了一个极简网络协议栈，架构如下：

```
+------------------+
|   ICMP / ping    |
+------------------+
|       IP         |
+------------------+
|    ARP / 以太网   |
+------------------+
|  RTL8139 驱动    |
+------------------+
```

- **以太网层**：帧收发，EtherType 分发（ARP / IP）
- **ARP**：请求/应答，16条目缓存，自动学习，超时重试
- **IP层**：IPv4 收发，校验和计算，简单路由（同网段直发，跨网段走网关）
- **ICMP**：Echo Request / Reply，自动回复收到的 ping 请求
- **ping**：发送指定数量的 Echo Request，统计 RTT 和丢包率
- **网络配置**：静态 IP，默认 `10.0.2.15 / GW 10.0.2.2`（QEMU 用户网络）

### ELF 加载器
- 解析 ELF32 Header 和 Program Header
- 为每个 LOAD 段分配物理页并映射到用户虚拟地址空间
- 分配用户栈（`0xBFFFE000`），在栈上布置 `argc` / `argv`
- 同步内核高半核页目录（PDE 768~1023）

### 用户态 C 运行库（mylibc）
自行实现的用户态 C 标准库，无任何宿主 OS 依赖：

- **格式化 I/O**：`printf` / `scanf` / `sprintf` / `snprintf` / `fprintf`，支持 `%d %u %x %s %c %p` 及宽度、对齐、补零
- **文件 I/O**：`fopen` / `fclose` / `fread` / `fwrite` / `fseek` / `ftell` / `fgets` / `fputs`
- **内存**：`malloc`（bump allocator）/ `realloc` / `free` / `memcpy` / `memset` / `memmove` / `memcmp`
- **字符串**：`strlen` / `strcpy` / `strcat` / `strcmp` / `strchr` / `strstr` / `strdup` 等
- **数值转换**：`atoi` / `strtol` / `strtoul` / `strtod`
- **字符分类**：`isdigit` / `isalpha` / `toupper` / `tolower` 等
- **进程控制**：`exit` / `sbrk`

### Shell
交互式命令行，支持：

| 命令 | 功能 |
|------|------|
| `ls [path]` | 列出文件和目录 |
| `cd <path>` | 切换目录 |
| `cat <file>` | 打印文件内容 |
| `write <file>` | 多行编辑文件（Ctrl+C 保存退出）|
| `append <file> <text>` | 追加内容到文件 |
| `touch <file>` | 创建文件 |
| `mkdir <dir>` | 创建目录 |
| `rm <file>` | 删除文件 |
| `run <file.elf> [args]` | 加载并执行 ELF 程序 |
| `tcc <input.c> [-o out]` | 编译 C 源文件（封装 TinyCC）|
| `ping <IP> [count]` | 向目标 IP 发送 ICMP ping |
| `shutdown` | 关机 |

### TinyCC 移植
将 [TinyCC](https://bellard.org/tcc/) 编译器移植到 MyOS：

- 编写 `myos_port.h` 适配层，将 TCC 的 POSIX 文件 I/O、内存分配等接口桥接到 `mylibc`
- 实现带 offset 的文件随机写（`fat_write_file_at`），支持 TCC 回写 ELF Header
- 禁用 `TCC_IS_NATIVE` 避免结构体布局错乱
- 实现用户态 bump allocator 保证 TCC 堆分配稳定性

---

## 快速开始

### 依赖
```bash
# Ubuntu / Debian
apt install qemu-system-x86 i686-elf-gcc nasm make
```

### 构建

```bash
# 编译内核和用户程序
make

# 构建 TinyCC（在 tinycc/ 目录下）
cd tinycc && bash build_tcc.sh && cd ..

# 打包磁盘镜像
make disk

# 运行
make run
```

### 在 OS 内编写并运行 C 程序

```
/> write /hello.c
int main() {
    printf("Hello, MyOS!\n");
    return 0;
}
^C
[Saved]
/> tcc /hello.c -o /hello.elf
/> run /hello.elf
Hello, MyOS!
[Task Exited]
/>
```

### 测试网络

```
/> ping 10.0.2.2
PING 10.0.2.2 16 bytes of data.
16 bytes from 10.0.2.2: icmp_seq=1 ttl=64 time=2 ms
16 bytes from 10.0.2.2: icmp_seq=2 ttl=64 time=0 ms
16 bytes from 10.0.2.2: icmp_seq=3 ttl=64 time=0 ms
16 bytes from 10.0.2.2: icmp_seq=4 ttl=64 time=0 ms
--- 10.0.2.2 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss

/> ping 8.8.8.8
PING 8.8.8.8 16 bytes of data.
16 bytes from 8.8.8.8: icmp_seq=1 ttl=64 time=176 ms
16 bytes from 8.8.8.8: icmp_seq=2 ttl=64 time=181 ms
16 bytes from 8.8.8.8: icmp_seq=3 ttl=64 time=178 ms
16 bytes from 8.8.8.8: icmp_seq=4 ttl=64 time=169 ms
--- 8.8.8.8 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss
```

---

## 目录结构

```
├── src/
│   ├── kernel/       # 内核核心（syscall、task、elf、gdt、idt）
│   ├── mm/           # 内存管理（pmm、vmm、kheap）
│   ├── drivers/      # 驱动（terminal、keyboard、ide、rtl8139）
│   ├── fs/           # FAT16 文件系统
│   ├── net/          # 网络协议栈（ARP、IP、ICMP）
│   ├── apps/         # 用户程序（shell、hello、ping）
│   └── lib/          # 用户态 mylibc
├── include/
│   ├── drivers/      # 驱动头文件
│   ├── kernel/       # 内核头文件
│   ├── mm/           # 内存管理头文件
│   ├── net/          # 网络头文件
│   └── lib/          # 用户态库头文件
├── tinycc/           # TinyCC 移植
│   ├── build_tcc.sh  # 编译脚本
│   ├── tcc_entry.c   # 入口适配
│   ├── myos_port.h   # 移植适配层
│   ├── fake_inc/     # 空头文件（拦截标准库 include）
│   └── config.h      # TCC 配置
├── linker.ld         # 内核链接脚本
├── user.ld           # 用户程序链接脚本
└── Makefile
```

---

## 项目亮点

从零实现了一个支持多进程、FAT16 文件系统、ELF 加载、网络通信的 x86 32位操作系统。实现了完整的 RTL8139 网卡驱动和 Ethernet/ARP/IP/ICMP 协议栈，支持 ping 命令。并成功将 TinyCC 编译器移植其上，实现了在 OS 内部编写、编译、运行 C 程序的完整闭环，整个过程无任何宿主操作系统参与。