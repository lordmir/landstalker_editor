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
    Tile(uint16_t value);
    
    void SetIndex(uint16_t index);
    uint16_t GetIndex() const;
    
    TileAttributes& Attributes();
    const TileAttributes& Attributes() const;
    
    std::string Print() const;
    void SetTileValue(uint16_t value);
    uint16_t GetTileValue() const;
    
private:
    TileAttributes m_attrs;
    uint16_t m_index;
};

#endif // TILE_H
