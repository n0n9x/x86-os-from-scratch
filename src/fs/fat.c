#include <fs/fat.h>
#include <lib/string.h>
#include <drivers/ide.h>
#include <mm/kheap.h>
#include <drivers/terminal.h>

// 假设我们已经初始化了 BPB 信息
uint32_t fat_start_lba; //FAT 表起始扇区
uint32_t root_dir_lba; //根目录区起始扇区
uint32_t data_start_lba;//数据区起始扇区
uint32_t root_dir_sectors;//根目录区占用的扇区数

void fat_init()
{
    uint8_t boot_buf[512];
    ide_read_sector(0, boot_buf); // 读取引导扇区
    fat_bpb_t *bpb = (fat_bpb_t *)boot_buf;

    // 计算各个区域的起始位置
    fat_start_lba = bpb->reserved_sectors;
    root_dir_lba = fat_start_lba + (bpb->fat_count * bpb->sectors_per_fat);
    // 根目录区占用扇区数 = (条目数 * 32 字节) / 512
    root_dir_sectors = (bpb->root_entries * 32) / 512;
    data_start_lba = root_dir_lba + root_dir_sectors;
}

// 根据当前簇号，从 FAT 表中获取下一个簇号
uint16_t fat_get_next_cluster(uint16_t cluster) {
    uint8_t fat_buf[512];
    uint32_t fat_sector = fat_start_lba + (cluster * 2 / 512);
    uint32_t entry_offset = (cluster * 2) % 512;

    ide_read_sector(fat_sector, fat_buf);
    return *(uint16_t *)&fat_buf[entry_offset];
}

// 将簇号转换为磁盘的 LBA 地址
uint32_t cluster_to_lba(uint16_t cluster)
{
    // 数据区起始 LBA + (簇号 - 2) * 每个簇的扇区数
    // 注意：FAT 簇号从 2 开始计数
    return data_start_lba + (cluster - 2);
}

// 完善后的读取函数
int fat_read_file(const char *path, uint8_t *buffer, uint32_t limit)
{
    fat_dirent_t dirent;
    if (fat_path_to_dirent(path, &dirent, NULL, NULL) != 0) return -1;

    uint16_t cluster = dirent.first_cluster_low;
    uint32_t total_to_read = (dirent.file_size < limit) ? dirent.file_size : limit; 
    uint32_t bytes_read = 0;
    uint8_t temp_sector[512];

    while (cluster >= 2 && cluster < 0xFFF8 && bytes_read < total_to_read)
    {
        uint32_t lba = cluster_to_lba(cluster);
        uint32_t remaining = total_to_read - bytes_read;

        if (remaining < 512)//剩下的未读数据不足一个扇区
        {
            ide_read_sector(lba, temp_sector);
            for (uint32_t i = 0; i < remaining; i++)
            {
                buffer[bytes_read + i] = temp_sector[i];
            }
            bytes_read += remaining;
            break; 
        }
        else// 至少还需要读取一个完整扇区
        {
            ide_read_sector(lba, buffer + bytes_read);
            bytes_read += 512;
        }

        // 3. 查 FAT 表获取下一个簇号
        cluster = fat_get_next_cluster(cluster);
    }

    return 0;
}

