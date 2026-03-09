#include <drivers/keyboard.h>
#include <drivers/io.h>
#include <kernel/task.h>
#include <drivers/terminal.h>

unsigned char kbd_us[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0};

unsigned char kbd_us_shift[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0};

#define KBD_BUF_SIZE 512
static char kbd_buffer[KBD_BUF_SIZE];
static int kbd_head = 0, kbd_tail = 0;
static int shift_active = 0;
static int ctrl_active  = 0;

// 行缓冲区，用于存放尚未按回车的字符
static char line_buffer[KBD_BUF_SIZE];
static int line_pos = 0;

// 内部函数：将字符放入缓冲区
static void kbd_enqueue(char c)
{
    int next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next != kbd_tail)
    {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

// 检查缓冲区是否为空
int kbd_is_empty()
{
    return (kbd_head == kbd_tail);
}

// 供内核调用的读取接口 (保持不变)
char kbd_get_char()
{
    if (kbd_head == kbd_tail)
        return 0;
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return c;
}

// 核心：处理从端口读取的原始扫描码
void keyboard_handler_main()
{
    uint8_t scancode = inb(0x60);

    // 处理扩展扫描码
    if (scancode == 0xE0)
        return;

    // 处理 Shift 键状态机
    switch (scancode)
    {
    case 0x2A: // 左 Shift 按下
    case 0x36: // 右 Shift 按下
        shift_active = 1;
        break;
    case 0xAA: // 左 Shift 松开
    case 0xB6: // 右 Shift 松开
        shift_active = 0;
        break;
    case 0x1D: // 左 Ctrl 按下
        ctrl_active = 1;
        break;
    case 0x9D: // 左 Ctrl 松开
        ctrl_active = 0;
        break;
    default:
        // 过滤掉其他按键的松开事件 (Break Code)
        if (scancode & 0x80)
            break;

        // Ctrl+C：发送 0x03，立即唤醒进程
        if (ctrl_active && scancode == 0x2E) // 'c' 的扫描码
        {
            line_pos = 0;
            kbd_enqueue(0x03);
            task_wakeup(&wait_queue);
            break;
        }

        // 处理按下事件 (Make Code)
        if (scancode < 128)
        {
            unsigned char *current_table = shift_active ? kbd_us_shift : kbd_us;
            unsigned char c = current_table[scancode];
            if (c == '\n')
            {
                // 1. 用户按下回车，将行缓冲区内容全部转入正式缓冲区
                for (int i = 0; i < line_pos; i++)
                {
                    kbd_enqueue(line_buffer[i]);
                }
                kbd_enqueue('\n'); // 放入回车符
                line_pos = 0;      // 清空行缓冲区

                terminal_putchar('\n'); // 屏幕换行

                // 2. 【关键】只有此时才唤醒等待的进程
                task_wakeup(&wait_queue);
            }
            else if (c == '\b')
            {
                // 处理退格：从行缓冲区删除，并让屏幕回删
                if (line_pos > 0)
                {
                    line_pos--;
                    terminal_putchar('\b'); // 触发 terminal.c 的擦除逻辑
                }
            }
            else
            {
                // 普通字符：暂存到行缓冲区并回显，但不唤醒进程
                if (line_pos < KBD_BUF_SIZE - 1)
                {
                    line_buffer[line_pos++] = c;
                    terminal_putchar(c);
                }
            }
        }
        break;
    }
}