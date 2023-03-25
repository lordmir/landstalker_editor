#include "SpriteData.h"

#include <set>
#include <numeric>

SpriteData::SpriteData(const filesystem::path& asm_file)
	: DataManager(asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadSpriteFrames())
	{
		throw std::runtime_error(std::string("Unable to load sprite frame data from \'") + m_sprite_frames_data_file.str() + '\'');
	}
	if (!AsmLoadSpritePointers())
	{
		throw std::runtime_error(std::string("Unable to load sprite pointer data from \'") + m_sprite_anim_frames_file.str() + '\'');
	}
	InitCache();
}

SpriteData::SpriteData(const Rom& rom)
	: DataManager(rom)
{
	SetDefaultFilenames();
	if (!RomLoadSpriteFrames(rom))
	{
		throw std::runtime_error("Unable to load sprite frame data from ROM");
	}

	InitCache();
}

SpriteData::~SpriteData()
{
}

bool SpriteData::Save(const filesystem::path& dir)
{
	auto directory = dir;
	if (directory.exists() && directory.is_file())
	{
		directory = directory.parent_path();
	}
	if (!CreateDirectoryStructure(directory))
	{
		throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.str() + '\'');
	}
	if (!AsmSaveSpriteFrames(directory))
	{
		throw std::runtime_error(std::string("Unable to save sprite frame data to \'") + directory.str() + '\'');
	}
	if (!AsmSaveSpritePointers(directory))
	{
		throw std::runtime_error(std::string("Unable to save sprite frame data to \'") + m_sprite_frames_data_file.str() + '\'');
	}
	CommitAllChanges();
	return true;
}

bool SpriteData::Save()
{
	return Save(GetBasePath());
}

bool SpriteData::HasBeenModified() const
{
	auto entry_pred = [](const auto& e) {return e != nullptr && e->HasDataChanged(); };
	auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasDataChanged(); };
	if (std::any_of(m_frames.begin(), m_frames.end(), pair_pred))
	{
		return true;
	}
	if (m_sprite_mystery_data_orig != m_sprite_mystery_data)
	{
		return true;
	}
	if (m_animations_orig != m_animations)
	{
		return true;
	}
	if (m_animation_frames_orig != m_animation_frames)
	{
		return true;
	}
	if (m_frames_orig != m_frames)
	{
		return true;
	}
	return false;
}

void SpriteData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	if (!RomPrepareInjectSpriteFrames(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare sprite frame data for ROM injection"));
	}
}

void SpriteData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	std::for_each(m_frames.begin(), m_frames.end(), pair_commit);
	m_frames_orig = m_frames;
	m_animations_orig = m_animations;
	m_animation_frames_orig = m_animation_frames;
	m_sprite_mystery_data_orig = m_sprite_mystery_data;
	m_pending_writes.clear();
}

bool SpriteData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().str());
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP, m_unknown_sprite_lookup_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_VISIBILITY_FLAGS, m_sprite_visibility_flags_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::ONE_TIME_EVENT_FLAGS, m_one_time_event_flags_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::ROOM_CLEAR_FLAGS, m_room_clear_flags_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::LOCKED_DOOR_SPRITE_FLAGS, m_locked_door_sprite_flags_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::PERMANENT_SWITCH_FLAGS, m_permanent_switch_flags_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SACRED_TREE_FLAGS, m_sacred_tree_flags_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_GFX_IDX_LOOKUP, m_sprite_gfx_idx_lookup_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_DIMENSIONS_LOOKUP, m_sprite_dimensions_lookup_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::ROOM_SPRITE_TABLE_OFFSETS, m_room_sprite_table_offsets_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::ENEMY_STATS, m_enemy_stats_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::ROOM_SPRITE_TABLE, m_room_sprite_table_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::ITEM_PROPERTIES, m_item_properties_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_BEHAVIOUR_OFFSETS, m_sprite_behaviour_offset_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_BEHAVIOUR_TABLE, m_sprite_behaviour_table_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_FRAMES_DATA, m_sprite_frames_data_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_LUT, m_sprite_lut_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_ANIM_PTR_DATA, m_sprite_anims_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::SPRITE_FRAME_PTR_DATA, m_sprite_anim_frames_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::PALETTE_DATA, m_palette_data_file);
		AsmFile p(GetBasePath() / m_palette_data_file);
		retval = retval && GetFilenameFromAsm(p, RomOffsets::Sprites::PALETTE_LUT, m_palette_lut_file);
		return retval;
	}
	catch (std::exception&)
	{
		throw;
	}
	return false;
}

