#include "BlockmapIsometric.h"

BlockmapIsometric::BlockmapIsometric(std::size_t width, std::size_t height, std::size_t left, std::size_t top, uint8_t palette)
: Blockmap2D(width, height, left, top, palette)
{
}

TilePoint BlockmapIsometric::XYToTilePoint(const wxPoint& point) const
{
	TilePoint ret{ 0, 0 };
	int xgrid = (point.x - GetLeft()) / TILEWIDTH;
	int ygrid = (2 * (point.y - GetTop())) / TILEHEIGHT;
	ret.x = static_cast<std::size_t>((ygrid + xgrid - GetHeight() + 1) / 2);
	ret.y = static_cast<std::size_t>((ygrid - xgrid + GetHeight() - 1) / 2);
	return ret;
}

wxPoint BlockmapIsometric::ToXYPoint(const TilePoint& point) const
{
	int ix = (point.x - point.y + (GetHeight() - 1)) * TILEWIDTH + GetLeft();
	int iy = (point.x + point.y) * TILEHEIGHT / 2 + GetTop();
	return wxPoint{ ix, iy };
}

wxPoint BlockmapIsometric::ToXYPoint3D(const TilePoint3D& point) const
{
	int ix = (point.x - point.y + (GetHeight() - 1)) * TILEWIDTH + GetLeft();
	int iy = (point.x + point.y - point.z * 2) * TILEHEIGHT / 2 + GetTop();
	return wxPoint{ ix, iy };
}

std::size_t BlockmapIsometric::GetBitmapWidth() const
{
	return (GetWidth() + GetHeight()) * TILEWIDTH;
}

std::size_t BlockmapIsometric::GetBitmapHeight() const
{
	return (GetWidth() + GetHeight() + 1) * TILEHEIGHT / 2;
}
