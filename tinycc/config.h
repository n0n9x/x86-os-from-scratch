/* config.h - 手动为 myos 移植版生成，替代 ./configure 的输出 */
#ifndef CONFIG_H
#define CONFIG_H

#define TCC_VERSION         "0.9.27-myos"
#define TCC_TARGET_I386     1

/* 路径配置：TCC 在 OS 里查找头文件和库的位置 */
#define CONFIG_SYSROOT          ""
#define CONFIG_TCCDIR           "/tcc"
#define CONFIG_TCC_SYSINCLUDEPATHS "/include"
#define CONFIG_TCC_LIBPATHS     "/lib"
#define CONFIG_TCC_CRT_PREFIX   "/lib"
#define CONFIG_TCC_ELFINTERP    ""

/* 禁用不需要的功能 */
#define CONFIG_TCC_STATIC       1   /* 不使用 dlopen/dlsym */
#define CONFIG_TCC_SEMLOCK      0   /* 不需要线程锁 */
#define CONFIG_TCC_BACKTRACE    0   /* 不需要 backtrace */
#define CONFIG_TCC_BCHECK       0   /* 不需要边界检查 */

/* 禁用其他架构 */
#undef TCC_TARGET_X86_64
#undef TCC_TARGET_ARM
#undef TCC_TARGET_ARM64
#undef TCC_TARGET_RISCV64
#undef TCC_TARGET_C67

#endif /* CONFIG_H */