void SpriteData::SetDefaultFilenames()
{
	if (m_unknown_sprite_lookup_file.empty())        m_unknown_sprite_lookup_file = RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP_FILE;
	if (m_sprite_visibility_flags_file.empty())      m_sprite_visibility_flags_file = RomOffsets::Sprites::SPRITE_VISIBILITY_FLAGS_FILE;
	if (m_one_time_event_flags_file.empty())         m_one_time_event_flags_file = RomOffsets::Sprites::ONE_TIME_EVENT_FLAGS_FILE;
	if (m_room_clear_flags_file.empty())             m_room_clear_flags_file = RomOffsets::Sprites::ROOM_CLEAR_FLAGS_FILE;
	if (m_locked_door_sprite_flags_file.empty())     m_locked_door_sprite_flags_file = RomOffsets::Sprites::LOCKED_DOOR_SPRITE_FLAGS_FILE;
	if (m_permanent_switch_flags_file.empty())       m_permanent_switch_flags_file = RomOffsets::Sprites::PERMANENT_SWITCH_FLAGS_FILE;
	if (m_sacred_tree_flags_file.empty())            m_sacred_tree_flags_file = RomOffsets::Sprites::SACRED_TREE_FLAGS_FILE;
	if (m_sprite_gfx_idx_lookup_file.empty())        m_sprite_gfx_idx_lookup_file = RomOffsets::Sprites::SPRITE_GFX_IDX_LOOKUP_FILE;
	if (m_sprite_dimensions_lookup_file.empty())     m_sprite_dimensions_lookup_file = RomOffsets::Sprites::SPRITE_DIMENSIONS_LOOKUP_FILE;
	if (m_room_sprite_table_offsets_file.empty())    m_room_sprite_table_offsets_file = RomOffsets::Sprites::ROOM_SPRITE_TABLE_OFFSETS_FILE;
	if (m_enemy_stats_file.empty())                  m_enemy_stats_file = RomOffsets::Sprites::ENEMY_STATS_FILE;
	if (m_room_sprite_table_file.empty())            m_room_sprite_table_file = RomOffsets::Sprites::ROOM_SPRITE_TABLE_FILE;
	if (m_item_properties_file.empty())              m_item_properties_file = RomOffsets::Sprites::ITEM_PROPERTIES_FILE;
	if (m_sprite_behaviour_offset_file.empty())      m_sprite_behaviour_offset_file = RomOffsets::Sprites::SPRITE_BEHAVIOUR_OFFSET_FILE;
	if (m_sprite_behaviour_table_file.empty())       m_sprite_behaviour_table_file = RomOffsets::Sprites::SPRITE_BEHAVIOUR_TABLE_FILE;
	if (m_palette_data_file.empty())                 m_palette_data_file = RomOffsets::Sprites::PALETTE_DATA_FILE;
	if (m_palette_lut_file.empty())                  m_palette_lut_file = RomOffsets::Sprites::PALETTE_LUT_FILE;
	if (m_sprite_lut_file.empty())                   m_sprite_lut_file = RomOffsets::Sprites::SPRITE_LUT_FILE;
	if (m_sprite_anims_file.empty())                 m_sprite_anims_file = RomOffsets::Sprites::SPRITE_ANIMS_FILE;
	if (m_sprite_anim_frames_file.empty())           m_sprite_anim_frames_file = RomOffsets::Sprites::SPRITE_ANIM_FRAMES_FILE;
	if (m_sprite_frames_data_file.empty())           m_sprite_frames_data_file = RomOffsets::Sprites::SPRITE_FRAME_DATA_FILE;
}

