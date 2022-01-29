#ifndef BLOCKMAP2D_H
#define BLOCKMAP2D_H

#include "Tilemap.h"

#include <vector>
#include "Tile.h"
#include "Block.h"

class Blockmap2D : public Tilemap
{
public:
	Blockmap2D(std::size_t width, std::size_t height, std::size_t left, std::size_t top, uint8_t palette);
	virtual ~Blockmap2D() = default;
	virtual TilePoint XYToTilePoint(const wxPoint& point) const;
	virtual wxPoint ToXYPoint(const TilePoint& point) const;
	virtual void Draw(ImageBuffer& imgbuf) const;
	void SetTileset(std::shared_ptr<Tileset> tileset);
	std::shared_ptr<Tileset> GetTileset();
	std::shared_ptr<const Tileset> GetTileset() const;
	void SetBlockset(std::shared_ptr<std::vector<Block>> blockset);
	std::shared_ptr<std::vector<Block>> GetBlockset();
	std::shared_ptr<const std::vector<Block>> GetBlockset() const;
	const Block& GetBigTile(const TilePoint& point) const;
	virtual std::size_t GetBitmapWidth() const;
	virtual std::size_t GetBitmapHeight() const;

protected:
	const std::size_t TILEWIDTH = 16;
	const std::size_t TILEHEIGHT = 16;

private:
	std::shared_ptr<Tileset> m_tileset;
	std::shared_ptr<std::vector<Block>> m_blockset;
};

#endif // TILEMAP2D_H