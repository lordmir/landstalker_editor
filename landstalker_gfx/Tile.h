#ifndef TILE_H
#define TILE_H

#include <cstdint>
#include <string>
#include "TileAttributes.h"

class Tile
{
public:
    Tile();
    Tile(const TileAttributes& attrs, uint16_t index);
    
    void setIndex(uint16_t index);
    uint16_t getIndex() const;
    
    TileAttributes& attributes();
    const TileAttributes& attributes() const;
    
    std::string print() const;
    
//    void Draw(Tileset, Palette, x, y, zoom);
    
private:
    uint16_t getTileValue() const;

    TileAttributes attrs_;
    uint16_t index_;
};

#endif // TILE_H