bool SpriteData::CreateDirectoryStructure(const filesystem::path& dir)
{
	bool retval = true;
	retval = retval && CreateDirectoryTree(dir / m_unknown_sprite_lookup_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_visibility_flags_file);
	retval = retval && CreateDirectoryTree(dir / m_one_time_event_flags_file);
	retval = retval && CreateDirectoryTree(dir / m_room_clear_flags_file);
	retval = retval && CreateDirectoryTree(dir / m_locked_door_sprite_flags_file);
	retval = retval && CreateDirectoryTree(dir / m_permanent_switch_flags_file);
	retval = retval && CreateDirectoryTree(dir / m_sacred_tree_flags_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_gfx_idx_lookup_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_dimensions_lookup_file);
	retval = retval && CreateDirectoryTree(dir / m_room_sprite_table_offsets_file);
	retval = retval && CreateDirectoryTree(dir / m_enemy_stats_file);
	retval = retval && CreateDirectoryTree(dir / m_room_sprite_table_file);
	retval = retval && CreateDirectoryTree(dir / m_item_properties_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_behaviour_offset_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_behaviour_table_file);
	retval = retval && CreateDirectoryTree(dir / m_palette_data_file);
	retval = retval && CreateDirectoryTree(dir / m_palette_lut_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_lut_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_anims_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_anim_frames_file);
	retval = retval && CreateDirectoryTree(dir / m_sprite_frames_data_file);
	for (const auto& m : m_frames)
	{
		retval = retval && CreateDirectoryTree(dir / m.second->GetFilename());
	}
	return retval;
}

void SpriteData::InitCache()
{
	m_animations_orig = m_animations;
	m_animation_frames_orig = m_animation_frames;
	m_frames_orig = m_frames;
	m_sprite_mystery_data_orig = m_sprite_mystery_data;
}

