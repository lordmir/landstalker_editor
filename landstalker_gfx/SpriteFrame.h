#ifndef SPRITE_FRAME_H
#define SPRITE_FRAME_H

#include <cstdint>
#include <vector>
#include "Tileset.h"

class SpriteFrame
{
public:
	struct SubSprite
	{
		std::size_t x;
		std::size_t y;
		std::size_t w;
		std::size_t h;
		std::size_t tile_idx;
	};

	SpriteFrame(const uint8_t* src);

	std::vector<SubSprite> m_subsprites;
	Tileset m_sprite_gfx;
};

#endif // SPRITE_FRAME_H