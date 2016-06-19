#ifndef LSBIGTILES_H
#define LSBIGTILES_H

#include <cstdlib>
#include <cstdint>

class lsBigTiles
{
public:
    static size_t ExtractBigTiles(const uint8_t* src, uint8_t* dst);
private:
    lsBigTiles();
};

#endif // LSBIGTILES_H
