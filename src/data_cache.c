#include "data_cache.h"
data_cache cache;
int data_cache_delay = 0;
void init_data_cache()
{
    for (int i = 0; i < SET_SIZE; i++)
    {
        for (int j = 0; j < WAYS; j++)
        {
            cache.sets[i].lines[j].valid = false;
            cache.sets[i].lines[j].dirty = false;
            cache.sets[i].lines[j].tag = 0;
            cache.sets[i].lines[j].lru = 0;
            for (int k = 0; k < BLOCK_SIZE; k++)
            {
                cache.sets[i].lines[j].data[k] = 0;
            }
        }
    }
}
void data_update_lru(uint32_t set_index, uint32_t line_index)
{
    for (int i = 0; i < WAYS; i++)
    {
        if (i != line_index && cache.sets[set_index].lines[i].valid)
        {
            cache.sets[set_index].lines[i].lru++; // 只更新其他行的 LRU 值
        }
    }
    cache.sets[set_index].lines[line_index].lru = 0; // 更新命中行的 LRU 为 0
}
int data_find_line_in_set(uint32_t set_index, uint32_t tag)
{
    for (int i = 0; i < WAYS; i++)
    {
        if (cache.sets[set_index].lines[i].valid && cache.sets[set_index].lines[i].tag == tag)
        {
            data_update_lru(set_index, i);
            return i;
        }
    }
    return -1;
}
uint32_t data_cache_read(uint32_t addr)
{
    uint32_t block_index = addr & 0x1f;      // 取低5位，得到块内偏移量
    uint32_t set_index = (addr >> 5) & 0xFF; // 右移5位，0xFF 对应的二进制值是 1111 1111。所以，(addr>>5)&0xFF 的结果就是 addr 的低 8 位，即索引值。
    uint32_t tag = addr >> 13;
    for (int i = 0; i < WAYS; i++)
    {
        if (cache.sets[set_index].lines[i].valid && cache.sets[set_index].lines[i].tag == tag)
        {
            data_update_lru(set_index, i);
            uint32_t word = (cache.sets[set_index].lines[i].data[block_index] << 0) | (cache.sets[set_index].lines[i].data[block_index + 1] << 8) | (cache.sets[set_index].lines[i].data[block_index + 2] << 16) | (cache.sets[set_index].lines[i].data[block_index + 3] << 24);
            return word;
        }
    }
    // 没有命中
    data_cache_delay += 49;
    get_ram_data(addr);
    int line_index = data_find_line_in_set(set_index, tag);
    if (line_index == -1)
    {
        printf("Error: data cache miss\n");
        return 0;
    }
    uint32_t word = (cache.sets[set_index].lines[line_index].data[block_index] << 0) |
                    (cache.sets[set_index].lines[line_index].data[block_index + 1] << 8) |
                    (cache.sets[set_index].lines[line_index].data[block_index + 2] << 16) |
                    (cache.sets[set_index].lines[line_index].data[block_index + 3] << 24);
    return word;
}
void data_cache_write(uint32_t addr, uint32_t data)
{
    uint32_t block_index = addr & 0x1f;      // 取低5位，得到块内偏移量
    uint32_t set_index = (addr >> 5) & 0xFF; // 右移5位，0xFF 对应的二进制值是 1111 1111。所以，(addr>>5)&0xFF 的结果就是 addr 的低 8 位，即索引值。
    uint32_t tag = addr >> 13;
    for (int i = 0; i < WAYS; i++)
    {
        if (cache.sets[set_index].lines[i].valid && cache.sets[set_index].lines[i].tag == tag)
        {
            data_update_lru(set_index, i);
            cache.sets[set_index].lines[i].dirty = true;
            cache.sets[set_index].lines[i].data[block_index] = (data >> 0) & 0xFF;
            cache.sets[set_index].lines[i].data[block_index + 1] = (data >> 8) & 0xFF;
            cache.sets[set_index].lines[i].data[block_index + 2] = (data >> 16) & 0xFF;
            cache.sets[set_index].lines[i].data[block_index + 3] = (data >> 24) & 0xFF;
            return;
        }
    }
    data_cache_delay += 49;
    get_ram_data(addr);
    int line_index = data_find_line_in_set(set_index, tag);
    if (line_index == -1)
    {
        printf("Error: data cache miss\n");
        return;
    }
    cache.sets[set_index].lines[line_index].dirty = true;
    cache.sets[set_index].lines[line_index].data[block_index] = (data >> 0) & 0xFF;
    cache.sets[set_index].lines[line_index].data[block_index + 1] = (data >> 8) & 0xFF;
    cache.sets[set_index].lines[line_index].data[block_index + 2] = (data >> 16) & 0xFF;
    cache.sets[set_index].lines[line_index].data[block_index + 3] = (data >> 24) & 0xFF;
}
void get_ram_data(uint32_t addr)
{
    uint32_t block_index = addr & 0x1f;      // 取低5位，得到块内偏移量
    uint32_t set_index = (addr >> 5) & 0xFF; // 右移5位，0xFF 对应的二进制值是 1111 1111。所以，(addr>>5)&0xFF 的结果就是 addr 的低 8 位，即索引值。
    uint32_t tag = addr >> 13;
    uint32_t block_addr = (addr >> 5) << 5;
    int line_index = -1; // 写入的行
    for (int i = 0; i < WAYS; i++)
    {
        if (cache.sets[set_index].lines[i].valid == false) // 如果当前行是无效的（即该行为空闲行）
        {
            line_index = i; //// 找到空闲行，记录该行的索引
            cache.sets[set_index].lines[i].tag = tag;
            cache.sets[set_index].lines[i].valid = 1; // 将该行标记为有效
            cache.sets[set_index].lines[i].lru = 0;   // // 将该行的 LRU 值设置为 0（表示最近使用）
            break;
        }
    }
    // 更新其他行的lru
    if (line_index != -1)
    {
        data_update_lru(set_index, line_index);
        uint8_t *dest = cache.sets[set_index].lines[line_index].data;
        for (int i = 0; i < BLOCK_SIZE / 4; i++) // 循环 8 次，每次写入 4 字节
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
        for (int i = 0; i < WAYS; i++)
        {
            if (max_lru < cache.sets[set_index].lines[i].lru)
            {
                max_lru = cache.sets[set_index].lines[i].lru;
                line_index = i;
                write_tag = cache.sets[set_index].lines[i].tag;
            }
        }
        if (cache.sets[set_index].lines[line_index].dirty) // 是脏数据
        {
            uint32_t ram_address = 0;
            ram_address = (write_tag << 13) | (set_index << 5); // 计算地址
            uint8_t *src = cache.sets[set_index].lines[line_index].data;
            for (int i = 0; i < BLOCK_SIZE / 4; i++)
            {
                uint32_t word = (src[0] << 0) | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
                mem_write_32(ram_address, word); // 写回内存
                src += 4;
                ram_address += 4;
            }
            // data_cache_delay += 49; // 写回延迟，如果脏数据写回和加载新数据块是并行处理的（某些架构支持），则这两部分延迟可以合并，而不需要分别添加。
        }
        cache.sets[set_index].lines[line_index].tag = tag;
        cache.sets[set_index].lines[line_index].valid = 1;
        cache.sets[set_index].lines[line_index].lru = 0;
        data_update_lru(set_index, line_index);
        uint8_t *dest = cache.sets[set_index].lines[line_index].data;
        for (int i = 0; i < BLOCK_SIZE / 4; i++) // 循环 8 次，每次写入 4 字节
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