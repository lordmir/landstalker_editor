#ifndef LZ77_H
#define LZ77_H

#include <cstdlib>
#include <cstdint>

class LZ77
{
public:
    static size_t Decode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf, size_t& elen);
    static size_t Encode(const uint8_t* inbuf, size_t bufsize, uint8_t* outbuf);
private:    
    LZ77();
};

#endif // LZ77_H
