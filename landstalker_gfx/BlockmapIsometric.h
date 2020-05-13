#ifndef BLOCKMAP_ISOMETRIC_H
#define BLOCKMAP_ISOMETRIC_H

#include "Blockmap2D.h"

#include <vector>
#include "Tile.h"
#include "BigTile.h"

struct TilePoint3D
{
	size_t x;
	size_t y;
	size_t z;
};

class BlockmapIsometric : public Blockmap2D
{
public:
	BlockmapIsometric(size_t width, size_t height, size_t left, size_t top, uint8_t palette);
	virtual ~BlockmapIsometric() = default;
	virtual TilePoint XYToTilePoint(const wxPoint& point) const;
	virtual wxPoint ToXYPoint(const TilePoint& point) const;
	wxPoint ToXYPoint3D(const TilePoint3D& point) const;
	virtual size_t GetBitmapWidth() const;
	virtual size_t GetBitmapHeight() const;

private:
	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<std::vector<BigTile>> m_blockset;
};

#endif // TILEMAP2D_H