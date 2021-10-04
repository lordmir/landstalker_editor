#include "Tileset.h"
#include <algorithm>
#include <sstream>
#include "Utils.h"

Tileset::Tileset()
{
}

Tileset::~Tileset()
{
}

void Tileset::setBits(const uint8_t* src, std::size_t num_tiles)
{
    m_tiles.clear();
    m_tiles.assign(num_tiles, std::vector<uint8_t>(WIDTH * HEIGHT));
    for(auto& t : m_tiles)
    {
        for (std::size_t i = 0; i < (WIDTH * HEIGHT / 2); ++i)
        {
            t[i * 2] = *src >> 4;
            t[i * 2 + 1] = *src++ & 0x0F;
        }
    }
}

std::vector<uint8_t> Tileset::getTile(const Tile& tile) const
{
    std::size_t idx = tile.GetIndex();
    if (idx >= m_tiles.size())
    {
        std::ostringstream ss;
        ss << "Attempt to obtain out-of-range tile " << idx;
        Debug(ss.str());
        idx = 0;
    }
    std::vector<uint8_t> ret(m_tiles[idx]);
    if (tile.Attributes().getAttribute(TileAttributes::ATTR_VFLIP))
    {
        for (std::size_t i = 0; i < HEIGHT/2; ++i)
        {
            auto source_it = ret.begin() + WIDTH * i;
            auto dest_it = ret.end() - WIDTH * (i + 1);
            std::swap_ranges(source_it, source_it + WIDTH, dest_it);
        }
    }
    if (tile.Attributes().getAttribute(TileAttributes::ATTR_HFLIP))
    {
        for (std::size_t i = 0; i < WIDTH; ++i)
        {
            auto source_it = ret.begin() + WIDTH * i;
            std::reverse(source_it, source_it + WIDTH);
        }
    }
    return ret;
}

std::size_t Tileset::size() const
{
    return m_tiles.size();
}
