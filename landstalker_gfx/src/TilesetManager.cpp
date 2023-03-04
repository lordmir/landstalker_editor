#include <TilesetManager.h>

#include <wjakob/filesystem/path.h>

#include <exception>
#include <set>
#include <algorithm>
#include <LZ77.h>
#include <RomOffsets.h>
#include <Utils.h>

TilesetManager::TilesetManager(const filesystem::path& asm_file)
	: m_asm_filename(asm_file),
	  m_tilesets_defaultpal{}
{
	if (!GetTilesetAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load tileset data from \'") + asm_file.str() + '\'');
	}
	if (!LoadAsmAnimatedTilesetData())
	{
		throw std::runtime_error(std::string("Unable to load animated tileset data from \'") + m_tileset_anim_filename.str() + '\'');
	}
	if (!LoadAsmTilesetPointerData())
	{
		throw std::runtime_error(std::string("Unable to load tileset pointers from \'") + m_tileset_ptrtab_filename.str() + '\'');
	}
	if (!LoadAsmTilesetFilenames())
	{
		throw std::runtime_error(std::string("Unable to load tileset data from \'") + m_tileset_data_filename.str() + '\'');
	}
	if (!LoadAsmBlocksetData())
	{
		throw std::runtime_error(std::string("Unable to load blockset data from \'") + m_blockset_data_filename.str() + '\'');
	}
	if (!LoadAsmBlocksetPointerData())
	{
		throw std::runtime_error(std::string("Unable to load blockset pointer data from \'") + m_blockset_pri_ptr_filename.str() + '\'');
	}
	LoadAsmTilesetData();
}

TilesetManager::TilesetManager(const Rom& rom)
 : m_tilesets_defaultpal{}
{
	if (!LoadRomAnimatedTilesetData(rom))
	{
		throw std::runtime_error("Unable to load animated tileset data from ROM");
	}
	if (!LoadRomTilesetPointerData(rom))
	{
		throw std::runtime_error("Unable to load tileset pointer table from ROM");
	}
	LoadRomBlocksetData(rom);
	LoadRomTilesetData(rom);
}

bool TilesetManager::CheckDataWillFitInRom(const Rom& rom, int& tileset_space, int& anim_table_space) const
{
	tileset_space = 0;
	anim_table_space = 0;
	// First calculate the animation table size
	std::size_t anim_ts_count = std::count_if(m_tilesets_by_name.begin(),
	                                m_tilesets_by_name.end(),
	                                [](const auto& ts) {return ts.second->type == Type::ANIMATED_MAP; });
	// one byte for base tileset, two for offset, two for length, one for speed,
	// one for frames and four for pointer
	anim_table_space = anim_ts_count * (1 + 2 + 2 + 1 + 1 + 4);
	anim_table_space++;
	// Fix alignment
	anim_table_space += anim_table_space & 1;
	// Next, the pointer table
	// 4 bytes per pointer, one pointer for start of map tiles, one pointer for each anim/font tileset and
	// 32 pointers for the map tilesets.
	tileset_space += 4 * (1 + m_tilesets_listorder.size() + m_tileset_list_x.size());
	// Now add up the size of all of the tilesets
	std::unordered_map<std::shared_ptr<TilesetEntry>, bool> written;
	auto prepare_write = [&](const std::shared_ptr<TilesetEntry>& ts) {
		if (ts != nullptr && written[ts] == false)
		{
			if (ts->type != Type::MAP)
			{
				// Fix alignment for non-compressed tilesets
				tileset_space += tileset_space & 1;
			}
			ts->start_address = rom.get_section(RomOffsets::Tilesets::DATA_LOC).begin + tileset_space;
			m_pending_write[ts] = GetTilesetBits(ts->name);
			ts->end_address = ts->start_address + m_pending_write[ts].size();
			tileset_space += m_pending_write[ts].size();
		}
	};
	for (const auto& ts : m_tileset_list_x)
	{
		prepare_write(ts);
	}
	for (const auto& ts : m_tilesets_listorder)
	{
		prepare_write(ts);
	}
	tileset_space = rom.get_section(RomOffsets::Tilesets::DATA_LOC).size() - tileset_space;
	anim_table_space = rom.get_section(RomOffsets::Tilesets::ANIM_DATA_LOC).size() - anim_table_space;
	return (tileset_space >= 0) && (anim_table_space >= 0);
}

bool TilesetManager::InjectIntoRom(Rom& rom)
{
	SaveTilesetsToRom(rom);
	SaveRomTilesetPointerData(rom);
	SaveRomAnimatedTilesetData(rom);
	m_pending_write.clear();

	return true;
}

bool TilesetManager::Save(filesystem::path dir)
{
	if (dir.exists() && dir.is_file())
	{
		dir = dir.parent_path();
	}
	SaveTilesetsToDisk(dir);
	if (!SaveAsmTilesetFilenames(dir))
	{
		throw std::runtime_error(std::string("Unable to save tileset file list to \'") + m_tileset_data_filename.str() + '\'');
	}
	if (!SaveAsmTilesetPointerData(dir))
	{
		throw std::runtime_error(std::string("Unable to save tileset pointers to \'") + m_tileset_ptrtab_filename.str() + '\'');
	}
	if (!SaveAsmAnimatedTilesetData(dir))
	{
		throw std::runtime_error(std::string("Unable to save animated tileset data to \'") + m_tileset_anim_filename.str() + '\'');
	}
	if (!SaveAsmBlocksetData(dir))
	{
		throw std::runtime_error(std::string("Unable to save blockset data to \'") + m_blockset_data_filename.str() + '\'');
	}
	if (!SaveAsmBlocksetPointerData(dir))
	{
		throw std::runtime_error(std::string("Unable to save blockset pointer data to \'") + m_blockset_pri_ptr_filename.str() + '\'');
	}

	return true;
}

