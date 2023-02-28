#ifndef SPRITE_FRAME_H
#define SPRITE_FRAME_H

#include <cstdint>
#include <vector>
#include <optional>
#include "Tileset.h"

class SpriteFrame
{
public:
	struct SubSprite
	{
		int x;
		int y;
		std::size_t w;
		std::size_t h;
		std::size_t tile_idx;
	};

	SpriteFrame(const std::vector<uint8_t>& src);
	SpriteFrame(const std::string& filename);
	bool Open(const std::string& filename);
	std::vector<uint8_t> GetBits();
	void SetBits(const std::vector<uint8_t>& src);
	bool Save(const std::string& filename, bool use_compression = false);
	void Clear();
	std::vector<uint8_t>  GetTile(const Tile& tile) const;
	std::pair<int, int>   GetTilePosition(const Tile& tile) const;
	std::vector<uint8_t>& GetTilePixels(int tile_index);
	std::vector<uint8_t>  GetTileRGB(const Tile& tile, std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const;
	std::vector<uint8_t>  GetTileA(const Tile& tile, std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const;
	std::vector<uint32_t> GetTileRGBA(const Tile& tile, std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const;
	const Tileset& GetTileset() const;
	Tileset& GetTileset();

	std::size_t GetTileCount() const;
	std::size_t GetExpectedTileCount() const;
	std::size_t GetSubSpriteCount() const;
	std::size_t GetTileWidth() const;
	std::size_t GetTileHeight() const;
	std::size_t GetTileBitDepth() const;
	bool        GetCompressed() const;
	void        SetCompressed(bool compressed);

	SubSprite& GetSubSprite(std::size_t idx);
	SubSprite  GetSubSprite(std::size_t idx) const;
	SubSprite& AddSubSpriteBefore(std::size_t idx);
	void       DeleteSubSprite(std::size_t idx);
	void       SwapSubSprite(std::size_t src1, std::size_t src2);

	PaletteO    GetSpritePalette(std::optional<PaletteO> low_palette, std::optional<PaletteO> high_palette) const;

private:
	std::vector<SubSprite> m_subsprites;
	Tileset m_sprite_gfx;
	bool m_compressed;
};

#endif // SPRITE_FRAME_H