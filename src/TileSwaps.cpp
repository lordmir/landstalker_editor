#include <TileSwaps.h>
#include <cassert>
#include <algorithm>
#include <iterator>

TileSwaps::TileSwaps(const std::vector<uint8_t>& bytes)
{
	assert(bytes.size() % 16 == 0);
	for (std::size_t i = 0; i < bytes.size(); i += 16)
	{
		uint16_t room = bytes[i + 12] << 8 | bytes[i + 13];
		if (room == 0xFFFF)
		{
			break;
		}
		uint8_t idx = bytes[i + 14] >> 3;
		TileSwap swap(std::vector<uint8_t>(bytes.cbegin() + i, bytes.cbegin() + i + 16));
		if (m_swaps.count(room) == 0)
		{
			m_swaps.insert({ idx, {} });
		}
		if (idx >= m_swaps[room].size())
		{
			m_swaps[room].resize(idx + 1);
		}
		m_swaps[room][idx] = swap;
	}
}

TileSwaps::TileSwaps()
{
}

bool TileSwaps::operator==(const TileSwaps& rhs) const
{
	return this->m_swaps == rhs.m_swaps;
}

bool TileSwaps::operator!=(const TileSwaps& rhs) const
{
	return !(*this == rhs);
}

std::vector<uint8_t> TileSwaps::GetData() const
{
	std::vector<uint8_t> out;
	for (const auto& r : m_swaps)
	{
		for (uint8_t i = 0; i < static_cast<uint8_t>(r.second.size()); ++i)
		{
			if (r.second[i].active == true)
			{
				auto bytes = r.second[i].GetBytes(r.first, i);
				out.insert(out.end(), bytes.cbegin(), bytes.cend());
			}
		}
	}
	std::fill_n(std::back_inserter(out), 16, 0xFF);
	return out;
}

std::vector<TileSwap> TileSwaps::GetSwapsForRoom(uint16_t room) const
{
	if (m_swaps.count(room) > 0)
	{
		return m_swaps.at(room);
	}
	else
	{
		return {};
	}
}

bool TileSwaps::RoomHasSwaps(uint16_t room) const
{
	return (m_swaps.count(room) > 0);
}

void TileSwaps::SetRoomSwaps(uint16_t room, const std::vector<TileSwap>& swaps)
{
	if (m_swaps.count(room) != 0 && swaps.empty())
	{
		m_swaps.erase(room);
	}
	else
	{
		m_swaps[room] = swaps;
	}
}

TileSwap::TileSwap(const std::vector<uint8_t>& in)
	: active(true)
{
	assert(in.size() == 16);
	map.src_x = in[0];
	map.src_y = in[1];
	map.dst_x = in[2];
	map.dst_y = in[3];
	map.width = in[4] + 1;
	map.height = in[5] + 1;
	heightmap.src_x = std::clamp<uint8_t>(in[6] - 12, 0, 0x3F);
	heightmap.src_y = std::clamp<uint8_t>(in[7] - 12, 0, 0x3F);
	heightmap.dst_x = std::clamp<uint8_t>(in[8] - 12, 0, 0x3F);
	heightmap.dst_y = std::clamp<uint8_t>(in[9] - 12, 0, 0x3F);
	heightmap.width = in[10] + 1;
	heightmap.height = in[11] + 1;
	mode = static_cast<Mode>(in[15]);
}

std::vector<uint8_t> TileSwap::GetBytes(uint16_t room, uint8_t idx) const
{
	std::vector<uint8_t> data(16);
	data[0] = map.src_x;
	data[1] = map.src_y;
	data[2] = map.dst_x;
	data[3] = map.dst_y;
	data[4] = map.width - 1;
	data[5] = map.height - 1;
	data[6] = heightmap.src_x + 12;
	data[7] = heightmap.src_y + 12;
	data[8] = heightmap.dst_x + 12;
	data[9] = heightmap.dst_y + 12;
	data[10] = heightmap.width - 1;
	data[11] = heightmap.height - 1;
	data[12] = room >> 8;
	data[13] = room & 0xFF;
	data[14] = idx << 3;
	data[15] = static_cast<uint8_t>(mode);
	return data;
}

bool TileSwap::operator==(const TileSwap& rhs) const
{
	return (this->map.src_x == rhs.map.src_x &&
		    this->map.src_y == rhs.map.src_y &&
		    this->map.dst_x == rhs.map.dst_x &&
		    this->map.dst_y == rhs.map.dst_y &&
		    this->map.width == rhs.map.width &&
		    this->map.height == rhs.map.height &&
		    this->heightmap.src_x == rhs.heightmap.src_x &&
		    this->heightmap.src_y == rhs.heightmap.src_y &&
		    this->heightmap.dst_x == rhs.heightmap.dst_x &&
		    this->heightmap.dst_y == rhs.heightmap.dst_y &&
		    this->heightmap.width == rhs.heightmap.width &&
		    this->heightmap.height == rhs.heightmap.height &&
		    this->mode == rhs.mode);
}

bool TileSwap::operator!=(const TileSwap& rhs) const
{
	return false;
}
