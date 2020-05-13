#ifndef TILEMAP2D_H
#define TILEMAP2D_H

#include "Tilemap.h"

#include <vector>
#include "Tile.h"

class Tilemap2D : public Tilemap
{
public:
	Tilemap2D(size_t width, size_t height, size_t left, size_t top, uint8_t palette);
	virtual ~Tilemap2D() = default;
	virtual TilePoint XYToTilePoint(const wxPoint& point) const;
	virtual wxPoint ToXYPoint(const TilePoint& point) const;
	virtual void Draw(ImageBuffer& imgbuf) const;
	virtual void SetTileset(std::shared_ptr<Tileset> tileset);
	virtual std::shared_ptr<Tileset> GetTileset();
	virtual std::shared_ptr<const Tileset> GetTileset() const;
	Tile GetTile(const TilePoint& point) const;
	void SetTile(const TilePoint& point, const Tile& tile);
	virtual size_t GetBitmapWidth() const;
	virtual size_t GetBitmapHeight() const;

protected:
	const size_t TILEWIDTH = 8;
	const size_t TILEHEIGHT = 8;

private:
	std::shared_ptr<Tileset> m_tileset;
};

#endif // TILEMAP2D_H