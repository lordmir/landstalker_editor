#ifndef _BLOCKSETCMP_H
#define _BLOCKSETCMP_H

#include <cstdint>
#include <vector>
#include <landstalker/blockset/include/Block.h>

class BlocksetCmp
{
public:
    static uint16_t Decode(const uint8_t* src, size_t length, Blockset& blocks);
    static uint16_t Encode(const Blockset& blocks, uint8_t* dst, size_t bufsize);
private:
    BlocksetCmp();
};

#endif // _BLOCKSETCMP_H
