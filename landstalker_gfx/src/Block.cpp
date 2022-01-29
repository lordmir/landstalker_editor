#include "Block.h"
#include <vector>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <string>

Block::Block()
{
}

Block::Block(const std::vector<Tile>::const_iterator& begin, const std::vector<Tile>::const_iterator& end)
{
    assert(std::distance(begin, end) == 4);
    std::vector<Tile>::const_iterator it = begin;
    std::copy(begin, end, tiles.begin());
}

std::string Block::Print() const
{
    std::ostringstream ss;
    for(auto t: tiles)
    {
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
           << t.GetTileValue() << " ";
    }
    return ss.str();
}

const Tile& Block::GetTile(size_t tileIndex) const
{
    return tiles[tileIndex];
}