#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <lib/unistd_.h>

// 通过 SYS_EXEC 系统调用让内核加载并执行一个 ELF 文件，支持参数传递
static int exec_elf(const char *path, char **argv, int argc)
{
    uint32_t ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_EXEC), "b"(path), "c"(argv), "d"(argc));
    return (int)ret;
}

#define MAX_PATH 256
char c_path[MAX_PATH] = "/";

void merge_path(const char *input, char *output)
{
    if (input[0] == '/')
        strcpy(output, input);
    else
    {
        strcpy(output, c_path);
        if (strcmp(c_path, "/") != 0)
            strcat(output, "/");
        strcat(output, input);
    }
}

static void unescape(char *s)
{
    char *r = s, *w = s;
    while (*r)
    {
        if (*r == '\\' && *(r + 1) == 'n')
        {
            *w++ = '\n';
            r += 2;
        }
        else if (*r == '\\' && *(r + 1) == 't')
        {
            *w++ = '\t';
            r += 2;
        }
        else
        {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

void shell_main()
{
    char cmd_line[256];
    char arg_path[MAX_PATH];

    printf_simple("----------welcome to Shell!----------\n");

    while (1)
    {
        printf_simple(c_path);
        printf_simple("> ");

        memset(cmd_line, 0, 256);
        scanf_simple(cmd_line, 255);

        if (strlen(cmd_line) == 0)
            continue;

        // ls
        if (strncmp(cmd_line, "ls", 2) == 0)
        {
            char *p = cmd_line + 2;
            while (*p == ' ')
                p++;
            if (*p == '\0')
                flist(c_path);
            else
            {
                merge_path(p, arg_path);
                flist(arg_path);
            }
        }
        // cd
        else if (strncmp(cmd_line, "cd", 2) == 0)
        {
            char *p = cmd_line + 2;
            while (*p == ' ')
                p++;
            if (strcmp(p, "..") == 0)
            {
                if (strcmp(c_path, "/") != 0)
                {
                    char *s = strrchr(c_path, '/');
                    if (s == c_path)
                        *(s + 1) = '\0';
                    else
                        *s = '\0';
                }
            }
            else
            {
                merge_path(p, arg_path);
                if (open(arg_path) != -1)
                    strcpy(c_path, arg_path);
                else
                    printf_simple("Error: Directory not found.\n");
            }
        }
        // cat
        else if (strncmp(cmd_line, "cat", 3) == 0)
        {
            char *p = cmd_line + 3;
            while (*p == ' ')
                p++;
            merge_path(p, arg_path);
            uint8_t buf[512];
            memset(buf, 0, 512);
            if (fread(arg_path, buf, 511) >= 0)
            {
                printf_simple((char *)buf);
                printf_simple("\n");
            }
            else
                printf_simple("Error: Cannot read file.\n");
        }
        // mkdir
        else if (strncmp(cmd_line, "mkdir", 5) == 0)
        {
            char *p = cmd_line + 5;
            while (*p == ' ')
                p++;
            if (*p == '\0')
                printf_simple("Usage: mkdir <dirname>\n");
            else
            {
                merge_path(p, arg_path);
                if (mkdir(arg_path) == 0)
                    printf_simple("Directory created.\n");
                else
                    printf_simple("Error: Failed to create directory.\n");
            }
        }
        // write
        else if (strncmp(cmd_line, "write", 5) == 0)
        {
            char *f = cmd_line + 5;
            while (*f == ' ')
                f++;
            if (*f == '\0')
            {
                printf_simple("Usage: write <file>\n");
            }
            else
            {
                merge_path(f, arg_path);
                // 先删除再创建，清空文件
                fdelete(arg_path);
                fcreate(arg_path);
                printf_simple("Editing ");
                printf_simple(arg_path);
                printf_simple(" (Ctrl+C to save and exit)\n");

                char line[256];
                while (1)
                {
                    memset(line, 0, 256);
                    scanf_simple(line, 255);

                    // Ctrl+C (0x03) 结束编辑
                    if (line[0] == 0x03)
                    {
                        printf_simple("[Saved]\n");
                        break;
                    }

                    // 追加这一行（加换行符）
                    int len = strlen(line);
                    fappend(arg_path, (uint8_t *)line, len);
                    fappend(arg_path, (uint8_t *)"\n", 1);
                }
            }
        }
        // append
        else if (strncmp(cmd_line, "append", 6) == 0)
        {
            char *f = cmd_line + 6;
            while (*f == ' ')
                f++;
            char *c = f;
            while (*c && *c != ' ')
                c++;
            if (*c == ' ')
            {
                *c = '\0';
                c++;
                unescape(c);
                merge_path(f, arg_path);
                fappend(arg_path, (uint8_t *)c, strlen(c));
                printf_simple("Content appended.\n");
            }
            else
                printf_simple("Usage: append <file> <text>\n");
        }
        // touch
        else if (strncmp(cmd_line, "touch", 5) == 0)
        {
            char *p = cmd_line + 5;
            while (*p == ' ')
                p++;
            if (*p == '\0')
                printf_simple("Usage: touch <filename>\n");
            else
            {
                merge_path(p, arg_path);
                if (fcreate(arg_path) == 0)
                    printf_simple("File created.\n");
                else
                    printf_simple("Error: Failed to create file.\n");
            }
        }
        // rm / fdelete
        else if (strncmp(cmd_line, "fdelete", 7) == 0 || strncmp(cmd_line, "rm", 2) == 0)
        {
            char *p = cmd_line + (cmd_line[0] == 'f' ? 7 : 2);
            while (*p == ' ')
                p++;
            if (*p == '\0')
                printf_simple("Usage: rm <filename>\n");
            else
            {
                merge_path(p, arg_path);
                if (fdelete(arg_path) == 0)
                    printf_simple("File deleted successfully.\n");
                else
                    printf_simple("Error: Could not delete file.\n");
            }
        }
        // run
        else if (strncmp(cmd_line, "run", 3) == 0)
        {
            char *argv[16];
            int argc = 0;
            char *p = cmd_line + 3;
            while (*p == ' ')
                p++;
            if (*p == '\0')
            {
                printf_simple("Usage: run <file.elf> [args...]\n");
            }
            else
            {
                while (*p && argc < 15)
                {
                    argv[argc++] = p;
                    while (*p && *p != ' ')
                        p++;
                    if (*p == ' ')
                    {
                        *p = '\0';
                        p++;
                    }
                    while (*p == ' ')
                        p++;
                }
                argv[argc] = NULL;
                merge_path(argv[0], arg_path);
                argv[0] = arg_path;
                int ret = exec_elf(arg_path, argv, argc);
                if (ret != 0)
                {
                    printf_simple("Error: Failed to run ");
                    printf_simple(arg_path);
                    printf_simple("\n");
                }
            }
        }
        // tcc — 快捷编译，自动补全固定参数
        else if (strncmp(cmd_line, "tcc", 3) == 0)
        {
            char *p = cmd_line + 3;
            while (*p == ' ')
                p++;
            if (*p == '\0')
            {
                printf_simple("Usage: tcc <input.c> [-o <output.elf>]\n");
            }
            else
            {
                static char arg_bufs[12][MAX_PATH];
                char *argv[20];
                int argc = 0;

                strcpy(arg_bufs[0], "/tcc.elf");
                argv[argc++] = arg_bufs[0];

                while (*p && argc < 11)
                {
                    char *start = p;
                    while (*p && *p != ' ')
                        p++;
                    int len = p - start;
                    if (len >= MAX_PATH)
                        len = MAX_PATH - 1;
                    memcpy(arg_bufs[argc], start, len);
                    arg_bufs[argc][len] = '\0';
                    argv[argc] = arg_bufs[argc];
                    argc++;
                    while (*p == ' ')
                        p++;
                }

                argv[argc++] = "/crt0.o";
                argv[argc++] = "/mylibc.o";
                argv[argc++] = "/sysarch.o";
                argv[argc++] = "-nostdlib";
                argv[argc++] = "-I/include";
                argv[argc] = NULL;

                int ret = exec_elf("/tcc.elf", argv, argc);
                if (ret != 0)
                    printf_simple("Error: tcc failed\n");
            }
        }
        // ping
        else if (strncmp(cmd_line, "ping", 4) == 0)
        {
            char *p = cmd_line + 4;
            while (*p == ' ')
                p++;
            if (*p == '\0')
            {
                printf_simple("Usage: ping <IP> [count]\n");
            }
            else
            {
                // 解析 IP
                uint32_t ip = 0;
                int shift = 0;
                uint32_t octet = 0;
                char *s = p;
                while (1)
                {
                    char c = *s;
                    if (c >= '0' && c <= '9')
                    {
                        octet = octet * 10 + (uint32_t)(c - '0');
                    }
                    else if (c == '.' || c == ' ' || c == '\0')
                    {
                        ip |= (octet & 0xFF) << shift;
                        shift += 8;
                        octet = 0;
                        if (c != '.')
                            break;
                    }
                    else
                        break;
                    s++;
                }
                // 解析可选的 count 参数
                int count = 4;
                while (*s == ' ')
                    s++;
                if (*s >= '0' && *s <= '9')
                {
                    count = 0;
                    while (*s >= '0' && *s <= '9')
                        count = count * 10 + (*s++ - '0');
                }
                syscall_2(SYS_PING, ip, (uint32_t)count, 0);
            }
        }
        // shutdown / exit
        else if (strcmp(cmd_line, "shutdown") == 0 || strcmp(cmd_line, "exit") == 0)
        {
            printf_simple("System is shutting down...\n");
            shutdown();
        }
        // help
        else if (strcmp(cmd_line, "help") == 0)
        {
            printf_simple("Commands:\n");
            printf_simple("  ls [path]              - list files\n");
            printf_simple("  cd <path>              - change directory\n");
            printf_simple("  cat <file>             - print file content\n");
            printf_simple("  touch <file>           - create a file\n");
            printf_simple("  mkdir <dir>            - create a directory\n");
            printf_simple("  write <file>           - edit file line by line (Ctrl+C to save)\n");
            printf_simple("  append <file> <text>   - append text to file\n");
            printf_simple("  rm <file>              - delete a file\n");
            printf_simple("  run <file.elf> [args]  - execute an ELF file\n");
            printf_simple("  tcc <input.c> [-o out] - compile C file with TCC\n");
            printf_simple("  shutdown / exit        - shut down the system\n");
            printf_simple("  ping <IP> [count]      - ping a host\n");
        }
        else
        {
            printf_simple("Unknown command.\n");
        }
    }
}