#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <vector>
#include <cstdint>

#include "Rom.h"

class Colour
{
public:
	Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0)
		: m_r(r), m_g(g), m_b(b), m_a(a)
	{
	}
	Colour(uint16_t c, bool transparent = false)
	{
		m_b = std::min(0xFF, ((c & 0x0E00) >> 9) * 36);
		m_g = std::min(0xFF, ((c & 0x00E0) >> 5) * 36);
		m_r = std::min(0xFF, ((c & 0x000E) >> 1) * 36);
		m_a = transparent ? 0x00 : 0xFF;
	}
	Colour()
		: Colour(0x0000, true)
	{}

	inline uint8_t GetR() const
	{
		return m_r;
	}
	inline uint8_t GetG() const
	{
		return m_g;
	}
	inline uint8_t GetB() const
	{
		return m_b;
	}
	inline uint8_t GetA() const
	{
		return m_a;
	}
	uint16_t GetGenesis() const
	{
		uint16_t retval = 0x0000;
		retval |= std::min(0x07, m_r / 36) << 1;
		retval |= std::min(0x07, m_g / 36) << 5;
		retval |= std::min(0x07, m_b / 36) << 9;
		return retval;
	}
	uint32_t GetRGB(bool include_alpha) const
	{
		uint32_t result = 0;
		if (include_alpha == true)
		{
			result = m_a << 24;
		}
		result |= m_r << 16;
		result |= m_g << 8;
		result |= m_b;
		return result;
	}

	template<class Iter>
	inline Iter& CopyRGBA(Iter& begin)
	{
		*begin++ = m_a;
		*begin++ = m_r;
		*begin++ = m_g;
		*begin++ = m_b;
		return begin;
	}
	template<class Iter>
	inline Iter& CopyRGB(Iter& begin)
	{
		*begin++ = m_r;
		*begin++ = m_g;
		*begin++ = m_b;
		return begin;
	}
	template<class Iter>
	inline Iter& CopyA(Iter& begin)
	{
		*begin++ = m_a;
		return begin;
	}
private:
	uint8_t m_r;
	uint8_t m_g;
	uint8_t m_b;
	uint8_t m_a;
};

class Palette
{
public:

	enum class Type
	{
		NONE,
		FULL,
		LOW8,
		HUD,
		ROOM,
		SPRITE_HIGH,
		SPRITE_LOW,
		SPRITE_FULL,
		PROJECTILE,
		SWORD,
		ARMOUR,
		LAVA,
		WARP,
		SEGA_LOGO,
		CLIMAX_LOGO,
		TITLE_BLUE_FADE,
		TITLE_YELLOW,
		TITLE_SINGLE_COLOUR,
		END_CREDITS
	};

	Palette(const Type& type = Type::FULL);
	Palette(const std::vector<Colour>& colours, const Type& type);
	Palette(const std::vector<uint8_t>& bytes, const Palette::Type& type);

	std::vector<uint8_t> GetBytes() const;

	void Clear();
	void LoadDebugPal();
	void AddHighPalette(const Palette& high_palette);
	void RemoveHighPalette();
	void AddLowPalette(const Palette& low_palette);
	void RemoveLowPalette();

	uint8_t  getR(uint8_t index) const;
	uint8_t  getG(uint8_t index) const;
	uint8_t  getB(uint8_t index) const;
	uint8_t  getA(uint8_t index) const;
	uint32_t getRGBA(uint8_t index) const;
	uint16_t getGenesisColour(uint8_t index) const;
	Colour GetColour(uint8_t index) const;
	void setGenesisColour(uint8_t index, uint16_t colour);
	bool ColourInRange(uint8_t colour) const;
	bool ColourEditable(uint8_t colour) const;
	const std::vector<bool>& GetLockedColours() const;
	int GetSize() const;
	bool IsVarWidth() const;
private:

	Type m_type;
	std::vector<Colour> m_pal;
};

std::vector<Palette> CreatePalettes(const std::vector<uint8_t>& bytes, const Palette::Type& type);
std::vector<uint8_t> GetPaletteBytes(const std::vector<Palette>& palettes);

#endif // PALETTE_H
