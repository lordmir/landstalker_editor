#include "Tile.h"

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

void Tile::setIndex(uint16_t index)
{
    index_ = index;
}

uint16_t Tile::getIndex() const
{
    return index_;
}

TileAttributes& Tile::attributes()
{
    return attrs_;
}

const TileAttributes& Tile::attributes() const
{
    return attrs_;
}