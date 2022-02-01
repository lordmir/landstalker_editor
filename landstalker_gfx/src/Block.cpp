#include "Block.h"
#include <vector>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <string>

template<std::size_t N, std::size_t M>
Block<N,M>::Block()
{
}

template<std::size_t N, std::size_t M>
Block<N,M>::Block(const std::vector<Tile>::const_iterator& begin, const std::vector<Tile>::const_iterator& end)
{
    assert(std::distance(begin, end) == GetBlockSize());
    std::vector<Tile>::const_iterator it = begin;
    std::copy(begin, end, tiles.begin());
}

template<std::size_t N, std::size_t M>
std::string Block<N,M>::Print() const
{
    std::ostringstream ss;
    for(auto t: tiles)
    {
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
           << t.GetTileValue() << " ";
    }
    return ss.str();
}

template<std::size_t N, std::size_t M>
const Tile& Block<N,M>::GetTile(size_t tileIndex) const
{
    return tiles[tileIndex];
}

template<std::size_t N, std::size_t M>
const Tile& Block<N, M>::GetTile(std::size_t x, std::size_t y) const
{
    assert(x < N);
    assert(y < M);
    return GetTile(x + y * N);
}

template class Block<2,2>;