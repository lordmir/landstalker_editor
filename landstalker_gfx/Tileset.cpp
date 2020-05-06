#include "Tileset.h"
#include <algorithm>

Tileset::Tileset()
{
}

Tileset::~Tileset()
{
}

void Tileset::setBits(const uint8_t* src, size_t num_tiles)
{
    m_tiles.clear();
    m_tiles.assign(num_tiles, std::vector<uint8_t>(WIDTH * HEIGHT));
    for(auto& t : m_tiles)
    {
        for (size_t i = 0; i < (WIDTH * HEIGHT / 2); ++i)
        {
            t[i * 2] = *src >> 4;
            t[i * 2 + 1] = *src++ & 0x0F;
        }
    }
}

std::vector<uint8_t> Tileset::getTile(const Tile& tile) const
{
    std::vector<uint8_t> ret(m_tiles[tile.getIndex()]);
    if (tile.attributes().getAttribute(TileAttributes::ATTR_VFLIP))
    {
        for (size_t i = 0; i < HEIGHT/2; ++i)
        {
            auto source_it = ret.begin() + WIDTH * i;
            auto dest_it = ret.end() - WIDTH * (i + 1);
            std::swap_ranges(source_it, source_it + WIDTH, dest_it);
        }
    }
    if (tile.attributes().getAttribute(TileAttributes::ATTR_HFLIP))
    {
        for (size_t i = 0; i < WIDTH; ++i)
        {
            auto source_it = ret.begin() + WIDTH * i;
            std::reverse(source_it, source_it + WIDTH);
        }
    }
    return ret;
}

size_t Tileset::size() const
{
    return m_tiles.size();
}
