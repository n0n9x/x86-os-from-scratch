#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_MAGIC 0x464C457F  // "\x7FELF" 的小端表示

// ELF Header
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;     // 程序入口虚拟地址
    uint32_t e_phoff;     // 程序头表偏移
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;     // 程序头表条目数
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

// Program Header
typedef struct {
    uint32_t p_type;      // 段类型 (1 = LOAD)
    uint32_t p_offset;    // 段在文件中的偏移
    uint32_t p_vaddr;     // 段在虚拟内存中的地址
    uint32_t p_paddr;
    uint32_t p_filesz;    // 段在文件中的大小
    uint32_t p_memsz;     // 段在内存中的大小 (可能大于 filesz，如 .bss)
    uint32_t p_flags;     // 权限 (1=X, 2=W, 4=R)
    uint32_t p_align;
} __attribute__((packed)) elf_phdr_t;

#define PT_LOAD 1

// 加载 ELF 并运行的函数声明
int load_elf(const char *filename);

#endif