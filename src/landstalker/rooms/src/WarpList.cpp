#include <landstalker/rooms/include/WarpList.h>

#include <cassert>
#include <queue>

#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/RomLabels.h>

WarpList::WarpList(const std::filesystem::path& warp_path, const std::filesystem::path& fall_dest_path, const std::filesystem::path& climb_dest_path, const std::filesystem::path& transition_path)
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
	uint32_t fall_start_addr = Disasm::ReadOffset16(rom, RomLabels::Rooms::FALL_TABLE_LEA_LOC);
	uint32_t climb_start_addr = Disasm::ReadOffset16(rom, RomLabels::Rooms::CLIMB_TABLE_LEA_LOC);
	uint32_t transition_start_addr = Disasm::ReadOffset16(rom, RomLabels::Rooms::TRANSITION_TABLE_LEA_LOC1);

	uint32_t start = rom.read<uint32_t>(RomLabels::Rooms::ROOM_EXITS_PTR);
	auto warp_bytes = rom.read_array<uint8_t>(start, FindMarker(rom, start) - start);
	auto fall_bytes = rom.read_array<uint8_t>(fall_start_addr, FindMarker(rom, fall_start_addr) - fall_start_addr);
	auto climb_bytes = rom.read_array<uint8_t>(climb_start_addr, FindMarker(rom, climb_start_addr) - climb_start_addr);
	auto transition_bytes = rom.read_array<uint8_t>(transition_start_addr, FindMarker(rom, transition_start_addr) - transition_start_addr + 2);

	ProcessWarpList(warp_bytes);
	ProcessRouteList(fall_bytes, m_fall_dests);
	ProcessRouteList(climb_bytes, m_climb_dests);
	ProcessTransitionList(transition_bytes);
}

bool WarpList::operator==(const WarpList& rhs) const
{
	return ((this->m_warps == rhs.m_warps) &&
            (this->m_fall_dests == rhs.m_fall_dests) &&
            (this->m_climb_dests == rhs.m_climb_dests) &&
            (this->m_transitions == rhs.m_transitions));
}

bool WarpList::operator!=(const WarpList& rhs) const
{
	return !(*this == rhs);
}

std::vector<WarpList::Warp> WarpList::GetWarpsForRoom(uint16_t room) const
{
	std::vector<Warp> warps;
	for (auto it = m_warps.begin(); it != m_warps.end(); ++it)
	{
		if (it->room1 == room || it->room2 == room)
		{
			warps.push_back(*it);
		}
	}
	return warps;
}