bool TilesetManager::Save()
{
	return Save(m_base_path);
}

std::shared_ptr<Tileset> TilesetManager::GetTileset(const std::string& name)
{
	if (m_tilesets_by_name.find(name) != m_tilesets_by_name.end())
	{
		return m_tilesets_by_name.at(name)->tileset;
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<const TilesetManager::TilesetEntry> TilesetManager::GetTilesetByName(const std::string& name) const
{
	if (m_tilesets_by_name.find(name) == m_tilesets_by_name.end())
	{
		return nullptr;
	}
	else
	{
		return m_tilesets_by_name.at(name);
	}
}

std::shared_ptr<TilesetManager::TilesetEntry> TilesetManager::GetTilesetByName(const std::string& name)
{
	if (m_tilesets_by_name.find(name) == m_tilesets_by_name.end())
	{
		return nullptr;
	}
	else
	{
		return m_tilesets_by_name.at(name);
	}
}

std::shared_ptr<const TilesetManager::TilesetEntry> TilesetManager::GetTilesetByPtr(std::shared_ptr<Tileset> ts) const
{
	if (m_tilesets_by_ptr.find(ts) == m_tilesets_by_ptr.end())
	{
		return nullptr;
	}
	else
	{
		return m_tilesets_by_ptr.at(ts);
	}
}

std::shared_ptr<TilesetManager::TilesetEntry> TilesetManager::GetTilesetByPtr(std::shared_ptr<Tileset> ts)
{
	if (m_tilesets_by_ptr.find(ts) == m_tilesets_by_ptr.end())
	{
		return nullptr;
	}
	else
	{
		return m_tilesets_by_ptr.at(ts);
	}
}

std::shared_ptr<const TilesetManager::TilesetEntry> TilesetManager::GetTilesetByIdx(uint8_t ts) const
{
	return m_tileset_list_x[ts];
}

std::shared_ptr<TilesetManager::TilesetEntry> TilesetManager::GetTilesetByIdx(uint8_t ts)
{
	return m_tileset_list_x[ts];
}

std::vector<std::string> TilesetManager::GetTilesetList() const
{
	std::vector<std::string> list;
	list.reserve(m_tilesets_by_name.size());
	std::transform(m_tilesets_by_name.begin(),
		m_tilesets_by_name.end(),
		std::back_inserter(list),
		[](const auto& ts) { return ts.first; });
	return list;
}

std::vector<std::string> TilesetManager::GetTilesetList(Type type) const
{
	std::vector<std::string> list;
	list.reserve(m_tilesets_by_name.size());
	for (const auto& ts : m_tilesets_by_name)
	{
		if (ts.second->type == type)
		{
			list.push_back(ts.first);
		}
	}
	return list;
}

std::vector<std::pair<uint8_t, std::string>> TilesetManager::GetPriBlocksetList() const
{
	std::vector<std::pair<uint8_t, std::string>> retval;
	std::set<uint8_t> pri;
	for (const auto& b : m_blocksets)
	{
		auto pri_idx = b.first.first;
		if (pri.find(pri_idx) == pri.end())
		{
			auto name = StrPrintf(RomOffsets::Blocksets::SEC_LABEL, (pri_idx & 0x1F) + 1);
			if (pri_idx > 0x1F)
			{
				name += "A";
			}
			retval.push_back({ pri_idx, name });
			pri.insert(pri_idx);
		}
	}
	return retval;
}

std::vector<std::string> TilesetManager::GetSecBlocksetList(uint8_t pri) const
{
	std::vector<std::string> retval;
	for (const auto& b : m_blocksets)
	{
		if (b.first.first == pri)
		{
			retval.push_back(b.second->name);
		}
	}
	return retval;
}

std::shared_ptr<TilesetManager::BlocksetEntry> TilesetManager::GetBlockset(const std::string& name) const
{
	return m_blocksets_by_ptr.find(name)->second;
}

std::shared_ptr<TilesetManager::BlocksetEntry> TilesetManager::GetBlockset(uint8_t pri, uint8_t sec) const
{
	return m_blocksets.find({ pri, sec })->second;
}

bool TilesetManager::RenameTileset(const std::string& origname, const std::string& newname)
{
	if ((m_tilesets_by_name.find(origname) == m_tilesets_by_name.end()) ||
		(m_tilesets_by_name.find(newname) != m_tilesets_by_name.end()))
	{
		return false;
	}

	auto ts = m_tilesets_by_name[origname];
	ts->name = newname;
	m_tilesets_by_name.erase(origname);
	m_tilesets_by_name.insert({ newname, ts });

	return true;
}

bool TilesetManager::DeleteTileset(const std::string& name)
{
	return false;
}

std::shared_ptr<TilesetManager::TilesetEntry> TilesetManager::MakeNewTileset(const std::string& name, Type type)
{
	return std::shared_ptr<TilesetEntry>();
}

std::array<std::string, 32> TilesetManager::GetTilesetOrder() const
{
	// TODO: insert return statement here
	std::array<std::string, 32> names;

	for (std::size_t i = 0; i < names.size(); i++)
	{
		names[i] = m_tileset_list_x[i]->name;
	}

	return names;
}

void TilesetManager::SetTilesetOrder(const std::array<std::string, 32>& order)
{

	for (std::size_t i = 0; i < order.size(); i++)
	{
		if (m_tilesets_by_name.find(order[i]) != m_tilesets_by_name.end())
		{
			m_tileset_list_x[i] = m_tilesets_by_name[order[i]];
		}
		else
		{
			m_tileset_list_x[i] = nullptr;
		}
	}
}

std::string TilesetManager::GetTilesetSavedPalette(const std::shared_ptr<TilesetEntry> ts) const
{
	auto result = m_tilesets_defaultpal.find(ts);
	if (result == m_tilesets_defaultpal.end())
	{
		return "";
	}
	else
	{
		return result->second;
	}
}

void TilesetManager::SetTilesetSavedPalette(std::shared_ptr<TilesetEntry> ts, const std::string& palette_name)
{
	m_tilesets_defaultpal[ts] = palette_name;
}

bool TilesetManager::GetTilesetAsmFilenames()
{
	try
	{
		AsmFile f(m_asm_filename.str());
		m_base_path = m_asm_filename.parent_path();
		f.Goto(RomOffsets::Tilesets::DATA_LOC);
		f >> m_tileset_data_filename;
		f.Goto(RomOffsets::Tilesets::PTRTAB_LOC);
		f >> m_tileset_ptrtab_filename;
		f.Goto(RomOffsets::Tilesets::ANIM_DATA_LOC);
		f >> m_tileset_anim_filename;
		f.Goto(RomOffsets::Blocksets::PRI_PTRS);
		f >> m_blockset_pri_ptr_filename;
		f.Goto(RomOffsets::Blocksets::SEC_PTRS);
		f >> m_blockset_sec_ptr_filename;
		f.Goto(RomOffsets::Blocksets::DATA);
		f >> m_blockset_data_filename;
		if (filesystem::path(m_base_path / m_tileset_anim_filename).exists() &&
			filesystem::path(m_base_path / m_tileset_ptrtab_filename).exists() &&
			filesystem::path(m_base_path / m_tileset_data_filename).exists() &&
			filesystem::path(m_base_path / m_blockset_pri_ptr_filename).exists() &&
			filesystem::path(m_base_path / m_blockset_sec_ptr_filename).exists() &&
			filesystem::path(m_base_path / m_blockset_data_filename).exists())
		{
			return true;
		}
	}
	catch(const std::exception&)
	{
	}
	return false;
}

bool TilesetManager::LoadAsmAnimatedTilesetData()
{
	try
	{
		AsmFile file(m_base_path / m_tileset_anim_filename);
		uint16_t base;
		uint16_t length;
		uint8_t speed;
		uint8_t frames;
		std::string ptrname;
		file.Goto(RomOffsets::Tilesets::ANIM_LIST_LOC);
		while(file.IsGood())
		{
			file >> base >> length >> speed >> frames >> ptrname;
			auto ts = std::make_shared<AnimatedTileset>(base, length, speed, frames);
			auto ts_entry = std::make_shared<TilesetEntry>(ptrname, ts, Type::ANIMATED_MAP);
			m_tilesets_by_name.insert({ ptrname, ts_entry });
			m_tilesets_by_ptr.insert({ ts, ts_entry });
			m_animated_ts_ptrorder.push_back(ts_entry);
		}
		file.Goto(RomOffsets::Tilesets::ANIM_IDX_LOC);
		std::vector<uint8_t> basets;
		int8_t last = 0;
		for (const auto& at : m_animated_ts_ptrorder)
		{
			file >> last;
			if (last == -1)
			{
				break;
			}
			else
			{
				auto pats = std::static_pointer_cast<AnimatedTileset, Tileset>(at->tileset);
				pats->SetBaseTileset(last);
			}
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool TilesetManager::LoadAsmTilesetPointerData()
{
	try
	{
		AsmFile file(m_base_path / m_tileset_ptrtab_filename);
		file.Goto(RomOffsets::Tilesets::PTR_LOC);
		// First item is always a pointer to the start of the regular tileset list.
		// We can safely ignore this
		std::string name;
		file >> name;
		std::size_t i = 0;
		while (file.IsGood())
		{
			AsmFile::Label lbl;
			// If it has a label, then it is either the intro font, or an animated tileset.
			if (file.IsLabel() && !file.IsLabel(RomOffsets::Tilesets::PTRTAB_BEGIN_LOC))
			{
				file >> lbl >> name;
				auto it = m_tilesets_by_name.find(lbl);
				if (it != m_tilesets_by_name.end() && it->second->type == Type::ANIMATED_MAP)
				{
					// Animated tileset
					// Rename to match proper name
					it->second->ptrname = it->first;
					RenameTileset(it->first, name);
				}
				else
				{
					// Must be the intro font
					auto ts = std::make_shared<Tileset>(8, 16);
					auto ts_entry = std::make_shared<TilesetEntry>(name, ts, Type::FONT);
					ts_entry->ptrname = lbl.label;
					m_tilesets_by_name.insert({ name, ts_entry });
					m_tilesets_by_ptr.insert({ ts, ts_entry });
				}
				m_tilesets_listorder.push_back(m_tilesets_by_name[name]);
			}
			else
			{
				file >> name;
				if ((m_tilesets_by_name.find(name) == m_tilesets_by_name.end()) &&
				    (name != Hex(uint32_t(0))))
				{
					// This will be a pointer to a regular tileset
					auto ts = std::make_shared<Tileset>();
					auto ts_entry = std::make_shared<TilesetEntry>(name, ts, Type::MAP);
					m_tilesets_by_name.insert({ name, ts_entry });
					m_tilesets_by_ptr.insert({ ts, ts_entry });
					if (i < m_tileset_list_x.size())
					{
						m_tileset_list_x[i++] = ts_entry;
					}
				}
				else
				{
					// This is a dummy entry and can be ignored
					if (i < m_tileset_list_x.size())
					{
						m_tileset_list_x[i++] = nullptr;
					}
				}
			}
		}
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what();
	}
	return false;
}

bool TilesetManager::LoadAsmTilesetFilenames()
{
	try
	{
		AsmFile file(m_base_path / m_tileset_data_filename);
		AsmFile::IncludeFile inc;
		for (const auto& ts : m_tilesets_by_name)
		{
			file.Goto(ts.first);
			file >> inc;
			ts.second->filename = inc.path;
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

void TilesetManager::LoadAsmTilesetData()
{
	for (const auto& ts : m_tilesets_by_name)
	{
		auto filename = m_base_path / ts.second->filename;
		auto data = std::make_shared<std::vector<uint8_t>>(ReadBytes(filename));
		ts.second->raw_data = data;
		if (ts.second->type == Type::MAP)
		{
			auto dec = std::make_shared<std::vector<uint8_t>>(65536);
			std::size_t encsize;
			std::size_t decsize = LZ77::Decode(data->data(), data->size(), dec->data(), encsize);
			dec->resize(decsize);
			ts.second->decompressed_data = dec;
			ts.second->tileset->SetBits(*dec);
		}
		else
		{
			ts.second->tileset->SetBits(*data);
		}
	}
}

bool TilesetManager::LoadAsmBlocksetPointerData()
{
	try
	{
		m_blockset_pri_ptrs.clear();
		m_blockset_sec_ptrs.clear();

		AsmFile pri_file(m_base_path / m_blockset_pri_ptr_filename);
		AsmFile sec_file(m_base_path / m_blockset_sec_ptr_filename);

		pri_file.Goto(RomOffsets::Blocksets::POINTER);
		// First item is always a pointer to the start of the regular tileset list.
		// We can safely ignore this
		std::string name;
		AsmFile::Label lbl;
		pri_file >> lbl >> name;
		uint8_t p = 0, s = 0;
		while (pri_file.IsGood())
		{
			pri_file >> name;
			if (m_blocksets_by_ptr.find(name) == m_blocksets_by_ptr.end())
			{
				if (m_blockset_pri_ptrs.find(name) == m_blockset_pri_ptrs.end())
				{
					m_blockset_pri_ptrs.insert({ name, p });
				}
				else
				{
					m_blockset_pri_ptrs[name] = p;
				}
			}
			p++;
		}
		while (sec_file.IsGood())
		{
			if (sec_file.IsLabel())
			{
				sec_file >> lbl;
				p = m_blockset_pri_ptrs[lbl];
				s = 0;
			}
			sec_file >> name;
			if (m_blocksets_by_ptr.find(name) != m_blocksets_by_ptr.end())
			{
				m_blockset_sec_ptrs.insert({ name, {p, s++} });
			}
		}
		for (const auto& ptr : m_blockset_sec_ptrs)
		{
			if (m_blocksets_by_ptr.find(ptr.first) != m_blocksets_by_ptr.end())
			{
				auto e = m_blocksets_by_ptr[ptr.first];
				e->primary_idx = ptr.second.first >> 5;
				e->secondary_idx = ptr.second.second;
				e->tileset = ptr.second.first & 0x1F;
				m_blocksets.insert({ ptr.second, e });
			}
		}
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what();
	}
	return false;
}

bool TilesetManager::LoadAsmBlocksetData()
{
	try
	{
		m_blocksets_by_ptr.clear();
		m_blocksets.clear();
		AsmFile dfile(m_base_path / m_blockset_data_filename);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		while (dfile.IsGood())
		{
			dfile >> lbl >> inc;
			std::shared_ptr<BlocksetEntry> ep = std::make_shared<BlocksetEntry>();
			ep->filename = inc.path;
			ep->raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(m_base_path / ep->filename));
			ep->name = lbl;
			ep->primary_idx = 0;
			ep->secondary_idx = 0;
			ep->start_address = 0;
			ep->end_address = 0;
			ep->blockset_orig = std::make_shared<Blockset>();
			BlocksetCmp::Decode(&ep->raw_data->at(0), ep->raw_data->size(), *ep->blockset_orig);
			ep->blockset = std::make_shared<Blockset>(*ep->blockset_orig);
			m_blocksets_by_ptr.insert({ ep->name, ep });
		}
		return true;
	}
	catch (...)
	{
	}
	return false;
}

bool TilesetManager::LoadRomAnimatedTilesetData(const Rom& rom)
{
	std::size_t addr = rom.get_section(RomOffsets::Tilesets::ANIM_DATA_LOC).begin;
	std::map<int, std::set<std::string>> animts_list;
	std::vector<uint8_t> idx_list;
	// Read in animation index table
	for (uint8_t next = rom.inc_read<uint8_t>(addr); next != 0xFF; next = rom.inc_read<uint8_t>(addr))
	{
		idx_list.push_back(next);
	};
	addr += addr & 1; // Fix alignment
	// Next, read in the animation list
	uint16_t base, length;
	uint8_t speed, frames;
	uint32_t ts_addr;
	
	for (auto idx = idx_list.begin(); idx != idx_list.end(); idx++)
	{
		base = rom.inc_read<uint16_t>(addr);
		length = rom.inc_read<uint16_t>(addr);
		speed = rom.inc_read<uint8_t>(addr);
		frames = rom.inc_read<uint8_t>(addr);
		ts_addr = rom.inc_read<uint32_t>(addr);
		auto ts = std::make_shared<AnimatedTileset>(base, length, speed, frames);
		auto ts_entry = std::make_shared<TilesetEntry>(Hex(ts_addr), ts, Type::ANIMATED_MAP);
		ts_entry->start_address = ts_addr;
		if (animts_list.find(*idx) == animts_list.end())
		{
			animts_list.insert({ *idx, std::set<std::string>() });
		}
		animts_list[*idx].insert(ts_entry->name);
		std::size_t subts = animts_list[*idx].size();
		ts_entry->name = StrPrintf(RomOffsets::Tilesets::ANIM_LABEL_FORMAT_STRING, *idx + 1, subts);
		ts_entry->ptrname = StrPrintf(RomOffsets::Tilesets::ANIM_PTR_LABEL_FORMAT_STRING, *idx + 1, subts);
		ts_entry->filename = StrPrintf(RomOffsets::Tilesets::ANIM_FILENAME_FORMAT_STRING, *idx + 1, subts);
		ts->SetBaseTileset(*idx);
		m_animated_ts_ptrorder.push_back(ts_entry);
		m_tilesets_by_name.insert({ ts_entry->name, ts_entry });
		m_tilesets_by_ptr.insert({ ts, ts_entry });
	}
	return true;
}

bool TilesetManager::LoadRomTilesetPointerData(const Rom& rom)
{
	std::size_t addr = rom.get_section(RomOffsets::Tilesets::DATA_LOC).begin;
	const std::size_t ts_begin_ptr = rom.inc_read<uint32_t>(addr);
	const std::size_t introfont_ptr = rom.read<uint32_t>(rom.get_address(RomOffsets::Tilesets::INTRO_FONT_PTR));
	while(addr != ts_begin_ptr)
	{
		auto it = std::find_if(m_animated_ts_ptrorder.begin(),
		                       m_animated_ts_ptrorder.end(),
		                       [addr](const auto& ts_entry) { return ts_entry->start_address == addr; });
		
		if (addr == introfont_ptr)
		{
			// This is the intro font
			std::size_t ts_addr = rom.inc_read<uint32_t>(addr);
			auto ts = std::make_shared<Tileset>(8, 16);
			auto ts_entry = std::make_shared<TilesetEntry>(RomOffsets::Tilesets::INTRO_FONT_NAME, ts, Type::FONT);
			ts_entry->ptrname = RomOffsets::Tilesets::INTRO_FONT_PTR;
			ts_entry->filename = RomOffsets::Tilesets::INTRO_FONT_FILENAME;
			ts_entry->start_address = ts_addr;
			m_tilesets_by_name.insert({ ts_entry->name, ts_entry });
			m_tilesets_by_ptr.insert({ ts, ts_entry });
			m_tilesets_listorder.push_back(ts_entry);
		}
		else if (it != m_animated_ts_ptrorder.end())
		{
			// This is an animated tileset entry. Update with the correct pointer
			std::size_t ts_addr = rom.inc_read<uint32_t>(addr);
			(*it)->start_address = ts_addr;
			m_tilesets_listorder.push_back(*it);
		}
		else
		{
			throw std::runtime_error("Found unknown pointer at address " + Hex(addr));
		}
	}
	// Read the TS pointer list
	std::size_t ts_idx = 0;
	for (std::size_t i = 0; i < m_tileset_list_x.size(); ++i)
	{
		std::size_t ts_addr = rom.inc_read<uint32_t>(addr);
		auto it = std::find_if(m_animated_ts_ptrorder.begin(),
		                       m_animated_ts_ptrorder.end(),
		                       [ts_addr](const auto& ts_entry) { return ts_entry->start_address == ts_addr; });
		if (it == m_animated_ts_ptrorder.end() && ts_addr != 0)
		{
			ts_idx++;
			std::string name = StrPrintf(RomOffsets::Tilesets::LABEL_FORMAT_STRING, ts_idx);
			std::string filename = StrPrintf(RomOffsets::Tilesets::FILENAME_FORMAT_STRING, ts_idx);
			auto ts = std::make_shared<Tileset>();
			auto ts_entry = std::make_shared<TilesetEntry>(name, ts, Type::MAP);
			ts_entry->filename = filename;
			ts_entry->start_address = ts_addr;
			m_tilesets_by_name.insert({ name, ts_entry });
			m_tilesets_by_ptr.insert({ ts, ts_entry });
			m_tileset_list_x[i] = ts_entry;
		}
		else
		{
			m_tileset_list_x[i] = nullptr;
		}
	}
	return true;
}

void TilesetManager::LoadRomTilesetData(const Rom& rom)
{
	std::set<std::size_t> ptrlist;
	for (const auto& ts : m_tilesets_by_name)
	{
		ptrlist.insert(ts.second->start_address);
	}
	ptrlist.insert(rom.get_section(RomOffsets::Tilesets::DATA_LOC).end);

	for (auto it = ptrlist.begin(); it != std::prev(ptrlist.end()); ++it)
	{
		std::size_t beg = *it;
		std::size_t end = *(std::next(it));
		auto ts = std::find_if(m_tilesets_by_name.begin(),
		                       m_tilesets_by_name.end(),
		                       [beg](const auto& ts) { return ts.second->start_address == beg; });
		auto data = std::make_shared<std::vector<uint8_t>>(rom.read_array<uint8_t>(beg, end - beg));
		ts->second->raw_data = data;
		ts->second->end_address = end;
		if (ts->second->type == Type::MAP)
		{
			ts->second->tileset->SetBits(*data, true);
			ts->second->decompressed_data = std::make_shared<std::vector<uint8_t>>(ts->second->tileset->GetBits(false));
		}
		else
		{
			ts->second->tileset->SetBits(*data);
		}
	}

	// Unfortunately, there is no way to calculate the size of the intro font
	// just by looking at the data in the ROM. We have to guess: the data
	// immediately after the tileset is all 0xFF, so we can look for that and delete.
	auto introfont = m_tilesets_by_name.at(RomOffsets::Tilesets::INTRO_FONT_NAME);
	std::vector<uint8_t> bad_tile(introfont->tileset->GetTileWidth() * introfont->tileset->GetTileHeight());
	std::fill(bad_tile.begin(), bad_tile.end(), 0x0F);
	for (std::size_t i = 0; i < introfont->tileset->GetTileCount(); ++i)
	{
		auto tile = introfont->tileset->GetTile(i);
		if (tile == bad_tile)
		{
			while (introfont->tileset->GetTileCount() > i)
			{
				introfont->tileset->DeleteTile(i);
			}
			break;
		}
	}
	*introfont->raw_data = introfont->tileset->GetBits();
	introfont->end_address = introfont->start_address + introfont->raw_data->size();
}

void TilesetManager::LoadRomBlocksetData(const Rom& rom)
{
	uint32_t blockset_begin = rom.read<uint32_t>(RomOffsets::Blocksets::POINTER);
	std::map<uint32_t, uint8_t> pri_ptrs;
	std::map<uint32_t, std::pair<uint8_t, uint8_t>> sec_ptrs;
	std::set<uint32_t> b_ptrs;
	for (uint8_t i = 0; i < 64; ++i)
	{
		uint32_t ptr = rom.read<uint32_t>(blockset_begin + i * 4);
		pri_ptrs[ptr] = i;
	}
	auto p_it = pri_ptrs.cbegin();
	while (p_it != std::prev(pri_ptrs.cend()))
	{
		uint8_t pri = p_it->second;
		uint8_t sec = 0;
		uint32_t begin = p_it->first;
		++p_it;
		uint32_t end = p_it->first;
		for (uint32_t addr = begin; addr < end; addr += 4)
		{
			sec_ptrs[addr] = { pri, sec++ };
		}
	}
	for(auto s_it = sec_ptrs.cbegin(); s_it != sec_ptrs.cend(); ++s_it)
	{
		uint32_t begin = rom.read<uint32_t>(s_it->first);
		uint32_t end;
		if (std::next(s_it) != sec_ptrs.cend())
		{
			end = rom.read<uint32_t>(std::next(s_it)->first);
		}
		else
		{
			end = rom.get_section(RomOffsets::Blocksets::SECTION).end;
		}
		LoadRomBlockset(rom, s_it->second.first, s_it->second.second, begin, end);
	}
}

void TilesetManager::LoadRomBlockset(const Rom& rom, uint8_t pri, uint8_t sec, uint32_t begin, uint32_t end)
{
	auto data = rom.read_array<uint8_t>(begin, end - begin);
	BlocksetPtr blockset = std::make_shared<Blockset>();
	int len = BlocksetCmp::Decode(&data.at(0), data.size(), *blockset);
	data.resize(len);
	auto e = std::make_shared<BlocksetEntry>();
	e->start_address = begin;
	e->end_address = begin + len;
	e->blockset_orig = blockset;
	e->blockset = std::make_shared<Blockset>(*blockset);
	e->primary_idx = pri >> 5;
	e->secondary_idx = sec;
	e->tileset = pri & 0x1F;
	e->raw_data = std::make_shared<std::vector<uint8_t>>(data);
	e->name = StrPrintf(RomOffsets::Blocksets::BLOCKSET_LABEL, (pri & 0x1F) + 1, sec + 10 * (pri >> 5));
	e->filename = StrPrintf(RomOffsets::Blocksets::BLOCKSET_FILE, (pri & 0x1F) + 1, sec + 10 * (pri >> 5));
	m_blocksets_by_ptr.insert({ e->name, e });
	m_blocksets.insert({ {pri, sec}, e });
}

bool TilesetManager::HasTilesetBeenModified(const std::string& tileset) const
{
	auto it = m_tilesets_by_name.find(tileset);
	if (it == m_tilesets_by_name.end())
	{
		return false;
	}
	const std::vector<uint8_t>& buf = it->second->tileset->GetBits();
	if (it->second->decompressed_data == nullptr)
	{
		return (buf != *it->second->raw_data);
	}
	else
	{
		return (buf != *it->second->decompressed_data);
	}
}

bool TilesetManager::HasBeenModified() const
{
	for (const auto& ts : m_tilesets_by_name)
	{
		if (HasTilesetBeenModified(ts.first))
		{
			return true;
		}
	}
	return false;
}

bool TilesetManager::UpdateTilesetCache(const std::string& tileset)
{
	auto ts = m_tilesets_by_name[tileset];
	auto buf = ts->tileset->GetBits();
	if (ts->decompressed_data != nullptr)
	{
		if (buf != *ts->decompressed_data)
		{
			*ts->decompressed_data = buf;
			*ts->raw_data = ts->tileset->GetBits(true);
			return true;
		}
	}
	else
	{
		*ts->raw_data = buf;
		return true;
	}
	return false;
}

std::vector<uint8_t> TilesetManager::GetTilesetBits(const std::string& tileset) const
{
	auto ts = m_tilesets_by_name.find(tileset);
	if (!HasTilesetBeenModified(tileset))
	{
		return *ts->second->raw_data;
	}
	if (ts->second->decompressed_data != nullptr)
	{
		return ts->second->tileset->GetBits(true);
	}
	else
	{
		return ts->second->tileset->GetBits(false);
	}
}

void TilesetManager::SaveTilesetsToDisk(const filesystem::path& dir)
{
	for (const auto& ts : m_tilesets_by_name)
	{
		SaveTilesetToFile(dir, ts.first);
		UpdateTilesetCache(ts.first);
	}
}

void TilesetManager::SaveTilesetsToRom(Rom& rom)
{
	if (m_pending_write.empty())
	{
		int tileset_space, anim_table_space;
		if (!CheckDataWillFitInRom(rom, tileset_space, anim_table_space))
		{
			std::cerr << "Warning: Data will not fit in ROM without overwriting existing structures!\n"
				<< "  Animated tileset table has"
				<< ((anim_table_space < 0) ? " overrun by " : " spare space of ")
				<< std::abs(anim_table_space) << " bytes.\n"
				<< "  Tileset listing has"
				<< ((tileset_space < 0) ? " overrun by " : " spare space of ")
				<< std::abs(tileset_space) << " bytes." << std::endl;
		}
	}
	for (const auto& wr : m_pending_write)
	{
		rom.write_array(wr.second, wr.first->start_address);
		if ((wr.first->decompressed_data == nullptr) && (wr.first->end_address & 0x01))
		{
			rom.write<uint8_t>(0xFF, wr.first->end_address);
		}
		UpdateTilesetCache(wr.first->name);
	}
}

void TilesetManager::SaveTilesetToFile(const filesystem::path& dir, const std::string& name) const
{
	auto ts = m_tilesets_by_name.find(name);
	if (ts == m_tilesets_by_name.end())
	{
		throw std::runtime_error(std::string("Tileset with name \'") + name + "\' cannot be found.");
	}
	auto fname = dir / ts->second->filename;
	std::vector<uint8_t> buf = ts->second->tileset->GetBits();
	if (!fname.parent_path().is_directory() &&
		!filesystem::create_directories(fname.parent_path()))
	{
		throw std::runtime_error(std::string("Unable to create directory \"") + dir.parent_path().str() + "\"");
	}
	WriteBytes(GetTilesetBits(ts->first), fname);
}

bool TilesetManager::SaveAsmTilesetFilenames(const filesystem::path& dir)
{
	try
	{
		if (m_tileset_data_filename.empty())
		{
			m_tileset_data_filename = RomOffsets::Tilesets::INCLUDE_FILE;
		}
		AsmFile file;
		file.WriteFileHeader(m_tileset_data_filename, "Tileset data include file");
		std::set<decltype(m_tileset_list_x)::value_type> tilesets;
		for (const auto& ts : m_tileset_list_x)
		{
			if (ts == nullptr)
			{
				continue;
			}
			if (tilesets.count(ts) == 0)
			{
				file << AsmFile::Label(ts->name) << AsmFile::IncludeFile(ts->filename, AsmFile::BINARY);
				tilesets.insert(ts);
			}
		}
		file << AsmFile::Align(2);
		for (const auto& ts : m_tilesets_listorder)
		{
			file << AsmFile::Label(ts->name) << AsmFile::IncludeFile(ts->filename, AsmFile::BINARY);
		}
		auto f = dir / m_tileset_data_filename;
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

bool TilesetManager::SaveAsmAnimatedTilesetData(const filesystem::path& dir)
{
	try
	{
		if (m_tileset_anim_filename.empty())
		{
			m_tileset_anim_filename = RomOffsets::Tilesets::ANIM_FILE;
		}
		AsmFile file;
		file.WriteFileHeader(m_tileset_anim_filename, "Animated tileset definition file");

		std::vector<uint8_t> idxs;
		for (const auto& ts : m_animated_ts_ptrorder)
		{
			auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(ts->tileset);
			idxs.push_back(ats->GetBaseTileset());
		}
		idxs.push_back(-1);
		file << AsmFile::Label(RomOffsets::Tilesets::ANIM_IDX_LOC) << idxs;
		file << AsmFile::Align(2) << AsmFile::Label(RomOffsets::Tilesets::ANIM_LIST_LOC);

		for (const auto& ts : m_animated_ts_ptrorder)
		{
			auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(ts->tileset);
			file << ats->GetBaseBytes() << ats->GetFrameSizeBytes()
				<< ats->GetAnimationSpeed() << ats->GetAnimationFrames()
				<< ts->ptrname;
		}
		auto f = dir / m_tileset_anim_filename;
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

bool TilesetManager::SaveRomAnimatedTilesetData(Rom& rom)
{
	std::size_t addr = rom.get_section(RomOffsets::Tilesets::ANIM_DATA_LOC).begin;
	// First, write out base tileset list:
	for (const auto& ts : m_animated_ts_ptrorder)
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(ts->tileset);
		rom.inc_write<uint8_t>(ats->GetBaseTileset(), addr);
	}
	do
	{
		rom.inc_write<uint8_t>(0xFF, addr);
	} while (addr & 0x01);
	// Next, write out the animation list
	for (const auto& ts : m_animated_ts_ptrorder)
	{
		auto ats = std::static_pointer_cast<AnimatedTileset, Tileset>(ts->tileset);
		rom.inc_write<uint16_t>(ats->GetBaseBytes(), addr);
		rom.inc_write<uint16_t>(ats->GetFrameSizeBytes(), addr);
		rom.inc_write<uint8_t>(ats->GetAnimationSpeed(), addr);
		rom.inc_write<uint8_t>(ats->GetAnimationFrames(), addr);
		// Calculate pointer offset. Animation pointers are listed from one
		// position down in the tileset pointer list
		std::size_t ptraddr = rom.get_section(RomOffsets::Tilesets::DATA_LOC).begin + sizeof(uint32_t);
		auto it = std::find_if(m_tilesets_listorder.begin(),
			m_tilesets_listorder.end(),
			[&ts](const auto& lts) {return ts->name == lts->name; });
		ptraddr += (it - m_tilesets_listorder.begin()) * sizeof(uint32_t);
		rom.inc_write<uint32_t>(ptraddr, addr);
	}
	return true;
}

bool TilesetManager::SaveAsmTilesetPointerData(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		if (m_tileset_ptrtab_filename.empty())
		{
			m_tileset_ptrtab_filename = RomOffsets::Tilesets::PTRTAB_FILE;
		}
		file.WriteFileHeader(m_tileset_ptrtab_filename, "Tileset pointer table file");

		file << AsmFile::Label(RomOffsets::Tilesets::PTR_LOC) << RomOffsets::Tilesets::PTRTAB_BEGIN_LOC;
		for (const auto& ts : m_tilesets_listorder)
		{
			file << AsmFile::Label(ts->ptrname) << ts->name;
		}
		file << AsmFile::Label(RomOffsets::Tilesets::PTRTAB_BEGIN_LOC);
		for (const auto& ts : m_tileset_list_x)
		{
			if (ts == nullptr)
			{
				file << m_tilesets_listorder.front()->name;
			}
			else
			{
				file << ts->name;
			}
		}
		auto f = dir / m_tileset_ptrtab_filename;
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

bool TilesetManager::SaveRomTilesetPointerData(Rom& rom)
{
	const std::size_t tilesets_begin = rom.get_section(RomOffsets::Tilesets::DATA_LOC).begin;
	std::size_t addr = tilesets_begin;
	// First entry: pointer to start of TS table
	std::size_t ts_table_begin = tilesets_begin + (1 + m_tilesets_listorder.size()) * sizeof(uint32_t);
	rom.inc_write<uint32_t>(ts_table_begin, addr);
	// Next: list the pointers to the non-compressed tilesets
	for (const auto& ts : m_tilesets_listorder)
	{
		rom.inc_write<uint32_t>(ts->start_address, addr);
	}
	// Next, write out the tileset pointer table
	for (const auto& ts : m_tileset_list_x)
	{
		if (ts != nullptr)
		{
			rom.inc_write<uint32_t>(ts->start_address, addr);
		}
		else
		{
			rom.inc_write<uint32_t>(m_tilesets_listorder.front()->start_address, addr);
		}
	}
	// Finally, update the intro font pointer
	auto it = std::find_if(m_tilesets_listorder.begin(), m_tilesets_listorder.end(),
		[](const auto& ts) { return ts->type == Type::FONT; });
	if (it != m_tilesets_listorder.end())
	{
		std::size_t ptr = tilesets_begin + (1 + it - m_tilesets_listorder.begin()) * sizeof(uint32_t);
		rom.write<uint32_t>(ptr, rom.get_address(RomOffsets::Tilesets::INTRO_FONT_PTR));
	}
	return true;
}

bool TilesetManager::SaveAsmBlocksetPointerData(const filesystem::path& dir)
{
	try
	{
		if (m_blockset_pri_ptr_filename.empty())
		{
			m_blockset_pri_ptr_filename = RomOffsets::Blocksets::PRI_PTR_FILE;
		}
		if (m_blockset_sec_ptr_filename.empty())
		{
			m_blockset_sec_ptr_filename = RomOffsets::Blocksets::SEC_PTR_FILE;
		}
		AsmFile pri_file;
		AsmFile sec_file;
		pri_file.WriteFileHeader(m_blockset_pri_ptr_filename, "Blockset primary pointer file");
		sec_file.WriteFileHeader(m_blockset_sec_ptr_filename, "Blockset secondary pointer file");

		pri_file << AsmFile::Label(RomOffsets::Blocksets::POINTER) << RomOffsets::Blocksets::PRI_LABEL;
		pri_file << AsmFile::Label(RomOffsets::Blocksets::PRI_LABEL);

		int pri_ptr_count = 0;
		int pri = -1;
		for (const auto& bs : m_blocksets)
		{
			if (bs.first.first != pri)
			{
				pri = bs.first.first;
				auto name = StrPrintf(RomOffsets::Blocksets::SEC_LABEL, (pri & 0x1F) + 1);
				if (pri > 0x1F)
				{
					name += "A";
				}
				do 
				{
					pri_file << name;
				} while (++pri_ptr_count < (pri + 1));
				sec_file << AsmFile::Label(name);
			}
			sec_file << bs.second->name;
		}
		while (pri_ptr_count++ < 64)
		{
			pri_file << m_blocksets.begin()->second->name;
		}

		auto prifname = dir / m_blockset_pri_ptr_filename;
		auto secfname = dir / m_blockset_sec_ptr_filename;
		CreateDirectoryTree(prifname);
		CreateDirectoryTree(secfname);
		pri_file.WriteFile(prifname);
		sec_file.WriteFile(secfname);
		return true;
	}
	catch (...)
	{
	}
	return false;
}

bool TilesetManager::SaveAsmBlocksetData(const filesystem::path& dir)
{
	try
	{
		if (m_blockset_data_filename.empty())
		{
			m_blockset_data_filename = RomOffsets::Blocksets::DATA_FILE;
		}
		AsmFile file;
		file.WriteFileHeader(m_blockset_data_filename, "Blockset data include file");
		for (const auto& bs : m_blocksets)
		{
			if (*bs.second->blockset != *bs.second->blockset_orig)
			{
				bs.second->raw_data->resize(65536);
				uint16_t len = BlocksetCmp::Encode(*bs.second->blockset, &bs.second->raw_data->at(0), bs.second->raw_data->size());
				bs.second->raw_data->resize(len);
				*bs.second->blockset_orig = *bs.second->blockset;
			}
			if (bs.second->filename.empty())
			{
				bs.second->filename = StrPrintf(RomOffsets::Blocksets::BLOCKSET_FILE,
					bs.second->tileset, bs.second->secondary_idx + 10 * bs.second->primary_idx);
			}
			file << AsmFile::Label(bs.second->name) << AsmFile::IncludeFile(bs.second->filename, AsmFile::BINARY);
			auto fname = dir / bs.second->filename;
			CreateDirectoryTree(fname);
			WriteBytes(*bs.second->raw_data, fname);
		}
		auto incfname = dir / m_blockset_data_filename;
		CreateDirectoryTree(incfname);
		file.WriteFile(incfname);
		return true;
	}
	catch (...)
	{
	}
	return false;
}
