export PATH := $(HOME)/opt/cross/bin:$(PATH)

# 1. 路径定义
SRCDIR = src
INCDIR = include
BINDIR = bin
DISK_IMG = test_disk.img
TARGET = myos.bin
USER_ELF = shell.elf

# 2. 编译器设置
CC = i686-elf-gcc
AS = nasm
CFLAGS = -std=gnu99 -ffreestanding -g -O2 -Wall -Wextra -I$(INCDIR)
LDFLAGS_KERN = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc -g
LDFLAGS_USER = -T user.ld -ffreestanding -O2 -nostdlib -lgcc -g

# 3. 目标文件自动寻找逻辑
# 内核 C 文件：排除 src/apps 目录和 src/lib/mylibc.c
KERNEL_CSOURCES = $(shell find $(SRCDIR) -path "$(SRCDIR)/apps" -prune -o -name '*.c' -print | grep -v 'mylibc\.c')
# 内核汇编文件
KERNEL_ASOURCES = $(shell find $(SRCDIR) -path "$(SRCDIR)/apps" -prune -o -name '*.s' -print)
KERNEL_OBJS = $(KERNEL_CSOURCES:$(SRCDIR)/%.c=$(BINDIR)/%.o) $(KERNEL_ASOURCES:$(SRCDIR)/%.s=$(BINDIR)/%.o)

# 用户态公共库
USER_LIB_OBJS = $(BINDIR)/lib/stdio.o $(BINDIR)/lib/stdlib.o $(BINDIR)/lib/string.o $(BINDIR)/lib/syscall_arch.o
SHELL_OBJ = $(BINDIR)/apps/shell.o $(BINDIR)/apps/start.o

MYLIBC_OBJS = $(BINDIR)/lib/mylibc.o $(BINDIR)/lib/syscall_arch.o
HELLO_ELF   = hello.elf
HELLO_OBJS  = $(BINDIR)/apps/hello.o $(BINDIR)/apps/ustart.o $(BINDIR)/apps/start_main.o

# ── 新增：ping ──────────────────────────────────────────────────────
PING_ELF  = ping.elf
PING_OBJS = $(BINDIR)/apps/ping.o $(BINDIR)/apps/ustart.o

TCC_ELF = tinycc/tcc.elf

# ── all 目标加入 ping.elf ───────────────────────────────────────────
all: $(TARGET) $(USER_ELF) $(HELLO_ELF) $(PING_ELF) $(DISK_IMG)

# 4. 链接内核
$(TARGET): $(KERNEL_OBJS)
	@echo "--- 正在链接内核 ---"
	$(CC) $(LDFLAGS_KERN) -o $(TARGET) $(KERNEL_OBJS)

start_main.o: src/apps/start_main.c
	$(CC) $(CFLAGS) -c $< -o $@

# 5. 链接用户 ELF
$(USER_ELF): $(SHELL_OBJ) $(USER_LIB_OBJS)
	@echo "--- 正在生成独立 ELF: $(USER_ELF) ---"
	$(CC) $(LDFLAGS_USER) -o $(USER_ELF) $(SHELL_OBJ) $(USER_LIB_OBJS)

$(HELLO_ELF): $(HELLO_OBJS) $(MYLIBC_OBJS)
	$(CC) $(LDFLAGS_USER) -o $(HELLO_ELF) $(HELLO_OBJS) $(MYLIBC_OBJS)

# ── 新增：ping.elf 链接规则 ─────────────────────────────────────────
$(PING_ELF): $(PING_OBJS) $(USER_LIB_OBJS)
	@echo "--- 正在生成独立 ELF: $(PING_ELF) ---"
	$(CC) $(LDFLAGS_USER) -o $(PING_ELF) $(PING_OBJS) $(USER_LIB_OBJS)

# 6. 自动模式规则
$(BINDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.s
	@mkdir -p $(dir $@)
	$(AS) -f elf32 -g -F dwarf $< -o $@

# 7. 磁盘镜像（依赖加入 ping.elf，镜像内容加入 ping.elf）
$(DISK_IMG): $(USER_ELF) $(HELLO_ELF) $(PING_ELF) start_main.o
	@echo "--- 正在生成并格式化磁盘镜像 ---"
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=32
	mkfs.fat -F 16 -s 1 $(DISK_IMG)
	@echo "--- 拷贝 ELF 文件 ---"
	mcopy -i $(DISK_IMG) $(USER_ELF)  ::/shell.elf
	mcopy -i $(DISK_IMG) $(HELLO_ELF) ::/hello.elf
	mcopy -i $(DISK_IMG) $(PING_ELF)  ::/ping.elf
	mcopy -i $(DISK_IMG) $(TCC_ELF)   ::/tcc.elf
	@echo "--- 创建 /include 目录并拷贝头文件 ---"
	mmd -i $(DISK_IMG) ::/include
	mmd -i $(DISK_IMG) ::/include/lib
	mcopy -i $(DISK_IMG) $(INCDIR)/lib/mylibc.h  ::/include/lib/mylibc.h
	mcopy -i $(DISK_IMG) $(INCDIR)/lib/unistd_.h ::/include/lib/unistd_.h
	mcopy -i $(DISK_IMG) src/apps/hello.c         ::/hello.c
	mcopy -i $(DISK_IMG) tinycc/include/tccdefs.h ::/include/tccdefs.h
	nasm -f elf32 src/apps/ustart.s -o ustart.o
	mcopy -i $(DISK_IMG) ustart.o      ::/ustart.o
	mcopy -i $(DISK_IMG) start_main.o  ::/smain.o
	mcopy -i $(DISK_IMG) $(BINDIR)/lib/mylibc.o       ::/mylibc.o
	mcopy -i $(DISK_IMG) $(BINDIR)/lib/syscall_arch.o ::/sysarch.o
	@echo "--- 检查镜像内容 ---"
	mdir -i $(DISK_IMG) ::/

# 8. 运行（新增 -netdev/-device 启用 RTL8139 虚拟网卡）
run: all
	qemu-system-i386 -kernel $(TARGET) \
    -drive file=$(DISK_IMG),format=raw,index=0,media=disk \
    -netdev user,id=net0 \
    -device rtl8139,netdev=net0 \
    -append "nokaslr" \
    -d int \
    -D qemu.log

debug: all
	@echo "--- 正在启动 QEMU 调试模式 ---"
	qemu-system-i386 -S -s -kernel $(TARGET) \
		-drive file=$(DISK_IMG),format=raw,index=0,media=disk \
		-netdev user,id=net0 \
		-device rtl8139,netdev=net0 \
		-d int,cpu_reset -D qemu.log -monitor stdio

clean:
	rm -rf $(BINDIR) $(TARGET) $(USER_ELF) $(HELLO_ELF) $(PING_ELF) $(DISK_IMG) qemu.log ustart.o start_main.o