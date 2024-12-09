#include "cache.h"
Cache cache;
void init_cache()
{
    for (int i = 0; i < LINE_SIZE; i++)
    {
        cache.line[i].tag = 0;
        cache.line[i].valid = false;
        for (int j = 0; j < BLOCK_SIZE; j++)
        {
            cache.line[i].data[j] = 0;
        }
    }
}
void cache_write(uint32_t addr, uint32_t data)
{
    uint32_t index = (addr >> 5) & 0x7ff;
    uint32_t tag = addr >> 16;

    cache.line[index].tag = tag;
    cache.line[index].valid = true;
    cache.line[index].data[addr & 0x1f] = data;
}
uint32_t cache_read(uint32_t addr)
{
    uint32_t index = (addr >> 5) & 0x7ff; // 右移五位，0x7FF 对应的二进制值是 0000 0111 1111 1111。所以，(addr>>5)&0x7ff 的结果就是 addr 的低 11 位，即索引值。
    uint32_t tag = addr >> 16;            // 右移16位，得到tag
    if (cache.line[index].valid && cache.line[index].tag == tag)
    {
        return cache.line[index].data[addr & 0x1f]; // 如果cache命中，则直接返回数据
    }
    else
    {
        // 未命中
        uint32_t data = mem_read_32(addr);
        cache_write(addr, data); // 将数据写入cache
        return data;
    }
}