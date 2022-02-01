#ifndef BLOCKSETCMP_H
#define BLOCKSETCMP_H

#include <cstdint>
#include <vector>
#include "Block.h"

class BlocksetCmp
{
public:
    static uint16_t Decode(const uint8_t* src, size_t length, std::vector<Block<2,2>>& blocks);
    static uint16_t Encode(const std::vector<Block<2,2>>& blocks, uint8_t* dst, size_t bufsize);
private:
    BlocksetCmp();
};

#endif // BLOCKSETCMP_H