// 将用户输入的文件名转换为 FAT 8.3 格式
static void to_83_format(const char *src, char *dst)
{
    for (int i = 0; i < 11; i++)
        dst[i] = ' ';
    int i = 0, j = 0;
    while (src[i] && j < 8 && src[i] != '.')
        dst[j++] = src[i++];
    if (src[i] == '.')
    {
        i++;
        j = 8;
        while (src[i] && j < 11)
            dst[j++] = src[i++];
    }
    // 转大写（FAT 默认大写）
    for (int k = 0; k < 11; k++)
    {
        if (dst[k] >= 'a' && dst[k] <= 'z')
            dst[k] -= 32;
    }
}
//在当前目录下寻找文件
int fat_find_entry_in_dir(uint16_t start_cluster, const char *name, fat_dirent_t *out_dirent, uint32_t *out_lba, int *out_idx) {
    uint8_t buf[512];
    char fat_name[11];
    to_83_format(name, fat_name);

    // 情况 A：处理根目录 (Root Directory)
    if (start_cluster == 0) {
        for (uint32_t s = 0; s < root_dir_sectors; s++) {
            uint32_t sector_lba = root_dir_lba + s;
            ide_read_sector(sector_lba, buf);
            fat_dirent_t *dir = (fat_dirent_t *)buf;

            for (int i = 0; i < 16; i++) {
                if (dir[i].name[0] == 0x00) return -1; // 目录结束
                if ((uint8_t)dir[i].name[0] == 0xE5) continue; // 已删除
                
                if (memcmp(dir[i].name, fat_name, 11) == 0) {
                    if (out_dirent) *out_dirent = dir[i];
                    if (out_lba) *out_lba = sector_lba;
                    if (out_idx) *out_idx = i;
                    return 0;
                }
            }
        }
    } 
    // 情况 B：处理子目录 (沿簇链查找)
    else {
        uint16_t curr = start_cluster;
        while (curr >= 2 && curr < 0xFFF8) {
            uint32_t sector_lba = cluster_to_lba(curr);
            ide_read_sector(sector_lba, buf);
            fat_dirent_t *dir = (fat_dirent_t *)buf;

            for (int i = 0; i < 16; i++) {
                if (dir[i].name[0] == 0x00) return -1;
                if ((uint8_t)dir[i].name[0] == 0xE5) continue;
                
                if (memcmp(dir[i].name, fat_name, 11) == 0) {
                    if (out_dirent) *out_dirent = dir[i];
                    if (out_lba) *out_lba = sector_lba;
                    if (out_idx) *out_idx = i;
                    return 0;
                }
            }
            curr = fat_get_next_cluster(curr);
        }
    }
    return -1;
}

// 解析绝对路径，并返回最终目标的元数据及物理位置
int fat_path_to_dirent(const char *path, fat_dirent_t *out_dirent, uint32_t *out_lba, int *out_idx) {
    if (!path || path[0] != '/') return -1;
    if (strcmp(path, "/") == 0) return -1; // 根目录没有 dirent 结构

    uint16_t current_dir_cluster = 0; 
    fat_dirent_t current_entry;
    char segment[13];
    const char *ptr = path + 1;

    while (*ptr) {
        int i = 0;
        while (*ptr && *ptr != '/' && i < 12) {
            segment[i++] = *ptr++;
        }
        segment[i] = '\0';

        uint32_t last_lba;
        int last_idx;
        // 在当前簇下查找
        if (fat_find_entry_in_dir(current_dir_cluster, segment, &current_entry, &last_lba, &last_idx) != 0) {
            return -1; 
        }

        if (*ptr == '/') {
            // 如果还没到结尾，当前项必须是目录
            if (!(current_entry.attr & ATTR_DIRECTORY)) return -1; 
            current_dir_cluster = current_entry.first_cluster_low;
            while (*ptr == '/') ptr++; // 跳过重复的斜杠
            
            if (*ptr == '\0') { // 路径以 / 结尾，如 "/DATA/"
                if (out_dirent) *out_dirent = current_entry;
                if (out_lba) *out_lba = last_lba;
                if (out_idx) *out_idx = last_idx;
                return 0;
            }
        } else {
            // 正常到达末尾
            if (out_dirent) *out_dirent = current_entry;
            if (out_lba) *out_lba = last_lba;
            if (out_idx) *out_idx = last_idx;
            return 0;
        }
    }
    return -1;
}

// --- 辅助函数：更新 FAT 表项并同步到磁盘 ---
void fat_set_cluster(uint16_t cluster, uint16_t next_value)
{
    uint8_t fat_buf[512];
    uint32_t fat_offset = cluster * 2; // FAT16 每个条目 2 字节
    uint32_t fat_sector = fat_start_lba + (fat_offset / 512);
    uint32_t ent_offset = fat_offset % 512;

    ide_read_sector(fat_sector, fat_buf);
    *(uint16_t *)&fat_buf[ent_offset] = next_value;
    ide_write_sector(fat_sector, fat_buf); // 必须写回，否则重启失效
}

// --- 辅助函数：寻找一个空闲簇 (0x0000) ---
uint16_t fat_allocate_cluster()
{
    uint8_t fat_buf[512];
    // 遍历所有 FAT 扇区（由 BPB 的 sectors_per_fat 定义）
    // 注意：需先获取 bpb 里的 sectors_per_fat
    for (uint32_t s = 0; s < 32; s++)
    { // 假设 sectors_per_fat 是 32
        ide_read_sector(fat_start_lba + s, fat_buf);
        uint16_t *table = (uint16_t *)fat_buf;

        for (int i = 0; i < 256; i++)
        {
            // 跳过保留的前两个条目 (i=0, 1)
            if (s == 0 && i < 2)
                continue;

            if (table[i] == 0x0000)
            {
                uint16_t cluster = (s * 256) + i;
                // --- 关键修改：找到后立即在 FAT 表中标记为已占用 ---
                fat_set_cluster(cluster, 0xFFFF); 
                return cluster;
            }
        }
    }
    return 0xFFFF; // 磁盘已满
}

