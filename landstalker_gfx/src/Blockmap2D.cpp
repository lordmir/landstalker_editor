#include "Blockmap2D.h"
#include <sstream>
#include <memory>
#include "Utils.h"

Blockmap2D::Blockmap2D(std::size_t width, std::size_t height, std::size_t tile_width, std::size_t tile_height, std::size_t left, std::size_t top, uint8_t palette)
	: Tilemap(width, height, tile_width, tile_height, left, top, palette)
{
}

TilePoint Blockmap2D::XYToTilePoint(const wxPoint& point) const
{
	return TilePoint({ (point.x - GetLeft()) / GetBlockWidth(), (point.y - GetTop()) / GetBlockHeight()});
}

wxPoint Blockmap2D::ToXYPoint(const TilePoint& point) const
{
	return wxPoint((point.x + GetLeft()) * GetBlockWidth(), (point.y * GetBlockHeight()) + GetTop());
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

void Blockmap2D::SetTileset(std::shared_ptr<const Tileset> tileset)
{
	m_tileset = tileset;
}

std::shared_ptr<const Tileset> Blockmap2D::GetTileset() const
{
	return m_tileset;
}

void Blockmap2D::SetBlockset(BlocksetPtr blockset)
{
	m_blockset = blockset;
}

BlocksetPtr Blockmap2D::GetBlockset()
{
	return m_blockset;
}

std::shared_ptr<const Blockset> Blockmap2D::GetBlockset() const
{
	return m_blockset;
}

const MapBlock& Blockmap2D::GetBigTile(const TilePoint& point) const
{
	return m_blockset->at(GetTileValue(point));
}

const std::size_t Blockmap2D::GetBlockWidth() const
{
	return TILE_WIDTH * MapBlock::GetBlockWidth();
}

const std::size_t Blockmap2D::GetBlockHeight() const
{
	return TILE_HEIGHT * MapBlock::GetBlockHeight();
}

std::size_t Blockmap2D::GetWidthInTiles() const
{
	return GetWidth() * MapBlock::GetBlockWidth() + GetLeft();
}

std::size_t Blockmap2D::GetHeightInTiles() const
{
	return GetHeight() * MapBlock::GetBlockHeight() + GetTop();
}
