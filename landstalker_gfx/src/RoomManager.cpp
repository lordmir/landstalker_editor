#include "RoomManager.h"
#include "RomOffsets.h"
#include <set>
#include <cassert>

RoomManager::RoomManager(const filesystem::path& asm_file)
	: m_asm_filename(asm_file),
	  m_roomlist_pending_modifications(false)
{
	if (!GetAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load room data from \'") + asm_file.str() + '\'');
	}
	if (!LoadAsmRoomTable())
	{
		throw std::runtime_error(std::string("Unable to load room table data from \'") + m_room_data_filename.str() + '\'');
	}
	if (!LoadAsmMapData())
	{
		throw std::runtime_error(std::string("Unable to load map data from \'") + m_map_data_filename.str() + '\'');
	}
	if (!LoadAsmWarpData())
	{
		throw std::runtime_error(std::string("Unable to load warp data from \'") + m_warp_data_filename.str() + '\'');
	}
}

RoomManager::RoomManager(const Rom& rom)
	: m_roomlist_pending_modifications(false)
{
	if (!LoadRomRoomTable(rom))
	{
		throw std::runtime_error(std::string("Unable to load room data from ROM"));
	}
	if (!LoadRomWarpData(rom))
	{
		throw std::runtime_error(std::string("Unable to load warp data from ROM"));
	}
}

bool RoomManager::Save(filesystem::path dir)
{
	if (dir.exists() && dir.is_file())
	{
		dir = dir.parent_path();
	}
	SaveMapsToDisk(dir);
	if (!SaveAsmMapFilenames(dir))
	{
		throw std::runtime_error(std::string("Unable to save map file list to \'") + m_map_data_filename.str() + '\'');
	}
	if (!SaveAsmRoomData(dir))
	{
		throw std::runtime_error(std::string("Unable to save room data to \'") + m_room_data_filename.str() + '\'');
	}
	return true;
}

bool RoomManager::Save()
{
	return Save(m_base_path);
}

bool RoomManager::HasBeenModified()
{
	for (const auto& map : m_maps)
	{
		if (HasMapBeenModified(map.first))
		{
			return true;
		}
	}
	return m_roomlist_pending_modifications;
}

bool RoomManager::HasMapBeenModified(const std::string& map)
{
	auto it = m_maps.find(map);
	if (it == m_maps.end())
	{
		return true;
	}
	return *it->second->map != *it->second->orig_map;
}

std::size_t RoomManager::GetRoomCount() const
{
	return m_roomlist.size();
}

const Room& RoomManager::GetRoom(uint16_t idx) const
{
	assert(idx < m_roomlist.size());
	return m_roomlist[idx];
}

Room& RoomManager::GetRoom(uint16_t idx)
{
	assert(idx < m_roomlist.size());
	return m_roomlist[idx];
}

std::vector<std::string> RoomManager::GetMapList() const
{
	std::vector<std::string> ret;
	std::transform(m_maps.begin(), m_maps.end(), std::back_inserter(ret), [](const auto& val) {
		return val.first;
	});
	return ret;
}

std::shared_ptr<RoomManager::MapEntry> RoomManager::GetMap(const std::string& name)
{
	auto result = m_maps.find(name);
	if (result != m_maps.end())
	{
		return result->second;
	}
	else
	{
		return std::shared_ptr<MapEntry>();
	}
}

std::shared_ptr<RoomManager::MapEntry> RoomManager::GetMap(uint16_t roomnum)
{
	assert(roomnum < m_roomlist.size());
	auto name = m_roomlist[roomnum].map;
	return GetMap(name);
}

std::list<WarpList::Warp> RoomManager::GetWarpsForRoom(uint16_t roomnum)
{
	return m_warps.GetWarpsForRoom(roomnum);
}

bool RoomManager::HasFallDestination(uint16_t room) const
{
	return m_warps.HasFallDestination(room);
}

