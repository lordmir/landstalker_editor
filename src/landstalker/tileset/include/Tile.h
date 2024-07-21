#ifndef _TILE_H_
#define _TILE_H_

#include <cstdint>
#include <string>
#include <landstalker/tileset/include/TileAttributes.h>

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

    Tile operator+(const Tile& rhs) const;
    Tile operator-(const Tile& rhs) const;
    Tile operator~() const; /* Toggle HFlip */
    Tile operator!() const; /* Toggle VFlip */
    Tile operator*() const; /* Toggle Priority */

    Tile& operator+=(const Tile& rhs);
    Tile& operator-=(const Tile& rhs);

    Tile& operator++();
    Tile& operator--();
    Tile operator++(int);
    Tile operator--(int);

    bool operator==(const Tile& rhs) const;
    bool operator!=(const Tile& rhs) const;
    bool operator<(const Tile& rhs) const;
    bool operator>(const Tile& rhs) const;
    bool operator<=(const Tile& rhs) const;
    bool operator>=(const Tile& rhs) const;

    
private:
    TileAttributes m_attrs;
    uint16_t m_index;
};

#endif // _TILE_H_
