#!/bin/bash
# build_tcc.sh - 放在 tinycc/ 目录下运行
# 用法: bash build_tcc.sh

CC=/home/N0ne/opt/cross/bin/i686-elf-gcc
LD=/home/N0ne/opt/cross/bin/i686-elf-gcc
MYOS=..   # 项目根目录

CFLAGS="\
  -ffreestanding \
  -O1 \
  -nostdlib \
  -fno-builtin \
  -m32 \
  -std=c99 \
  -isystem /home/N0ne/opt/cross/lib/gcc/i686-elf/12.2.0/include \
  -isystem /home/N0ne/opt/cross/i686-elf/include \
  -I./fake_inc \
  -DONE_SOURCE=1 \
  -DTCC_TARGET_I386 \
  -DCONFIG_TCC_STATIC \
  -DCONFIG_TCC_SEMLOCK=0 \
  -DTCC_VERSION='\"myos-port\"' \
  -DCONFIG_SYSROOT='\"\"' \
  -DCONFIG_TCC_SYSINCLUDEPATHS='\"/include\"' \
  -DCONFIG_TCC_LIBPATHS='\"/lib\"' \
  -DCONFIG_TCC_CRT_PREFIX='\"/lib\"' \
  -DCONFIG_TCC_ELFINTERP='\"\"' \
  -include myos_port.h \
  -U__i386__ \
  -I. \
  -I${MYOS}/include \
  -w"

# user.ld 链接脚本（和 shell.elf 一样的用户态布局）
LDFLAGS="\
  -T ${MYOS}/user.ld \
  -ffreestanding \
  -nostdlib \
  -lgcc"

echo "=== 编译 tcc.c ==="
$CC $CFLAGS -c tcc.c -o tcc.o
if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

echo "=== 编译 mylibc ==="
$CC $CFLAGS -std=gnu99 -c ${MYOS}/src/lib/mylibc.c -o mylibc.o
if [ $? -ne 0 ]; then
    echo "mylibc 编译失败"
    exit 1
fi

echo "=== 编译 syscall_arch ==="
$CC $CFLAGS -std=gnu99 -c ${MYOS}/src/lib/syscall_arch.c -o syscall_arch.o
if [ $? -ne 0 ]; then
    echo "syscall_arch 编译失败"
    exit 1
fi

echo "=== 编译 ustart.s ==="
nasm -f elf32 ${MYOS}/src/apps/ustart.s -o ustart.o
if [ $? -ne 0 ]; then
    echo "ustart 编译失败"
    exit 1
fi

echo "=== 编译 tcc_entry ==="
$CC $CFLAGS -std=gnu99 -c tcc_entry.c -o tcc_entry.o
if [ $? -ne 0 ]; then
    echo "tcc_entry 编译失败"
    exit 1
fi

echo "=== 链接 tcc.elf ==="
$LD -T ${MYOS}/user.ld -ffreestanding -nostdlib \
    ustart.o tcc_entry.o tcc.o mylibc.o syscall_arch.o \
    -lgcc -o tcc.elf
if [ $? -ne 0 ]; then
    echo "链接失败"
    exit 1
fi

echo "=== 成功：tcc.elf ==="
ls -lh tcc.elf