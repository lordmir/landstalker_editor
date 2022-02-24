#ifndef BLOCKMAP2D_H
#define BLOCKMAP2D_H

#include "Tilemap.h"

#include <vector>
#include "Tile.h"
#include "Block.h"

class Blockmap2D : public Tilemap
{
public:
	Blockmap2D(std::size_t width, std::size_t height, std::size_t tile_width, std::size_t tile_height, std::size_t left, std::size_t top, uint8_t palette);
	virtual ~Blockmap2D() = default;
	virtual TilePoint XYToTilePoint(const wxPoint& point) const;
	virtual wxPoint ToXYPoint(const TilePoint& point) const;
	virtual void Draw(ImageBuffer& imgbuf) const;
	void SetTileset(std::shared_ptr<Tileset> tileset);
	std::shared_ptr<Tileset> GetTileset();
	std::shared_ptr<const Tileset> GetTileset() const;
	void SetBlockset(BlockSetPtr blockset);
	std::shared_ptr<std::vector<MapBlock>> GetBlockset();
	std::shared_ptr<const std::vector<MapBlock>> GetBlockset() const;
	const MapBlock& GetBigTile(const TilePoint& point) const;
	const std::size_t GetBlockWidth() const;
	const std::size_t GetBlockHeight() const;
	virtual std::size_t GetWidthInTiles() const;
	virtual std::size_t GetHeightInTiles() const;
private:
	std::shared_ptr<Tileset> m_tileset;
	BlockSetPtr m_blockset;
};

#endif // TILEMAP2D_H