int fat_add_entry(uint16_t parent_cluster, const char *fat_name, uint8_t attr, uint16_t first_cluster) {
    uint8_t sector_buf[512];

    if (parent_cluster == 0) {
        // A. 在根目录区寻找空位
        for (uint32_t s = 0; s < root_dir_sectors; s++) {
            uint32_t sector_lba = root_dir_lba + s;
            ide_read_sector(sector_lba, sector_buf);
            fat_dirent_t *dir = (fat_dirent_t *)sector_buf;

            for (int i = 0; i < 16; i++) {
                uint8_t head = (uint8_t)dir[i].name[0];
                if (head == 0x00 || head == 0xE5) { // 找到空槽位
                    memcpy(dir[i].name, fat_name, 11);
                    dir[i].attr = attr; // 使用传入的属性
                    dir[i].first_cluster_low = first_cluster;
                    dir[i].first_cluster_high = 0;
                    dir[i].file_size = 0;
                    ide_write_sector(sector_lba, sector_buf);
                    return 0;
                }
            }
        }
    } else {
        // B. 在子目录簇链中寻找空位
        uint16_t curr = parent_cluster;
        while (curr >= 2 && curr < 0xFFF8) {
            uint32_t sector_lba = cluster_to_lba(curr);
            ide_read_sector(sector_lba, sector_buf);
            fat_dirent_t *dir = (fat_dirent_t *)sector_buf;

            for (int i = 0; i < 16; i++) {
                uint8_t head = (uint8_t)dir[i].name[0];
                if (head == 0x00 || head == 0xE5) {
                    memcpy(dir[i].name, fat_name, 11);
                    dir[i].attr = attr;
                    dir[i].first_cluster_low = first_cluster;
                    dir[i].first_cluster_high = 0;
                    dir[i].file_size = 0;
                    ide_write_sector(sector_lba, sector_buf);
                    return 0;
                }
            }
            curr = fat_get_next_cluster(curr); // 沿着目录簇链找
        }
    }
    return -1; // 目录空间已满
}

int fat_create_file(const char *path) {
    char parent_path[128];
    char filename[13];
    char fat_name[11];
    fat_dirent_t temp;
    
    // 1. 分离路径和文件名
    char *last_slash = strrchr(path, '/');
    if (!last_slash) return -1;
    
    int parent_len = last_slash - path;
    if (parent_len == 0) { 
        strcpy(parent_path, "/");
    } else {
        strncpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
    }
    strcpy(filename, last_slash + 1);

    // 2. 检查文件是否已存在 (避免重复创建)
    if (fat_path_to_dirent(path, &temp, NULL, NULL) == 0) return -2;

    // 3. 获取父目录的起始簇号
    uint16_t parent_cluster = 0;
    if (strcmp(parent_path, "/") != 0) {
        if (fat_path_to_dirent(parent_path, &temp, NULL, NULL) != 0) return -3;
        parent_cluster = temp.first_cluster_low;
    }

    // 4. 为新文件分配首簇
    uint16_t first_cluster = fat_allocate_cluster();
    if (first_cluster == 0xFFFF) return -1;

    // 5. 格式化文件名
    to_83_format(filename, fat_name);

    // 6. 【关键同步】调用通用函数添加条目
    // 传入 0x20 表示 ATTR_ARCHIVE (普通文件)
    if (fat_add_entry(parent_cluster, fat_name, 0x20, first_cluster) == 0) {
        return 0; // 创建成功
    }

    // 如果添加条目失败（目录满），记得释放刚才分配的簇
    fat_set_cluster(first_cluster, 0x0000); 
    return -1;
}

static file_loc_t fat_get_file_location(const char *path) {
    file_loc_t loc = {0, -1, 0xFFFF};
    fat_dirent_t dirent;
    uint32_t lba;
    int idx;

    if (fat_path_to_dirent(path, &dirent, &lba, &idx) == 0) {
        loc.sector_lba = lba;
        loc.entry_index = idx;
        loc.first_cluster = dirent.first_cluster_low;
    }
    return loc;
}

