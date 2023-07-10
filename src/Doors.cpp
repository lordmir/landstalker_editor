#include <Doors.h>

Doors::Doors(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& bytes)
{
	int offset = 0;
	for (uint16_t i = 0; i < offsets.size(); ++i)
	{
		if (offsets[i] > 0)
		{
			while (bytes[offset] != 0xFF)
			{
				Door door{ bytes[offset] & 0x3F, bytes[offset + 1] & 0x3F,
					static_cast<Door::Size>(((bytes[offset] & 0x40) >> 4) | ((bytes[offset + 1] & 0xC0) >> 6))};
				if (m_doors.count(i) > 0)
				{
					m_doors[i].push_back(door);
				}
				else
				{
					m_doors.insert({ i, { door } });
				}
				offset += 2;
			}
			++offset;
		}
	}
}

Doors::Doors()
{
}

bool Doors::operator==(const Doors& rhs) const
{
	return this->m_doors == rhs.m_doors;
}

bool Doors::operator!=(const Doors& rhs) const
{
	return !(*this == rhs);
}

std::pair< std::vector<uint8_t>, std::vector<uint8_t>> Doors::GetData(int roomcount) const
{
	std::vector<uint8_t> offsets;
	offsets.reserve(roomcount);
	std::vector<uint8_t> doors;
	int lastsz = 0;
	for (int i = 0; i < roomcount; ++i)
	{
		if (m_doors.count(i) > 0)
		{
			offsets.push_back(lastsz + 1);
			lastsz = m_doors.at(i).size() * 2 + 1;
			for (const auto& d : m_doors.at(i))
			{
				uint8_t sz = static_cast<uint8_t>(d.size);
				doors.push_back((d.x & 0x3F) | ((sz & 0x04) << 4));
				doors.push_back((d.y & 0x3F) | ((sz & 0x03) << 6));
			}
			doors.push_back(0xFF);
		}
		else
		{
			offsets.push_back(0);
		}
	}
	return {offsets,doors};
}

std::vector<Door> Doors::GetDoorsForRoom(uint16_t room) const
{
	if (m_doors.count(room) > 0)
	{
		return m_doors.at(room);
	}
	else
	{
		return {};
	}
}

bool Doors::RoomHasDoors(uint16_t room) const
{
	return (m_doors.count(room) > 0);
}

void Doors::SetRoomDoors(uint16_t room, const std::vector<Door>& swaps)
{
	if (m_doors.count(room) > 0)
	{
		m_doors[room] = swaps;
	}
	else
	{
		m_doors.insert({room, swaps});
	}
}

bool Door::operator==(const Door& rhs) const
{
	return (this->x == rhs.x && this->y == rhs.y && this->size == rhs.size);
}

bool Door::operator!=(const Door& rhs) const
{
	return !(*this == rhs);
}
