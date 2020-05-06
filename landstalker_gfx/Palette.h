#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <cstdint>

class Palette
{
public:

    enum PaletteType
    {
        FULL_PALETTE,
        ROOM_PALETTE,
        SPRITE_LOW_PALETTE,
        SPRITE_HIGH_PALETTE,
    };

    Palette();
    Palette(const uint8_t* src, size_t offset, const PaletteType& type);

    void Load(const uint8_t* src, size_t offset, const PaletteType& type);

    uint8_t getR(uint8_t index) const;
    uint8_t getG(uint8_t index) const;
    uint8_t getB(uint8_t index) const;
    uint8_t getA(uint8_t index) const;
    uint32_t getRGBA(uint8_t index) const;
private:
    struct PaletteEntry
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    std::array<PaletteEntry, 16> pal_;
};

#endif // PALETTE_H
