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

		SubSprite(int xp, int yp, int wp, int hp, int tidx = 0)
			: x(xp), y(yp), w(wp), h(hp), tile_idx(tidx) {}
		SubSprite()
			: x(0), y(0), w(0), h(0), tile_idx(0) {}

		bool operator==(const SubSprite& rhs) const
		{
			return ((this->x == rhs.x) &&
			        (this->y == rhs.y) &&
			        (this->w == rhs.w) &&
			        (this->h == rhs.h) &&
			        (this->tile_idx == rhs.tile_idx));
		}
		bool operator!=(const SubSprite& rhs) const
		{
			return !(*this == rhs);
		}
	};

	SpriteFrame(const std::vector<uint8_t>& src);
	SpriteFrame(const std::string& filename);
	SpriteFrame();
	SpriteFrame(const SpriteFrame& rhs);
	SpriteFrame& operator=(const SpriteFrame& rhs);

	bool operator==(const SpriteFrame& rhs) const;
	bool operator!=(const SpriteFrame& rhs) const;

	bool Open(const std::string& filename);
	std::vector<uint8_t> GetBits();
	std::vector<uint8_t> GetBits(bool compressed);
	std::size_t SetBits(const std::vector<uint8_t>& src);
	bool Save(const std::string& filename);
	bool Save(const std::string& filename, bool compressed);
	void Clear();

	int GetLeft() const;
	int GetRight() const;
	int GetWidth() const;
	int GetTop() const;
	int GetBottom() const;
	int GetHeight() const;

	std::vector<uint8_t>  GetTile(const Tile& tile) const;
	std::pair<int, int>   GetTilePosition(const Tile& tile) const;
	std::vector<uint8_t>& GetTilePixels(int tile_index);
	std::shared_ptr<const Tileset> GetTileset() const;
	std::shared_ptr<Tileset> GetTileset();

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
	void       SetSubSprites(const std::vector<SubSprite>& subs);

private:
	std::vector<SubSprite> m_subsprites;
	std::shared_ptr<Tileset> m_sprite_gfx;
	bool m_compressed;
};

#endif // SPRITE_FRAME_H