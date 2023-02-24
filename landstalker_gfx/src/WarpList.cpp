#include "WarpList.h"

#include <cassert>

WarpList::WarpList(const filesystem::path& path)
{
	auto buf = ReadBytes(path);
	assert(buf.size() % 8 == 2);
	auto it = buf.begin();
	uint16_t begin = (*(it) << 8) | *(it + 1);
	while (it != buf.end() && begin != 0xFFFF)
	{
		auto bits = std::vector<uint8_t>(it, it + 8);
		m_warps.emplace_back(bits);
		it += 8;
		begin = (*(it) << 8) | *(it + 1);
	}
}

WarpList::WarpList(const Rom& rom)
{
	uint32_t addr = rom.read<uint32_t>(RomOffsets::Rooms::ROOM_EXITS_PTR);
	uint16_t begin = rom.read<uint16_t>(addr);
	while (begin != 0xFFFF)
	{
		auto bits = rom.read_array<uint8_t>(addr, 8);
		m_warps.emplace_back(bits);
		addr += 8;
		begin = rom.read<uint16_t>(addr);
	}
}

std::list<WarpList::Warp> WarpList::GetWarpsForRoom(uint16_t room) const
{
	std::list<Warp> warps;
	for (const auto& warp : m_warps)
	{
		if (warp.room1 == room || warp.room2 == room)
		{
			warps.push_back(warp);
		}
	}
	return warps;
}

WarpList::Warp::Warp(const std::vector<uint8_t>& raw)
{
	assert(raw.size() == 8);
	uint16_t r1 = (raw[0] << 8) | raw[1];
	uint16_t r2 = (raw[4] << 8) | raw[5];
	room1 = r1 & 0x03FF;
	room2 = r2 & 0x03FF;
	x1 = raw[2];
	y1 = raw[3];
	x2 = raw[6];
	y2 = raw[7];
	x_size = (raw[0] & 0x08) ? 2 : 1;
	y_size = (raw[0] & 0x10) ? 2 : 1;
	if (raw[0] & 0x04)
	{
		if (x_size == 2)
		{
			x_size = 3;
		}
		if (y_size == 2)
		{
			y_size = 3;
		}
	}
	uint8_t type_bits = (raw[0] >> 5) & 0x03;
	switch (type_bits)
	{
	case 0:
		type = Type::NORMAL;
		break;
	case 1:
		type = Type::STAIR_SE;
		break;
	case 2:
		type = Type::STAIR_SW;
		break;
	default:
		type = Type::UNKNOWN;
		break;
	}
}

std::vector<uint8_t> WarpList::Warp::GetRaw()
{
	std::vector<uint8_t> retval(8);

	retval[0] = (room1 >> 8) & 0x03;
	retval[1] = room1 & 0xFF;
	retval[2] = x1;
	retval[3] = y1;
	retval[4] = (room2 >> 8) & 0x03;
	retval[5] = room2 & 0xFF;
	retval[6] = x2;
	retval[7] = y2;
	retval[0] |= (x_size == 3 || y_size == 3) ? 0x04 : 0;
	retval[0] |= (x_size > 1) ? 0x08 : 0;
	retval[0] |= (y_size > 1) ? 0x10 : 0;
	retval[0] |= (type == Type::STAIR_SE) ? 0x20 : 0;
	retval[0] |= (type == Type::STAIR_SW) ? 0x40 : 0;

	return retval;
}
