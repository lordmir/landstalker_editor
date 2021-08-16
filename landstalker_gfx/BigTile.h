#ifndef BIGTILE_H
#define BIGTILE_H

#include <array>
#include <vector>
#include "Tile.h"

class BigTile
{
public:
    typedef std::array<Tile, 4> TileArray;
    typedef std::vector<Tile> TileVector;
    
    BigTile();
    BigTile(const TileVector::const_iterator& begin, const TileVector::const_iterator& end);
    
    const Tile& getTile(std::size_t tileIndex) const;
    
    std::string print() const;
    
private:
    TileArray tiles;
};

#endif // BIGTILE_H
