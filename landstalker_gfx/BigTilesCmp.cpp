#include "BigTilesCmp.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "BitBarrel.h"


template<class T, size_t N>
class CircularBuffer
{
public:
    CircularBuffer() : m_idx(0)
    {
        erase();
    }
    CircularBuffer& operator=(T rhs)
    {
        *this[m_idx] = rhs;
        return *this;
    }
    const T& operator[](size_t idx) const
    {
        return (*this)[idx];
    }
    T& operator[](size_t idx)
    {
        return buf[(m_idx + idx) & MASK];
    }
    operator T&()
    {
        return buf[m_idx];
    }
    operator T() const
    {
        return *this;
    }
    T& operator++(int)
    {
        T tmp = *this;
        m_idx++;
        m_idx &= MASK;
        return tmp;
    }
    T operator++(int) const
    {
        return (*this)++;
    }
    T& operator++()
    {
        m_idx++;
        m_idx &= MASK;
        return *this;
    }
    T operator++() const
    {
        return ++(*this);
    }
    T& operator--(int)
    {
        T tmp = *this;
        m_idx--;
        m_idx &= MASK;
        return tmp;
    }
    T operator--(int) const
    {
        return --(*this);
    }
    T& operator--()
    {
        m_idx--;
        m_idx &= MASK;
        return *this;
    }
    T operator--() const
    {
        return (*this)--;
    }
    void erase()
    {
        std::memset(buf, 0, sizeof(buf));
    }
    T& popElem(size_t idx)
    {
        if(idx)
        {
            T tmp1 = (*this)[idx];
            for(size_t i = idx - 1; i != 0; --idx)
            {
                T tmp2 = (*this)[i];
                (*this)[i + 1] = tmp2;
            }
            (*this)[0] = tmp1;
        }
        return *this;
    }
private:
    T buf[N];
    static const size_t MASK = N - 1;
    size_t m_idx;
};

/* Gets compressed variable-width number. Number is in the form 2^Exp + Man */
/* Exp is the number of leading zeroes. The following bits make up the
 * mantissa. The same number of bits make up the exponent and mantissa */
uint16_t getCompNumber(BitBarrel& bb)
{
    int16_t exponent = 0;
    do
    {
        exponent--;
    } while(bb.getNextBit() == false);
    exponent = ~exponent;
    if(!exponent) return 0;
    
    uint16_t val = (2 << (exponent - 1)) - 1;
    val += bb.readBits(exponent);
    return val;
}

uint16_t getNextTile(BitBarrel& bb, CircularBuffer<uint16_t, 16>& cb)
{
    if(bb.getNextBit())
    {
        uint16_t shift = bb.readBits(4);
        cb.popElem(shift);
    }
    else
    {
        --cb = bb.readBits(10);
    }
    return cb;
}

void maskTiles(BitBarrel& bb, uint16_t total, uint16_t* dst, uint16_t mask)
{
    uint16_t count = 0;
    uint16_t tile = 0;
    do
    {
        uint16_t num = getCompNumber(bb);
        if(count || --num)
        {
            count += num + 1;
            for(int16_t i = 0; i <= num; i++)
            {
                *dst++ |= tile;
            }
        }
        tile ^= mask;
    } while(count < total);
}

uint16_t BigTilesCmp::Decode(const uint8_t* src, uint16_t* dst)
{
    BitBarrel bb(src);
    CircularBuffer<uint16_t, 16> cb;
    
    const uint16_t TOTAL = bb.readBits(16) << 2;
    
    memset(dst, 0, TOTAL * 2);
    maskTiles(bb, TOTAL, dst, 0x8000);
    maskTiles(bb, TOTAL, dst, 0x1000);
    maskTiles(bb, TOTAL, dst, 0x0800);
    
    uint16_t* dstptr = dst;
    uint16_t count = TOTAL >> 1;
    uint16_t tile = getNextTile(bb, cb);
    while(count--)
    {
        *dstptr++ |= tile;
        if(bb.getNextBit())
        {
            tile &= 0x7FF;
            if(0x0800 & *(dstptr - 1))
            {
                tile--;
            }
            else
            {
                tile++;
            }
            *dstptr++ |= tile;
        }
        else
        {
            tile = getNextTile(bb, cb);
            *dstptr++ |= tile;
        }
    }
    return TOTAL >> 2;
}