// 重构后的通用写入函数
int fat_write_file_at(const char *path, uint8_t *data, uint32_t len, uint32_t offset) {
    file_loc_t loc = fat_get_file_location(path);
    if (loc.sector_lba == 0) {
        if (fat_create_file(path) < 0) return -1;
        loc = fat_get_file_location(path);
    }

    uint32_t final_size = (offset + len > offset) ? (offset + len) : offset; 
    uint16_t current_cluster = loc.first_cluster;
    uint32_t current_offset = 0;

    // 1. 寻找或跳过到指定的 offset 所在的簇
    while (current_offset + 512 <= offset) {
        uint16_t next = fat_get_next_cluster(current_cluster);
        if (next >= 0xFFF8) break; 
        current_cluster = next;
        current_offset += 512;
    }

    uint32_t data_pos = 0;
    while (data_pos < len) {
        uint32_t offset_in_cluster = (offset + data_pos) % 512;
        uint32_t chunk = 512 - offset_in_cluster;
        if (chunk > (len - data_pos)) chunk = len - data_pos;

        uint8_t sector_buf[512];
        // 如果不是写满整个簇，需要先读出原内容
        if (chunk < 512) {
            ide_read_sector(cluster_to_lba(current_cluster), sector_buf);
        }

        // 填充数据
        for (uint32_t j = 0; j < chunk; j++) {
            sector_buf[offset_in_cluster + j] = data[data_pos + j];
        }
        ide_write_sector(cluster_to_lba(current_cluster), sector_buf);
        
        data_pos += chunk;

        // 如果还没写完，准备下一个簇
        if (data_pos < len) {
            uint16_t next = fat_get_next_cluster(current_cluster);
            if (next >= 0xFFF8 || next < 2) {
                next = fat_allocate_cluster();
                if (next == 0xFFFF) return -1;
                fat_set_cluster(current_cluster, next);
            }
            current_cluster = next;
        }
    }

    // 2. 更新目录项中的文件大小
    uint8_t dir_buf[512];
    ide_read_sector(loc.sector_lba, dir_buf);
    fat_dirent_t *dir = (fat_dirent_t *)dir_buf;
    if (final_size > dir[loc.entry_index].file_size) {
        dir[loc.entry_index].file_size = final_size;
        ide_write_sector(loc.sector_lba, dir_buf);
    }

    return 0;
}

// 封装原有的写入接口
int fat_write_file(const char *filename, uint8_t *data, uint32_t len) {
    return fat_write_file_at(filename, data, len, 0); 
}

// 封装追加写入接口
int fat_append_file(const char *path, uint8_t *data, uint32_t len) {
    fat_dirent_t dirent;
    if (fat_path_to_dirent(path, &dirent, NULL, NULL) != 0) return -1;
    return fat_write_file_at(path, data, len, dirent.file_size);
}

int fat_delete_file(const char *path) {
    file_loc_t loc = fat_get_file_location(path);
    if (loc.sector_lba == 0) return -1;

    // 1. 释放簇链
    uint16_t cluster = loc.first_cluster;
    while (cluster >= 2 && cluster < 0xFFF8) {
        uint16_t next = fat_get_next_cluster(cluster);
        fat_set_cluster(cluster, 0x0000);
        cluster = next;
    }

    // 2. 标记目录项已删除
    uint8_t dir_buf[512];
    ide_read_sector(loc.sector_lba, dir_buf);
    fat_dirent_t *dir = (fat_dirent_t *)dir_buf;
    dir[loc.entry_index].name[0] = (char)0xE5;
    ide_write_sector(loc.sector_lba, dir_buf);

    return 0;
}

