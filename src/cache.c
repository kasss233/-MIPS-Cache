#include "cache.h"
Cache cache;
void init_cache()
{
    for (int i = 0; i < LINE_SIZE; i++)
    {
        cache.line[i].tag = 0;
        cache.line[i].valid = 0;
        for (int j = 0; j < LINE_SIZE; j++)
        {
            cache.line[i].data[j] = 0;
        }
    }
}