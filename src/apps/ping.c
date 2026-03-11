#include <lib/unistd_.h>
#include <lib/stdlib.h>
#include <lib/stdio.h>

#define SYS_PING 16

static int do_ping(uint32_t ip, int count)
{
    return (int)syscall_2(SYS_PING, ip, (uint32_t)count, 0);
}

static uint32_t parse_ip(const char *s)
{
    uint32_t result = 0;
    int shift = 0;
    uint32_t octet = 0;
    for (int i = 0; ; i++) {
        char c = s[i];
        if (c >= '0' && c <= '9') {
            octet = octet * 10 + (uint32_t)(c - '0');
        } else if (c == '.' || c == '\0') {
            result |= (octet & 0xFF) << shift;
            shift += 8;
            octet = 0;
            if (c == '\0') break;
        } else {
            return 0;
        }
    }
    return result;
}

static int simple_atoi(const char *s)
{
    int n = 0;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return n;
}

void _start_main(int argc, char **argv)
{
    if (argc < 2) {
        printf_simple("Usage: ping <IP> [count]\n");
        user_task_exit();
        return;
    }

    uint32_t ip = parse_ip(argv[1]);
    if (ip == 0) {
        printf_simple("ping: invalid IP address\n");
        user_task_exit();
        return;
    }

    int count = 4;
    if (argc >= 3) {
        count = simple_atoi(argv[2]);
        if (count <= 0 || count > 100) count = 4;
    }

    do_ping(ip, count);
    user_task_exit();
}