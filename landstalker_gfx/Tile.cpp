#include "Tile.h"
#include <sstream>
#include <iomanip>
#include "Utils.h"

Tile::Tile()
: m_attrs(),
  m_index(0)
{
}

Tile::Tile(const TileAttributes& attrs, uint16_t index)
: m_attrs(attrs),
  m_index(index)
{
}

Tile::Tile(uint16_t value)
: m_attrs(TileAttributes((value & 0x0800) > 0, (value & 0x1000) > 0, (value & 0x8000) > 0)),
  m_index(value & 0x3FF)
{
}

void Tile::SetIndex(uint16_t index)
{
    m_index = index;
}

uint16_t Tile::GetIndex() const
{
    return m_index;
}

uint16_t Tile::GetTileValue() const
{
    uint16_t tv = m_index;
    
    if(m_attrs.getAttribute(TileAttributes::ATTR_PRIORITY)) tv |= 0x8000;
    if(m_attrs.getAttribute(TileAttributes::ATTR_VFLIP)) tv |= 0x1000;
    if(m_attrs.getAttribute(TileAttributes::ATTR_HFLIP)) tv |= 0x0800;
    
    return tv;
}

std::string Tile::Print() const
{
    return Hex(GetTileValue());
}

void Tile::SetTileValue(uint16_t value)
{
    m_index = value & 0x3FF;
    m_attrs = TileAttributes();
}

TileAttributes& Tile::Attributes()
{
    return m_attrs;
}

const TileAttributes& Tile::Attributes() const
{
    return m_attrs;
}