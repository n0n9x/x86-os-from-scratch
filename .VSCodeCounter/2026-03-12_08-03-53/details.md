# Details

Date : 2026-03-12 08:03:53

Directory c:\\msys64\\home\\N0ne\\myos

Total : 110 files,  37001 codes, 4309 comments, 4026 blanks, all 45336 lines

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)

## Files
| filename | language | code | comment | blank | total |
| :--- | :--- | ---: | ---: | ---: | ---: |
| [Makefile](/Makefile) | Makefile | 83 | 14 | 19 | 116 |
| [README.md](/README.md) | Markdown | 225 | 0 | 45 | 270 |
| [include/drivers/ide.h](/include/drivers/ide.h) | C++ | 25 | 4 | 7 | 36 |
| [include/drivers/io.h](/include/drivers/io.h) | C++ | 23 | 5 | 3 | 31 |
| [include/drivers/keyboard.h](/include/drivers/keyboard.h) | C++ | 8 | 1 | 6 | 15 |
| [include/drivers/pic.h](/include/drivers/pic.h) | C++ | 12 | 10 | 6 | 28 |
| [include/drivers/rtl8139.h](/include/drivers/rtl8139.h) | C++ | 38 | 9 | 11 | 58 |
| [include/drivers/terminal.h](/include/drivers/terminal.h) | C++ | 31 | 1 | 5 | 37 |
| [include/fs/fat.h](/include/fs/fat.h) | C++ | 60 | 12 | 12 | 84 |
| [include/kernel/elf.h](/include/kernel/elf.h) | C++ | 34 | 3 | 7 | 44 |
| [include/kernel/gdt.h](/include/kernel/gdt.h) | C++ | 55 | 3 | 7 | 65 |
| [include/kernel/idt.h](/include/kernel/idt.h) | C++ | 12 | 1 | 6 | 19 |
| [include/kernel/syscall.h](/include/kernel/syscall.h) | C++ | 24 | 1 | 4 | 29 |
| [include/kernel/task.h](/include/kernel/task.h) | C++ | 49 | 3 | 9 | 61 |
| [include/kernel/timer.h](/include/kernel/timer.h) | C++ | 6 | 0 | 3 | 9 |
| [include/lib/mylibc.h](/include/lib/mylibc.h) | C++ | 108 | 13 | 20 | 141 |
| [include/lib/stdio.h](/include/lib/stdio.h) | C++ | 6 | 0 | 3 | 9 |
| [include/lib/stdlib.h](/include/lib/stdlib.h) | C++ | 17 | 0 | 8 | 25 |
| [include/lib/string.h](/include/lib/string.h) | C++ | 14 | 0 | 2 | 16 |
| [include/lib/unistd\_.h](/include/lib/unistd_.h) | C++ | 37 | 1 | 4 | 42 |
| [include/mm/kheap.h](/include/mm/kheap.h) | C++ | 17 | 3 | 5 | 25 |
| [include/mm/pmm.h](/include/mm/pmm.h) | C++ | 12 | 1 | 5 | 18 |
| [include/mm/vmm.h](/include/mm/vmm.h) | C++ | 20 | 4 | 8 | 32 |
| [include/net/net.h](/include/net/net.h) | C++ | 78 | 32 | 23 | 133 |
| [isodir/boot/grub/grub.cfg](/isodir/boot/grub/grub.cfg) | Properties | 5 | 0 | 0 | 5 |
| [src/apps/crt0.c](/src/apps/crt0.c) | C | 18 | 0 | 4 | 22 |
| [src/apps/hello.c](/src/apps/hello.c) | C | 66 | 14 | 16 | 96 |
| [src/apps/ping.c](/src/apps/ping.c) | C | 55 | 0 | 8 | 63 |
| [src/apps/shell.c](/src/apps/shell.c) | C | 370 | 19 | 17 | 406 |
| [src/apps/start.s](/src/apps/start.s) | nasm | 7 | 0 | 0 | 7 |
| [src/apps/ustart.s](/src/apps/ustart.s) | nasm | 14 | 8 | 5 | 27 |
| [src/boot/boot.s](/src/boot/boot.s) | nasm | 174 | 43 | 29 | 246 |
| [src/boot/switch.s](/src/boot/switch.s) | nasm | 29 | 7 | 6 | 42 |
| [src/drivers/ide.c](/src/drivers/ide.c) | C | 82 | 37 | 31 | 150 |
| [src/drivers/keyboard.c](/src/drivers/keyboard.c) | C | 108 | 14 | 14 | 136 |
| [src/drivers/pic.c](/src/drivers/pic.c) | C | 32 | 7 | 8 | 47 |
| [src/drivers/rlt8139.c](/src/drivers/rlt8139.c) | C | 180 | 44 | 42 | 266 |
| [src/drivers/terminal.c](/src/drivers/terminal.c) | C | 149 | 22 | 15 | 186 |
| [src/fs/fat.c](/src/fs/fat.c) | C | 595 | 77 | 81 | 753 |
| [src/kernel/elf.c](/src/kernel/elf.c) | C | 112 | 20 | 23 | 155 |
| [src/kernel/gdt.c](/src/kernel/gdt.c) | C | 42 | 12 | 14 | 68 |
| [src/kernel/idt.c](/src/kernel/idt.c) | C | 106 | 10 | 20 | 136 |
| [src/kernel/irq.c](/src/kernel/irq.c) | C | 38 | 4 | 7 | 49 |
| [src/kernel/isr.c](/src/kernel/isr.c) | C | 88 | 8 | 9 | 105 |
| [src/kernel/kernel.c](/src/kernel/kernel.c) | C | 47 | 1 | 10 | 58 |
| [src/kernel/syscall.c](/src/kernel/syscall.c) | C | 297 | 40 | 44 | 381 |
| [src/kernel/task.c](/src/kernel/task.c) | C | 183 | 27 | 50 | 260 |
| [src/kernel/timer.c](/src/kernel/timer.c) | C | 12 | 1 | 3 | 16 |
| [src/lib/mylibc.c](/src/lib/mylibc.c) | C | 608 | 72 | 99 | 779 |
| [src/lib/stdio.c](/src/lib/stdio.c) | C | 15 | 3 | 3 | 21 |
| [src/lib/stdlib.c](/src/lib/stdlib.c) | C | 40 | 10 | 13 | 63 |
| [src/lib/string.c](/src/lib/string.c) | C | 69 | 8 | 13 | 90 |
| [src/lib/syscall\_arch.c](/src/lib/syscall_arch.c) | C | 38 | 1 | 5 | 44 |
| [src/mm/kheap.c](/src/mm/kheap.c) | C | 80 | 8 | 11 | 99 |
| [src/mm/pmm.c](/src/mm/pmm.c) | C | 35 | 4 | 6 | 45 |
| [src/mm/vmm.c](/src/mm/vmm.c) | C | 116 | 28 | 26 | 170 |
| [src/net/net.c](/src/net/net.c) | C | 314 | 68 | 71 | 453 |
| [tinycc/build\_tcc.sh](/tinycc/build_tcc.sh) | Shell Script | 73 | 4 | 10 | 87 |
| [tinycc/config.h](/tinycc/config.h) | C++ | 20 | 4 | 5 | 29 |
| [tinycc/dwarf.h](/tinycc/dwarf.h) | C++ | 837 | 118 | 92 | 1,047 |
| [tinycc/elf.h](/tinycc/elf.h) | C++ | 2,577 | 386 | 362 | 3,325 |
| [tinycc/fake\_inc/assert.h](/tinycc/fake_inc/assert.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/ctype.h](/tinycc/fake_inc/ctype.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/errno.h](/tinycc/fake_inc/errno.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/fcntl.h](/tinycc/fake_inc/fcntl.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/inttypes.h](/tinycc/fake_inc/inttypes.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/math.h](/tinycc/fake_inc/math.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/setjmp.h](/tinycc/fake_inc/setjmp.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/signal.h](/tinycc/fake_inc/signal.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/stdbool.h](/tinycc/fake_inc/stdbool.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/stddef.h](/tinycc/fake_inc/stddef.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/stdio.h](/tinycc/fake_inc/stdio.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/stdlib.h](/tinycc/fake_inc/stdlib.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/string.h](/tinycc/fake_inc/string.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/sys/mman.h](/tinycc/fake_inc/sys/mman.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/sys/stat.h](/tinycc/fake_inc/sys/stat.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/sys/time.h](/tinycc/fake_inc/sys/time.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/sys/types.h](/tinycc/fake_inc/sys/types.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/fake\_inc/time.h](/tinycc/fake_inc/time.h) | C++ | 24 | 1 | 5 | 30 |
| [tinycc/fake\_inc/unistd.h](/tinycc/fake_inc/unistd.h) | C++ | 0 | 1 | 1 | 2 |
| [tinycc/i386-asm.c](/tinycc/i386-asm.c) | C | 1,496 | 182 | 80 | 1,758 |
| [tinycc/i386-asm.h](/tinycc/i386-asm.h) | C++ | 410 | 19 | 62 | 491 |
| [tinycc/i386-gen.c](/tinycc/i386-gen.c) | C | 1,075 | 131 | 101 | 1,307 |
| [tinycc/i386-link.c](/tinycc/i386-link.c) | C | 262 | 35 | 33 | 330 |
| [tinycc/i386-tok.h](/tinycc/i386-tok.h) | C++ | 289 | 14 | 30 | 333 |
| [tinycc/include/float.h](/tinycc/include/float.h) | C++ | 57 | 8 | 11 | 76 |
| [tinycc/include/stdalign.h](/tinycc/include/stdalign.h) | C++ | 11 | 0 | 6 | 17 |
| [tinycc/include/stdarg.h](/tinycc/include/stdarg.h) | C++ | 10 | 1 | 4 | 15 |
| [tinycc/include/stdatomic.h](/tinycc/include/stdatomic.h) | C++ | 139 | 13 | 20 | 172 |
| [tinycc/include/stdbool.h](/tinycc/include/stdbool.h) | C++ | 7 | 1 | 4 | 12 |
| [tinycc/include/stddef.h](/tinycc/include/stddef.h) | C++ | 25 | 7 | 8 | 40 |
| [tinycc/include/stdnoreturn.h](/tinycc/include/stdnoreturn.h) | C++ | 4 | 1 | 3 | 8 |
| [tinycc/include/tccdefs.h](/tinycc/include/tccdefs.h) | C++ | 305 | 38 | 35 | 378 |
| [tinycc/include/tgmath.h](/tinycc/include/tgmath.h) | C++ | 71 | 12 | 7 | 90 |
| [tinycc/include/varargs.h](/tinycc/include/varargs.h) | C++ | 5 | 5 | 3 | 13 |
| [tinycc/libtcc.c](/tinycc/libtcc.c) | C | 1,925 | 168 | 180 | 2,273 |
| [tinycc/myos\_port.h](/tinycc/myos_port.h) | C++ | 219 | 46 | 38 | 303 |
| [tinycc/stab.h](/tinycc/stab.h) | C++ | 10 | 1 | 7 | 18 |
| [tinycc/tcc.c](/tinycc/tcc.c) | C | 382 | 23 | 24 | 429 |
| [tinycc/tcc.h](/tinycc/tcc.h) | C++ | 1,598 | 195 | 224 | 2,017 |
| [tinycc/tcc\_entry.c](/tinycc/tcc_entry.c) | C | 5 | 0 | 1 | 6 |
| [tinycc/tccasm.c](/tinycc/tccasm.c) | C | 1,255 | 126 | 86 | 1,467 |
| [tinycc/tccdbg.c](/tinycc/tccdbg.c) | C | 2,395 | 94 | 188 | 2,677 |
| [tinycc/tccelf.c](/tinycc/tccelf.c) | C | 3,354 | 433 | 330 | 4,117 |
| [tinycc/tccgen.c](/tinycc/tccgen.c) | C | 7,312 | 968 | 641 | 8,921 |
| [tinycc/tcclib.h](/tinycc/tcclib.h) | C++ | 61 | 10 | 10 | 81 |
| [tinycc/tccpp.c](/tinycc/tccpp.c) | C | 3,458 | 278 | 270 | 4,006 |
| [tinycc/tccrun.c](/tinycc/tccrun.c) | C | 1,338 | 112 | 107 | 1,557 |
| [tinycc/tcctok.h](/tinycc/tcctok.h) | C++ | 387 | 17 | 27 | 431 |
| [tinycc/tcctools.c](/tinycc/tcctools.c) | C | 487 | 102 | 63 | 652 |

[Summary](results.md) / Details / [Diff Summary](diff.md) / [Diff Details](diff-details.md)