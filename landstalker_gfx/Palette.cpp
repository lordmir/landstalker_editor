#include "Palette.h"

Palette::Palette(const uint8_t* src)
{
    pal_[0].r = 0x00;
    pal_[0].g = 0x00;
    pal_[0].b = 0x00;
    pal_[0].a = 0x00;
    
    pal_[1].r = 0xD8;
    pal_[1].g = 0xD8;
    pal_[1].b = 0xD8;
    pal_[1].a = 0xFF;
    
    pal_[15].r = 0x00;
    pal_[15].g = 0x00;
    pal_[15].b = 0x00;
    pal_[15].a = 0xFF;
    
    for(size_t i = 2; i < 15; ++i)
    {
        pal_[i].b = (*src++ & 0x0F) * 18;
        pal_[i].g = (*src >> 4)     * 18;
        pal_[i].r = (*src++ & 0x0F) * 18;
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
