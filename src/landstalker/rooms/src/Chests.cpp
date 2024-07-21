#include <landstalker/rooms/include/Chests.h>
#include <landstalker/main/include/GameData.h>

Chests::Chests(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& contents)
{
	uint8_t max_offset = offsets[0];
	uint16_t last_room = 0;
	std::map<uint16_t, uint8_t> chests;
	for (uint16_t i = 0; i < static_cast<uint16_t>(offsets.size() - 1); ++i)
	{
		if (offsets[i] == 0 && max_offset > 0)
		{
			m_enabled.insert(static_cast<uint16_t>(i));
		}
		else if (offsets[i] > max_offset)
		{
			chests.insert({ last_room, static_cast<uint8_t>(offsets[i] - max_offset) });
			max_offset = offsets[i];
			last_room = i;
		}
		else
		{
			last_room = i;
		}
	}
	if (max_offset < static_cast<int>(contents.size()))
	{
		chests.insert({ static_cast<uint16_t>(offsets.size() - 1),
			            static_cast<uint8_t>(contents.size() - max_offset) });
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
	for (uint16_t i = 0; i < static_cast<uint16_t>(roomcount); ++i)
	{
		if (m_enabled.count(i) > 0)
		{
			offsets.push_back(0);
		}
		else
		{
			offsets.push_back(static_cast<uint8_t>(offset));
			if (m_chests.count(i) > 0)
			{
				contents.insert(contents.end(), m_chests.at(i).begin(), m_chests.at(i).end());
				offset += m_chests.at(i).size();
			}
		}
	}
	return { offsets, contents };
}

std::vector<ChestItem> Chests::GetChestsForRoom(uint16_t room) const
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

void Chests::SetRoomChests(uint16_t room, const std::vector<ChestItem>& chests)
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

void Chests::SetRoomNoChestsFlag(uint16_t room)
{
	m_chests.erase(room);
	m_enabled.insert(room);
}

void Chests::ClearRoomNoChestsFlag(uint16_t room)
{
	m_enabled.erase(room);
}

bool Chests::CleanupRoomChests(const GameData& gd)
{
	int chest_count = 0;
	for (uint16_t r = 0; r < static_cast<uint16_t>(gd.GetRoomData()->GetRoomCount()); ++r)
	{
		auto ents = gd.GetSpriteData()->GetRoomEntities(r);
		std::size_t chests = 0;
		for (const auto& e : ents)
		{
			if (e.GetType() == 0x12)
			{
				chests++;
			}
		}
		if (m_enabled.count(r) != 0 && m_chests.count(r) != 0)
		{
			m_chests.erase(r);
		}
		else
		{
			if (m_chests.count(r) > 0 && m_chests[r].size() != chests)
			{
				m_chests[r].resize(chests);
			}
			else if(m_chests.count(r) == 0 && chests > 0)
			{
				m_chests[r] = std::vector<uint8_t>(chests);
			}
			chest_count += chests;
		}
	}
	return chest_count < 0x100;
}
