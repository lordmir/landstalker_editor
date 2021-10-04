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

    enum class PaletteType
    {
        ROOM_PALETTE,
        SPRITE_HIGH_PALETTE,
        SPRITE_LOW_PALETTE,
    };

	Palette();
    Palette(const Rom& rom);
    Palette(const Rom& rom, size_t offset, const PaletteType& type);

    void Load(PaletteType type, size_t index = 0);
	void Clear();
	void LoadDebugPal();
	size_t GetPaletteCount(PaletteType type) const;

    uint8_t  getR(uint8_t index) const;
    uint8_t  getG(uint8_t index) const;
    uint8_t  getB(uint8_t index) const;
    uint8_t  getA(uint8_t index) const;
    uint32_t getRGBA(uint8_t index) const;
    uint16_t getGenesisColour(uint8_t index) const;

	static PaletteEntry FromGenesisColour(uint16_t colour, bool transparent = false);
	static uint16_t     ToGenesisColour(const PaletteEntry& entry);
private:

	static bool m_cache_init;
	static std::vector<std::array<uint16_t, 7>> m_sprite_high_palettes;
	static std::vector<std::array<uint16_t, 6>> m_sprite_low_palettes;
	static std::vector<std::array<uint16_t, 13>> m_room_palettes;


    std::array<PaletteEntry, 16> m_pal;
};

#endif // PALETTE_H
