#ifndef DATACACHE
#define DATACACHE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mips.h"
#include "shell.h"
#define CACHE_SIZE 65536 // 64KB
#define BLOCK_SIZE 32
#define WAYS 8
#define SET_SIZE 256
#define LINE_SIZE (CACHE_SIZE / BLOCK_SIZE)

typedef struct
{
    bool valid;               // 有效位
    bool dirty;               // 脏位
    uint8_t lru;              // LRU位
    uint32_t tag;             // 标记位
    uint8_t data[BLOCK_SIZE]; // 数据
} cache_line;

typedef struct
{
    cache_line lines[WAYS];
} cache_set;

typedef struct
{
    cache_set sets[SET_SIZE];
} data_cache;
extern data_cache cache;
extern int data_cache_delay;
void init_data_cache();
void get_ram_data(uint32_t addr);
void data_cache_write(uint32_t addr, uint32_t data);
uint32_t data_cache_read(uint32_t addr);
#endif