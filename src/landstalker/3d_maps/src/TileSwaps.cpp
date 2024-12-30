#include <landstalker/3d_maps/include/TileSwaps.h>
#include <cassert>
#include <memory>
#include <algorithm>
#include <iterator>
#include <landstalker/misc/include/Literals.h>

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
		TileSwap swap(std::vector<uint8_t>(bytes.cbegin() + i, bytes.cbegin() + i + 16));
		if (m_swaps.count(room) == 0)
		{
			m_swaps.insert({ room, {} });
		}
		m_swaps[room].push_back(swap);
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
			if (r.second[i].active)
			{
				auto bytes = r.second[i].GetBytes(r.first, i);
				out.insert(out.end(), bytes.cbegin(), bytes.cend());
			}
		}
	}
	std::fill_n(std::back_inserter(out), 16, 0xFF_u8);
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
	return (m_swaps.count(room) > 0) && std::any_of(m_swaps.at(room).cbegin(), m_swaps.at(room).cend(), [](const TileSwap& swap) { return swap.active; });
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
	trigger = in[14] >> 3;
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

std::vector<std::pair<int, int>> TileSwap::GetMapRegionPoly(TileSwap::Region region, int tile_width, int tile_height) const
{
	std::vector<std::pair<int, int>> points;
	int xstep = 0;
	int ystep = 0;
	if (region == Region::SOURCE)
	{
		xstep = heightmap.src_x;
		ystep = heightmap.src_y;
	}
	else if (region == Region::DESTINATION)
	{
		xstep = heightmap.dst_x;
		ystep = heightmap.dst_y;
	}

	if (mode == TileSwap::Mode::WALL_NW)
	{
		xstep += tile_width;
		for (int i = 0; i < map.width; ++i)
		{
			points.push_back({ xstep, ystep });
			xstep -= tile_width;
			points.push_back({ xstep, ystep });
			ystep += tile_height / 2;
		}
		ystep -= tile_height / 2;
	}
	else
	{
		for (int i = 0; i < map.width; ++i)
		{
			points.push_back({ xstep, ystep });
			xstep += tile_width;
			points.push_back({ xstep, ystep });
			ystep += tile_height / 2;
		}
	}
	if (mode == TileSwap::Mode::FLOOR)
	{
		for (int i = 0; i < map.height; ++i)
		{
			ystep += tile_height / 2;
			points.push_back({ xstep, ystep });
			xstep -= tile_width;
			points.push_back({ xstep, ystep });
		}
	}
	else
	{
		ystep += tile_height * map.height;
	}
	if (mode == TileSwap::Mode::WALL_NW)
	{
		for (int i = 1; i < map.width; ++i)
		{
			points.push_back({ xstep, ystep });
			xstep += tile_width;
			points.push_back({ xstep, ystep });
			ystep -= tile_height / 2;
		}
	}
	else
	{
		ystep -= tile_height / 2;
		for (int i = 1; i < map.width; ++i)
		{
			points.push_back({ xstep, ystep });
			xstep -= tile_width;
			points.push_back({ xstep, ystep });
			ystep -= tile_height / 2;
		}
	}
	if (mode == TileSwap::Mode::FLOOR)
	{
		for (int i = 1; i < map.height; ++i)
		{
			ystep -= tile_height / 2;
			points.push_back({ xstep, ystep });
			xstep += tile_width;
			points.push_back({ xstep, ystep });
		}
		ystep -= tile_height / 2;
		points.push_back({ xstep, ystep });
	}
	else
	{
		points.push_back({ xstep, ystep });
		xstep += mode == TileSwap::Mode::WALL_NW ? static_cast<int>(tile_width) : -static_cast<int>(tile_width);
		points.push_back({ xstep, ystep });
		ystep -= tile_height * map.height;
		points.push_back({ xstep, ystep });
	}

	return points;
}

std::vector<std::pair<int, int>> TileSwap::OffsetRegionPoly(const std::vector<std::pair<int, int>>& points, const std::pair<int, int>& offset, int tile_width, int tile_height)
{
	std::vector<std::pair<int, int>> offsetted;
	std::transform(points.begin(), points.end(), std::back_inserter(offsetted), [&](const auto& pt)
		{
			return std::make_pair<int, int>((pt.first + offset.first) * tile_width, (pt.second + offset.second) * tile_height);
		});
	return offsetted;
}

