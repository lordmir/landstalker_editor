#ifndef BLOCKMAP2D_H
#define BLOCKMAP2D_H

#include "Tilemap.h"

#include <vector>
#include "Tile.h"
#include "BigTile.h"

class Blockmap2D : public Tilemap
{
public:
	Blockmap2D(size_t width, size_t height, size_t left, size_t top, uint8_t palette);
	virtual ~Blockmap2D() = default;
	virtual TilePoint XYToTilePoint(const wxPoint& point) const;
	virtual wxPoint ToXYPoint(const TilePoint& point) const;
	virtual void Draw(ImageBuffer& imgbuf) const;
	void SetTileset(std::shared_ptr<Tileset> tileset);
	std::shared_ptr<Tileset> GetTileset();
	std::shared_ptr<const Tileset> GetTileset() const;
	void SetBlockset(std::shared_ptr<std::vector<BigTile>> blockset);
	std::shared_ptr<std::vector<BigTile>> GetBlockset();
	std::shared_ptr<const std::vector<BigTile>> GetBlockset() const;
	const BigTile& GetBigTile(const TilePoint& point) const;
	virtual size_t GetBitmapWidth() const;
	virtual size_t GetBitmapHeight() const;

protected:
	const size_t TILEWIDTH = 16;
	const size_t TILEHEIGHT = 16;

private:
	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<std::vector<BigTile>> m_blockset;
};

#endif // TILEMAP2D_H