void WarpList::UpdateWarpsForRoom(uint16_t room, const std::vector<Warp>& warps)
{
	std::queue<std::vector<Warp>::iterator> iterators;
	for (auto it = m_warps.begin(); it != m_warps.end(); )
	{
		if (it->room1 == room || it->room2 == room)
		{
			if (iterators.size() < warps.size())
			{
				iterators.push(it++);
			}
			else
			{
				it = m_warps.erase(it);
			}
		}
		else
		{
			++it;
		}
	}
	for (const auto& warp : warps)
	{
		if (warp.IsValid())
		{
			if (iterators.empty())
			{
				m_warps.push_back(warp);
			}
			else
			{
				auto& it = iterators.front();
				*it = warp;
				iterators.pop();
			}
		}
	}
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

std::vector<WarpList::Transition> WarpList::GetAllTransitionsForRoom(uint16_t room) const
{
	std::vector<Transition> retval;
	for (const auto& t : m_transitions)
	{
		if ((t.src_rm == room) || (t.dst_rm == room))
		{
			retval.push_back(t);
		}
	}
	return retval;
}

std::vector<WarpList::Transition> WarpList::GetSrcTransitionsForRoom(uint16_t room) const
{
	std::vector<Transition> retval;
	for (const auto& t : m_transitions)
	{
		if (t.src_rm == room)
		{
			retval.push_back(t);
		}
	}
	return retval;
}

void WarpList::SetSrcTransitionsForRoom(uint16_t room, const std::vector<WarpList::Transition>& data)
{
	std::queue<std::vector<Transition>::iterator> iterators;
	std::size_t count = 0;
	// Delete excess
	for (auto it = m_transitions.begin(); it != m_transitions.end(); )
	{
		if (it->src_rm == room)
		{
			if (count < data.size())
			{
				count++;
				it++;
			}
			else
			{
				it = m_transitions.erase(it);
			}
		}
		else
		{
			++it;
		}
	}
	// Save list of iterators to existing entries
	for (auto it = m_transitions.begin(); it != m_transitions.end(); ++it)
	{
		if (it->src_rm == room)
		{
			iterators.push(it);
		}
	}
	// For each new entry, either change next iterator or, if none left, push on a new entry.
	for (const auto& f : data)
	{
		if (iterators.empty())
		{
			m_transitions.push_back(f);
		}
		else
		{
			auto& it = iterators.front();
			*it = f;
			iterators.pop();
		}
	}
}

void WarpList::SetHasFallDestination(uint16_t room, bool enabled)
{
	auto existing = m_fall_dests.find(room);
	if (existing != m_fall_dests.cend() && enabled == false)
	{
		m_fall_dests.erase(room);
	}
	else if (existing == m_fall_dests.cend() && enabled == true)
	{
		m_fall_dests[room] = 0;
	}
}

void WarpList::SetFallDestination(uint16_t room, uint16_t dest)
{
	m_fall_dests[room] = dest;
}

void WarpList::SetHasClimbDestination(uint16_t room, bool enabled)
{
	auto existing = m_climb_dests.find(room);
	if (existing != m_climb_dests.cend() && enabled == false)
	{
		m_climb_dests.erase(room);
	}
	else if (existing == m_climb_dests.cend() && enabled == true)
	{
		m_climb_dests[room] = 0;
	}
}

void WarpList::SetClimbDestination(uint16_t room, uint16_t dest)
{
	m_climb_dests[room] = dest;
}

std::vector<uint8_t> WarpList::GetWarpBytes() const
{
	std::vector<uint8_t> retval;
	retval.reserve(m_warps.size() * 8 + 2);

	for (const auto& warp : m_warps)
	{
		auto bytes = warp.GetRaw();
		retval.insert(retval.end(), bytes.begin(), bytes.end());
	}

	retval.push_back(0xFF);
	retval.push_back(0xFF);
	return retval;
}

std::vector<uint8_t> WarpList::GetFallBytes() const
{
	std::vector<uint8_t> retval;
	retval.reserve(m_fall_dests.size() * 4 + 2);

	for (const auto& dest : m_fall_dests)
	{
		retval.push_back(dest.first >> 8);
		retval.push_back(dest.first & 0xFF);
		retval.push_back(dest.second >> 8);
		retval.push_back(dest.second & 0xFF);
	}
	retval.push_back(0xFF);
	retval.push_back(0xFF);
	return retval;
}

std::vector<uint8_t> WarpList::GetClimbBytes() const
{
	std::vector<uint8_t> retval;
	retval.reserve(m_climb_dests.size() * 4 + 2);

	for (const auto& dest : m_climb_dests)
	{
		retval.push_back(dest.first >> 8);
		retval.push_back(dest.first & 0xFF);
		retval.push_back(dest.second >> 8);
		retval.push_back(dest.second & 0xFF);
	}
	retval.push_back(0xFF);
	retval.push_back(0xFF);
	return retval;
}

std::vector<uint8_t> WarpList::GetTransitionBytes() const
{
	std::vector<uint8_t> retval;
	std::vector<Transition> out(m_transitions);
	std::sort(out.begin(), out.end());
	retval.reserve(out.size() * 6 + 4);
	for (const auto& txn : out)
	{
		retval.push_back(txn.src_rm >> 8);
		retval.push_back(txn.src_rm & 0xFF);
		retval.push_back(txn.dst_rm >> 8);
		retval.push_back(txn.dst_rm & 0xFF);
		retval.push_back((txn.flag >> 3) & 0xFF);
		retval.push_back(txn.flag & 0x07);
	}
	retval.push_back(0xFF);
	retval.push_back(0xFF);
	retval.push_back(0xFF);
	retval.push_back(0xFF);
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
		m_transitions.push_back(Transition(target, destination, flag));
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

std::vector<uint8_t> WarpList::Warp::GetRaw() const
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

bool WarpList::Warp::IsValid() const
{
	return room1 != 0xFFFF && room2 != 0xFFFF;
}

bool WarpList::Warp::operator==(const Warp& rhs) const
{
	return ((this->type == rhs.type) && (this->x_size == rhs.x_size) &&
		    (this->y_size == rhs.y_size) &&
		    (((this->room1 == rhs.room1) && (this->room2 == rhs.room2) &&
			  (this->x1 == rhs.x1) && (this->y1 == rhs.y1) &&
			  (this->x2 == rhs.x2) && (this->y2 == rhs.y2)) ||
		     ((this->room1 == rhs.room2) && (this->room2 == rhs.room1) &&
		  	  (this->x1 == rhs.x2) && (this->y1 == rhs.y2) &&
			  (this->x2 == rhs.x1) && (this->y2 == rhs.y1))));
}

bool WarpList::Warp::operator!=(const Warp& rhs) const
{
	return !(*this == rhs);
}

bool WarpList::Transition::operator==(const Transition& rhs) const
{
	return (this->src_rm == rhs.src_rm && this->dst_rm == rhs.dst_rm
		&& this->flag == rhs.flag);
}

bool WarpList::Transition::operator!=(const Transition& rhs) const
{
	return !(*this == rhs);
}

bool WarpList::Transition::operator<(const Transition& rhs) const
{
	if (this->dst_rm < rhs.dst_rm)
	{
		return true;
	}
	if (this->dst_rm == rhs.dst_rm && this->src_rm > rhs.src_rm)
	{
		return true;
	}
	return false;
}

bool WarpList::Transition::operator<=(const Transition& rhs) const
{
	return (*this < rhs) || (*this == rhs);
}

bool WarpList::Transition::operator>(const Transition& rhs) const
{
	return !(*this <= rhs);
}

bool WarpList::Transition::operator>=(const Transition& rhs) const
{
	return !(*this < rhs);
}
