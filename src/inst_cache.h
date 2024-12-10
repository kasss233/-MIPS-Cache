#ifndef INSTCACHE
#define INSTCACHE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mips.h"
#include "shell.h"
#define ICACHE_SIZE 8192 // 64KB
#define IBLOCK_SIZE 32
#define IWAYS 4
#define ISET_SIZE 64
#define ILINE_SIZE (ICACHE_SIZE / IBLOCK_SIZE)

typedef struct
{
    bool valid;                // 有效位
    uint8_t lru;               // LRU位
    uint32_t tag;              // 标记位
    uint8_t data[IBLOCK_SIZE]; // 数据
} icache_line;

typedef struct
{
    icache_line lines[IWAYS];
} icache_set;

typedef struct
{
    icache_set sets[ISET_SIZE];
} inst_cache;
extern inst_cache icache;
extern int inst_cache_delay;
void init_inst_cache();
void inst_get_ram_data(uint32_t addr);
uint32_t inst_cache_read(uint32_t addr);
#endif