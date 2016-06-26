#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <cstdint>

class Palette
{
public:
    Palette(const uint8_t* src);

    uint8_t getR(uint8_t index) const;
    uint8_t getG(uint8_t index) const;
    uint8_t getB(uint8_t index) const;
    uint8_t getA(uint8_t index) const;
private:
    struct PaletteEntry
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    std::array<PaletteEntry, 16> pal_;
};

#endif // PALETTE_H
