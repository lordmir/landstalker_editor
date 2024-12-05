#include <landstalker/rooms/include/Room.h>

#include <cassert>

Room::Room(const std::string& name_, const std::string& map_name, uint16_t index_, const std::vector<uint8_t>& params)
    : map(map_name),
      name(name_),
      index(index_)
{
    assert(params.size() == 4);
    SetParams(params[0], params[1], params[2], params[3]);
}

Room::Room(const std::string& name_, const std::string& map_name, uint16_t index_, uint8_t params[4])
    : map(map_name),
      name(name_),
      index(index_)
{
    SetParams(params[0], params[1], params[2], params[3]);
}

bool Room::operator==(const Room& rhs) const
{
    return ((this->bgm == rhs.bgm) &&
            (this->map == rhs.map) &&
            (this->pri_blockset == rhs.pri_blockset) &&
            (this->room_palette == rhs.room_palette) &&
            (this->room_z_begin == rhs.room_z_begin) &&
            (this->room_z_end == rhs.room_z_end) &&
            (this->sec_blockset == rhs.sec_blockset) &&
            (this->tileset == rhs.tileset) &&
            (this->unknown_param1 == rhs.unknown_param1) &&
            (this->unknown_param2 == rhs.unknown_param2));
}

bool Room::operator!=(const Room& rhs) const
{
    return !(*this == rhs);
}

void Room::SetParams(uint8_t param0, uint8_t param1, uint8_t param2, uint8_t param3)
{
    unknown_param1 = param0 >> 6;
    pri_blockset = (param0 >> 5) & 0x01;
    tileset = param0 & 0x1F;
    unknown_param2 = param1 >> 6;
    room_palette = param1 & 0x3F;
    room_z_end = param2 >> 4;
    room_z_begin = param2 & 0x0F;
    sec_blockset = param3 >> 5;
    bgm = param3 & 0x1F;
}

std::array<uint8_t, 4> Room::GetParams() const
{
    std::array<uint8_t, 4> ret{};
    ret[0] = ((unknown_param1 & 0x03) << 6) | ((pri_blockset & 0x01) << 5) | (tileset & 0x1F);
    ret[1] = ((unknown_param2 & 0x03) << 6) | (room_palette & 0x3F);
    ret[2] = ((room_z_end & 0x0F) << 4) | (room_z_begin & 0x0F);
    ret[3] = ((sec_blockset & 0x07) << 5) | (bgm & 0x1F);
    return ret;
}

uint8_t Room::GetBlocksetId() const
{
    return pri_blockset << 5 | tileset;
}

std::wstring Room::GetDisplayName()
{
    return Labels::Get(Labels::C_ROOMS, index).value_or(std::wstring(name.cbegin(), name.cend()));
}