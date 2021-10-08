#include "BigTile.h"
#include <vector>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <string>

BigTile::BigTile()
{
}

BigTile::BigTile(const std::vector<Tile>::const_iterator& begin, const std::vector<Tile>::const_iterator& end)
{
    assert(std::distance(begin, end) == 4);
    std::vector<Tile>::const_iterator it = begin;
    std::copy(begin, end, tiles.begin());
}

uint16_t getTileValue(const Tile& tile)
{
    uint16_t tv = tile.GetIndex();
    
    if(tile.Attributes().getAttribute(TileAttributes::ATTR_PRIORITY)) tv |= 0x8000;
    if(tile.Attributes().getAttribute(TileAttributes::ATTR_VFLIP)) tv |= 0x1000;
    if(tile.Attributes().getAttribute(TileAttributes::ATTR_HFLIP)) tv |= 0x0800;
    
    return tv;
}

std::string BigTile::print() const
{
    std::ostringstream ss;
    for(auto t: tiles)
    {
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
           << getTileValue(t) << " ";
    }
    return ss.str();
}

const Tile& BigTile::getTile(std::size_t tileIndex) const
{
    return tiles[tileIndex];
}