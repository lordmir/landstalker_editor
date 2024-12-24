#include <landstalker/3d_maps/include/Doors.h>
#include <algorithm>
#include <iterator>
#include <landstalker/3d_maps/include/TileSwaps.h>

Doors::Doors(const std::vector<uint8_t>& offsets, const std::vector<uint8_t>& bytes)
{
	int offset = 0;
	for (uint16_t i = 0; i < offsets.size(); ++i)
	{
		if (offsets[i] > 0)
		{
			while (bytes[offset] != 0xFF)
			{
				Door door(bytes[offset], bytes[offset + 1]);
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
	for (uint16_t i = 0; i < static_cast<uint16_t>(roomcount); ++i)
	{
		if (m_doors.count(i) > 0)
		{
			offsets.push_back(static_cast<uint8_t>(lastsz + 1));
			lastsz = m_doors.at(i).size() * 2 + 1;
			for (const auto& d : m_doors.at(i))
			{
				auto b = d.GetBytes();
				doors.push_back(b.first);
				doors.push_back(b.second);
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

Door::Door(uint8_t b1, uint8_t b2)
{
	y = (b1 & 0x3F) - 12;
	x = (b2 & 0x3F) - 12;
	size = static_cast<Door::Size>(((b1 & 0x40) >> 4) | ((b2 & 0xC0) >> 6));
}

std::pair<uint8_t, uint8_t> Door::GetBytes() const
{
	uint8_t sz = static_cast<uint8_t>(size);
	return { static_cast<uint8_t>(((y + 12) & 0x3F) | ((sz & 0x04) << 4)),
			 static_cast<uint8_t>(((x + 12) & 0x3F) | ((sz & 0x03) << 6)) };
}

std::pair<bool, std::vector<std::pair<int, int>>> Door::GetMapRegionPoly(std::shared_ptr<const Tilemap3D> tilemap, int tile_width, int tile_height) const
{
	Tilemap3D::FloorType type = Tilemap3D::FloorType::NORMAL;
	TileSwap ts;
	ts.map.src_x = 0;
	ts.map.src_y = 0;
	ts.map.dst_x = 0;
	ts.map.dst_y = 0;
	ts.map.width = SIZES.at(size).first;
	ts.map.height = SIZES.at(size).second;
	bool valid = true;
	if (tilemap)
	{
		type = static_cast<Tilemap3D::FloorType>(tilemap->GetCellType({ x, y }));
	}
	if (type != Tilemap3D::FloorType::DOOR_NE && type != Tilemap3D::FloorType::DOOR_NW)
	{
		valid = false;
	}
	ts.mode = (type == Tilemap3D::FloorType::DOOR_NE) ? TileSwap::Mode::WALL_NE : TileSwap::Mode::WALL_NW;
	return {valid, ts.GetMapRegionPoly(TileSwap::Region::UNDEFINED, tile_width, tile_height)};
}

std::vector<std::pair<int, int>> Door::OffsetRegionPoly(const std::vector<std::pair<int, int>>& points, const std::pair<int, int>& offset)
{
	std::vector<std::pair<int, int>> offsetted;
	std::transform(points.begin(), points.end(), std::back_inserter(offsetted), [&](const auto& pt)
		{
			return std::make_pair<int, int>(pt.first + offset.first, pt.second + offset.second);
		});
	return offsetted;
}

std::pair<int, int> Door::GetTileOffset(std::shared_ptr<const Tilemap3D> tilemap, const Tilemap3D::Layer& layer) const
{
	return tilemap ? GetTileOffset(*tilemap, layer) : std::make_pair<int, int>(0, 0);
}

std::pair<int, int> Door::GetTileOffset(const Tilemap3D& tilemap, const Tilemap3D::Layer& layer) const
{
	Tilemap3D::FloorType type = Tilemap3D::FloorType::NORMAL;
	std::pair<int, int> offset = { x + 12, y + 12 };
	offset.first -= Door::SIZES.at(size).second;
	offset.second -= Door::SIZES.at(size).second;
	if (x < tilemap.GetHeightmapWidth() && y < tilemap.GetHeightmapHeight())
	{
		type = static_cast<Tilemap3D::FloorType>(tilemap.GetCellType({ x, y }));
		int z = tilemap.GetHeight({ x, y });
		offset.first -= tilemap.GetLeft();
		offset.second -= tilemap.GetTop();
		offset.first -= z;
		offset.second -= z;
	}
	if (type == Tilemap3D::FloorType::DOOR_NE)
	{
		if (layer == Tilemap3D::Layer::FG)
		{
			offset.first += 1;
			offset.second += 0;
		}
		else
		{
			offset.first += 1;
			offset.second += 1;
		}
	}
	else if (type == Tilemap3D::FloorType::DOOR_NW)
	{
		if (layer == Tilemap3D::Layer::FG)
		{
			offset.first += 1;
			offset.second += 1;
		}
		else
		{
			offset.first += 0;
			offset.second += 1;
		}
	}
	return offset;
}

void Door::DrawDoor(Tilemap3D& tilemap, Tilemap3D::Layer layer) const
{
	if (layer == Tilemap3D::Layer::BG)
	{
		return;
	}
	Tilemap3D::FloorType type = static_cast<Tilemap3D::FloorType>(tilemap.GetCellType({ x, y }));
	int z = tilemap.GetHeight({ x, y });

	TileSwap ts;
	ts.map.src_x = 255;
	ts.map.src_y = 255;
	ts.map.dst_x = static_cast<uint8_t>(12 + x - z - Door::SIZES.at(size).second + (type == Tilemap3D::FloorType::DOOR_NW ? 1 : 0));
	ts.map.dst_y = static_cast<uint8_t>(12 + y - z - Door::SIZES.at(size).second + (type == Tilemap3D::FloorType::DOOR_NW ? 1 : 0));
	ts.map.width = SIZES.at(size).first;
	ts.map.height = SIZES.at(size).second;
	if (type != Tilemap3D::FloorType::DOOR_NE && type != Tilemap3D::FloorType::DOOR_NW)
	{
		return;
	}
	ts.mode = (type == Tilemap3D::FloorType::DOOR_NE) ? TileSwap::Mode::WALL_NE : TileSwap::Mode::WALL_NW;
	ts.DrawSwap(tilemap, layer);
}

bool Door::operator==(const Door& rhs) const
{
	return (this->x == rhs.x && this->y == rhs.y && this->size == rhs.size);
}

bool Door::operator!=(const Door& rhs) const
{
	return !(*this == rhs);
}
