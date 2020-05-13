#include "Blockmap2D.h"
#include <sstream>
#include "Utils.h"

Blockmap2D::Blockmap2D(size_t width, size_t height, size_t left, size_t top, uint8_t palette)
	: Tilemap(width, height, left, top, palette)
{
}

TilePoint Blockmap2D::XYToTilePoint(const wxPoint& point) const
{
	return TilePoint({ (point.x - GetLeft()) / TILEWIDTH, (point.y - GetTop()) / TILEHEIGHT });
}

wxPoint Blockmap2D::ToXYPoint(const TilePoint& point) const
{
	return wxPoint(point.x * TILEWIDTH + GetLeft(), point.y * TILEHEIGHT + GetTop());
}

void Blockmap2D::Draw(ImageBuffer& imgbuf) const
{
	TilePoint tilepos{ 0,0 };
	for (auto tile : m_tilevals)
	{
		wxPoint loc(ToXYPoint(tilepos));
		if (tile >= m_blockset->size())
		{
			std::ostringstream ss;
			ss << "Attempt to index out of range block " << std::hex << tile << " - maximum is " << (m_blockset->size() - 1);
			Debug(ss.str());
			tile = 0;
		}
		imgbuf.InsertBlock(loc.x, loc.y, GetPalette(), m_blockset->at(tile), *GetTileset());
		tilepos.x++;
		if (tilepos.x == GetWidth())
		{
			tilepos.x = 0;
			tilepos.y++;
		}
	}
}

void Blockmap2D::SetTileset(std::shared_ptr<Tileset> tileset)
{
	m_tileset = tileset;
}

std::shared_ptr<Tileset> Blockmap2D::GetTileset()
{
	return m_tileset;
}

std::shared_ptr<const Tileset> Blockmap2D::GetTileset() const
{
	return m_tileset;
}

void Blockmap2D::SetBlockset(std::shared_ptr<std::vector<BigTile>> blockset)
{
	m_blockset = blockset;
}

std::shared_ptr<std::vector<BigTile>> Blockmap2D::GetBlockset()
{
	return m_blockset;
}

std::shared_ptr<const std::vector<BigTile>> Blockmap2D::GetBlockset() const
{
	return m_blockset;
}

const BigTile& Blockmap2D::GetBigTile(const TilePoint& point) const
{
	return m_blockset->at(GetTileValue(point));
}

size_t Blockmap2D::GetBitmapWidth() const
{
	return GetWidth() * TILEWIDTH + GetLeft();
}

size_t Blockmap2D::GetBitmapHeight() const
{
	return GetHeight() * TILEHEIGHT + GetTop();
}
