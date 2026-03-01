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
# 注意：C 编译选项保持一致
CFLAGS = -std=gnu99 -ffreestanding -g -O2 -Wall -Wextra -I$(INCDIR)
# 内核链接选项：需要 linker.ld
LDFLAGS_KERN = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc -g
# 用户态链接选项：需要 user.ld
LDFLAGS_USER = -T user.ld -ffreestanding -O2 -nostdlib -lgcc -g

# 3. 目标文件自动寻找逻辑
# 内核 C 文件：排除 src/apps 目录下的所有文件 
KERNEL_CSOURCES = $(shell find $(SRCDIR) -path "$(SRCDIR)/apps" -prune -o -name '*.c' -print) 
# 内核汇编文件 
KERNEL_ASOURCES = $(shell find $(SRCDIR) -path "$(SRCDIR)/apps" -prune -o -name '*.s' -print) 
KERNEL_OBJS = $(KERNEL_CSOURCES:$(SRCDIR)/%.c=$(BINDIR)/%.o) $(KERNEL_ASOURCES:$(SRCDIR)/%.s=$(BINDIR)/%.o) 

# 用户态 Shell 文件及其依赖库
# 注意：Shell 必须链接你写好的 stdio, stdlib 等 
USER_LIB_OBJS = $(BINDIR)/lib/stdio.o $(BINDIR)/lib/stdlib.o $(BINDIR)/lib/string.o $(BINDIR)/lib/syscall_arch.o
SHELL_OBJ = $(BINDIR)/apps/shell.o $(BINDIR)/apps/start.o

all: $(TARGET) $(USER_ELF) $(DISK_IMG) 

# 4. 链接内核
$(TARGET): $(KERNEL_OBJS)
	@echo "--- 正在链接内核 ---"
	$(CC) $(LDFLAGS_KERN) -o $(TARGET) $(KERNEL_OBJS)

# 5. 链接 Shell ELF (关键：剥离出来)
$(USER_ELF): $(SHELL_OBJ) $(USER_LIB_OBJS)
	@echo "--- 正在生成独立 ELF: $(USER_ELF) ---"
	$(CC) $(LDFLAGS_USER) -o $(USER_ELF) $(SHELL_OBJ) $(USER_LIB_OBJS)

# 6. 自动模式规则
$(BINDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.s
	@mkdir -p $(dir $@)
	$(AS) -f elf32 -g -F dwarf $< -o $@

# 7. 磁盘镜像逻辑：包含自动拷贝 shell.elf
$(DISK_IMG): $(USER_ELF)
	@echo "--- 正在生成并格式化磁盘镜像 ---"
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=10
	mkfs.fat -F 16 -s 1 $(DISK_IMG)
	@echo "--- 正在拷贝 $(USER_ELF) 到镜像 ---"
	mcopy -i $(DISK_IMG) $(USER_ELF) ::/shell.elf
	@echo "--- 检查镜像内容 ---"
	mdir -i $(DISK_IMG) ::/

# 8. 运行与调试命令 (补全 debug)
run: all
	qemu-system-i386 -kernel $(TARGET) \
    -drive file=$(DISK_IMG),format=raw,index=0,media=disk \
    -append "nokaslr" \
    -d int \
    -D qemu.log

debug: all
	@echo "--- 正在启动 QEMU 调试模式 ---"
	qemu-system-i386 -S -s -kernel $(TARGET) \
		-drive file=$(DISK_IMG),format=raw,index=0,media=disk \
		-d int,cpu_reset -D qemu.log -monitor stdio 

clean:
	rm -rf $(BINDIR) $(TARGET) $(USER_ELF) $(DISK_IMG) qemu.log