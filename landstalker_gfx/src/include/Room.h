#ifndef _ROOM_H_
#define _ROOM_H_

#include <string>
#include <cstdint>
#include <vector>
#include <array>

class Room
{
    // Stored in ROM in following format:
    // MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM
    // UU1TTTTT UUPPPPPP EEEESSSS 222BBBBB
    // M - Map offset
    // U - Unknown / reserved
    // 1 - Primary Blockset ID
    // 2 - Secondary Blockset ID
    // T - Tileset ID
    // P - Palette ID
    // E - Max Z height
    // S - Min Z height
    // B - BGM
public:
    std::string map;
    uint8_t tileset;
    uint8_t pri_blockset;
    uint8_t sec_blockset;
    uint8_t room_palette;
    uint8_t bgm;
    uint8_t room_z_begin;
    uint8_t room_z_end;
    uint8_t unknown_param1;
    uint8_t unknown_param2;

    Room(const std::string& map_name, const std::vector<uint8_t>& params);
    Room(const std::string& map_name, uint8_t params[4]);

    void SetParams(uint8_t param0, uint8_t param1, uint8_t param2, uint8_t param3);
    std::array<uint8_t, 4> GetParams() const;
    uint8_t GetBlocksetId() const;
};

#endif // _ROOM_H_
