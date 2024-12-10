#include "inst_cache.h"
inst_cache icache;
int inst_cache_delay = 0;
void init_inst_cache()
{
    for (int i = 0; i < ISET_SIZE; i++)
    {
        for (int j = 0; j < IWAYS; j++)
        {
            icache.sets[i].lines[j].valid = false;
            icache.sets[i].lines[j].dirty = false;
            icache.sets[i].lines[j].tag = 0;
            icache.sets[i].lines[j].lru = 0;
            for (int k = 0; k < IBLOCK_SIZE; k++)
            {
                icache.sets[i].lines[j].data[k] = 0;
            }
        }
    }
}
void inst_update_lru(uint32_t set_index, uint32_t line_index)
{

    for (int i = 0; i < IWAYS; i++)
    {
        if (i != line_index && icache.sets[set_index].lines[i].valid)
        {
            icache.sets[set_index].lines[i].lru++; // 只更新其他行的 LRU 值
        }
    }
    icache.sets[set_index].lines[line_index].lru = 0; // 更新命中行的 LRU 为 0
}
int inst_find_line_in_set(uint32_t set_index, uint32_t tag)
{
    for (int i = 0; i < IWAYS; i++)
    {
        if (icache.sets[set_index].lines[i].valid && icache.sets[set_index].lines[i].tag == tag)
        {
            inst_update_lru(set_index, i);
            return i;
        }
    }
    return -1;
}
uint32_t inst_cache_read(uint32_t addr)
{
    uint32_t block_index = addr & 0x1f;
    uint32_t set_index = (addr >> 5) & 0x3f; // 取六位
    uint32_t tag = addr >> 11;
    for (int i = 0; i < IWAYS; i++)
    {
        if (icache.sets[set_index].lines[i].valid && icache.sets[set_index].lines[i].tag == tag)
        {
            icache.sets[set_index].lines[i].lru = 0;
            uint32_t word = (icache.sets[set_index].lines[i].data[block_index] << 0) | (icache.sets[set_index].lines[i].data[block_index + 1] << 8) | (icache.sets[set_index].lines[i].data[block_index + 2] << 16) | (icache.sets[set_index].lines[i].data[block_index + 3] << 24);
            inst_update_lru(set_index, i);
            return word;
        }
    }
    inst_cache_delay = 49;
    inst_get_ram_data(addr);
    int line_index = inst_find_line_in_set(set_index, tag);
    if (line_index == -1)
    {
        printf("Error: data cache miss\n");
        return 0;
    }
    uint32_t word = (icache.sets[set_index].lines[line_index].data[block_index] << 0) |
                    (icache.sets[set_index].lines[line_index].data[block_index + 1] << 8) |
                    (icache.sets[set_index].lines[line_index].data[block_index + 2] << 16) |
                    (icache.sets[set_index].lines[line_index].data[block_index + 3] << 24);
    return word;
}
void inst_get_ram_data(uint32_t addr)
{
    uint32_t block_index = addr & 0x1f;      // 取低5位，得到块内偏移量
    uint32_t set_index = (addr >> 5) & 0x3F; // 右移5位，0xFF 对应的二进制值是 1111 1111。所以，(addr>>5)&0xFF 的结果就是 addr 的低 8 位，即索引值。
    uint32_t tag = addr >> 11;
    uint32_t block_addr = (addr >> 5) << 5;
    int line_index = -1; // 写入的行
    for (int i = 0; i < IWAYS; i++)
    {
        if (icache.sets[set_index].lines[i].valid == false) // 如果当前行是无效的（即该行为空闲行）
        {
            line_index = i; //// 找到空闲行，记录该行的索引
            icache.sets[set_index].lines[i].tag = tag;
            icache.sets[set_index].lines[i].valid = 1; // 将该行标记为有效
            icache.sets[set_index].lines[i].lru = 0;   // // 将该行的 LRU 值设置为 0（表示最近使用）
            break;
        }
    }
    // 更新其他行的lru
    if (line_index != -1)
    {
        inst_update_lru(set_index, line_index);
        uint8_t *dest = icache.sets[set_index].lines[line_index].data;
        for (int i = 0; i < IBLOCK_SIZE / 4; i++) // 循环 8 次，每次写入 4 字节
        {
            uint32_t word = mem_read_32(block_addr);
            *dest++ = (uint8_t)(word >> 0) & 0xff;
            *dest++ = (uint8_t)(word >> 8) & 0xff;
            *dest++ = (uint8_t)(word >> 16) & 0xff;
            *dest++ = (uint8_t)(word >> 24) & 0xff;
            block_addr += 4;
        }
        return;
    }
    else // 没有找到空闲行
    {
        uint8_t max_lru = 0;    // 找到最近最少使用
        uint32_t write_tag = 0; // 标识缓存行中存储的数据所属的内存地址
        for (int i = 0; i < IWAYS; i++)
        {
            if (max_lru < icache.sets[set_index].lines[i].lru)
            {
                max_lru = icache.sets[set_index].lines[i].lru;
                line_index = i;
                write_tag = icache.sets[set_index].lines[i].tag;
            }
        }
        if (icache.sets[set_index].lines[line_index].dirty) // 是脏数据
        {
            uint32_t ram_address = 0;
            ram_address = (write_tag << 11) | (set_index << 5); // 计算地址
            uint8_t *src = icache.sets[set_index].lines[line_index].data;
            for (int i = 0; i < IBLOCK_SIZE / 4; i++)
            {
                uint32_t word = (src[0] << 0) | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
                mem_write_32(ram_address, word); // 写回内存
                src += 4;
                ram_address += 4;
            }
            // inst_cache_delay += 49; // 写回延迟，如果脏数据写回和加载新数据块是并行处理的（某些架构支持），则这两部分延迟可以合并，而不需要分别添加。
        }
        icache.sets[set_index].lines[line_index].tag = tag;
        icache.sets[set_index].lines[line_index].valid = 1;
        icache.sets[set_index].lines[line_index].lru = 0;
        inst_update_lru(set_index, line_index);
        uint8_t *dest = icache.sets[set_index].lines[line_index].data;
        for (int i = 0; i < IBLOCK_SIZE / 4; i++) // 循环 8 次，每次写入 4 字节
        {
            uint32_t word = mem_read_32(block_addr);
            *dest++ = (uint8_t)(word >> 0) & 0xff;
            *dest++ = (uint8_t)(word >> 8) & 0xff;
            *dest++ = (uint8_t)(word >> 16) & 0xff;
            *dest++ = (uint8_t)(word >> 24) & 0xff;
            block_addr += 4;
        }
        return;
    }
}