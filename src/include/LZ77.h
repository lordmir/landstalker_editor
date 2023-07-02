#ifndef LZ77_H
#define LZ77_H

#include <cstdlib>
#include <cstdint>
#include <vector>

class LZ77
{
public:
    static std::size_t Decode(const uint8_t* inbuf, std::size_t bufsize, uint8_t* outbuf, std::size_t& elen);
    static std::size_t Encode(const uint8_t* inbuf, std::size_t bufsize, uint8_t* outbuf);
private:
    LZ77();
};

#endif // LZ77_H
