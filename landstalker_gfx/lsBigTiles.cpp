#include "lsBigTiles.h"

uint32_t subD184(uint32_t& d6, uint32_t& d5, const uint8_t*& src)
{
    int32_t d1;
    uint32_t d0, d2;
    d1 = -1;
    do
    {
        if(d6-- == 0)
        {
            d6 = 7;
            d5 = *src++;
        }
        d5 <<= 1;
    } while(d5 & 0x80);
    
    d1 = -d1;
    if(d1)
    {
        d1 <<= 2;
        d2 = 1;
        d0 = 0;
        for(; d1 > 0; d1--)
        {
            if(d6-- == 0)
            {
                d6 = 7;
                d5 = *src++;
            }
            d0 <<= 1;
            d0 += (d5 & 0x80) ? 1 : 0;
            d5 <<= 1;
        }
        d2 += d0;
    }
    else
    {
        d2 = 0;
    }
    return d2;
}

size_t lsBigTiles::ExtractBigTiles(const uint8_t* src, uint8_t* dst)
{
    uint32_t d0, d1, d2, d3, d4, d5, d6, d7;
    uint8_t* buf[8];
    uint8_t* bufptr;
    d6 = 0;
    d4 = 0;
    d7 = 15;
    for(d7 = 15; d7 > 0; d7--)
    {
        if(d6-- == 0)
        {
            d6 = 7;
            d5 = *src++;
        }
        d4 <<= 1;
        d4 += (d5 & 0x80) ? 1 : 0;
        d4 &= 0xFFFF;
        d5 <<= 1;
        d5 &= 0xFF;
    }
    d4 <<= 2;
    bufptr = buf[0];
    for(d3 = 15; d3 > 0; d3--)
    {
        *bufptr++ = 0;
    }
    bufptr = dst;
    d3 = 0;
    d7 = 0;
    d2 = subD184(d6, src);
    
    
    return 0;
}

