#include <drivers/terminal.h>
#include <drivers/io.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
// 统一使用这个常量作为显存基地址
static uint16_t *VGA_BUFFER = (uint16_t *)0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

// 1. 提前声明函数，防止隐式声明导致的错误
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
void update_cursor(size_t x, size_t y);
static void terminal_scroll(void);
// 生成颜色单元，一个字节
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}
// 生成显存单元 两个字节
static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t)uc | (uint16_t)color << 8;
}
// 初始化
void terminal_initialize(void)
{
    VGA_BUFFER = (uint16_t *)VIDEO_VIRT_ADDR;
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_clear();
    update_cursor(0, 0); // 初始化光标位置
}
// 清屏
void terminal_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = vga_entry(' ', terminal_color);
        }
    }
}
// 屏幕滚动
static void terminal_scroll(void)
{
    // 上移一行
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t src_index = (y + 1) * VGA_WIDTH + x;
            const size_t dest_index = y * VGA_WIDTH + x;
            VGA_BUFFER[dest_index] = VGA_BUFFER[src_index];
        }
    }
    // 清空最后一行
    for (size_t x = 0; x < VGA_WIDTH; x++)
    {
        terminal_putentryat(' ', terminal_color, x, VGA_HEIGHT - 1);
    }
}
// 显存写入
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    VGA_BUFFER[index] = vga_entry(c, color); // 直接操作显存地址
}
// 打印一个字符
void terminal_putchar(char c)
{
    uint32_t eflags;

    // 1. 保存当前 EFLAGS 状态并关闭中断 (cli)
    // 这样如果函数在中断处理程序中被调用，退出时依然会保持关中断状态
    asm volatile(
        "pushfl      \n\t"
        "pop %0      \n\t"
        "cli         "
        : "=rm"(eflags)
        :
        : "memory");

    // --- 临界区开始：保护全局变量 terminal_column 和 terminal_row ---
    if (c == '\b')
    {
        if (terminal_column > 0)
        {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
            update_cursor(terminal_column, terminal_row);
        }
    }
    else if (c == '\n')
    {
        terminal_column = 0;
        terminal_row++;
    }
    else
    {
        terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
        if (++terminal_column == VGA_WIDTH)
        {
            terminal_column = 0;
            terminal_row++;
        }
    }

    if (terminal_row >= VGA_HEIGHT)
    {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }

    update_cursor(terminal_column, terminal_row);
    // --- 临界区结束 ---

    // 2. 恢复进入函数前的 EFLAGS 状态
    // 如果进入前是开中断(sti)，则现在恢复开中断；如果是关中断(cli)，则继续保持关中断
    asm volatile(
        "push %0     \n\t"
        "popfl       "
        :
        : "rm"(eflags)
        : "memory");
}

void terminal_writestring(const char *data)
{
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}
// 更新光标
void update_cursor(size_t x, size_t y)
{
    // 保护：防止坐标超出屏幕范围导致光标消失
    if (x >= VGA_WIDTH)
        x = VGA_WIDTH - 1;
    if (y >= VGA_HEIGHT)
        y = VGA_HEIGHT - 1;

    uint16_t pos = y * VGA_WIDTH + x;

    // 通知 VGA 控制器更新光标位置
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
}

// 十六进制打印
void terminal_write_hex(uint32_t n)
{
    char *hex = "0123456789ABCDEF";
    terminal_writestring("0x");
    for (int i = 7; i >= 0; i--)
    {
        terminal_putchar(hex[(n >> (i * 4)) & 0xF]);
    }
}
// 十进制打印
void terminal_write_dec(uint32_t n)
{
    if (n == 0)
    {
        terminal_putchar('0');
        return;
    }

    char buf[11];
    int i = 10;
    buf[i--] = '\0';

    while (n > 0 && i >= 0)
    {
        buf[i--] = (n % 10) + '0';
        n /= 10;
    }

    terminal_writestring(&buf[i + 1]);
}