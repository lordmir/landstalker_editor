#include <landstalker/misc/include/BitBarrel.h>

#include <climits>
#include <stdexcept>

BitBarrel::BitBarrel(const uint8_t* buf)
: m_start(buf),
  m_val(0),
  m_pos(8),
  m_buf(const_cast<uint8_t*>(buf))
{
}

BitBarrel::BitBarrel()
: m_start(0),
  m_val(0),
  m_pos(0),
  m_buf(NULL)
{
}

bool BitBarrel::empty()
{
    return(m_pos == 0);
}

void BitBarrel::newByte(uint8_t byte)
{
    m_val = byte;
    m_pos = 8;
}

BitBarrel::operator bool()
{
    bool retval = false;
    if(m_pos == 0)
    {
        throw std::runtime_error("Out of bits!");
    }
    else
    {
        retval = (m_val >= 0x80);
        m_val <<= 1;
        --m_pos;
    }
    return retval;
}

bool BitBarrel::full()
{
    return (m_pos == 8);
}

bool BitBarrel::operator()(bool rhs)
{
    if(m_pos < 8)
    {
        m_val <<= 1;
        m_val |= (rhs ? 1 : 0);
        m_pos++;
    }
    return rhs;
}

uint8_t BitBarrel::out()
{
    uint8_t tmp = m_val;
    m_pos = 0;
    m_val = 0;
    return tmp;
}

template <class T>
T BitBarrel::read() const
{
    return static_cast<T>(readBits(sizeof(T) * CHAR_BIT));
}

template <>
bool BitBarrel::read() const
{
    return getNextBit();
}

uint32_t BitBarrel::readBits(size_t numBits) const
{
    uint32_t retval = 0;
    for(size_t i = 0; i < numBits; ++i)
    {
        retval <<= 1;
        retval |= static_cast<uint32_t>(getNextBit());
    }
    return retval;
}

bool BitBarrel::getNextBit() const
{
    if(m_pos == 0)
    {
        m_pos = 8;
        m_buf++;
    }
    return (*m_buf & (1 << --m_pos)) ? true : false;
}

size_t BitBarrel::getBytePosition() const
{
    return m_buf - m_start;
}

void BitBarrel::advanceNextByte()
{
    if(m_pos != 8)
    {
        m_pos = 8;
        m_buf++;
    }
}

template bool     BitBarrel::read() const;
template uint8_t  BitBarrel::read() const;
template uint16_t BitBarrel::read() const;
template uint32_t BitBarrel::read() const;
template int8_t   BitBarrel::read() const;
template int16_t  BitBarrel::read() const;
template int32_t  BitBarrel::read() const;