#ifndef LSTILEMAPCMP_H
#define LSTILEMAPCMP_H

#include <cstdint>
#include <vector>

struct HeightMapCell
{
    HeightMapCell(uint16_t cell) : restrictions(cell >> 12),
                                   height((cell >> 8) & 0x0F),
                                   classification(cell & 0xFF)
    {}
    
    uint8_t restrictions;
    uint8_t height;
    uint8_t classification;
};

struct Tilemap
{
    void reset()
    {
        foreground.clear();
        background.clear();
        heightmap.clear();
        
        left = 0;
        top= 0;
        width = 0;
        height = 0;
        hmwidth = 0;
        hmheight = 0;
    }
    std::vector<uint16_t> foreground;
    std::vector<uint16_t> background;
    std::vector<HeightMapCell> heightmap;
    uint8_t left;
    uint8_t top;
    uint8_t width;
    uint8_t height;
    uint8_t hmwidth;
    uint8_t hmheight;
};

class LSTilemapCmp
{
public:
    static uint16_t Decode(const uint8_t* src, Tilemap& tilemap);
private:
    LSTilemapCmp();
};

#endif // LSTILEMAPCMP_H
