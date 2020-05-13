#ifndef LSTILEMAPCMP_H
#define LSTILEMAPCMP_H

#include <cstdint>
#include <vector>
#include "BlockmapIsometric.h"

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

struct RoomTilemap
{
    RoomTilemap()
    : foreground(0, 0, 0, 0, 0),
      background(0, 0, 16, 0, 0),
      left(0), top(0), width(0), height(0)
    {
    }

    RoomTilemap(uint8_t left_in, uint8_t top_in, uint8_t width_in, uint8_t height_in)
    : foreground(width, height, 0, 0, 0),
      background(width, height, 16, 0, 0),
      left(left_in), top(top_in), width(width_in), height(height_in)
    {
    }

    void reset()
    {
        foreground.Resize(0,0);
        background.Resize(0,0);
        heightmap.clear();
        
        left = 0;
        top= 0;
        width = 0;
        height = 0;
        hmwidth = 0;
        hmheight = 0;
    }

    void set(uint8_t left_in, uint8_t top_in, uint8_t width_in, uint8_t height_in)
    {
        left = left_in;
        top = top_in;
        width = width_in;
        height = height_in;
        hmwidth = 0;
        hmheight = 0;

        foreground.Resize(width, height);
        background.Resize(width, height);
        heightmap.clear();
    }

    uint8_t GetLeft() const { return left; }
    uint8_t GetTop() const { return top; }
    uint8_t GetWidth() const { return width; }
    uint8_t GetHeight() const { return height; }

    BlockmapIsometric foreground;
    BlockmapIsometric background;
    std::vector<HeightMapCell> heightmap;
    uint8_t hmwidth;
    uint8_t hmheight;
private:
    uint8_t left;
    uint8_t top;
    uint8_t width;
    uint8_t height;
};

class LSTilemapCmp
{
public:
    static uint16_t Decode(const uint8_t* src, RoomTilemap& tilemap);
private:
    LSTilemapCmp();
};

#endif // LSTILEMAPCMP_H
