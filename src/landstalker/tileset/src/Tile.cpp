#include <landstalker/tileset/include/Tile.h>

#include <sstream>
#include <iomanip>

#include <landstalker/misc/include/Utils.h>

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
  m_index(value & 0x7FF)
{
}

void Tile::SetIndex(uint16_t index)
{
    m_index = index & 0x7FF;
}

uint16_t Tile::GetIndex() const
{
    return m_index;
}

uint16_t Tile::GetTileValue() const
{
    uint16_t tv = m_index;
    
    if(m_attrs.getAttribute(TileAttributes::Attribute::ATTR_PRIORITY)) tv |= 0x8000;
    if(m_attrs.getAttribute(TileAttributes::Attribute::ATTR_VFLIP)) tv |= 0x1000;
    if(m_attrs.getAttribute(TileAttributes::Attribute::ATTR_HFLIP)) tv |= 0x0800;
    
    return tv;
}

Tile Tile::operator+(const Tile& rhs) const
{
	Tile t(*this);
	t.SetIndex(t.GetIndex() + rhs.GetIndex());
	return t;
}

Tile Tile::operator-(const Tile& rhs) const
{
	Tile t(*this);
	t.SetIndex(t.GetIndex() - rhs.GetIndex());
	return t;
}

Tile Tile::operator~() const
{
	Tile t(*this);
	t.m_attrs.toggleAttribute(TileAttributes::Attribute::ATTR_VFLIP);
	return t;
}

Tile Tile::operator!() const
{
	Tile t(*this);
	t.m_attrs.toggleAttribute(TileAttributes::Attribute::ATTR_HFLIP);
	return t;
}

Tile Tile::operator*() const
{
	Tile t(*this);
	t.m_attrs.toggleAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
	return t;
}

Tile& Tile::operator+=(const Tile& rhs)
{
	this->SetIndex(this->GetIndex() + rhs.GetIndex());
	return *this;
}

Tile& Tile::operator-=(const Tile& rhs)
{
	this->SetIndex(this->GetIndex() - rhs.GetIndex());
	return *this;
}

Tile& Tile::operator++()
{
	this->SetIndex(this->GetIndex() + 1);
	return *this;
}

Tile& Tile::operator--()
{
	this->SetIndex(this->GetIndex() - 1);
	return *this;
}

Tile Tile::operator++(int)
{
	Tile t(*this);
	this->SetIndex(this->GetIndex() + 1);
	return t;
}

Tile Tile::operator--(int)
{
	Tile t(*this);
	this->SetIndex(this->GetIndex() - 1);
	return t;
}

bool Tile::operator==(const Tile& rhs) const
{
	return (this->GetTileValue() == rhs.GetTileValue());
}

bool Tile::operator!=(const Tile& rhs) const
{
	return (this->GetTileValue() != rhs.GetTileValue());
}

bool Tile::operator<(const Tile& rhs) const
{
	return (this->GetTileValue() < rhs.GetTileValue());
}

bool Tile::operator>(const Tile& rhs) const
{
	return (this->GetTileValue() > rhs.GetTileValue());
}

bool Tile::operator<=(const Tile& rhs) const
{
	return (this->GetTileValue() <= rhs.GetTileValue());
}

bool Tile::operator>=(const Tile& rhs) const
{
	return (this->GetTileValue() >= rhs.GetTileValue());
}

std::string Tile::Print() const
{
    return Hex(GetTileValue());
}

void Tile::SetTileValue(uint16_t value)
{
    m_index = value & 0x7FF;
    m_attrs = TileAttributes();
    if (value && 0x8000) m_attrs.setAttribute(TileAttributes::Attribute::ATTR_PRIORITY);
    if (value && 0x1000) m_attrs.setAttribute(TileAttributes::Attribute::ATTR_VFLIP);
    if (value && 0x0800) m_attrs.setAttribute(TileAttributes::Attribute::ATTR_HFLIP);
}

TileAttributes& Tile::Attributes()
{
    return m_attrs;
}

const TileAttributes& Tile::Attributes() const
{
    return m_attrs;
}