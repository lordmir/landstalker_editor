#ifndef BLOCKMAP_ISOMETRIC_H
#define BLOCKMAP_ISOMETRIC_H

#include "Blockmap2D.h"

#include <vector>
#include "Tile.h"
#include "BigTile.h"

struct TilePoint3D
{
	std::size_t x;
	std::size_t y;
	std::size_t z;
};

class BlockmapIsometric : public Blockmap2D
{
public:
	BlockmapIsometric(std::size_t width, std::size_t height, std::size_t left, std::size_t top, uint8_t palette);
	virtual ~BlockmapIsometric() = default;
	virtual TilePoint XYToTilePoint(const wxPoint& point) const;
	virtual wxPoint ToXYPoint(const TilePoint& point) const;
	wxPoint ToXYPoint3D(const TilePoint3D& point) const;
	virtual std::size_t GetBitmapWidth() const;
	virtual std::size_t GetBitmapHeight() const;

private:
	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<std::vector<BigTile>> m_blockset;
};

#endif // TILEMAP2D_H