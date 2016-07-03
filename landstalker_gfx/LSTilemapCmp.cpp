#include "LSTilemapCmp.h"

#include "BitBarrel.h"

uint16_t getCodedNumber(BitBarrel& bb)
{
    uint16_t exp = 0, num = 0;
    
    while(!bb.getNextBit())
    {
        exp++;
    }
    
    if(exp)
    {
        num = 1 << exp;
        num += bb.readBits(exp);
    }
    
    return num;
}

uint16_t ilog2(uint16_t num)
{
    uint16_t ret = 0;
    while(num)
    {
        num >>= 1;
        ret++;
    }
    return ret;
}

uint16_t LSTilemapCmp::Decode(const uint8_t* src, Tilemap& tilemap)
{
    BitBarrel bb(src);
    
    tilemap.reset();

    tilemap.left   = bb.readBits(8);
    tilemap.top    = bb.readBits(8);
    tilemap.width  = bb.readBits(8) + 1;
    tilemap.height = (bb.readBits(8) + 1) / 2;
    
    uint16_t tileDictionary[2] = {0, 0};
    uint16_t offsetDictionary[14] = {0xFFFF,
                                     1,
                                     2,
                                     static_cast<uint16_t>(tilemap.width),
                                     static_cast<uint16_t>(tilemap.width*2u),
                                     static_cast<uint16_t>(tilemap.width + 1),
                                     0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t t = tilemap.width * tilemap.height * 2;
    tilemap.foreground.assign(t, 0);
    
    tileDictionary[1] = bb.readBits(10);
    tileDictionary[0] = bb.readBits(10);
    
    for(size_t i = 6; i < 14; ++i)
    {
        offsetDictionary[i] = bb.readBits(12);
    }
    
    int16_t dst_addr = -1;
    
    while(true)
    {
        uint16_t start = getCodedNumber(bb);
        
        if(!start) start++;
        dst_addr += start;
        
        if(dst_addr >= t)
        {
            break;
        }
        
        uint8_t command = bb.readBits(3);
        if(command > 5)
        {
            command = 6 + (((command & 1) << 2) | bb.readBits(2));
        }
        tilemap.foreground[dst_addr] = offsetDictionary[command];
        
        if(bb.getNextBit())
        {
            uint16_t row_addr = dst_addr;
            bool width_offset = bb.getNextBit();
            do
            {
                do
                {
                    row_addr += tilemap.width + (width_offset ? 1 : 0);
                    tilemap.foreground[row_addr] = offsetDictionary[command];
                } while(bb.getNextBit());
                width_offset = !width_offset;
            } while(bb.getNextBit());
        }
    }
    
    uint16_t tiles[2] = {tileDictionary[0], tileDictionary[1]};
    dst_addr = 0;
    do
    {
        uint16_t operand = tilemap.foreground[dst_addr];
        uint16_t offset;
        if(operand != 0xFFFF)
        {
            offset = dst_addr - operand;
            do
            {
                tilemap.foreground[dst_addr++] = tilemap.foreground[offset++];
            } while ((dst_addr < t) && (tilemap.foreground[dst_addr] == 0));
        }
        else
        {
            do
            {
                operand = bb.readBits(2);
                uint16_t value = 0;
                switch(operand)
                {
                    case 0:
                        if(tiles[0])
                        {
                            value = bb.readBits(ilog2(tiles[0]));
                        }
                        tilemap.foreground[dst_addr++] = value;
                        break;
                    case 1:
                        if(tiles[1] != tileDictionary[1])
                        {
                            value = bb.readBits(ilog2(tiles[1] - tileDictionary[1]));
                        }
                        value += tileDictionary[1];
                        tilemap.foreground[dst_addr++] = value;
                        break;
                    case 2:
                        value = tiles[0]++;
                        tilemap.foreground[dst_addr++] = value;
                        break;
                    case 3:
                        value = tiles[1]++;
                        tilemap.foreground[dst_addr++] = value;
                        break;
                }
            } while ((dst_addr < t) && (tilemap.foreground[dst_addr] == 0));
        }
    } while(dst_addr < t);
    
    tilemap.background.assign(tilemap.foreground.begin() + t/2, tilemap.foreground.end());
    tilemap.foreground.erase(tilemap.foreground.begin() + t/2, tilemap.foreground.end());
    
    bb.advanceNextByte();
    tilemap.hmwidth = bb.readBits(8);
    tilemap.hmheight = bb.readBits(8);
    
    uint16_t hm_pattern = 0;
    uint16_t hm_rle_count = 0;
    
    tilemap.heightmap.assign(tilemap.hmwidth * tilemap.hmheight, 0);
    dst_addr = 0;
    for(size_t y = 0; y <  tilemap.hmheight; y++)
    {
        for(size_t x = 0; x <  tilemap.hmwidth; x++)
        {
            if(!hm_rle_count--)
            {
                uint8_t read_count = 0;
                hm_rle_count = 0;
                hm_pattern = bb.readBits(16);
                do
                {
                    read_count = bb.readBits(8);
                    hm_rle_count += read_count;
                } while(read_count == 0xFF);
            }
            tilemap.heightmap[dst_addr++] = hm_pattern;
        }
    }
    return t;
}