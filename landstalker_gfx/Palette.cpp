#include "Palette.h"

Palette::Palette()
{
    pal_[0].r = 0x00;
    pal_[0].g = 0x00;
    pal_[0].b = 0x00;
    pal_[0].a = 0x00;

    pal_[1].r = 0xD8;
    pal_[1].g = 0xD8;
    pal_[1].b = 0xD8;
    pal_[1].a = 0xFF;

    for (std::size_t i = 0; i < 16; ++i)
    {
        pal_[15].r = 0x00;
        pal_[15].g = 0x00;
        pal_[15].b = 0x00;
        pal_[15].a = 0xFF;
    }
}

Palette::Palette(const uint8_t* src, std::size_t offset, const PaletteType& type)
    : Palette()
{
    Load(src, offset, type);
}

void Palette::Load(const uint8_t* src, std::size_t offset, const PaletteType& type)
{
    std::size_t begin = 0;
    std::size_t length = 16;
    switch (type)
    {
    case ROOM_PALETTE:
        begin = 2;
        length = 13;
        break;
    case SPRITE_HIGH_PALETTE:
        begin = 8;
        length = 7;
        break;
    case SPRITE_LOW_PALETTE:
        begin = 2;
        length = 6;
        break;
    case FULL_PALETTE:
    default:
        break;
    }

    src += offset * length * 2;
    
    for(std::size_t i = begin; i < (begin + length); ++i)
    {
        pal_[i].b = std::min(0xFF, (*src++ & 0x0F) * 18);
        pal_[i].g = std::min(0xFF, (*src >> 4)     * 18);
        pal_[i].r = std::min(0xFF, (*src++ & 0x0F) * 18);
        pal_[i].a = 0xFF;
    }
}

uint8_t Palette::getR(uint8_t index) const
{
    return pal_[index].r;
}

uint8_t Palette::getG(uint8_t index) const
{
    return pal_[index].g;
}

uint8_t Palette::getB(uint8_t index) const
{
    return pal_[index].b;
}

uint8_t Palette::getA(uint8_t index) const
{
    return pal_[index].a;
}

uint32_t Palette::getRGBA(uint8_t index) const
{
	uint32_t ret = pal_[index].r;
    ret |= pal_[index].g << 8;
    ret |= pal_[index].b << 16;
    ret |= pal_[index].a << 24;
    return ret;
}
