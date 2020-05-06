#ifndef SPRITE_H
#define SPRITE_H

#include <cstdint>
#include "Palette.h"

class Sprite
{
public:
	Sprite();
	Sprite(uint8_t graphics);
	uint8_t GetGraphicsIdx() const;
	void SetHighPalette(uint8_t id);
	void SetLowPalette(uint8_t id);
	Palette GetPalette(const uint8_t* high_src, const uint8_t* low_src) const;
private:
	uint8_t m_sprite_gfx_idx;
	int m_high_palette;
	int m_low_palette;
};

#endif // SPRITE_H
