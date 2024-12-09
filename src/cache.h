#ifndef CACHE
#define CACHE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mips.h"
#include "shell.h"
#define BLOCK_SIZE 32
#define CACHE_SIZE 65536 // 64KB
#define LINE_SIZE (CACHE_SIZE / BLOCK_SIZE)
typedef struct
{
    bool valid;               // 有效位
    uint32_t tag;             // 标记位
    uint8_t data[BLOCK_SIZE]; // 数据
} cache_line;

typedef struct
{
    cache_line line[LINE_SIZE];
} Cache;
extern Cache cache;
void init_cache();
void cache_write(uint32_t addr, uint32_t data);
uint32_t cache_read(uint32_t addr);
#endif