bool SpriteData::AsmLoadSpriteFrames()
{
	try
	{
		AsmFile file(GetBasePath() / m_sprite_frames_data_file);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			auto bytes = ReadBytes(GetBasePath() / inc.path);
			auto e = SpriteFrameEntry::Create(this, bytes, lbl, inc.path);
			m_frames.insert({ e->GetName(), e });
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool SpriteData::AsmLoadSpritePointers()
{
	std::vector<std::pair<uint16_t, uint16_t>> lut;
	try
	{
		auto lut_bytes = ReadBytes(GetBasePath() / m_sprite_lut_file);
		assert(lut_bytes.size() % 4 == 0);
		for (int i = 0; i < lut_bytes.size(); i += 4)
		{
			lut.push_back({ (lut_bytes[i] << 8) | lut_bytes[i + 1], (lut_bytes[i + 2] << 8) | lut_bytes[i + 3] });
		}

		AsmFile anim_file(GetBasePath() / m_sprite_anims_file);
		std::string ptrname;
		for (int spr = 0; spr < lut.size(); ++spr)
		{
			m_sprite_mystery_data[spr] = lut[spr].second;
			m_animations.insert({ spr, std::vector<std::string>() });
			int anim_end = 0xFFFF;
			if (spr < (lut.size() - 1))
			{
				anim_end = lut[spr + 1].first - lut[spr].first;
			}
			for (int anim = 0; anim < anim_end; ++anim)
			{
				if (!anim_file.IsGood())
				{
					break;
				}
				anim_file >> ptrname;
				m_animations[spr].push_back(ptrname);
			}
		}

		AsmFile frame_file(GetBasePath() / m_sprite_anim_frames_file);
		for (const auto& spr : m_animations)
		{
			for (const auto& animation : spr.second)
			{
				m_animation_frames.insert({ animation, std::vector<std::string>() });
				frame_file.Goto(animation);
				do
				{
					frame_file >> ptrname;
					assert(m_frames.find(ptrname) != m_frames.cend());
					m_animation_frames[animation].push_back(ptrname);
					m_frames[ptrname]->SetSprite(spr.first);
				} while (frame_file.IsGood() && !frame_file.IsLabel());
			}
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool SpriteData::RomLoadSpriteFrames(const Rom& rom)
{
	const uint32_t sprites_begin = rom.get_address(RomOffsets::Sprites::POINTER);
	const uint32_t sprites_end = rom.get_section(RomOffsets::Sprites::SPRITE_SECTION).end;
	const uint32_t anim_ptrs_begin = rom.read<uint32_t>(sprites_begin);

	// Read offset table
	const uint32_t lookup_table_begin = sprites_begin + sizeof(uint32_t);
	const uint32_t lookup_table_size = (anim_ptrs_begin - lookup_table_begin) / sizeof(uint16_t);
	auto offset_table = rom.read_array<uint16_t>(lookup_table_begin, lookup_table_size);

	// Read anim pointers
	uint32_t min_address = 0xFFFFFF;
	std::vector<uint32_t> anim_idxs;
	uint32_t addr = anim_ptrs_begin;
	uint32_t ptr;
	while (addr < min_address)
	{
		ptr = rom.inc_read<uint32_t>(addr);
		min_address = std::min(min_address, ptr);
		anim_idxs.push_back(ptr);
	}
	const uint32_t frame_ptr_begin = min_address;
	std::for_each(anim_idxs.begin(), anim_idxs.end(), [frame_ptr_begin](uint32_t& val) {
		val = (val - frame_ptr_begin) / sizeof(uint32_t);
	});

	// Read frame pointers
	addr = min_address;
	min_address = 0xFFFFFF;
	std::vector<uint32_t> anim_frame_ptrs;
	while(addr < min_address)
	{
		ptr = rom.inc_read<uint32_t>(addr);
		min_address = std::min(min_address, ptr);
		anim_frame_ptrs.push_back(ptr);
	}

	// Parse anim and frame pointer list, segregate by sprite/animation as appropriate
	assert((offset_table.size() & 1) == 0);
	auto anim_it = anim_idxs.cbegin();
	std::map<uint32_t, std::string> frames;
	std::map<uint32_t, std::string> frame_filenames;
	std::map<uint32_t, uint8_t> frame_sprite;
	int total_anim_count = 0;
	for (int i = 0; i < offset_table.size() / 2; ++i)
	{
		int sprite_frame_count = 0;
		m_sprite_mystery_data.insert({ i, offset_table[i * 2 + 1] });
		uint16_t anim_count;
		if ((i * 2 + 2) < offset_table.size())
		{
			anim_count = offset_table[i * 2 + 2] - offset_table[i * 2];
		}
		else
		{
			anim_count = anim_idxs.size() - offset_table[i * 2];
		}
		m_animations.insert({ i, std::vector<std::string>() });
		m_animations[i].reserve(anim_count);
		for (int j = 0; j < anim_count; ++j)
		{
			std::string anim_name = StrPrintf(RomOffsets::Sprites::SPRITE_ANIM, i, j);
			m_animations[i].push_back(anim_name);
			uint16_t frame_count;
			if ((total_anim_count + 1) < anim_idxs.size())
			{
				frame_count = anim_idxs[total_anim_count + 1] - anim_idxs[total_anim_count];
			}
			else
			{
				frame_count = anim_frame_ptrs.size() - anim_idxs[total_anim_count];
			}
			m_animation_frames.insert({ anim_name, std::vector<std::string>() });
			m_animation_frames[anim_name].reserve(frame_count);
			for (int k = 0; k < frame_count; ++k)
			{
				std::string frame_name;
				uint32_t frame_ptr = anim_frame_ptrs[anim_idxs[total_anim_count] + k];
				auto res = frames.find(frame_ptr);
				if (res != frames.end())
				{
					frame_name = res->second;
				}
				else
				{
					frame_name = StrPrintf(RomOffsets::Sprites::SPRITE_FRAME, i, sprite_frame_count);
					frames.insert({ frame_ptr, frame_name });
					frame_filenames.insert({ frame_ptr, StrPrintf(RomOffsets::Sprites::SPRITE_FRAME_FILE, i, sprite_frame_count++) });
					frame_sprite.insert({ frame_ptr, i });
				}
				m_animation_frames[anim_name].push_back(frame_name);
			}
			total_anim_count++;
		}
	}

	// Get begin, end addresses for sprite frame data
	auto it = frames.cbegin();
	std::vector<std::pair<uint32_t, uint32_t>> frame_addrs;
	for (int i = 0; i < (frames.size() - 1); ++i)
	{
		frame_addrs.push_back({ it->first, (++it)->first});
	}
	frame_addrs.push_back({ it->first, sprites_end });

	// Read each frame and store
	for (const auto& addr : frame_addrs)
	{
		auto bytes = rom.read_array<uint8_t>(addr.first, addr.second - addr.first);
		auto e = SpriteFrameEntry::Create(this, bytes, frames[addr.first], frame_filenames[addr.first]);
		e->SetStartAddress(addr.first);
		e->SetSprite(frame_sprite[addr.first]);
		m_frames.insert({ e->GetName(), e });
	}

	return true;
}

bool SpriteData::AsmSaveSpriteFrames(const filesystem::path& dir)
{
	return std::all_of(m_frames.begin(), m_frames.end(), [&](auto& f) { return f.second->Save(dir); });
}

bool SpriteData::AsmSaveSpritePointers(const filesystem::path& dir)
{
	try
	{
		AsmFile frm_file, anim_file, spr_file;

		frm_file.WriteFileHeader(m_sprite_frames_data_file, "Sprite Frame Data");
		for (const auto& frame : m_frames)
		{
			frm_file << AsmFile::Label(frame.first) << AsmFile::IncludeFile(frame.second->GetFilename(), AsmFile::BINARY);
		}
		frm_file.WriteFile(dir / m_sprite_frames_data_file);
		anim_file.WriteFileHeader(m_sprite_anim_frames_file, "Sprite Frame Pointers");
		for (const auto& anim : m_animation_frames)
		{
			anim_file << AsmFile::Label(anim.first);
			for (const auto& frame : anim.second)
			{
				anim_file << frame;
			}
		}
		anim_file.WriteFile(dir / m_sprite_anim_frames_file);
		spr_file.WriteFileHeader(m_sprite_anims_file, "Sprite Animation Pointers");
		spr_file << AsmFile::Label(RomOffsets::Sprites::SPRITE_SECTION);
		for (const auto& spr : m_animations)
		{
			spr_file << AsmFile::Label(StrPrintf(RomOffsets::Sprites::SPRITE_GFX, spr.first));
			for (const auto& anim : spr.second)
			{
				spr_file << anim;
			}
		}
		spr_file.WriteFile(dir / m_sprite_anims_file);
		ByteVector lut;
		lut.reserve(m_animations.size() * 2 * sizeof(uint16_t));
		uint16_t anim_count = 0;
		for (const auto& spr : m_animations)
		{
			lut.push_back((anim_count >> 8) & 0xFF);
			lut.push_back(anim_count & 0xFF);
			lut.push_back((m_sprite_mystery_data[spr.first] >> 8) & 0xFF);
			lut.push_back(m_sprite_mystery_data[spr.first] & 0xFF);
			anim_count += spr.second.size();
		}
		WriteBytes(lut, dir / m_sprite_lut_file);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool SpriteData::RomPrepareInjectSpriteFrames(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Sprites::SPRITE_SECTION).begin;
	uint32_t lut_size = m_animations.size() * 2 * sizeof(uint16_t);
	uint32_t anim_ptr_table_size = m_animation_frames.size() * sizeof(uint32_t);
	uint32_t frame_ptr_table_size = std::accumulate(m_animation_frames.cbegin(), m_animation_frames.cend(), 0,
		[](int sum, const auto& elem) {
			return sum + elem.second.size();
		}) * sizeof(uint32_t);

	// Reserve enough space for pointers, etc.
	const uint32_t anim_ptrs_begin = begin + sizeof(uint32_t) + lut_size;
	const uint32_t frame_ptrs_begin = anim_ptrs_begin + anim_ptr_table_size;
	const uint32_t frames_begin = frame_ptrs_begin + frame_ptr_table_size;
	ByteVectorPtr bytes(std::make_shared<ByteVector>(frames_begin - begin));
	std::unordered_map<std::string, uint32_t> frame_ptrs;

	for (const auto& f : m_frames)
	{
		auto b = f.second->GetBytes();
		frame_ptrs.insert({ f.first, begin + bytes->size() });
		bytes->insert(bytes->end(), b->begin(), b->end());
		if (((bytes->size() + begin) & 1) == 1)
		{
			bytes->push_back(0xFF);
		}
	}

	auto it = Insert<uint32_t>(bytes->begin(), anim_ptrs_begin);
	uint16_t anim_count = 0;
	for (const auto& spr : m_animations)
	{
		it = Insert<uint16_t>(it, anim_count);
		it = Insert<uint16_t>(it, m_sprite_mystery_data[spr.first]);
		anim_count += spr.second.size();
	}
	uint32_t frame_count = 0;
	for (const auto& anim : m_animation_frames)
	{
		it = Insert<uint32_t>(it, frame_count * sizeof(uint32_t) + frame_ptrs_begin);
		frame_count += anim.second.size();
	}
	for (const auto& anim : m_animation_frames)
	{
		for (const auto& frame : anim.second)
		{
			it = Insert<uint32_t>(it, frame_ptrs[frame]);
		}
	}

	m_pending_writes.push_back({ RomOffsets::Sprites::SPRITE_SECTION, bytes });

	return true;
}