uint16_t RoomManager::GetFallDestination(uint16_t room) const
{
	return m_warps.GetFallDestination(room);
}

bool RoomManager::HasClimbDestination(uint16_t room) const
{
	return m_warps.HasClimbDestination(room);
}

uint16_t RoomManager::GetClimbDestination(uint16_t room) const
{
	return m_warps.GetClimbDestination(room);
}

std::map<std::pair<uint16_t, uint16_t>, uint16_t> RoomManager::GetTransitions(uint16_t room) const
{
	return m_warps.GetTransitions(room);
}

bool RoomManager::LoadRomRoomTable(const Rom& rom)
{
	uint32_t addr = rom.read<uint32_t>(rom.get_address(RomOffsets::Rooms::ROOM_DATA_PTR));
	const uint32_t map_end_addr = rom.read<uint32_t>(rom.get_address("room_palette_ptr"));

	uint32_t table_end_addr = map_end_addr;
	uint32_t map_addr = map_end_addr;
	std::set<uint32_t> map_list;
	do
	{
		map_addr = rom.read<uint32_t>(addr);
		table_end_addr = std::min(map_addr, table_end_addr);
		map_list.insert(map_addr);
		m_roomlist.push_back(Room(Hex(map_addr), rom.read_array<uint8_t>(addr+4, 4)));
		addr += 8;
	} while (addr < table_end_addr);
	std::map<std::string, std::string> map_names;
	unsigned int count = 0;
	for(auto it = map_list.begin(); it != map_list.end(); ++it)
	{
		uint32_t begin = *it;
		uint32_t end;
		if (std::next(it) == map_list.end())
		{
			end = map_end_addr;
		}
		else
		{
			end = *std::next(it);
		}
		std::string name = StrPrintf(RomOffsets::Rooms::MAP_FORMAT_STRING, ++count);
		map_names[Hex(begin)] = name;
		auto map_entry = std::make_shared<MapEntry>();
		map_entry->raw_data = std::make_shared<std::vector<uint8_t>>(rom.read_array<uint8_t>(begin, end - begin));
		map_entry->orig_map = std::make_shared<Tilemap3D>(&map_entry->raw_data->at(0));
		map_entry->map = std::make_shared<Tilemap3D>(*map_entry->orig_map);
		map_entry->name = name;
		map_entry->start_address = begin;
		map_entry->end_address = end;
		map_entry->filename = StrPrintf(RomOffsets::Rooms::MAP_FILENAME_FORMAT_STRING, name.c_str());
		m_maps.insert(std::make_pair(name, map_entry));
	}
	for (auto& room : m_roomlist)
	{
		room.map = map_names[room.map];
	}

	return true;
}

bool RoomManager::LoadRomWarpData(const Rom& rom)
{
	m_warps = WarpList(rom);
	return true;
}

bool RoomManager::LoadRoomPalettes(const Rom& rom)
{
	return false;
}

