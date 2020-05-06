#include "Tile.h"
#include <sstream>
#include <iomanip>

Tile::Tile()
: attrs_(),
  index_(0)
{
}

Tile::Tile(const TileAttributes& attrs, uint16_t index)
    : attrs_(attrs),
    index_(index)
{
}

Tile::Tile(uint16_t index)
: attrs_(),
  index_(index)
{
}

void Tile::setIndex(uint16_t index)
{
    index_ = index;
}

uint16_t Tile::getIndex() const
{
    return index_;
}

uint16_t Tile::getTileValue() const
{
    uint16_t tv = getIndex();
    
    if(attributes().getAttribute(TileAttributes::ATTR_PRIORITY)) tv |= 0x8000;
    if(attributes().getAttribute(TileAttributes::ATTR_VFLIP)) tv |= 0x1000;
    if(attributes().getAttribute(TileAttributes::ATTR_HFLIP)) tv |= 0x0800;
    
    return tv;
}

std::string Tile::print() const
{
    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
           << getTileValue() << " ";
    return ss.str();
}

TileAttributes& Tile::attributes()
{
    return attrs_;
}

const TileAttributes& Tile::attributes() const
{
    return attrs_;
}