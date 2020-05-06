#include "Sprite.h"

Sprite::Sprite()
	: m_sprite_gfx_idx(-1), m_high_palette(-1), m_low_palette(-1)
{
}

Sprite::Sprite(uint8_t graphics)
	: m_sprite_gfx_idx(graphics), m_high_palette(-1), m_low_palette(-1)
{
}

uint8_t Sprite::GetGraphicsIdx() const
{
	return m_sprite_gfx_idx;
}

void Sprite::SetHighPalette(uint8_t id)
{
	m_high_palette = id;
}

void Sprite::SetLowPalette(uint8_t id)
{
	m_low_palette = id;
}

Palette Sprite::GetPalette(const uint8_t* high_src, const uint8_t* low_src) const
{
	Palette pal;
	if (m_high_palette != -1)
	{
		pal.Load(high_src, m_high_palette, Palette::SPRITE_HIGH_PALETTE);
	}
	if (m_low_palette != -1)
	{
		pal.Load(low_src, m_low_palette, Palette::SPRITE_LOW_PALETTE);
	}
	return pal;
}
