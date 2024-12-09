#ifndef CACHE
#define CACHE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mips.h"
#define LINE_SIZE 32 // 缓存行数
#define DATA_SIZE 32 // 数据块大小
typedef struct
{
    bool valid;               // 有效位
    uint32_t tag;             // 标记位
    uint32_t data[DATA_SIZE]; // 数据
} cache_line;

typedef struct
{
    cache_line line[LINE_SIZE]; // 32行缓存
} Cache;
extern Cache cache;
void init_cache();
void cache_write(uint32_t addr, uint32_t data);
uint32_t cache_read(uint32_t addr);
#endif