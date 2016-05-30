#ifndef LZ77_H
#define LZ77_H

#include <cstdlib>
#include <cstdint>

class LZ77
{
public:
    LZ77();
    ~LZ77();
    static size_t Decode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf);
    static size_t Encode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf);
};

#endif // LZ77_H
