#ifndef BIGTILESCMP_H
#define BIGTILESCMP_H

#include <cstdint>

class BigTilesCmp
{
public:
    static uint16_t Decode(const uint8_t* src, uint16_t* dst);
private:
    BigTilesCmp();
};

#endif // BIGTILESCMP_H
