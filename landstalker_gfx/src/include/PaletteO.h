#ifndef PALETTE_O_H
#define PALETTE_O_H

#include <array>
#include <vector>
#include <cstdint>

#include "Rom.h"

struct PaletteEntryO
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

class PaletteO
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

	PaletteO();
    PaletteO(const Rom& rom);
    PaletteO(const Rom& rom, std::size_t offset, const Type& type);

    void Load(std::size_t index = 0);
	void Clear();
	void LoadDebugPal();
    void AddHighPalette(const PaletteO& high_palette);
    void RemoveHighPalette();
    void AddLowPalette(const PaletteO& low_palette);
    void RemoveLowPalette();
	size_t GetPaletteCount(Type type) const;

    uint8_t  getR(uint8_t index) const;
    uint8_t  getG(uint8_t index) const;
    uint8_t  getB(uint8_t index) const;
    uint8_t  getA(uint8_t index) const;
    uint32_t getRGBA(uint8_t index) const;
    uint16_t getGenesisColour(uint8_t index) const;
	PaletteEntryO GetColour(uint8_t index) const;
    void setGenesisColour(uint8_t index, uint16_t colour);
    bool ColourInRange(uint8_t colour) const;
    bool ColourEditable(uint8_t colour) const;
    const std::array<bool, 16>& GetLockedColours() const;

	static PaletteEntryO FromGenesisColour(uint16_t colour, bool transparent = false);
	static uint16_t     ToGenesisColour(const PaletteEntryO& entry);

    static void Reset();
private:


	static bool m_cache_init;
	static std::vector<std::array<uint16_t, 7>>  m_sprite_high_palettes;
	static std::vector<std::array<uint16_t, 6>>  m_sprite_low_palettes;
	static std::vector<std::array<uint16_t, 13>> m_room_palettes;
    static std::vector<std::array<uint16_t, 2>>  m_lava_palettes;
    static std::vector<std::array<uint16_t, 2>>  m_equipped_sword_palette;
    static std::vector<std::array<uint16_t, 2>>  m_equipped_armour_palette;
    static std::vector<std::array<uint16_t, 16>> m_full_palettes;
    static std::vector<std::array<uint16_t, 5>>  m_hud_palette;
    static std::vector<std::array<uint16_t, 8>>  m_inventory_palette_1_8col;
    static std::vector<std::array<uint16_t, 2>>  m_kazalt_warp_palette;
    static std::vector<std::array<uint16_t, 7>>  m_sega_logo_palette;
    static std::vector<std::array<uint16_t, 28>> m_title_seq_blue_palette;
    static std::vector<std::array<uint16_t, 5>>  m_title_seq_yellow_pallette;
    static std::vector<std::array<uint16_t, 1>>  m_title_seq_single_colours;
    static std::vector<std::array<uint16_t, 4>>  m_climax_logo_palette;
    static std::vector<std::array<uint16_t, 4>>  m_end_credits_palette;
    static std::vector<std::array<uint16_t, 2>>  m_projectiles_palette;

    Type m_type;
    std::array<PaletteEntryO, 16> m_pal;
};

class Colour
{
public:
	Colour(PaletteEntryO p)
	{
		colour = p;
	}
	Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0)
	{
		colour = { r, g, b, a };
	}
	Colour(uint16_t c)
	{
		colour = PaletteO::FromGenesisColour(c);
	}
	Colour(uint8_t idx, const PaletteO& pal)
	{
		colour = pal.GetColour(idx);
	}
	Colour(uint32_t c, bool alpha = true)
	{
		if (alpha == true)
		{
			colour.a = (c >> 24) & 0xFF;
		}
		else
		{
			colour.a = 0;
		}
		colour.r = (c >> 16) & 0xFF;
		colour.g = (c >> 8) & 0xFF;
		colour.b = c & 0xFF;
	}

	inline uint8_t GetR() const
	{
		return colour.r;
	}
	inline uint8_t GetG() const
	{
		return colour.g;
	}
	inline uint8_t GetB() const
	{
		return colour.b;
	}
	inline uint8_t GetA() const
	{
		return colour.a;
	}
	uint16_t GetGenesis() const
	{
		return PaletteO::ToGenesisColour(colour);
	}
	uint32_t GetRGB(bool include_alpha) const
	{
		uint32_t result = 0;
		if (include_alpha == true)
		{
			result = colour.a << 24;
		}
		result |= colour.r << 16;
		result |= colour.g << 8;
		result |= colour.b;
		return result;
	}

	template<class Iter>
	inline Iter& CopyRGBA(Iter& begin)
	{
		*begin++ = colour.a;
		*begin++ = colour.r;
		*begin++ = colour.g;
		*begin++ = colour.b;
		return begin;
	}
	template<class Iter>
	inline Iter& CopyRGB(Iter& begin)
	{
		*begin++ = colour.r;
		*begin++ = colour.g;
		*begin++ = colour.b;
		return begin;
	}
	template<class Iter>
	inline Iter& CopyA(Iter& begin)
	{
		*begin++ = colour.a;
		return begin;
	}
private:
	PaletteEntryO colour;
};

#endif // PALETTE_O_H
