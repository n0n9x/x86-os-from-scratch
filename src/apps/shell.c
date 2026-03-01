#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>

#define MAX_PATH 256
char c_path[MAX_PATH] = "/"; // 用户态维护的当前路径

// 辅助函数：将相对路径转换为绝对路径
void merge_path(const char *input, char *output)
{
    if (input[0] == '/')
    {
        strcpy(output, input);
    }
    else
    {
        strcpy(output, c_path);
        if (strcmp(c_path, "/") != 0)
        {
            strcat(output, "/");
        }
        strcat(output, input);
    }
}

void shell_main()
{
    char cmd_line[128];
    char arg_path[MAX_PATH];

    printf_simple("----------welcome to Shell!----------\n");

    while (1)
    {
        printf_simple(c_path);
        printf_simple("> ");

        memset(cmd_line, 0, 128);
        scanf_simple(cmd_line, 127);

        if (strlen(cmd_line) == 0)
            continue;

        // --- 命令解析逻辑 ---

        // 1. ls [路径]: 列出文件
        if (strncmp(cmd_line, "ls", 2) == 0)
        {
            char *path_ptr = cmd_line + 2;
            while (*path_ptr == ' ')
                path_ptr++;

            if (*path_ptr == '\0')
            {
                flist(c_path);
            }
            else
            {
                merge_path(path_ptr, arg_path);
                flist(arg_path);
            }
        }
        // 2. cd <路径>: 切换目录
        else if (strncmp(cmd_line, "cd", 2) == 0)
        {
            char *path_ptr = cmd_line + 2;
            while (*path_ptr == ' ')
                path_ptr++;

            if (strcmp(path_ptr, "..") == 0)
            {
                if (strcmp(c_path, "/") != 0)
                {
                    char *last_slash = strrchr(c_path, '/');
                    if (last_slash == c_path)
                        *(last_slash + 1) = '\0';
                    else
                        *last_slash = '\0';
                }
            }
            else
            {
                merge_path(path_ptr, arg_path);
                if (open(arg_path) != -1)
                {
                    strcpy(c_path, arg_path);
                }
                else
                {
                    printf_simple("Error: Directory not found.\n");
                }
            }
        }
        // 3. cat <文件>: 读取并打印内容
        else if (strncmp(cmd_line, "cat", 3) == 0)
        {
            char *path_ptr = cmd_line + 3;
            while (*path_ptr == ' ')
                path_ptr++;
            merge_path(path_ptr, arg_path);

            uint8_t file_buf[512];
            memset(file_buf, 0, 512);
            if (fread(arg_path, file_buf, 511) >= 0)
            {
                printf_simple((char *)file_buf);
                printf_simple("\n");
            }
            else
            {
                printf_simple("Error: Cannot read file.\n");
            }
        }
        // 4. mkdir <路径>: 创建目录 (补全)
        else if (strncmp(cmd_line, "mkdir", 5) == 0)
        {
            char *path_ptr = cmd_line + 5;
            while (*path_ptr == ' ')
                path_ptr++;
            if (*path_ptr == '\0')
            {
                printf_simple("Usage: mkdir <dirname>\n");
            }
            else
            {
                merge_path(path_ptr, arg_path);
                if (mkdir(arg_path) == 0)
                {
                    printf_simple("Directory created.\n");
                }
                else
                {
                    printf_simple("Error: Failed to create directory.\n");
                }
            }
        }
        // 5: write <file> <text>
        else if (strncmp(cmd_line, "write", 5) == 0)
        {
            char *file_name = cmd_line + 5;
            while (*file_name == ' ')
                file_name++;

            char *content = strrchr(file_name, ' ');
            if (content)
            {
                *content = '\0'; // 分割文件名和内容
                content++;
                merge_path(file_name, arg_path);
                fwrite(arg_path, (uint8_t *)content, strlen(content));
                printf_simple("File written.\n");
            }
            else
            {
                printf_simple("Usage: write <file> <text>\n");
            }
        }

        // --- 6. append: 追加内容 ---
        else if (strncmp(cmd_line, "append", 6) == 0)
        {
            char *file_name = cmd_line + 6;
            while (*file_name == ' ')
                file_name++;

            char *content = strrchr(file_name, ' ');
            if (content)
            {
                *content = '\0';
                content++;
                merge_path(file_name, arg_path);
                fappend(arg_path, (uint8_t *)content, strlen(content));
                printf_simple("Content appended.\n");
            }
        }
        // 7. touch <文件>: 创建新文件 (新增)
        else if (strncmp(cmd_line, "touch", 5) == 0)
        {
            char *path_ptr = cmd_line + 5;
            while (*path_ptr == ' ')
                path_ptr++;
            if (*path_ptr == '\0')
            {
                printf_simple("Usage: touch <filename>\n");
            }
            else
            {
                merge_path(path_ptr, arg_path);
                if (fcreate(arg_path) == 0)
                {
                    printf_simple("File created.\n");
                }
                else
                {
                    printf_simple("Error: Failed to create file.\n");
                }
            }
        }
        else if (strncmp(cmd_line, "rm", 2) == 0)
        {
            char *path_ptr = cmd_line + 2;
            while (*path_ptr == ' ')
                path_ptr++; // 跳过空格

            if (*path_ptr == '\0')
            {
                printf_simple("Usage: rm <filename>\n");
            }
            else
            {
                merge_path(path_ptr, arg_path); // 转换为绝对路径

                // 调用 stdlib.c 封装的 fdelete
                if (fdelete(arg_path) == 0)
                {
                    printf_simple("File deleted successfully.\n");
                }
                else
                {
                    printf_simple("Error: Could not delete file (not found or is a directory).\n");
                }
            }
        }
        else if (strcmp(cmd_line, "shutdown") == 0 || strcmp(cmd_line, "exit") == 0)
        {
            printf_simple("System is shutting down...\n");
            shutdown(); // 调用 stdlib.c 封装的函数
        }
        // 8. help
        else if (strcmp(cmd_line, "help") == 0)
        {
            printf_simple("Commands: ls, cd, cat, mkdir, touch, help\n");
        }
        else
        {
            printf_simple("Unknown command.\n");
        }
    }
}