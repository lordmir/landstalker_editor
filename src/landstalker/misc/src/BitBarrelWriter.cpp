#include <landstalker/misc/include/BitBarrelWriter.h>

#include <climits>

BitBarrelWriter::BitBarrelWriter()
    :m_bitpos(-1)
{}

template <class T>
void BitBarrelWriter::Write(T value)
{
    WriteBits(static_cast<uint32_t>(value), sizeof(value) * CHAR_BIT);
}

template <>
void BitBarrelWriter::Write(bool value)
{
    SetNextBit(value);
}

void BitBarrelWriter::WriteBits(uint32_t value, size_t numBits)
{
    while (numBits--)
    {
        SetNextBit((value & (1 << numBits)) > 0);
    }
}

void BitBarrelWriter::AdvanceNextByte()
{
    if (m_bitpos != 7)
    {
        m_bitpos = 7;
        m_buffer.push_back(0);
    }
}

void BitBarrelWriter::SetNextBit(bool value)
{
    if (m_bitpos < 0)
    {
        AdvanceNextByte();
    }
    m_buffer.back() |= static_cast<uint8_t>(value) << m_bitpos;
    m_bitpos--;
}

size_t BitBarrelWriter::GetByteCount() const
{
    return m_buffer.size();
}

std::vector<uint8_t>::const_iterator BitBarrelWriter::Begin() const
{
    return m_buffer.cbegin();
}

std::vector<uint8_t>::const_iterator BitBarrelWriter::End() const
{
    return m_buffer.cend();
}

template void BitBarrelWriter::Write(bool);
template void BitBarrelWriter::Write(uint8_t);
template void BitBarrelWriter::Write(uint16_t);
template void BitBarrelWriter::Write(uint32_t);
template void BitBarrelWriter::Write(int8_t);
template void BitBarrelWriter::Write(int16_t);
template void BitBarrelWriter::Write(int32_t);
