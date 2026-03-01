#ifndef STDLIB_H
#define STDLIB_H

#include <lib/unistd_.h>


void* sbrk_simple(int increment);
void user_task_exit();

int disk_read(uint32_t lba, uint8_t *buf);
int disk_write(uint32_t lba, uint8_t *buf);

int fcreate(const char *name);
int open(const char *name);
int fread(const char *name, uint8_t *buf, uint32_t len);
int fwrite(const char *name, uint8_t *buf, uint32_t len);
int fdelete(const char *filename);
int fappend(const char *filename, uint8_t *buf, uint32_t len);

void flist(const char *path);
int mkdir(const char *path);

void shutdown();

#endif