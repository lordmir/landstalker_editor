#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <vector>
#include <memory>
#include "Tile.h"

template<std::size_t N, std::size_t M>
class Block
{
public:
    typedef std::array<Tile, N*M> TileArray;
    typedef std::vector<Tile> TileVector;
    
    Block();
    Block(const TileVector::const_iterator& begin, const TileVector::const_iterator& end);
    
    const Tile& GetTile(std::size_t tileIndex) const;
    const Tile& GetTile(std::size_t x, std::size_t y) const;
    static constexpr std::size_t GetBlockWidth() { return N; }
    static constexpr std::size_t GetBlockHeight() { return M; }
    static constexpr std::size_t GetBlockSize() { return N*M; }
    
    std::string Print() const;
    
private:
    TileArray tiles;
};

typedef Block<2, 2> MapBlock;
typedef std::vector<MapBlock> BlockSet;
typedef std::shared_ptr<BlockSet> BlockSetPtr;
typedef std::pair<BlockSetPtr, BlockSetPtr> BlockSets;

#endif // BLOCK_H
