#include<kernel/gdt.h>


//定义6个条目：空条目、内核代码段、内核数据段、用户代码段、用户数据段、TSS段
gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;
// 在汇编中定义的函数，用来真正刷新 GDT
extern void gdt_flush(uint32_t);

// 设置一个 GDT 条目
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

// 加载 TSS 的汇编指令（让 CPU 知道 TSS 描述符在 GDT 的第几项）
extern void tss_flush(); 
void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    // 修正 1: limit 是长度减 1，不要加 base
    uint32_t limit = sizeof(tss_entry) - 1; 

    // 初始化 TSS 内容 (清零)
    for(uint32_t i = 0; i < sizeof(tss_entry); i++) {
        ((uint8_t*)&tss_entry)[i] = 0;
    }

    // 修正 2: Access Byte 改为 0x89 (Present, DPL 0, Type: 32-bit TSS Available)
    gdt_set_gate(num, base, limit, 0x89, 0x00);

    tss_entry.ss0  = ss0;  // 0x10
    tss_entry.esp0 = esp0; // 初始内核栈顶   TSS始终存放当前进程的内核栈
    
    // 修正 3: iomap_base 必须指向 TSS 之外，表示没有 IO 位图
    tss_entry.iomap_base = sizeof(tss_entry); 
}
// 初始化 GDT
void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // 空条目
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // 内核代码段
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // 内核数据段
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // 用户代码段
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // 用户数据段

    // TSS 初始化（传入栈顶）
    // 注意：这里的 0x10 是内核数据段
    extern uint32_t stack_top;
    write_tss(5, 0x10, (uint32_t)&stack_top);

    gdt_flush((uint32_t)&gdt_ptr);//栈内先压4B参数，后压返回地址
    tss_flush(); // 告诉 CPU TSS 在 0x28 位置
}

// 更新内核栈顶的接口，供调度器在切换任务时调用
void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}