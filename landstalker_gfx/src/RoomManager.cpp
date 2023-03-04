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
	if (!LoadAsmRoomPalettes())
	{
		throw std::runtime_error(std::string("Unable to load palette data from \'") + m_palette_data_filename.str() + '\'');
	}
	if (!LoadAsmMiscPalettes())
	{
		throw std::runtime_error(std::string("Unable to load misc palette data from \'") + asm_file.str() + '\'');
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
	if (!LoadRomRoomPalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to load palette data from ROM"));
	}
	if (!LoadRomMiscPalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to load misc palette data from ROM"));
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
	if (!SaveAsmWarpData(dir))
	{
		throw std::runtime_error(std::string("Unable to save warp data to \'") + m_asm_filename.str() + '\'');
	}
	if (!SaveAsmRoomPalettes(dir))
	{
		throw std::runtime_error(std::string("Unable to save palettes to \'") + m_palette_data_filename.str() + '\'');
	}
	if (!SaveAsmPaletteFilenames(dir))
	{
		throw std::runtime_error(std::string("Unable to save palette data to \'") + m_palette_data_filename.str() + '\'');
	}
	if (!SaveAsmMiscPalettes(dir))
	{
		throw std::runtime_error(std::string("Unable to save misc palette data to \'") + m_asm_filename.str() + '\'');
	}
	return true;
}

bool RoomManager::Save()
{
	return Save(m_base_path);
}

bool RoomManager::HasBeenModified() const
{
	for (const auto& map : m_maps)
	{
		if (map.second->orig_map == nullptr ||
			*map.second->map != *map.second->orig_map)
		{
			return true;
		}
	}
	for (const auto& pal : m_room_pals)
	{
		if (pal.orig_pal == nullptr ||
			*pal.pal != *pal.orig_pal)
		{
			return true;
		}
	}
	if ((m_warps != m_warps_orig) || (m_roomlist != m_roomlist_orig))
	{
		return true;
	}
	return false;
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

bool RoomManager::InjectIntoRom(Rom& rom)
{
	return false;
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
	const uint32_t map_end_addr = rom.read<uint32_t>(rom.get_address(RomOffsets::Rooms::ROOM_PALS_PTR));

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

bool RoomManager::LoadRomRoomPalettes(const Rom& rom)
{
	uint32_t palettes_begin = rom.read<uint32_t>(rom.get_address(RomOffsets::Rooms::ROOM_PALS_PTR));
	uint32_t palettes_end = rom.read<uint32_t>(rom.get_address(RomOffsets::Rooms::ROOM_EXITS_PTR));
	assert(palettes_end > palettes_begin);
	uint32_t palettes_size = palettes_end - palettes_begin;
	auto bytes = rom.read_array<uint8_t>(palettes_begin, palettes_size);
	auto palettes = CreatePalettes(bytes, Palette::Type::ROOM);
	unsigned int i = 0;
	unsigned int addr = palettes_begin;
	while (addr < palettes_end)
	{
		PaletteEntry e;
		unsigned int size = std::min<unsigned int>(palettes_end - addr, Palette::GetSize(Palette::Type::ROOM) * 2);
		unsigned int end_addr = addr + size;
		auto d = std::make_shared<std::vector<uint8_t>>(rom.read_array<uint8_t>(addr, size));
		auto p = std::make_shared<Palette>(*d, Palette::Type::ROOM);
		e.index = i++;
		e.name = StrPrintf(RomOffsets::Rooms::PALETTE_FORMAT_STRING, i);
		e.start_address = addr;
		e.end_address = end_addr;
		e.orig_pal = p;
		e.pal = std::make_shared<Palette>(*p);
		e.filename = StrPrintf(RomOffsets::Rooms::PALETTE_FILENAME_FORMAT_STRING, e.name.c_str());
		e.raw_data = d;
		m_room_pals.push_back(e);
		addr = end_addr;
	}
	return true;
}

bool RoomManager::LoadRomMiscPalettes(const Rom& rom)
{
	auto load_pal_array = [&](const std::string& name, const filesystem::path& fname, const Palette::Type& ptype)
	{
		uint32_t addr = rom.get_section(name).begin;
		uint32_t end = rom.get_section(name).end;
		uint32_t size = Palette::GetSize(ptype) * 2;
		unsigned int i = 0;
		std::vector<PaletteEntry> ret;
		for (; addr < end; addr += size)
		{
			auto bytes = std::make_shared<std::vector<uint8_t>>(rom.read_array<uint8_t>(addr, size));
			PaletteEntry e;
			e.raw_data = bytes;
			e.start_address = addr;
			e.end_address = addr + size;
			e.index = i++;
			e.name = name;
			e.filename = fname;
			auto p = std::make_shared<Palette>(*bytes, ptype);
			e.orig_pal = p;
			e.pal = std::make_shared<Palette>(*p);
			ret.push_back(e);
		}
		return ret;
	};
	m_lava_palette = load_pal_array(RomOffsets::Rooms::PALETTE_LAVA, RomOffsets::Rooms::PALETTE_LAVA_FILENAME, Palette::Type::LAVA);
	m_warp_palette = load_pal_array(RomOffsets::Rooms::PALETTE_WARP, RomOffsets::Rooms::PALETTE_WARP_FILENAME, Palette::Type::WARP);
	m_labrynth_lit_palette = load_pal_array(RomOffsets::Rooms::PALETTE_LANTERN, RomOffsets::Rooms::PALETTE_LANTERN_FILENAME, Palette::Type::ROOM).front();
	return true;
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
		f.Goto(RomOffsets::Rooms::ROOM_PALS);
		f >> m_palette_data_filename;
		f.Goto(RomOffsets::Rooms::PALETTE_LANTERN);
		f >> m_lantern_pal_data_filename;
		f.Goto(RomOffsets::Rooms::PALETTE_LAVA);
		f >> m_lava_pal_data_filename;
		f.Goto(RomOffsets::Rooms::PALETTE_WARP);
		f >> m_warp_pal_data_filename;
		if (filesystem::path(m_base_path / m_room_data_filename).exists() &&
			filesystem::path(m_base_path / m_map_data_filename).exists() &&
			filesystem::path(m_base_path / m_warp_data_filename).exists() &&
			filesystem::path(m_base_path / m_climb_data_filename).exists() &&
			filesystem::path(m_base_path / m_fall_data_filename).exists() &&
			filesystem::path(m_base_path / m_transition_data_filename).exists() &&
			filesystem::path(m_base_path / m_palette_data_filename).exists() &&
			filesystem::path(m_base_path / m_lantern_pal_data_filename).exists() &&
			filesystem::path(m_base_path / m_lava_pal_data_filename).exists() &&
			filesystem::path(m_base_path / m_warp_pal_data_filename).exists())
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

bool RoomManager::SaveAsmWarpData(const filesystem::path& dir)
{
	auto warp_bytes = m_warps.GetWarpBytes();
	auto fall_bytes = m_warps.GetFallBytes();
	auto climb_bytes = m_warps.GetClimbBytes();
	auto transition_bytes = m_warps.GetTransitionBytes();

	if (m_warp_data_filename.empty())
	{
		m_warp_data_filename = RomOffsets::Rooms::WARP_FILENAME;
	}
	if (m_climb_data_filename.empty())
	{
		m_climb_data_filename = RomOffsets::Rooms::CLIMB_DEST_FILENAME;
	}
	if (m_fall_data_filename.empty())
	{
		m_fall_data_filename = RomOffsets::Rooms::FALL_DEST_FILENAME;
	}
	if (m_transition_data_filename.empty())
	{
		m_transition_data_filename = RomOffsets::Rooms::TRANSITION_FILENAME;
	}

	CreateDirectoryTree(dir / m_warp_data_filename);
	WriteBytes(warp_bytes, dir / m_warp_data_filename);
	CreateDirectoryTree(dir / m_fall_data_filename);
	WriteBytes(fall_bytes, dir / m_fall_data_filename);
	CreateDirectoryTree(dir / m_climb_data_filename);
	WriteBytes(climb_bytes, dir / m_climb_data_filename);
	CreateDirectoryTree(dir / m_transition_data_filename);
	WriteBytes(transition_bytes, dir / m_transition_data_filename);

	return true;
}

bool RoomManager::SaveAsmRoomPalettes(const filesystem::path& dir)
{
	for (auto& palette : m_room_pals)
	{
		if (palette.filename.empty())
		{
			palette.filename = StrPrintf(RomOffsets::Rooms::PALETTE_FILENAME_FORMAT_STRING, palette.name.c_str());
		}
		auto fname = dir / palette.filename;
		if (*palette.pal != *palette.orig_pal)
		{
			palette.raw_data = std::make_shared<std::vector<uint8_t>>(palette.pal->GetBytes());
			*palette.orig_pal = *palette.pal;
		}
		CreateDirectoryTree(fname);
		WriteBytes(*palette.raw_data, fname);
	}
	return true;
}

bool RoomManager::SaveAsmMiscPalettes(const filesystem::path& dir)
{
	if (*m_labrynth_lit_palette.pal != *m_labrynth_lit_palette.orig_pal)
	{
		m_labrynth_lit_palette.raw_data = std::make_shared<std::vector<uint8_t>>(m_labrynth_lit_palette.pal->GetBytes());
		*m_labrynth_lit_palette.orig_pal = *m_labrynth_lit_palette.pal;
	}
	auto combine_palette_array = [](std::vector<PaletteEntry>& pals)
	{
		std::vector<uint8_t> ret;
		for (auto& p : pals)
		{
			if (*p.pal != *p.orig_pal)
			{
				p.raw_data = std::make_shared<std::vector<uint8_t>>(p.pal->GetBytes());
				*p.orig_pal = *p.pal;
			}
			ret.insert(ret.end(), p.raw_data->begin(), p.raw_data->end());
		}
		return ret;
	};
	auto lava_pal_bytes = combine_palette_array(m_lava_palette);
	auto warp_pal_bytes = combine_palette_array(m_warp_palette);

	if (m_lantern_pal_data_filename.empty())
	{
		m_lantern_pal_data_filename = RomOffsets::Rooms::PALETTE_LANTERN_FILENAME;
	}
	if (m_lava_pal_data_filename.empty())
	{
		m_lava_pal_data_filename = RomOffsets::Rooms::PALETTE_LAVA_FILENAME;
	}
	if (m_warp_pal_data_filename.empty())
	{
		m_warp_pal_data_filename = RomOffsets::Rooms::PALETTE_WARP_FILENAME;
	}
	CreateDirectoryTree(dir / m_lantern_pal_data_filename);
	WriteBytes(*m_labrynth_lit_palette.raw_data, dir / m_lantern_pal_data_filename);
	CreateDirectoryTree(dir / m_lava_pal_data_filename);
	WriteBytes(lava_pal_bytes, dir / m_lava_pal_data_filename);
	CreateDirectoryTree(dir / m_warp_pal_data_filename);
	WriteBytes(warp_pal_bytes, dir / m_warp_pal_data_filename);
	return true;
}

bool RoomManager::SaveAsmPaletteFilenames(const filesystem::path& dir)
{
	try
	{
		if (m_palette_data_filename.empty())
		{
			m_palette_data_filename = RomOffsets::Rooms::PALETTE_DATA_FILE;
		}
		AsmFile file;
		file.WriteFileHeader(m_palette_data_filename, "Room palette data include file");
		for (const auto& palette : m_room_pals)
		{
			file << AsmFile::IncludeFile(palette.filename, AsmFile::BINARY);
		}
		auto f = dir / m_palette_data_filename;
		CreateDirectoryTree(f);
		file.WriteFile(f);
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
			map_entry->start_address = 0;
			map_entry->end_address = 0;
			m_maps[lbl] = map_entry;
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool RoomManager::LoadAsmRoomPalettes()
{
	try
	{
		AsmFile file(m_base_path / m_palette_data_filename);
		AsmFile::IncludeFile inc;
		unsigned int i = 0;
		while (file.IsGood())
		{
			file >> inc;
			PaletteEntry e;
			e.index = i++;
			auto palfile = m_base_path / inc.path;
			auto raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(palfile));
			auto palette = std::make_shared<Palette>(*raw_data, Palette::Type::ROOM);
			e.filename = inc.path;
			e.name = e.filename.filename();
			e.name = e.name.substr(0, e.name.find_last_of('.'));
			e.raw_data = raw_data;
			e.orig_pal = palette;
			e.pal = std::make_shared<Palette>(*e.orig_pal);
			e.start_address = 0;
			e.end_address = 0;
			m_room_pals.push_back(e);
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool RoomManager::LoadAsmMiscPalettes()
{
	auto load_pal_array = [&](const std::string& name, const filesystem::path& fname, const Palette::Type& ptype)
	{
		auto path = m_base_path / fname;
		auto data = std::make_shared<std::vector<uint8_t>>(ReadBytes(path));
		auto pals = CreatePalettes(*data, ptype);
		std::vector<PaletteEntry> ret;
		int idx = 0;
		for (const auto& p : pals)
		{
			PaletteEntry e;
			e.start_address = 0;
			e.end_address = 0;
			e.index = idx++;
			e.name = name;
			e.filename = fname;
			e.orig_pal = std::make_shared<Palette>(p);
			e.pal = std::make_shared<Palette>(p);
			e.raw_data = std::make_shared<std::vector<uint8_t>>(p.GetBytes());;
			ret.push_back(e);
		}
		return ret;
	};

	m_lava_palette = load_pal_array(RomOffsets::Rooms::PALETTE_LAVA, m_lava_pal_data_filename, Palette::Type::LAVA);
	m_warp_palette = load_pal_array(RomOffsets::Rooms::PALETTE_WARP, m_warp_pal_data_filename, Palette::Type::WARP);
	m_labrynth_lit_palette = load_pal_array(RomOffsets::Rooms::PALETTE_LANTERN, m_lantern_pal_data_filename, Palette::Type::ROOM).front();
	return true;
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
			map.second->filename = StrPrintf(RomOffsets::Rooms::MAP_FILENAME_FORMAT_STRING, map.first.c_str());
		}
		auto fname = dir / map.second->filename;
		if (*map.second->map != *map.second->orig_map)
		{
			map.second->raw_data->resize(65536);
			auto ret = map.second->map->Encode(&map.second->raw_data->at(0), map.second->raw_data->size());
			map.second->raw_data->resize(ret);
			*map.second->orig_map = *map.second->map;
		}
		CreateDirectoryTree(fname);
		WriteBytes(*map.second->raw_data, fname);
	}
	return true;
}

bool RoomManager::SaveAsmMapFilenames(const filesystem::path& dir)
{
	try
	{
		if (m_map_data_filename.empty())
		{
			m_map_data_filename = RomOffsets::Rooms::MAP_DATA_FILE;
		}
		AsmFile file;
		file.WriteFileHeader(m_map_data_filename, "Map data include file");
		for (const auto& map : m_maps)
		{
			file << AsmFile::Label(map.first) << AsmFile::IncludeFile(map.second->filename, AsmFile::BINARY);
		}
		auto f = dir / m_map_data_filename;
		CreateDirectoryTree(f);
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
		if (m_room_data_filename.empty())
		{
			m_room_data_filename = RomOffsets::Rooms::ROOM_DATA_FILE;
		}
		AsmFile file;
		file.WriteFileHeader(m_room_data_filename, "Room data include file");
		for (const auto& room : m_roomlist)
		{
			auto params = room.GetParams();
			file << room.map << params;
		}
		auto f = dir / m_room_data_filename;
		CreateDirectoryTree(f);
		file.WriteFile(f);
		m_roomlist_pending_modifications = false;
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

std::shared_ptr<const Palette> RoomManager::GetRoomPalette(unsigned int index) const
{
	assert(index < m_room_pals.size());
	return m_room_pals[index].pal;
}

std::shared_ptr<Palette> RoomManager::GetRoomPalette(unsigned int index)
{
	assert(index < m_room_pals.size());
	return m_room_pals[index].pal;
}
