#ifndef BIGTILESCMP_H
#define BIGTILESCMP_H

#include <cstdint>
#include <vector>
#include "BigTile.h"

class BigTilesCmp
{
public:
    static uint16_t Decode(const uint8_t* src, std::vector<BigTile>& tiles);
private:
    BigTilesCmp();
};

#endif // BIGTILESCMP_H
