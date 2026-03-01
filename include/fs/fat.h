#ifndef FAT_H
#define FAT_H

#include <stdint.h>

// FAT 属性定义
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

// FAT16/32 引导扇区中的 BPB 结构
typedef struct
{
    uint8_t jmp[3];
    char oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_small;
    uint8_t media_type;
    uint16_t sectors_per_fat;
} __attribute__((packed)) fat_bpb_t;

// 目录项结构 (32 字节)
typedef struct
{
    char name[11];             // 文件名 (8字节) + 扩展名 (3字节)，不包含点号 '.'
    uint8_t attr;              // 文件属性 (0x10目录, 0x20归档, 0x01只读, 0x02隐藏等)
    uint8_t reserved;          // 保留位，通常设为 0
    uint8_t create_time_ms;    // 文件创建时间的毫秒部分 (0-199)
    uint16_t create_time;      // 文件创建时间 (时:分:秒，按位域编码)
    uint16_t create_date;      // 文件创建日期 (年-月-日，按位域编码)
    uint16_t last_access_date; // 最后访问日期
    uint16_t first_cluster_high; // 簇号的高16位 (在 FAT16 中始终为 0，FAT32 才会用到)
    uint16_t last_modify_time;   // 最后修改时间
    uint16_t last_modify_date;   // 最后修改日期
    uint16_t first_cluster_low;  // 簇号的低16位 (文件数据存放的起始簇号)
    uint32_t file_size;          // 文件的实际大小 (以字节为单位)
} __attribute__((packed)) fat_dirent_t;

// 文件位置元数据，用于快速定位目录项
typedef struct
{
    uint32_t sector_lba;   // 该文件目录项所在的磁盘物理扇区地址 (LBA)
    int entry_index;       // 在上述扇区内的目录项索引 (0-15，因为一个扇区512字节/32字节=16项)
    uint16_t first_cluster; // 文件的首簇号，指向文件实际数据存储的起始位置
} file_loc_t;

// --- 1. 基础管理与初始化 ---
void fat_init();
uint32_t cluster_to_lba(uint16_t cluster);
uint16_t fat_get_next_cluster(uint16_t current_cluster);

// --- 2. 路径解析核心函数 (非常重要，解决隐式声明警告) ---
// 解析绝对路径，填充元数据到 out_dirent，并返回物理位置
int fat_path_to_dirent(const char *path, fat_dirent_t *out_dirent, uint32_t *out_lba, int *out_idx);

// 在指定目录下查找特定的名称
int fat_find_entry_in_dir(uint16_t start_cluster, const char *name, fat_dirent_t *out_dirent, uint32_t *out_lba, int *out_idx);

// --- 3. 文件操作接口 ---
int fat_read_file(const char *path, uint8_t *buffer, uint32_t limit);
int fat_create_file(const char *path);
int fat_write_file(const char *path, uint8_t *data, uint32_t len);
int fat_write_file_at(const char *path, uint8_t *data, uint32_t len, uint32_t offset);
int fat_append_file(const char *path, uint8_t *data, uint32_t len);
int fat_delete_file(const char *path);
void fat_list_files(const char *path);

// --- 4. 目录操作接口 ---
int fat_mkdir(const char *path);
// 通用的添加目录项函数，被 fat_create_file 和 fat_mkdir 共同调用
int fat_add_entry(uint16_t parent_cluster, const char *fat_name, uint8_t attr, uint16_t first_cluster);

// --- 5. 簇管理低级函数 ---
uint16_t fat_allocate_cluster();
void fat_set_cluster(uint16_t cluster, uint16_t next_value);

#endif