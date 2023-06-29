#include <Chests.h>

Chests::Chests(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& contents)
{
	int max_offset = offsets[0];
	int last_room = 0;
	std::map<uint16_t, uint8_t> chests;
	for (int i = 0; i < offsets.size() - 1; ++i)
	{
		if (offsets[i] == 0 && max_offset > 0)
		{
			m_enabled.insert(i);
		}
		else if (offsets[i] > max_offset)
		{
			chests.insert({ last_room, offsets[i] - max_offset });
			max_offset = offsets[i];
			last_room = i;
		}
		else
		{
			last_room = i;
		}
	}
	if (max_offset < contents.size())
	{
		chests.insert({ offsets.size() - 1, contents.size() - max_offset });
	}
	int cur_offset = 0;
	for (const auto& c : chests)
	{
		std::vector<uint8_t> rcontent(contents.begin() + cur_offset, contents.begin() + cur_offset + c.second);
		m_chests.insert({ c.first, rcontent });
		cur_offset += c.second;
	}
}

Chests::Chests()
{
}

bool Chests::operator==(const Chests& rhs) const
{
	return (this->m_chests == rhs.m_chests && this->m_enabled == rhs.m_enabled);
}

bool Chests::operator!=(const Chests& rhs) const
{
	return !(*this == rhs);
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Chests::GetData(int roomcount) const
{
	std::vector<uint8_t> offsets;
	offsets.reserve(roomcount);
	std::vector<uint8_t> contents;
	contents.reserve(256);
	int offset = 0;
	for (int i = 0; i < roomcount; ++i)
	{
		if (m_enabled.count(i) > 0)
		{
			offsets.push_back(0);
		}
		else
		{
			offsets.push_back(offset);
			if (m_chests.count(i) > 0)
			{
				contents.insert(contents.end(), m_chests.at(i).begin(), m_chests.at(i).end());
				offset += m_chests.at(i).size();
			}
		}
	}
	return { offsets, contents };
}

std::vector<uint8_t> Chests::GetChestsForRoom(uint16_t room) const
{
	auto it = m_chests.find(room);
	if (it == m_chests.end())
	{
		return {};
	}
	else
	{
		return m_chests.at(room);
	}
}

bool Chests::RoomHasChests(uint16_t room) const
{
	auto it = m_chests.find(room);
	return it != m_chests.end() && !it->second.empty() && m_enabled.count(room) == 0;
}

bool Chests::RoomHasNoChestsSet(uint16_t room) const
{
	return m_enabled.count(room) != 0;
}

void Chests::SetRoomChests(uint16_t room, const std::vector<uint8_t>& chests)
{
	if (chests.empty())
	{
		m_chests.erase(room);
	}
	else
	{
		m_chests[room] = chests;
		m_enabled.erase(room);
	}
}

void Chests::ClearRoomChests(uint16_t room)
{
	m_chests.erase(room);
}

void Chests::SetRoomNoChests(uint16_t room)
{
	m_chests.erase(room);
	m_enabled.insert(room);
}

void Chests::ClearRoomNoChests(uint16_t room)
{
	m_enabled.erase(room);
}
