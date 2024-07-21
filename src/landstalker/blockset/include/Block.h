#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <vector>
#include <memory>
#include <landstalker/tileset/include/Tile.h>

template<std::size_t N, std::size_t M>
class Block
{
public:
    typedef std::array<Tile, N*M> TileArray;
    typedef std::vector<Tile> TileVector;
    
    Block();
    Block(const TileVector::const_iterator& begin, const TileVector::const_iterator& end);

    bool operator==(const Block& rhs) const;
    bool operator!=(const Block& rhs) const;
    
    const Tile& GetTile(std::size_t tileIndex) const;
    const Tile& GetTile(std::size_t x, std::size_t y) const;
    void SetTile(std::size_t tileIndex, const Tile& tile);
    void SetTile(std::size_t x, std::size_t y, const Tile& tile);
    static constexpr std::size_t GetBlockWidth() { return N; }
    static constexpr std::size_t GetBlockHeight() { return M; }
    static constexpr std::size_t GetBlockSize() { return N*M; }
    
    std::string Print() const;
    
private:
    TileArray tiles;
};

typedef Block<2, 2> MapBlock;
typedef std::vector<MapBlock> Blockset;
typedef std::shared_ptr<Blockset> BlocksetPtr;
typedef std::pair<BlocksetPtr, BlocksetPtr> BlocksetPair;

#endif // BLOCK_H
