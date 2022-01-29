#ifndef BLOCK_H
#define BLOCK_H

#include <array>
#include <vector>
#include "Tile.h"

class Block
{
public:
    typedef std::array<Tile, 4> TileArray;
    typedef std::vector<Tile> TileVector;
    
    Block();
    Block(const TileVector::const_iterator& begin, const TileVector::const_iterator& end);
    
    const Tile& GetTile(size_t tileIndex) const;
    
    std::string Print() const;
    
private:
    TileArray tiles;
};

#endif // BLOCK_H
