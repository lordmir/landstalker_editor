#ifndef SPRITE_H
#define SPRITE_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include "Palette.h"
#include "Rom.h"
#include "SpriteFrame.h"
#include "SpriteGraphic.h"
#include "ImageBuffer.h"

class Sprite
{
public:
	Sprite();
	Sprite(const Rom& rom, uint8_t id);
	int GetID() const;
	int GetGraphicsIdx() const;
	const SpriteGraphic& GetGraphics() const;
	Palette GetPalette() const;
	std::string GetName() const;
	void Draw(ImageBuffer& imgbuf, size_t animation = 0, size_t frame = 0, uint8_t palette_idx = 2, size_t x = 0, size_t y = 0, float scale = 1.0) const;
private:
	static bool m_cache_init;
	static std::unordered_multimap<uint8_t, uint8_t>  m_sprite_palette_lookup;
	static std::unordered_map<uint8_t, uint8_t>  m_sprite_gfx_lookup;
	static std::vector<SpriteFrame> m_sprite_frames;
	static std::vector<SpriteGraphic> m_sprite_graphics;
	int m_sprite_id;
};

#endif // SPRITE_H
