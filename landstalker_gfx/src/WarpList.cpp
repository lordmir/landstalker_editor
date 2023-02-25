#include "WarpList.h"

#include <cassert>

WarpList::WarpList(const filesystem::path& warp_path, const filesystem::path& fall_dest_path, const filesystem::path& climb_dest_path, const filesystem::path& transition_path)
{
	auto warp_buf = ReadBytes(warp_path);
	auto fall_buf = ReadBytes(fall_dest_path);
	auto climb_buf = ReadBytes(climb_dest_path);
	auto transition_buf = ReadBytes(transition_path);
	ProcessWarpList(warp_buf);
	ProcessRouteList(fall_buf, m_fall_dests);
	ProcessRouteList(climb_buf, m_climb_dests);
	ProcessTransitionList(transition_buf);
}

uint32_t FindMarker(const Rom& rom, uint32_t start_addr)
{
	uint32_t end = start_addr;
	uint16_t marker;
	do
	{
		end += 2;
		marker = rom.read<uint16_t>(end);
	} while (marker != 0xFFFF);
	return end + 2;
}

WarpList::WarpList(const Rom& rom)
{
	uint32_t start = rom.read<uint32_t>(RomOffsets::Rooms::ROOM_EXITS_PTR);
	auto warp_bytes = rom.read_array<uint8_t>(start, FindMarker(rom, start) - start);
	auto fall_bytes = rom.read_array<uint8_t>(RomOffsets::Rooms::ROOM_FALL_DEST);
	auto climb_bytes = rom.read_array<uint8_t>(RomOffsets::Rooms::ROOM_CLIMB_DEST);
	auto transition_bytes = rom.read_array<uint8_t>(RomOffsets::Rooms::ROOM_TRANSITIONS);
	ProcessWarpList(warp_bytes);
	ProcessRouteList(fall_bytes, m_fall_dests);
	ProcessRouteList(climb_bytes, m_climb_dests);
	ProcessTransitionList(transition_bytes);
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

bool WarpList::HasFallDestination(uint16_t room) const
{
	return m_fall_dests.find(room) != m_fall_dests.end();
}

uint16_t WarpList::GetFallDestination(uint16_t room) const
{
	if (!HasFallDestination(room))
	{
		return 0xFFFF;
	}
	return m_fall_dests.find(room)->second;
}

bool WarpList::HasClimbDestination(uint16_t room) const
{
	return m_climb_dests.find(room) != m_climb_dests.end();
}

uint16_t WarpList::GetClimbDestination(uint16_t room) const
{
	if (!HasClimbDestination(room))
	{
		return 0xFFFF;
	}
	return m_climb_dests.find(room)->second;
}

std::map<std::pair<uint16_t, uint16_t>, uint16_t> WarpList::GetTransitions(uint16_t room) const
{
	std::map<std::pair<uint16_t, uint16_t>, uint16_t> retval;
	for (const auto& t : m_transitions)
	{
		if ((t.first.first == room) || (t.first.second == room))
		{
			retval.emplace(t);
		}
	}
	return retval;
}

void WarpList::ProcessWarpList(const std::vector<uint8_t>& bytes)
{
	assert(bytes.size() % 8 == 2);
	auto it = bytes.begin();
	uint16_t begin = (*(it) << 8) | *(it + 1);
	while (it != bytes.end() && begin != 0xFFFF)
	{
		auto bits = std::vector<uint8_t>(it, it + 8);
		m_warps.emplace_back(bits);
		it += 8;
		begin = (*(it) << 8) | *(it + 1);
	}
}

void WarpList::ProcessRouteList(const std::vector<uint8_t>& bytes, std::map<uint16_t, uint16_t>& route)
{
	assert(bytes.size() % 4 == 2);
	auto it = bytes.begin();
	uint16_t target = (*it << 8) | *(it + 1);
	while (target != 0xFFFF)
	{
		uint16_t destination = (*(it + 2) << 8) | *(it + 3);
		route.insert({ target, destination });
		it += 4;
		target = (*it << 8) | *(it + 1);
	}
}

void WarpList::ProcessTransitionList(const std::vector<uint8_t>& bytes)
{
	assert(bytes.size() % 6 == 4);
	auto it = bytes.begin();
	uint16_t target = (*it << 8) | *(it + 1);
	while (target != 0xFFFF)
	{
		uint16_t destination = (*(it + 2) << 8) | *(it + 3);
		uint16_t flag = (*(it + 4) << 3) | *(it + 5);
		m_transitions.insert({{target, destination}, flag});
		it += 6;
		target = (*it << 8) | *(it + 1);
	}
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