bool RoomManager::GetAsmFilenames()
{
	try
	{
		AsmFile f(m_asm_filename.str());
		m_base_path = m_asm_filename.parent_path();
		f.Goto(RomOffsets::Rooms::ROOM_DATA);
		f >> m_room_data_filename;
		f.Goto(RomOffsets::Rooms::MAP_DATA);
		f >> m_map_data_filename;
		f.Goto(RomOffsets::Rooms::ROOM_EXITS);
		f >> m_warp_data_filename;
		f.Goto(RomOffsets::Rooms::ROOM_FALL_DEST);
		f >> m_fall_data_filename;
		f.Goto(RomOffsets::Rooms::ROOM_CLIMB_DEST);
		f >> m_climb_data_filename;
		f.Goto(RomOffsets::Rooms::ROOM_TRANSITIONS);
		f >> m_transition_data_filename;
		if (filesystem::path(m_base_path / m_room_data_filename).exists() &&
			filesystem::path(m_base_path / m_map_data_filename).exists() &&
			filesystem::path(m_base_path / m_warp_data_filename).exists() &&
			filesystem::path(m_base_path / m_climb_data_filename).exists() &&
			filesystem::path(m_base_path / m_fall_data_filename).exists() &&
			filesystem::path(m_base_path / m_transition_data_filename).exists())
		{
			return true;
		}
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool RoomManager::LoadAsmRoomTable()
{
	try
	{
		AsmFile file(m_base_path / m_room_data_filename);
		std::string map;
		uint8_t params[4];
		while (file.IsGood())
		{
			file >> map >> params;
			m_roomlist.push_back(Room(map, params));
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool RoomManager::LoadAsmMapData()
{
	try
	{
		AsmFile file(m_base_path / m_map_data_filename);
		AsmFile::IncludeFile inc;
		AsmFile::Label lbl;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			auto map_entry = std::make_shared<MapEntry>();
			auto mapfile = m_base_path / inc.path;
			auto raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(mapfile));
			auto map = std::make_shared<Tilemap3D>(&raw_data->at(0));
			map_entry->filename = inc.path;
			map_entry->name = lbl;
			map_entry->raw_data = raw_data;
			map_entry->orig_map = map;
			map_entry->map = std::make_shared<Tilemap3D>(*map_entry->orig_map);
			m_maps[lbl] = map_entry;
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool RoomManager::LoadAsmWarpData()
{
	m_warps = WarpList(m_base_path / m_warp_data_filename, 
	                   m_base_path / m_fall_data_filename,
		               m_base_path / m_climb_data_filename,
		               m_base_path / m_transition_data_filename);
	return true;
}

bool RoomManager::SaveMapsToDisk(const filesystem::path& dir)
{
	for (auto& map : m_maps)
	{
		if (map.second->filename.empty())
		{
			map.second->filename = StrPrintf(RomOffsets::Rooms::MAP_FILENAME_FORMAT_STRING, map.first);
		}
		auto fname = dir / map.second->filename;
		if (HasMapBeenModified(map.first))
		{
			map.second->raw_data->resize(65536);
			auto ret = map.second->map->Encode(&map.second->raw_data->at(0), map.second->raw_data->size());
			map.second->raw_data->resize(ret);
			*map.second->orig_map = *map.second->map;
		}
		if (!fname.parent_path().is_directory() &&
			!filesystem::create_directories(fname.parent_path()))
		{
			throw std::runtime_error(std::string("Unable to create directory \"") + dir.parent_path().str() + "\"");
		}
		WriteBytes(*map.second->raw_data, fname);
	}
	return true;
}

bool RoomManager::SaveAsmMapFilenames(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_map_data_filename, "Map data include file");
		for (const auto& map : m_maps)
		{
			file << AsmFile::Label(map.first) << AsmFile::IncludeFile(map.second->filename, AsmFile::BINARY);
		}
		if (m_map_data_filename.empty())
		{
			m_map_data_filename = RomOffsets::Rooms::MAP_DATA_FILE;
		}
		auto f = dir / m_map_data_filename;
		if (!f.parent_path().is_directory() &&
			!filesystem::create_directories(f.parent_path()))
		{
			throw std::runtime_error(std::string("Unable to create directory \"") + f.parent_path().str() + "\"");
		}
		file.WriteFile(f);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool RoomManager::SaveAsmRoomData(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_room_data_filename, "Room data include file");
		for (const auto& room : m_roomlist)
		{
			auto params = room.GetParams();
			file << room.map << params;
		}
		if (m_room_data_filename.empty())
		{
			m_room_data_filename = RomOffsets::Rooms::ROOM_DATA_FILE;
		}
		auto f = dir / m_room_data_filename;
		if (!f.parent_path().is_directory() &&
			!filesystem::create_directories(f.parent_path()))
		{
			throw std::runtime_error(std::string("Unable to create directory \"") + f.parent_path().str() + "\"");
		}
		file.WriteFile(f);
		m_roomlist_pending_modifications = false;
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}
