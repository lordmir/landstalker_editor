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
		int x;
		int y;
		size_t w;
		size_t h;
		size_t tile_idx;
	};

	SpriteFrame(const uint8_t* src);

	std::vector<SubSprite> m_subsprites;
	Tileset m_sprite_gfx;
};

#endif // SPRITE_FRAME_H