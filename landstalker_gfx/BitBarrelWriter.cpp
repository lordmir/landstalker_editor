#include "BitBarrelWriter.h"

#include <climits>

BitBarrelWriter::BitBarrelWriter(uint8_t* buf)
{
    m_buf = buf;
}

template <class T>
void BitBarrelWriter::write(T value)
{
    writeBits(static_cast<uint32_t>(value), sizeof(value) * CHAR_BIT);
}

template <>
void BitBarrelWriter::write(bool value)
{
    setNextBit(value);
}

void BitBarrelWriter::writeBits(uint32_t value, std::size_t numBits)
{
    for(std::size_t i = 0; i < numBits; ++i)
    {
        setNextBit(value & 1);
        value >>= 1;
    }
}

void BitBarrelWriter::setNextBit(bool value)
{
    if(m_pos == 0)
    {
        *m_buf++ = 0;
        m_pos = 7;
    }
    *m_buf |= static_cast<uint8_t>(value) << m_pos;
}

template void BitBarrelWriter::write(bool);
template void BitBarrelWriter::write(uint8_t);
template void BitBarrelWriter::write(uint16_t);
template void BitBarrelWriter::write(uint32_t);
template void BitBarrelWriter::write(int8_t);
template void BitBarrelWriter::write(int16_t);
template void BitBarrelWriter::write(int32_t);