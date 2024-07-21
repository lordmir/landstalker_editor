#ifndef BITBARRELWRITER_H
#define BITBARRELWRITER_H

#include <landstalker/misc/include/BitBarrel.h>
#include <vector>

class BitBarrelWriter
{
public:
    BitBarrelWriter();

    template <class T>
    void Write(T value);
    void WriteBits(uint32_t value, size_t numBits);
    void AdvanceNextByte();
    void SetNextBit(bool value);
    size_t GetByteCount() const;
    std::vector<uint8_t>::const_iterator Begin() const;
    std::vector<uint8_t>::const_iterator End() const;

private:
    std::vector<uint8_t> m_buffer;
    int m_bitpos;
};

#endif // BITBARRELWRITER_H
