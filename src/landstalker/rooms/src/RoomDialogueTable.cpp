#include <landstalker/rooms/include/RoomDialogueTable.h>

RoomDialogueTable::RoomDialogueTable(const std::vector<uint16_t>& data)
{
	std::size_t i = 0;
	while(i < data.size())
	{
		if (data[i] == 0xFFFF)
		{
			break;
		}
		uint16_t room = data[i] & 0x07FF;
		int count = data[i++] >> 11;
		std::vector<Character> chars;
		for (int j = 0; j < count; ++j, ++i)
		{
			uint16_t chr = data[i] & 0x7FF;
			const uint16_t run_length = data[i] >> 11;
			for (uint16_t k = 0; k < run_length; ++k)
			{
				chars.push_back(chr + k);
			}
		}
		m_dialogue_table.insert({ room, chars });
	}
}

RoomDialogueTable::RoomDialogueTable()
{
}

bool RoomDialogueTable::operator==(const RoomDialogueTable& rhs) const
{
	return this->m_dialogue_table == rhs.m_dialogue_table;
}

bool RoomDialogueTable::operator!=(const RoomDialogueTable& rhs) const
{
	return !(*this == rhs);
}

std::vector<uint16_t> RoomDialogueTable::GetData() const
{
	std::vector<uint16_t> data;
	for (const auto& d : m_dialogue_table)
	{
		uint16_t room = d.first;
		data.push_back(room);
		int room_idx = data.size() - 1;
		int chrcount = 0;
		for (const auto& c : d.second)
		{
			if (chrcount > 0 && ((data.back() & 0x7FF) == c - (data.back() >> 11)) && data.back() < 0xF800)
			{
				data.back() += 0x0800;
			}
			else
			{
				data.push_back(c | 0x0800);
				chrcount++;
			}
		}
		data[room_idx] |= chrcount << 11;
	}
	data.push_back(0xFFFF);
	return data;
}

std::vector<Character> RoomDialogueTable::GetRoomCharacters(uint16_t room) const
{
	if (m_dialogue_table.count(room) == 0)
	{
		return {};
	}
	else
	{
		return m_dialogue_table.at(room);
	}
}

void RoomDialogueTable::SetRoomCharacters(uint16_t room, const std::vector<Character>& chars)
{
	if (chars.size() > 0)
	{
		m_dialogue_table[room] = chars;
	}
	else
	{
		m_dialogue_table.erase(room);
	}
}