// 遍历并显示指定簇开始的目录内容
static void list_contents_at_cluster(uint16_t start_cluster) {
    uint8_t buf[512];
    char name[13];

    // 情况 A：根目录 (LBA 固定区域)
    if (start_cluster == 0) {
        for (uint32_t s = 0; s < root_dir_sectors; s++) {
            ide_read_sector(root_dir_lba + s, buf);
            fat_dirent_t *dir = (fat_dirent_t *)buf;
            for (int i = 0; i < 16; i++) {
                if (dir[i].name[0] == 0x00) return;
                if ((uint8_t)dir[i].name[0] == 0xE5 || dir[i].attr == 0x08) continue;

                // 格式化文件名逻辑
                int p = 0;
                for (int k = 0; k < 8; k++) if (dir[i].name[k] != ' ') name[p++] = dir[i].name[k];
                if (dir[i].name[8] != ' ') {
                    name[p++] = '.';
                    for (int k = 8; k < 11; k++) if (dir[i].name[k] != ' ') name[p++] = dir[i].name[k];
                }
                name[p] = '\0';

                terminal_writestring(name);
                for (int sp = 0; sp < (16 - p); sp++) terminal_writestring(" ");

                if (dir[i].attr & ATTR_DIRECTORY) {
                    terminal_writestring("<DIR>\n");
                } else {
                    terminal_write_dec(dir[i].file_size); // 调用 terminal 中的函数
                    terminal_writestring("\n");
                }
            }
        }
    } 
    // 情况 B：子目录簇链
    else {
        uint16_t curr = start_cluster;
        while (curr >= 2 && curr < 0xFFF8) {
            ide_read_sector(cluster_to_lba(curr), buf);
            fat_dirent_t *dir = (fat_dirent_t *)buf;
            for (int i = 0; i < 16; i++) {
                if (dir[i].name[0] == 0x00) return;
                if ((uint8_t)dir[i].name[0] == 0xE5 || dir[i].attr == 0x08) continue;

                int p = 0;
                for (int k = 0; k < 8; k++) if (dir[i].name[k] != ' ') name[p++] = dir[i].name[k];
                if (dir[i].name[8] != ' ') {
                    name[p++] = '.';
                    for (int k = 8; k < 11; k++) if (dir[i].name[k] != ' ') name[p++] = dir[i].name[k];
                }
                name[p] = '\0';

                terminal_writestring(name);
                for (int sp = 0; sp < (16 - p); sp++) terminal_writestring(" ");

                if (dir[i].attr & ATTR_DIRECTORY) {
                    terminal_writestring("<DIR>\n");
                } else {
                    terminal_write_dec(dir[i].file_size);
                    terminal_writestring("\n");
                }
            }
            curr = fat_get_next_cluster(curr);
        }
    }
}

void fat_list_files(const char *path) {
    terminal_writestring("\nListing: ");
    terminal_writestring(path);
    terminal_writestring("\nName            Size (Bytes)\n");
    terminal_writestring("----------------------------\n");

    if (strcmp(path, "/") == 0) {
        list_contents_at_cluster(0);
    } else {
        fat_dirent_t dirent;
        // 使用路径解析函数定位目录
        if (fat_path_to_dirent(path, &dirent, NULL, NULL) == 0) {
            if (dirent.attr & ATTR_DIRECTORY) {
                list_contents_at_cluster(dirent.first_cluster_low);
            } else {
                terminal_writestring("Not a directory.\n");
            }
        } else {
            terminal_writestring("Directory not found.\n");
        }
    }
}

//创建目录
int fat_mkdir(const char *path) {
    char parent_path[128];
    char dirname[13];
    char fat_name[11];
    fat_dirent_t temp;

    // 1. 分离路径和文件夹名
    char *last_slash = strrchr(path, '/');
    if (!last_slash) return -1;
    
    int parent_len = last_slash - path;
    if (parent_len == 0) { 
        strcpy(parent_path, "/");
    } else {
        strncpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
    }
    strcpy(dirname, last_slash + 1);

    // 2. 检查目录是否已存在
    if (fat_path_to_dirent(path, &temp, NULL, NULL) == 0) return -2;

    // 3. 获取父目录的起始簇号
    uint16_t parent_cluster = 0;
    if (strcmp(parent_path, "/") != 0) {
        if (fat_path_to_dirent(parent_path, &temp, NULL, NULL) != 0) return -3;
        parent_cluster = temp.first_cluster_low;
    }

    // 4. 为新目录分配一个簇
    uint16_t new_cluster = fat_allocate_cluster();
    if (new_cluster == 0xFFFF) return -1;

    // 5. 初始化新目录的内部 (创建 "." 和 "..")
    uint8_t sector_buf[512];
    memset(sector_buf, 0, 512);
    fat_dirent_t *dir = (fat_dirent_t *)sector_buf;

    // 设置 "." 条目 (指向自己)
    memcpy(dir[0].name, ".          ", 11);
    dir[0].attr = 0x10; // 目录属性
    dir[0].first_cluster_low = new_cluster;

    // 设置 ".." 条目 (指向父目录)
    memcpy(dir[1].name, "..         ", 11);
    dir[1].attr = 0x10;
    dir[1].first_cluster_low = parent_cluster;

    // 将初始化后的扇区写入新簇的第一个扇区
    ide_write_sector(cluster_to_lba(new_cluster), sector_buf);

    // 6. 在父目录中注册这个新目录
    to_83_format(dirname, fat_name);
    if (fat_add_entry(parent_cluster, fat_name, ATTR_DIRECTORY, new_cluster) == 0) {
        return 0; // 创建成功
    }

    // 失败回滚：释放簇
    fat_set_cluster(new_cluster, 0x0000);
    return -1;
}






