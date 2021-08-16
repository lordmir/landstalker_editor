#include "Tilemap2D.h"

Tilemap2D::Tilemap2D(std::size_t width, std::size_t height, std::size_t left, std::size_t top, uint8_t palette)
	: Tilemap(width, height, left, top, palette)
{
}

void Tilemap2D::SetTileset(std::shared_ptr<Tileset> tileset)
{
	m_tileset = tileset;
}

std::shared_ptr<Tileset> Tilemap2D::GetTileset()
{
	return m_tileset;
}

std::shared_ptr<const Tileset> Tilemap2D::GetTileset() const
{
	return m_tileset;
}

TilePoint Tilemap2D::XYToTilePoint(const wxPoint& point) const
{
	return TilePoint({ (point.x - GetLeft()) / TILEWIDTH, (point.y - GetTop()) / TILEHEIGHT });
}

wxPoint Tilemap2D::ToXYPoint(const TilePoint& point) const
{
	return wxPoint(point.x * TILEWIDTH + GetLeft(), point.y * TILEHEIGHT + GetTop());
}

void Tilemap2D::Draw(ImageBuffer& imgbuf) const
{
	TilePoint tilepos{ 0,0 };
	for (const auto& tile : m_tilevals)
	{
		wxPoint loc(ToXYPoint(tilepos));
		imgbuf.InsertTile(loc.x, loc.y, GetPalette(), Tile(tile), *GetTileset());
		tilepos.x++;
		if (tilepos.x == GetWidth())
		{
			tilepos.x = 0;
			tilepos.y++;
		}
	}
}

Tile Tilemap2D::GetTile(const TilePoint& point) const
{
	return Tile(GetTileValue(point));
}

void Tilemap2D::SetTile(const TilePoint& point, const Tile& tile)
{
	SetTileValue(point, tile.GetTileValue());
}

std::size_t Tilemap2D::GetBitmapWidth() const
{
	return GetWidth() * TILEWIDTH + GetLeft();
}

std::size_t Tilemap2D::GetBitmapHeight() const
{
	return GetHeight() * TILEHEIGHT + GetTop();
}
