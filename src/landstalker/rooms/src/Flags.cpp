#include <landstalker/rooms/include/Flags.h>
#include <cassert>

EntityFlag::EntityFlag(const std::array<uint8_t, 4>& data)
{
	room = (data[0] << 8) | data[1];
	entity = data[3] & 0x1F;
	flag = (data[2] & 0x7F) << 3 | (data[3] >> 5);
	set = (data[2] & 0x80) > 0;
}

std::array<uint8_t, EntityFlag::SIZE> EntityFlag::GetData() const
{
	std::array<uint8_t, SIZE> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = ((flag >> 3) & 0x7F) | (set ? 0x80 : 0x00);
	data[3] = (entity & 0x1F) | ((flag & 0x07) << 5);
	return data;
}

bool EntityFlag::operator==(const EntityFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->entity == rhs.entity &&
		this->flag == rhs.flag &&
		this->set == rhs.set);
}

bool EntityFlag::operator!=(const EntityFlag& rhs) const
{
	return !(*this == rhs);
}

OneTimeEventFlag::OneTimeEventFlag(const std::array<uint8_t, OneTimeEventFlag::SIZE>& data)
{
	room = (data[0] << 8) | data[1];
	entity = data[3] & 0x1F;
	flag_on = (data[2] & 0x7F) << 3 | (data[3] >> 5);
	flag_on_set = (data[2] & 0x80) > 0;
	flag_off = (data[4] & 0x7F) << 3 | (data[5] & 0x7);
	flag_off_set = (data[4] & 0x80) > 0;
}

std::array<uint8_t, OneTimeEventFlag::SIZE> OneTimeEventFlag::GetData() const
{
	std::array<uint8_t, SIZE> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = ((flag_on >> 3) & 0x7F) | (flag_on_set ? 0x80 : 0x00);
	data[3] = (entity & 0x1F) | ((flag_on & 0x07) << 5);
	data[4] = ((flag_off >> 3) & 0x7F) | (flag_off_set ? 0x80 : 0x00);
	data[5] = flag_off & 0x07;
	return data;
}

bool OneTimeEventFlag::operator==(const OneTimeEventFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->entity == rhs.entity &&
		this->flag_on == rhs.flag_on &&
		this->flag_on_set == rhs.flag_on_set &&
		this->flag_off == rhs.flag_off &&
		this->flag_off_set == rhs.flag_off_set);
}

bool OneTimeEventFlag::operator!=(const OneTimeEventFlag& rhs) const
{
	return !(*this == rhs);
}

SacredTreeFlag::SacredTreeFlag(const std::array<uint8_t, SacredTreeFlag::SIZE>& data)
{
	room = (data[0] << 8) | data[1];
	flag = data[2] << 3 | (data[3] & 0x07);
}

std::array<uint8_t, SacredTreeFlag::SIZE> SacredTreeFlag::GetData() const
{
	std::array<uint8_t, SIZE> data{};
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = (flag >> 3) & 0xFF;
	data[3] = flag & 0x07;
	return data;
}

bool SacredTreeFlag::operator==(const SacredTreeFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->flag == rhs.flag);
}

bool SacredTreeFlag::operator!=(const SacredTreeFlag& rhs) const
{
	return !(*this == rhs);
}

TileSwapFlag::TileSwapFlag(const std::array<uint8_t, TileSwapFlag::SIZE>& data)
{
	room = (data[0] << 8) | data[1];
	always = data[2] == 0xFF;
	flag = always ? 0 : (data[2] << 3) | (data[3] & 7);
	index = ((data[3]) >> 3);
}

std::array<uint8_t, TileSwapFlag::SIZE> TileSwapFlag::GetData() const
{
	std::array<uint8_t, SIZE> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = always ? 0xFF : static_cast<uint8_t>(flag >> 3);
	data[3] = (index << 3) & 0xF8;
	data[3] |= always ? 0 : (flag & 7);
	return data;
}

bool TileSwapFlag::operator==(const TileSwapFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->index == rhs.index &&
		this->flag == rhs.flag &&
		this->always == rhs.always);
}

bool TileSwapFlag::operator!=(const TileSwapFlag& rhs) const
{
	return !(*this == rhs);
}

TreeWarpFlag::TreeWarpFlag(const std::array<uint8_t, SIZE>& data)
{
	assert(data[2] == data[6]);
	assert(data[3] == data[7]);
	assert(data[3] % 2 == 0);

	room1 = (data[0] << 8) | data[1];
	room2 = (data[4] << 8) | data[5];
	flag = (data[2] << 3) | (data[3] & 0x06);
}

std::array<uint8_t, TreeWarpFlag::SIZE> TreeWarpFlag::GetData() const
{
	std::array<uint8_t, SIZE> data{};
	data[0] = room1 >> 8;
	data[1] = room1 & 0xFF;
	data[4] = room2 >> 8;
	data[5] = room2 & 0xFF;
	data[2] = data[6] = static_cast<uint8_t>(flag >> 3);
	data[3] = data[7] = flag & 0x06;
	return data;
}

bool TreeWarpFlag::operator==(const TreeWarpFlag& rhs) const
{
	return ((this->room1 == rhs.room1 && this->room2 == rhs.room2) ||
		(this->room2 == rhs.room1 && this->room1 == rhs.room2)) &&
		this->flag == rhs.flag;
}

bool TreeWarpFlag::operator!=(const TreeWarpFlag& rhs) const
{
	return !(*this == rhs);
}

RoomClearFlag::RoomClearFlag(const std::array<uint8_t, 4>& data)
{
	room = (data[0] << 8) | data[1];
	entity = data[3] & 0x1F;
	flag = (data[2] << 3) | (data[3] >> 5);
}

std::array<uint8_t, RoomClearFlag::SIZE> RoomClearFlag::GetData() const
{
	std::array<uint8_t, SIZE> data;
	data[0] = room >> 8;
	data[1] = room & 0xFF;
	data[2] = static_cast<uint8_t>(flag >> 3);
	data[3] = static_cast<uint8_t>((entity & 0x1F) | ((flag & 0x07) << 5));
	return data;
}

bool RoomClearFlag::operator==(const RoomClearFlag& rhs) const
{
	return (
		this->room == rhs.room &&
		this->entity == rhs.entity &&
		this->flag == rhs.flag);
}

bool RoomClearFlag::operator!=(const RoomClearFlag& rhs) const
{
	return !(*this == rhs);
}