#ifndef BITBARREL_H
#define BITBARREL_H

#include <cstdint>
#include <cstdlib>

class BitBarrel
{
public:
    BitBarrel(const uint8_t* buf);
    BitBarrel();
    bool empty();
    void newByte(uint8_t byte);
    operator bool();
    bool full();
    bool operator()(bool rhs);
    
    template <class T>
    T read() const;
    uint32_t readBits(size_t numBits) const;
    bool getNextBit() const;
    size_t getBytePosition() const;
    void advanceNextByte();
    
    uint8_t out();
protected:
    const uint8_t* const m_start;
    mutable uint8_t m_val;
    mutable uint8_t m_pos;
    mutable uint8_t* m_buf;
};
#endif // BITBARREL_H
