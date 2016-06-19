#ifndef BITBARRELWRITER_H
#define BITBARRELWRITER_H

#include "BitBarrel.h"

class BitBarrelWriter : public BitBarrel
{
public:
    BitBarrelWriter(uint8_t* buf);
    
    template <class T>
    void write(T value);
    void writeBits(uint32_t value, size_t numBits);
    void setNextBit(bool value);
};

#endif // BITBARRELWRITER_H
