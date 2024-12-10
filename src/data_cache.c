#include "data_cache.h"
data_cache cache;
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

uint32_t data_cache_read(uint32_t addr)
{
    uint32_t block_index = addr & 0x1f;      // 取低5位，得到块内偏移量
    uint32_t set_index = (addr >> 5) & 0xFF; // 右移5位，0xFF 对应的二进制值是 1111 1111。所以，(addr>>5)&0xFF 的结果就是 addr 的低 8 位，即索引值。
    uint32_t tag = addr >> 13;
    for (int i = 0; i < WAYS; i++)
    {
        if (cache.sets[set_index].lines[i].valid && cache.sets[set_index].lines[i].tag == tag)
        {
            uint32_t word = (cache.sets[set_index].lines[i].data[block_index] << 0) | (cache.sets[set_index].lines[i].data[block_index + 1] << 8) | (cache.sets[set_index].lines[i].data[block_index + 2] << 16) | (cache.sets[set_index].lines[i].data[block_index + 3] << 24);
            return word;
        }
    }
    // 没有命中
    get_ram_data(addr);
    return 0;
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
            cache.sets[set_index].lines[i].dirty = true;
            cache.sets[set_index].lines[i].data[block_index] = (data >> 0) & 0xFF;
            cache.sets[set_index].lines[i].data[block_index + 1] = (data >> 8) & 0xFF;
            cache.sets[set_index].lines[i].data[block_index + 2] = (data >> 16) & 0xFF;
            cache.sets[set_index].lines[i].data[block_index + 3] = (data >> 24) & 0xFF;
            return;
        }
    }
    get_ram_data(addr);
    data_cache_write(addr, data);
}
void get_ram_data(uint32_t addr)
{
    uint32_t block_index = addr & 0x1f;      // 取低5位，得到块内偏移量
    uint32_t set_index = (addr >> 5) & 0xFF; // 右移5位，0xFF 对应的二进制值是 1111 1111。所以，(addr>>5)&0xFF 的结果就是 addr 的低 8 位，即索引值。
    uint32_t tag = addr >> 13;
    uint32_t block_addr = (addr >> 5) << 5;
    int line_index = -1;    // 写入的行
    uint32_t write_tag = 0; // 标识缓存行中存储的数据所属的内存地址
    uint8_t lru = 0;
    for (int i = 0; i < WAYS; i++)
    {
        if (cache.sets[set_index].lines[i].valid == false) // 如果当前行是无效的（即该行为空闲行）
        {
            line_index = i;                           //// 找到空闲行，记录该行的索引
            lru = cache.sets[set_index].lines[i].lru; //// 记录该行的 LRU 值
            cache.sets[set_index].lines[i].tag = tag;
            cache.sets[set_index].lines[i].valid = 1; // 将该行标记为有效
            cache.sets[set_index].lines[i].lru = 0;   // // 将该行的 LRU 值设置为 0（表示最近使用）
            break;
        }
    }
    // 更新其他行的lru
    if (line_index != -1)
    {
        for (int i = 0; i < WAYS; i++)
        {
            if (i != line_index)
            {
                cache.sets[set_index].lines[i].lru++; // 表示比新加载的缓存行 最近未被使用。
            }
        }
    }
    else // 没有找到空闲行
    {
        uint8_t max_lru = 0; // 找到最近最少使用
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
            for (int i = 0; i < BLOCK_SIZE / 4; i++)
            {
                uint32_t word = (cache.sets[set_index].lines[line_index].data[i * 4] << 0) |
                                (cache.sets[set_index].lines[line_index].data[i * 4 + 1] << 8) |
                                (cache.sets[set_index].lines[line_index].data[i * 4 + 2] << 16) |
                                (cache.sets[set_index].lines[line_index].data[i * 4 + 3] << 24);
                mem_write_32(ram_address, word); // 写回内存
                ram_address += 4;
            }
        }
        cache.sets[set_index].lines[line_index].tag = tag;
        cache.sets[set_index].lines[line_index].valid = 1;
        cache.sets[set_index].lines[line_index].lru = 0;
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
    }
}