std::pair<int, int> TileSwap::GetTileOffset(TileSwap::Region region, std::shared_ptr<const Tilemap3D> tilemap, const Tilemap3D::Layer& layer) const
{
	std::pair<int, int> offset = GetRelTileOffset(layer);
	if (tilemap)
	{
		offset.first -= tilemap->GetLeft();
		offset.second -= tilemap->GetTop();
	}
	if (region == Region::SOURCE)
	{
		offset.first += this->map.src_x;
		offset.second += this->map.src_y;
	}
	else if (region == Region::DESTINATION)
	{
		offset.first += this->map.dst_x;
		offset.second += this->map.dst_y;
	}
	return offset;
}

std::pair<int, int> TileSwap::GetRelTileOffset(const Tilemap3D::Layer& layer) const
{
	switch (mode)
	{
	case Mode::WALL_NE:
		return { layer == Tilemap3D::Layer::FG ? 1 : 0, 0 };
	case Mode::WALL_NW:
		return { 0, layer == Tilemap3D::Layer::FG ? 0 : 1 };
	case Mode::FLOOR:
	default:
		return { 0,0 };
	}
}

void TileSwap::DrawSwap(Tilemap3D& tilemap, Tilemap3D::Layer layer) const
{
	for (int y = 0; y < tilemap.GetHeight(); ++y)
	{
		for (int x = 0; x < tilemap.GetWidth(); ++x)
		{
			uint16_t swapped = 0;
			int x_off = x - map.dst_x + tilemap.GetLeft();
			int y_off = y - map.dst_y + tilemap.GetTop();
			bool swap_condition = false;
			switch (mode)
			{
			case TileSwap::Mode::FLOOR:
				swap_condition = (x_off >= 0 && x_off < map.width) && (y_off >= 0 && y_off < map.height);
				break;
			case TileSwap::Mode::WALL_NE:
				if (layer == Tilemap3D::Layer::BG)
				{
					swap_condition = (x_off - y_off >= 0 && x_off - y_off < map.width) &&
						(y_off >= 0 && y_off < map.height);
				}
				else
				{
					swap_condition = (x_off - y_off - 1 >= 0 && x_off - y_off - 1 < map.width) &&
						(y_off >= 0 && y_off < map.height);
				}
				break;
			case TileSwap::Mode::WALL_NW:
				if (layer == Tilemap3D::Layer::BG)
				{
					swap_condition = (x_off >= 0 && x_off < map.height) &&
						(y_off > x_off && y_off <= x_off + map.width);
				}
				else
				{
					swap_condition = (x_off >= 0 && x_off < map.height) &&
						(y_off >= x_off && y_off < x_off + map.width);
				}
				break;
			}
			int src_x = map.src_x - map.dst_x + x;
			int src_y = map.src_y - map.dst_y + y;
			if (swap_condition)
			{
				swapped = (src_x >= 0 && src_x < tilemap.GetWidth() && src_y >= 0 && src_y < tilemap.GetHeight()) ?
					tilemap.GetBlock({ src_x, src_y }, layer) : 0;
				tilemap.SetBlock({ swapped, {x, y} }, layer);
			}
		}
	}
}

void TileSwap::DrawHeightmapSwap(Tilemap3D& tilemap) const
{
	for (int y = 0; y < heightmap.height; ++y)
	{
		for (int x = 0; x < heightmap.width; ++x)
		{
			auto src = HMPoint2D(x + heightmap.src_x, y + heightmap.src_y);
			auto dst = HMPoint2D(x + heightmap.dst_x, y + heightmap.dst_y);
			if (tilemap.IsHMPointValid(src) && tilemap.IsHMPointValid(dst))
			{
				tilemap.SetHeightmapCell(dst, tilemap.GetHeightmapCell(src));
			}
		}
	}
}

bool TileSwap::IsHeightmapPointInSwap(int x, int y) const
{
	return ((x >= heightmap.dst_x && x < heightmap.dst_x + heightmap.width) &&
		    (y >= heightmap.dst_y && y < heightmap.dst_y + heightmap.height));
}

bool TileSwap::operator==(const TileSwap& rhs) const
{
	return (this->trigger == rhs.trigger &&
		    this->map.src_x == rhs.map.src_x &&
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
	return !(*this == rhs);
}
