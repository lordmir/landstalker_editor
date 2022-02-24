#include "BlockmapIsometric.h"

BlockmapIsometric::BlockmapIsometric(std::size_t width, std::size_t height, std::size_t tile_width, std::size_t tile_height, std::size_t left, std::size_t top, uint8_t palette)
: Blockmap2D(width, height, tile_width, tile_height, left, top, palette)
{
}

TilePoint BlockmapIsometric::XYToTilePoint(const wxPoint& point) const
{
	TilePoint ret{ 0, 0 };
	int xgrid = (point.x - GetLeft()) / GetBlockWidth();
	int ygrid = (2 * (point.y - GetTop())) / GetBlockHeight();
	ret.x = static_cast<std::size_t>((ygrid + xgrid - GetHeight() + 1) / 2);
	ret.y = static_cast<std::size_t>((ygrid - xgrid + GetHeight() - 1) / 2);
	return ret;
}

wxPoint BlockmapIsometric::ToXYPoint(const TilePoint& point) const
{
	int ix = (point.x - point.y + (GetHeight() - 1)) * GetBlockWidth() + GetLeft();
	int iy = (point.x + point.y) * GetBlockHeight() / 2 + GetTop();
	return wxPoint{ ix, iy };
}

wxPoint BlockmapIsometric::ToXYPoint3D(const TilePoint3D& point) const
{
	int ix = (point.x - point.y + (GetHeight() - 1)) * GetBlockWidth() + GetLeft();
	int iy = (point.x + point.y - point.z * 2) * GetBlockHeight() / 2 + GetTop();
	return wxPoint{ ix, iy };
}

std::size_t BlockmapIsometric::GetWidthInTiles() const
{
	return (GetWidth() + GetHeight()) * MapBlock::GetBlockWidth();
}

std::size_t BlockmapIsometric::GetHeightInTiles() const
{
	return (GetWidth() + GetHeight() + 1);
}
