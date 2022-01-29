#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <vector>
#include <cstdint>

#include "Rom.h"

struct PaletteEntry
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
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

	Palette();
    Palette(const Rom& rom);
    Palette(const Rom& rom, std::size_t offset, const Type& type);

    void Load(std::size_t index = 0);
	void Clear();
	void LoadDebugPal();
    void AddHighPalette(const Palette& high_palette);
    void RemoveHighPalette();
    void AddLowPalette(const Palette& low_palette);
    void RemoveLowPalette();
	size_t GetPaletteCount(Type type) const;

    uint8_t  getR(uint8_t index) const;
    uint8_t  getG(uint8_t index) const;
    uint8_t  getB(uint8_t index) const;
    uint8_t  getA(uint8_t index) const;
    uint32_t getRGBA(uint8_t index) const;
    uint16_t getGenesisColour(uint8_t index) const;
    void setGenesisColour(uint8_t index, uint16_t colour);
    bool ColourInRange(uint8_t colour) const;
    bool ColourEditable(uint8_t colour) const;
    const std::array<bool, 16>& GetLockedColours() const;

	static PaletteEntry FromGenesisColour(uint16_t colour, bool transparent = false);
	static uint16_t     ToGenesisColour(const PaletteEntry& entry);

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
    std::array<PaletteEntry, 16> m_pal;
};

#endif // PALETTE_H
