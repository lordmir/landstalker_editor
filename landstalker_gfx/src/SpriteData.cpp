#include "SpriteData.h"

#include <set>
#include <numeric>
#include <stack>
#include "AsmUtils.h"


template <std::size_t N>
std::vector<std::array<uint8_t, N>> DeserialiseFixedWidth(const std::vector<uint8_t>& bytes)
{
	std::vector<std::array<uint8_t, N>> result;
	int i = 0, j = 0;
	result.push_back(std::array<uint8_t, N>());
	while (i < bytes.size())
	{
		if (j >= N)
		{
			if (bytes[i] == 0xFF)
			{
				break;
			}
			j = 0;
			result.push_back(std::array<uint8_t, N>());
		}
		result.back()[j++] = bytes[i++];
	}
	if (j != N)
	{
		result.pop_back();
	}
	return result;
}

template <std::size_t N>
std::vector<uint8_t> SerialiseFixedWidth(const std::vector<std::array<uint8_t, N>>& data, bool terminate_data = true)
{
	std::vector<uint8_t> result;
	result.reserve(data.size() * N);

	for (const auto& d : data)
	{
		for (const auto& e : d)
		{
			result.push_back(e);
		}
	}

	if (terminate_data)
	{
		result.push_back(0xFF);
		if ((result.size() & 1) == 1)
		{
			result.push_back(0xFF);
		}
	}
	return result;
}

template <std::size_t N>
std::map<uint8_t, std::array<uint8_t, N>> DeserialiseMap(const std::vector<uint8_t>& bytes)
{
	std::map<uint8_t, std::array<uint8_t, N>> result;
	int i = 0, j = 0;
	uint8_t key = bytes[i++];
	std::array<uint8_t, N> buf;
	while (i < bytes.size())
	{
		if (j >= N)
		{
			j = 0;
			result.insert({key, buf});
			key = bytes[i++];
			if (key == 0xFF)
			{
				break;
			}
		}
		buf[j++] = bytes[i++];
	}
	return result;
}

template <std::size_t N>
std::vector<uint8_t> SerialiseMap(const std::map<uint8_t, std::array<uint8_t, N>>& data, bool terminate_data = true)
{
	std::vector<uint8_t> result;
	result.reserve(data.size() * (N + sizeof(uint8_t)));

	for (const auto& d : data)
	{
		result.push_back(d.first);
		for (const auto& e : d.second)
		{
			result.push_back(e);
		}
	}

	if (terminate_data)
	{
		result.push_back(0xFF);
		if ((result.size() & 1) == 1)
		{
			result.push_back(0xFF);
		}
	}
	return result;
}

std::map<uint8_t, uint8_t> DeserialiseMap(const std::vector<uint8_t>& bytes, bool reverse_key_value = false)
{
	std::map<uint8_t, uint8_t> result;
	int i = 0, j = 0;
	while (i < bytes.size())
	{
		uint8_t key = bytes[i++];
		uint8_t val = bytes[i++];
		if (reverse_key_value)
		{
			std::swap(key, val);
		}
		if (val == 0xFF || key == 0xFF)
		{
			break;
		}
		result.insert({ key, val });
	}
	return result;
}

std::vector<uint8_t> SerialiseMap(const std::map<uint8_t, uint8_t>& data, bool reverse_key_value = false, bool terminate_data = true)
{
	std::vector<uint8_t> result;
	result.reserve(data.size() * 2);

	for (const auto& d : data)
	{
		if (reverse_key_value)
		{
			result.push_back(d.second);
			result.push_back(d.first);
		}
		else
		{
			result.push_back(d.first);
			result.push_back(d.second);
		}
	}

	if (terminate_data)
	{
		result.push_back(0xFF);
		result.push_back(0xFF);
	}
	return result;
}

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
	if (!AsmLoadSpritePalettes())
	{
		throw std::runtime_error(std::string("Unable to load sprite palette data from \'") + m_palette_data_file.str() + '\'');
	}
	if (!AsmLoadSpriteData())
	{
		throw std::runtime_error(std::string("Unable to load sprite data from \'") + asm_file.str() + '\'');
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
	if (!RomLoadSpritePalettes(rom))
	{
		throw std::runtime_error("Unable to load sprite palette data from ROM");
	}
	if (!RomLoadSpriteData(rom))
	{
		throw std::runtime_error("Unable to load sprite data from ROM");
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
	if (!AsmSaveSpritePalettes(directory))
	{
		throw std::runtime_error(std::string("Unable to save sprite palette data to \'") + m_palette_data_file.str() + '\'');
	}
	if (!AsmSaveSpriteData(directory))
	{
		throw std::runtime_error(std::string("Unable to save sprite data to \'") + directory.str() + '\'');
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
	if (std::any_of(m_palettes_by_name.begin(), m_palettes_by_name.end(), pair_pred))
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
	if (m_lo_palettes_orig != m_lo_palettes)
	{
		return true;
	}
	if (m_hi_palettes_orig != m_hi_palettes)
	{
		return true;
	}
	if (m_projectile1_palettes_orig != m_projectile1_palettes)
	{
		return true;
	}
	if (m_projectile2_palettes_orig != m_projectile2_palettes)
	{
		return true;
	}
	if (m_lo_palette_lookup_orig != m_lo_palette_lookup)
	{
		return true;
	}
	if (m_hi_palette_lookup_orig != m_hi_palette_lookup)
	{
		return true;
	}
	if (m_sprite_visibility_flags_orig != m_sprite_visibility_flags)
	{
		return true;
	}
	if (m_one_time_event_flags_orig != m_one_time_event_flags)
	{
		return true;
	}
	if (m_room_clear_flags_orig != m_room_clear_flags)
	{
		return true;
	}
	if (m_locked_door_flags_orig != m_locked_door_flags)
	{
		return true;
	}
	if (m_permanent_switch_flags_orig != m_permanent_switch_flags)
	{
		return true;
	}
	if (m_sacred_tree_flags_orig != m_sacred_tree_flags)
	{
		return true;
	}
	if (m_sprite_dimensions_orig != m_sprite_dimensions)
	{
		return true;
	}
	if (m_enemy_stats_orig != m_enemy_stats)
	{
		return true;
	}
	if (m_sprite_to_entity_lookup_orig != m_sprite_to_entity_lookup)
	{
		return true;
	}
	if (m_room_entities_orig != m_room_entities)
	{
		return true;
	}
	if (m_item_properties_orig != m_item_properties)
	{
		return true;
	}
	if (m_sprite_behaviours_orig != m_sprite_behaviours)
	{
		return true;
	}
	if (m_sprite_behaviour_offsets_orig != m_sprite_behaviour_offsets)
	{
		return true;
	}
	if (m_unknown_entity_lookup_orig != m_unknown_entity_lookup)
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
	if (!RomPrepareInjectSpritePalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare sprite palette data for ROM injection"));
	}
	if (!RomPrepareInjectSpriteData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare sprite data for ROM injection"));
	}
}

uint8_t SpriteData::GetSpriteFromEntity(uint8_t id) const
{
	assert(m_sprite_to_entity_lookup.find(id) != m_sprite_to_entity_lookup.cend());
	return m_sprite_to_entity_lookup.find(id)->second;
}

bool SpriteData::EntityHasSprite(uint8_t id) const
{
	return (m_sprite_to_entity_lookup.find(id) != m_sprite_to_entity_lookup.cend());
}

std::vector<uint8_t> SpriteData::GetEntitiesFromSprite(uint8_t id) const
{
	std::vector<uint8_t> results;
	for (const auto& lookup : m_sprite_to_entity_lookup)
	{
		if (lookup.second == id)
		{
			results.push_back(lookup.first);
		}
	}
	return results;
}

std::pair<uint8_t, uint8_t> SpriteData::GetSpriteHitbox(uint8_t id) const
{
	assert(m_sprite_dimensions.find(id) != m_sprite_dimensions.cend());
	const auto& result = m_sprite_dimensions.find(id)->second;
	return { result[0], result[1] };
}

std::pair<uint8_t, uint8_t> SpriteData::GetEntityHitbox(uint8_t id) const
{
	if (!EntityHasSprite(id))
	{
		return { 8, 10 };
	}
	uint8_t sprite_id = GetSpriteFromEntity(id);
	return GetSpriteHitbox(sprite_id);
}

void SpriteData::SetSpriteHitbox(uint8_t id, uint8_t height, uint8_t base)
{
	assert(m_sprite_dimensions.find(id) != m_sprite_dimensions.cend());
	auto& result = m_sprite_dimensions.find(id)->second;
	result[0] = height;
	result[1] = base;
}

bool SpriteData::IsEntity(uint8_t id) const
{
	return (m_sprite_to_entity_lookup.find(id) != m_sprite_to_entity_lookup.cend());
}

bool SpriteData::IsSprite(uint8_t id) const
{
	return (m_names.find(id) != m_names.cend());;
}

bool SpriteData::IsItem(uint8_t sprite_id) const
{
	auto entities = GetEntitiesFromSprite(sprite_id);
	return std::all_of(entities.cbegin(), entities.cend(), [](uint8_t v) { return v >= 0xC0; });
}

bool SpriteData::HasFrontAndBack(uint8_t entity_id) const
{
	auto sprite_id = GetSpriteFromEntity(entity_id);
	return !IsItem(sprite_id) && GetSpriteAnimationCount(sprite_id) > 1;
}

std::string SpriteData::GetSpriteName(uint8_t id) const
{
	assert(m_names.find(id) != m_names.cend());
	return m_names.find(id)->second;
}

uint8_t SpriteData::GetSpriteId(const std::string& name) const
{
	assert(m_ids.find(name) != m_ids.cend());
	return m_ids.find(name)->second;
}

uint32_t SpriteData::GetSpriteAnimationCount(uint8_t id) const
{
	assert(m_animations.find(id) != m_animations.cend());
	return m_animations.find(id)->second.size();
}

std::vector<std::string> SpriteData::GetSpriteAnimations(uint8_t id) const
{
	assert(m_animations.find(id) != m_animations.cend());
	return m_animations.find(id)->second;
}

std::vector<std::string> SpriteData::GetSpriteAnimations(const std::string& name) const
{
	auto id = GetSpriteId(name);
	assert(m_animations.find(id) != m_animations.cend());
	return m_animations.find(id)->second;
}

uint32_t SpriteData::GetSpriteFrameCount(uint8_t id) const
{
	assert(m_sprite_frames.find(id) != m_sprite_frames.cend());
	return m_sprite_frames.find(id)->second.size();
}

std::vector<std::string> SpriteData::GetSpriteFrames(uint8_t id) const
{
	assert(m_sprite_frames.find(id) != m_sprite_frames.cend());
	const auto& result = m_sprite_frames.find(id)->second;
	return std::vector<std::string> (result.cbegin(), result.cend());
}

std::vector<std::string> SpriteData::GetSpriteFrames(const std::string& name) const
{
	return GetSpriteFrames(GetSpriteId(name));
}

std::shared_ptr<SpriteFrameEntry> SpriteData::GetDefaultEntityFrame(uint8_t id) const
{
	uint8_t spr_id = GetSpriteFromEntity(id);
	if (id >= 0xC0)
	{
		// Item
		return GetSpriteFrame(spr_id, (id >> 3) & 7, id & 0x07);
	}
	else
	{
		if (GetSpriteAnimationCount(spr_id) > 1)
		{
			return GetSpriteFrame(spr_id, 1, 0);
		}
		else
		{
			return GetSpriteFrame(spr_id, 0, 0);
		}
	}
	
}

std::shared_ptr<SpriteFrameEntry> SpriteData::GetSpriteFrame(const std::string& name) const
{
	assert(m_frames.find(name) != m_frames.cend());
	return m_frames.find(name)->second;
}

std::shared_ptr<SpriteFrameEntry> SpriteData::GetSpriteFrame(uint8_t id, uint8_t frame) const
{
	assert(m_sprite_frames.find(id) != m_sprite_frames.cend());
	assert(m_sprite_frames.find(id)->second.size() > frame);
	auto it = m_sprite_frames.find(id)->second.cbegin();
	std::advance(it, frame);
	assert(m_frames.find(*it) != m_frames.cend());
	return m_frames.find(*it)->second;
}

std::shared_ptr<SpriteFrameEntry> SpriteData::GetSpriteFrame(uint8_t id, uint8_t anim, uint8_t frame) const
{
	if (m_animations.find(id) == m_animations.cend())
	{
		return nullptr;
	}
	assert(m_animations.find(id)->second.size() > anim);
	const auto& name = m_animations.find(id)->second[anim];
	return GetSpriteFrame(name, frame);
}

std::shared_ptr<SpriteFrameEntry> SpriteData::GetSpriteFrame(const std::string& anim_name, uint8_t frame) const
{
	assert(m_animation_frames.find(anim_name) != m_animation_frames.cend());
	assert(m_animation_frames.find(anim_name)->second.size() > frame);
	const auto& name = m_animation_frames.find(anim_name)->second[frame];
	assert(m_frames.find(name) != m_frames.cend());
	return m_frames.find(name)->second;
}

uint32_t SpriteData::GetSpriteAnimationFrameCount(uint8_t id, uint8_t anim_id) const
{
	return GetSpriteAnimationFrames(id, anim_id).size();
}

uint32_t SpriteData::GetSpriteAnimationFrameCount(const std::string& name) const
{
	assert(m_animation_frames.find(name) != m_animation_frames.cend());
	return m_animation_frames.find(name)->second.size();
}

std::vector<std::string> SpriteData::GetSpriteAnimationFrames(uint8_t id, uint8_t anim_id) const
{
	assert(m_animations.find(id) != m_animations.cend());
	assert(m_animations.find(id)->second.size() > anim_id);
	const auto& name = m_animations.find(id)->second[anim_id];
	return GetSpriteAnimationFrames(name);
}

std::vector<std::string> SpriteData::GetSpriteAnimationFrames(const std::string& anim) const
{
	assert(m_animation_frames.find(anim) != m_animation_frames.cend());
	return m_animation_frames.find(anim)->second;
}

std::vector<std::string> SpriteData::GetSpriteAnimationFrames(const std::string& name, uint8_t anim_id) const
{
	auto id = GetSpriteId(name);
	return GetSpriteAnimationFrames(id, anim_id);
}

std::vector<Entity> SpriteData::GetRoomEntities(uint16_t room) const
{
	auto it = m_room_entities.find(room);
	if (it == m_room_entities.cend())
	{
		return std::vector<Entity>();
	}
	else
	{
		return it->second;
	}
}

void SpriteData::SetRoomEntities(uint16_t room, const std::vector<Entity>& entities)
{
	m_room_entities[room] = entities;
}

std::vector<EntityVisibilityFlags> SpriteData::GetEntityVisibilityFlagsForRoom(uint16_t room)
{
	std::vector<EntityVisibilityFlags> data;
	for (const auto& f : m_sprite_visibility_flags)
	{
		if (f.room == room)
		{
			data.push_back(f);
		}
	}
	return data;
}

void SpriteData::SetEntityVisibilityFlagsForRoom(uint16_t room, const std::vector<EntityVisibilityFlags>& data)
{
	std::stack<std::vector<EntityVisibilityFlags>::iterator> iterators;
	for (auto it = m_sprite_visibility_flags.begin(); it != m_sprite_visibility_flags.end(); )
	{
		if (it->room == room)
		{
			if (iterators.size() < data.size())
			{
				iterators.push(it++);
			}
			else
			{
				it = m_sprite_visibility_flags.erase(it);
			}
		}
		else
		{
			++it;
		}
	}
	for (const auto& f : data)
	{
		if (iterators.empty())
		{
			m_sprite_visibility_flags.push_back(f);
		}
		else
		{
			auto& it = iterators.top();
			*it = f;
			iterators.pop();
		}
	}
}

const std::map<std::string, std::shared_ptr<PaletteEntry>>& SpriteData::GetAllPalettes() const
{
	return m_palettes_by_name;
}

std::shared_ptr<PaletteEntry> SpriteData::GetPalette(const std::string& name) const
{
	assert(m_palettes_by_name.find(name) != m_palettes_by_name.end());
	return m_palettes_by_name.find(name)->second;
}

std::shared_ptr<Palette> SpriteData::GetSpritePalette(int lo, int hi) const
{
	std::vector<std::shared_ptr<Palette>> pals;

	if (lo >= 0)
	{
		pals.push_back(m_lo_palettes.at(lo)->GetData());
	}
	if (hi >= 0)
	{
		pals.push_back(m_hi_palettes.at(hi)->GetData());
	}

	return std::make_shared<Palette>(pals);
}

std::shared_ptr<Palette> SpriteData::GetSpritePalette(uint8_t idx) const
{
	int lo = -1;
	int hi = -1;

	if (m_lo_palette_lookup.find(idx) != m_lo_palette_lookup.cend())
	{
		lo = m_lo_palette_lookup.find(idx)->second->GetIndex();
	}
	if (m_hi_palette_lookup.find(idx) != m_hi_palette_lookup.cend())
	{
		hi = m_hi_palette_lookup.find(idx)->second->GetIndex();
	}

	return GetSpritePalette(lo, hi);
}

std::pair<int, int> SpriteData::GetSpritePaletteIdxs(uint8_t idx) const
{
	int lo = -1;
	int hi = -1;

	if (m_lo_palette_lookup.find(idx) != m_lo_palette_lookup.cend())
	{
		lo = m_lo_palette_lookup.find(idx)->second->GetIndex();
	}
	if (m_hi_palette_lookup.find(idx) != m_hi_palette_lookup.cend())
	{
		hi = m_hi_palette_lookup.find(idx)->second->GetIndex();
	}

	return { lo, hi };
}

uint8_t SpriteData::GetLoPaletteCount() const
{
	return m_lo_palettes.size();
}

std::shared_ptr<PaletteEntry> SpriteData::GetLoPalette(uint8_t idx) const
{
	assert(idx < m_lo_palettes.size());
	return m_lo_palettes[idx];
}

uint8_t SpriteData::GetHiPaletteCount() const
{
	return m_hi_palettes.size();
}

std::shared_ptr<PaletteEntry> SpriteData::GetHiPalette(uint8_t idx) const
{
	assert(idx < m_hi_palettes.size());
	return m_hi_palettes[idx];
}

uint8_t SpriteData::GetProjectile1PaletteCount() const
{
	return m_projectile1_palettes.size();
}

std::shared_ptr<PaletteEntry> SpriteData::GetProjectile1Palette(uint8_t idx) const
{
	assert(idx < m_projectile1_palettes.size());
	return m_projectile1_palettes[idx];
}

uint8_t SpriteData::GetProjectile2PaletteCount() const
{
	return m_projectile2_palettes.size();
}

std::shared_ptr<PaletteEntry> SpriteData::GetProjectile2Palette(uint8_t idx) const
{
	assert(idx < m_projectile2_palettes.size());
	return m_projectile2_palettes[idx];
}

void SpriteData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	std::for_each(m_frames.begin(), m_frames.end(), pair_commit);
	std::for_each(m_palettes_by_name.begin(), m_palettes_by_name.end(), pair_commit);
	m_frames_orig = m_frames;
	m_animations_orig = m_animations;
	m_animation_frames_orig = m_animation_frames;
	m_sprite_mystery_data_orig = m_sprite_mystery_data;
	m_lo_palettes_orig = m_lo_palettes;
	m_hi_palettes_orig = m_hi_palettes;
	m_projectile1_palettes_orig = m_projectile1_palettes;
	m_projectile2_palettes_orig = m_projectile2_palettes;
	m_hi_palette_lookup_orig = m_hi_palette_lookup;
	m_lo_palette_lookup_orig = m_lo_palette_lookup;
	m_sprite_visibility_flags_orig = m_sprite_visibility_flags;
	m_one_time_event_flags_orig = m_one_time_event_flags;
	m_room_clear_flags_orig = m_room_clear_flags;
	m_locked_door_flags_orig = m_locked_door_flags;
	m_permanent_switch_flags_orig = m_permanent_switch_flags;
	m_sacred_tree_flags_orig = m_sacred_tree_flags;
	m_sprite_dimensions_orig = m_sprite_dimensions;
	m_enemy_stats_orig = m_enemy_stats;
	m_sprite_to_entity_lookup_orig = m_sprite_to_entity_lookup;
	m_room_entities_orig = m_room_entities;
	m_item_properties_orig = m_item_properties;
	m_sprite_behaviours_orig = m_sprite_behaviours;
	m_sprite_behaviour_offsets_orig = m_sprite_behaviour_offsets;
	m_unknown_entity_lookup_orig = m_unknown_entity_lookup;
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
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::PALETTE_PROJECTILE_1, m_proj1_pal_file);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Sprites::PALETTE_PROJECTILE_2, m_proj2_pal_file);
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
	if (m_unknown_sprite_lookup_file.empty())        m_unknown_sprite_lookup_file     = RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP_FILE;
	if (m_sprite_visibility_flags_file.empty())      m_sprite_visibility_flags_file   = RomOffsets::Sprites::SPRITE_VISIBILITY_FLAGS_FILE;
	if (m_one_time_event_flags_file.empty())         m_one_time_event_flags_file      = RomOffsets::Sprites::ONE_TIME_EVENT_FLAGS_FILE;
	if (m_room_clear_flags_file.empty())             m_room_clear_flags_file          = RomOffsets::Sprites::ROOM_CLEAR_FLAGS_FILE;
	if (m_locked_door_sprite_flags_file.empty())     m_locked_door_sprite_flags_file  = RomOffsets::Sprites::LOCKED_DOOR_SPRITE_FLAGS_FILE;
	if (m_permanent_switch_flags_file.empty())       m_permanent_switch_flags_file    = RomOffsets::Sprites::PERMANENT_SWITCH_FLAGS_FILE;
	if (m_sacred_tree_flags_file.empty())            m_sacred_tree_flags_file         = RomOffsets::Sprites::SACRED_TREE_FLAGS_FILE;
	if (m_sprite_gfx_idx_lookup_file.empty())        m_sprite_gfx_idx_lookup_file     = RomOffsets::Sprites::SPRITE_GFX_IDX_LOOKUP_FILE;
	if (m_sprite_dimensions_lookup_file.empty())     m_sprite_dimensions_lookup_file  = RomOffsets::Sprites::SPRITE_DIMENSIONS_LOOKUP_FILE;
	if (m_room_sprite_table_offsets_file.empty())    m_room_sprite_table_offsets_file = RomOffsets::Sprites::ROOM_SPRITE_TABLE_OFFSETS_FILE;
	if (m_enemy_stats_file.empty())                  m_enemy_stats_file               = RomOffsets::Sprites::ENEMY_STATS_FILE;
	if (m_room_sprite_table_file.empty())            m_room_sprite_table_file         = RomOffsets::Sprites::ROOM_SPRITE_TABLE_FILE;
	if (m_item_properties_file.empty())              m_item_properties_file           = RomOffsets::Sprites::ITEM_PROPERTIES_FILE;
	if (m_sprite_behaviour_offset_file.empty())      m_sprite_behaviour_offset_file   = RomOffsets::Sprites::SPRITE_BEHAVIOUR_OFFSET_FILE;
	if (m_sprite_behaviour_table_file.empty())       m_sprite_behaviour_table_file    = RomOffsets::Sprites::SPRITE_BEHAVIOUR_TABLE_FILE;
	if (m_palette_data_file.empty())                 m_palette_data_file              = RomOffsets::Sprites::PALETTE_DATA_FILE;
	if (m_palette_lut_file.empty())                  m_palette_lut_file               = RomOffsets::Sprites::PALETTE_LUT_FILE;
	if (m_sprite_lut_file.empty())                   m_sprite_lut_file                = RomOffsets::Sprites::SPRITE_LUT_FILE;
	if (m_sprite_anims_file.empty())                 m_sprite_anims_file              = RomOffsets::Sprites::SPRITE_ANIMS_FILE;
	if (m_sprite_anim_frames_file.empty())           m_sprite_anim_frames_file        = RomOffsets::Sprites::SPRITE_ANIM_FRAMES_FILE;
	if (m_sprite_frames_data_file.empty())           m_sprite_frames_data_file        = RomOffsets::Sprites::SPRITE_FRAME_DATA_FILE;
	if (m_proj1_pal_file.empty())                    m_proj1_pal_file                 = RomOffsets::Sprites::PALETTE_PROJECTILE_1_FILE;
	if (m_proj2_pal_file.empty())                    m_proj2_pal_file                 = RomOffsets::Sprites::PALETTE_PROJECTILE_2_FILE;
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
	retval = retval && CreateDirectoryTree(dir / m_proj1_pal_file);
	retval = retval && CreateDirectoryTree(dir / m_proj2_pal_file);
	for (const auto& m : m_frames)
	{
		retval = retval && CreateDirectoryTree(dir / m.second->GetFilename());
	}
	for (const auto& m : m_palettes_by_name)
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
	m_lo_palettes_orig = m_lo_palettes;
	m_hi_palettes_orig = m_hi_palettes;
	m_projectile1_palettes_orig = m_projectile1_palettes;
	m_projectile2_palettes_orig = m_projectile2_palettes;
	m_hi_palette_lookup_orig = m_hi_palette_lookup;
	m_lo_palette_lookup_orig = m_lo_palette_lookup;
	m_sprite_visibility_flags_orig = m_sprite_visibility_flags;
	m_one_time_event_flags_orig = m_one_time_event_flags;
	m_room_clear_flags_orig = m_room_clear_flags;
	m_locked_door_flags_orig = m_locked_door_flags;
	m_permanent_switch_flags_orig = m_permanent_switch_flags;
	m_sacred_tree_flags_orig = m_sacred_tree_flags;
	m_sprite_dimensions_orig = m_sprite_dimensions;
	m_enemy_stats_orig = m_enemy_stats;
	m_sprite_to_entity_lookup_orig = m_sprite_to_entity_lookup;
	m_room_entities_orig = m_room_entities;
	m_item_properties_orig = m_item_properties;
	m_sprite_behaviours_orig = m_sprite_behaviours;
	m_sprite_behaviour_offsets_orig = m_sprite_behaviour_offsets;
	m_unknown_entity_lookup_orig = m_unknown_entity_lookup;
}

ByteVector SpriteData::SerialisePaletteLUT() const
{
	ByteVector result;
	for (uint8_t i = 0; i < 0xFF; ++i)
	{
		if (m_lo_palette_lookup.find(i) != m_lo_palette_lookup.cend())
		{
			result.push_back(i);
			result.push_back(m_lo_palette_lookup.at(i)->GetIndex());
		}
		if (m_hi_palette_lookup.find(i) != m_hi_palette_lookup.cend())
		{
			result.push_back(i);
			result.push_back(0x80 | m_hi_palette_lookup.at(i)->GetIndex());
		}
	}
	result.push_back(0xFF);
	result.push_back(0xFF);
	return result;
}

void SpriteData::DeserialisePaletteLUT(const ByteVector& bytes)
{
	for (int i = 0; i < bytes.size(); i += 2)
	{
		uint8_t sprite = bytes[i];
		uint8_t pal = bytes[i + 1];
		if (sprite == 0xFF || pal == 0xFF)
		{
			break;
		}
		if ((pal & 0x80) == 0)
		{
			assert(pal < m_lo_palettes.size());
			m_lo_palette_lookup.insert({ sprite, m_lo_palettes[pal]});
		}
		else
		{
			pal &= 0x7F;
			assert(pal < m_hi_palettes.size());
			m_hi_palette_lookup.insert({ sprite, m_hi_palettes[pal] });
		}
	}
}

ByteVector SpriteData::SerialisePalArray(const std::vector<std::shared_ptr<PaletteEntry>>& pals) const
{
	ByteVector bytes;
	for (const auto& p : pals)
	{
		auto b = p->GetBytes();
		bytes.insert(bytes.end(), b->cbegin(), b->cend());
	}
	return bytes;
}

std::vector<std::shared_ptr<PaletteEntry>> SpriteData::DeserialisePalArray(const ByteVector& bytes, const std::string& name,
	const filesystem::path& path, Palette::Type type, bool unique_path)
{
	std::vector<std::shared_ptr<PaletteEntry>> result;
	const uint32_t size = Palette::GetSizeBytes(type);
	assert(bytes.size() % size == 0);
	auto it = bytes.cbegin();
	int idx = 0;
	bool format_name = (name.find('%') != std::string::npos);
	while (it != bytes.cend())
	{
		auto fname = format_name ? StrPrintf(name, idx + 1) : name + StrPrintf(":%d", idx);
		auto fpath = unique_path ? StrPrintf(path.str(), idx + 1) : path.str();
		auto b = ByteVector(it, it + size);
		if (b[0] > 0x0E)
		{
			// Invalid colour, probably signals end-of-data
			break;
		}
		auto e = PaletteEntry::Create(this, b, fname, fpath, type);
		e->SetIndex(idx);
		result.push_back(e);
		m_palettes_by_name.insert({ fname, e });
		it += size;
		idx++;
	}
	auto old_name = result.back()->GetName();
	if ((result.size() == 1) && (old_name.find(':') != std::string::npos))
	{
		auto pal = m_palettes_by_name.extract(old_name);
		auto new_name = old_name.substr(0, old_name.find(':'));
		pal.key() = new_name;
		pal.mapped()->SetName(new_name);
		m_palettes_by_name.insert(std::move(pal));
		
	}
	return result;
}

void SpriteData::DeserialiseRoomEntityTable(const ByteVector& offsets, const ByteVector& bytes)
{
	for (int i = 0; (i * 2) < offsets.size(); ++i)
	{
		uint16_t offset = (offsets[i * 2] << 8) | offsets[i * 2 + 1];
		if (offset == 0)
		{
			continue;
		}
		offset--;
		m_room_entities.insert({ i, std::vector<Entity>() });
		while (bytes[offset] != 0xFF || bytes[offset + 1] != 0xFF)
		{
			std::array<uint8_t, 8> data;
			std::copy_n(bytes.cbegin() + offset, 8, data.begin());
			m_room_entities[i].emplace_back(data);
			offset += 8;
		}
	}
}

std::pair<ByteVector, ByteVector> SpriteData::SerialiseRoomEntityTable() const
{
	ByteVector bytes, offsets;
	offsets.reserve((m_room_entities.rbegin()->first + 1) * sizeof(uint16_t));
	for (int i = 0; i <= m_room_entities.rbegin()->first; ++i)
	{
		auto res = m_room_entities.find(i);
		if (res == m_room_entities.cend())
		{
			offsets.push_back(0);
			offsets.push_back(0);
			continue;
		}
		uint16_t offset = bytes.size() + 1;
		offsets.push_back((offset >> 8) & 0xFF);
		offsets.push_back(offset & 0xFF);
		for (const auto& ent : res->second)
		{
			auto data = ent.GetData();
			bytes.insert(bytes.end(), std::begin(data), std::end(data));
		}
		bytes.push_back(0xFF);
		bytes.push_back(0xFF);
	}
	return { bytes, offsets };
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
			std::string sprname = StrPrintf(RomOffsets::Sprites::SPRITE_GFX, spr);
			if (anim_file.IsLabel())
			{
				AsmFile::Label lbl;
				anim_file >> lbl;
				if (lbl.label != RomOffsets::Sprites::SPRITE_SECTION)
				{
					sprname = lbl.label;
				}
			}
			m_names.insert({ spr, sprname });
			m_ids.insert({ sprname, spr });
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
					if (m_sprite_frames.find(spr.first) == m_sprite_frames.cend())
					{
						m_sprite_frames.insert({spr.first, std::set<std::string>()});
					}
					m_sprite_frames[spr.first].insert(ptrname);
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

bool SpriteData::AsmLoadSpritePalettes()
{
	std::vector<std::pair<uint16_t, uint16_t>> lut;
	try
	{
		AsmFile file(GetBasePath() / m_palette_data_file);
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Sprites::PALETTE_LUT);
		file >> inc;
		m_palette_lut_file = inc.path;
		int idx = 0;
		file.Goto(RomOffsets::Sprites::PALETTE_LO_DATA);
		do
		{
			file >> inc;
			std::string name = StrPrintf(RomOffsets::Sprites::PALETTE_LO, idx + 1);
			auto e = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, Palette::Type::SPRITE_LOW);
			e->SetIndex(idx++);
			m_lo_palettes.push_back(e);
			m_palettes_by_name.insert({ name, e });
		} while (file.IsGood() && !file.IsLabel(RomOffsets::Sprites::PALETTE_HI_DATA));
		idx = 0;
		file.Goto(RomOffsets::Sprites::PALETTE_HI_DATA);
		do
		{
			file >> inc;
			std::string name = StrPrintf(RomOffsets::Sprites::PALETTE_HI, idx + 1);
			auto e = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, Palette::Type::SPRITE_HIGH);
			e->SetIndex(idx++);
			m_hi_palettes.push_back(e);
			m_palettes_by_name.insert({ name, e });
		} while (file.IsGood() && !file.IsLabel(RomOffsets::Sprites::PALETTE_LO_DATA));
		DeserialisePaletteLUT(ReadBytes(GetBasePath() / m_palette_lut_file));
		auto proj1_bytes = ReadBytes(GetBasePath() / m_proj1_pal_file);
		auto proj2_bytes = ReadBytes(GetBasePath() / m_proj2_pal_file);
		m_projectile1_palettes = DeserialisePalArray(proj1_bytes, RomOffsets::Sprites::PALETTE_PROJECTILE_1, m_proj1_pal_file, Palette::Type::PROJECTILE);
		m_projectile2_palettes = DeserialisePalArray(proj2_bytes, RomOffsets::Sprites::PALETTE_PROJECTILE_2, m_proj2_pal_file, Palette::Type::PROJECTILE2);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool SpriteData::AsmLoadSpriteData()
{
	auto Decode = [](const auto& data_in, auto& data_out)
	{
		for (auto& d : data_in)
		{
			data_out.emplace_back(d);
		}
	};

	Decode(DeserialiseFixedWidth<4>(ReadBytes(GetBasePath() / m_sprite_visibility_flags_file)), m_sprite_visibility_flags);
	m_one_time_event_flags     = DeserialiseFixedWidth<6>(ReadBytes(GetBasePath() / m_one_time_event_flags_file));
	m_room_clear_flags         = DeserialiseFixedWidth<4>(ReadBytes(GetBasePath() / m_room_clear_flags_file));
	m_locked_door_flags        = DeserialiseFixedWidth<4>(ReadBytes(GetBasePath() / m_locked_door_sprite_flags_file));
	m_permanent_switch_flags   = DeserialiseFixedWidth<4>(ReadBytes(GetBasePath() / m_permanent_switch_flags_file));
	m_sacred_tree_flags        = DeserialiseFixedWidth<4>(ReadBytes(GetBasePath() / m_sacred_tree_flags_file));
	m_item_properties          = DeserialiseFixedWidth<4>(ReadBytes(GetBasePath() / m_item_properties_file));
	m_enemy_stats              = DeserialiseMap<5>(ReadBytes(GetBasePath() / m_enemy_stats_file));
	m_sprite_dimensions        = DeserialiseMap<2>(ReadBytes(GetBasePath() / m_sprite_dimensions_lookup_file));
	m_sprite_to_entity_lookup  = DeserialiseMap(ReadBytes(GetBasePath() / m_sprite_gfx_idx_lookup_file), true);
	m_unknown_entity_lookup    = DeserialiseMap(ReadBytes(GetBasePath() / m_unknown_sprite_lookup_file));
	m_sprite_behaviour_offsets = ReadBytes(GetBasePath() / m_sprite_behaviour_offset_file);
	m_sprite_behaviours        = ReadBytes(GetBasePath() / m_sprite_behaviour_table_file);
	DeserialiseRoomEntityTable(ReadBytes(GetBasePath() / m_room_sprite_table_offsets_file),
		ReadBytes(GetBasePath() / m_room_sprite_table_file));

	return true;
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
		std::string sprname = StrPrintf(RomOffsets::Sprites::SPRITE_GFX, i);
		m_sprite_mystery_data.insert({ i, offset_table[i * 2 + 1] });
		m_names.insert({ i, sprname });
		m_ids.insert({ sprname, i });
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
		if (m_sprite_frames.find(e->GetSprite()) == m_sprite_frames.cend())
		{
			m_sprite_frames.insert({ e->GetSprite(), std::set<std::string>() });
		}
		m_sprite_frames[e->GetSprite()].insert(e->GetName());
	}

	return true;
}

bool SpriteData::RomLoadSpritePalettes(const Rom& rom)
{
	uint32_t lut_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::PALETTE_LUT);
	uint32_t lo_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::PALETTE_LO_DATA);
	uint32_t hi_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::PALETTE_HI_DATA);
	uint32_t hi_end    = rom.get_section(RomOffsets::Sprites::PALETTE_DATA).end;
	uint32_t proj1_begin = Disasm::ReadOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_1_MOVEW1);
	uint32_t proj1_end = rom.get_section(RomOffsets::Sprites::PALETTE_PROJECTILE_1).end;
	uint32_t proj2_begin = Disasm::ReadOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_2_MOVEW1);
	uint32_t proj2_end = rom.get_section(RomOffsets::Sprites::PALETTE_PROJECTILE_2).end;

	uint32_t lut_size = (lo_begin - lut_begin);
	uint32_t lo_size = (hi_begin - lo_begin);
	uint32_t hi_size = (hi_end - hi_begin);
	uint32_t proj1_size = (proj1_end - proj1_begin);
	uint32_t proj2_size = (proj2_end - proj2_begin);
	lut_size -= (lut_size % 2);
	lo_size -= (lo_size % Palette::GetSizeBytes(Palette::Type::SPRITE_LOW));
	hi_size -= (hi_size % Palette::GetSizeBytes(Palette::Type::SPRITE_HIGH));
	proj1_size -= (proj1_size % Palette::GetSizeBytes(Palette::Type::PROJECTILE));
	proj2_size -= (proj2_size % Palette::GetSizeBytes(Palette::Type::PROJECTILE2));

	auto lut_bytes = rom.read_array<uint8_t>(lut_begin, lut_size);
	auto hi_bytes = rom.read_array<uint8_t>(hi_begin, hi_size);
	auto lo_bytes = rom.read_array<uint8_t>(lo_begin, lo_size);
	auto proj1_bytes = rom.read_array<uint8_t>(proj1_begin, proj1_size);
	auto proj2_bytes = rom.read_array<uint8_t>(proj2_begin, proj2_size);

	m_hi_palettes = DeserialisePalArray(hi_bytes, RomOffsets::Sprites::PALETTE_HI, RomOffsets::Sprites::PALETTE_HI_FILE,
		Palette::Type::SPRITE_HIGH, true);
	m_lo_palettes = DeserialisePalArray(lo_bytes, RomOffsets::Sprites::PALETTE_LO, RomOffsets::Sprites::PALETTE_LO_FILE,
		Palette::Type::SPRITE_LOW, true);
	m_projectile1_palettes = DeserialisePalArray(proj1_bytes, RomOffsets::Sprites::PALETTE_PROJECTILE_1,
		RomOffsets::Sprites::PALETTE_PROJECTILE_1_FILE, Palette::Type::PROJECTILE, false);
	m_projectile2_palettes = DeserialisePalArray(proj2_bytes, RomOffsets::Sprites::PALETTE_PROJECTILE_2,
		RomOffsets::Sprites::PALETTE_PROJECTILE_2_FILE, Palette::Type::PROJECTILE2, false);

	DeserialisePaletteLUT(lut_bytes);

	return true;
}

bool SpriteData::RomLoadSpriteData(const Rom& rom)
{
	const uint32_t items_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::ITEM_PROPERTIES);
	const uint32_t items_size = rom.get_section(RomOffsets::Sprites::ITEM_PROPERTIES_SECTION).end - items_begin;
	const uint32_t unk_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP);
	const uint32_t unk_size = rom.get_section(RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP_SECTION).end - unk_begin;

	const uint32_t behav_offsets_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::SPRITE_BEHAVIOUR_OFFSETS);
	const uint32_t behav_table_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::SPRITE_BEHAVIOUR_TABLE);
	const uint32_t behav_table_end = rom.get_section(RomOffsets::Sprites::SPRITE_BEHAVIOUR_SECTION).end;
	const uint32_t behav_offsets_size = behav_table_begin - behav_offsets_begin;
	const uint32_t behav_table_size = behav_table_end - behav_table_begin;


	const uint32_t visib_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::SPRITE_VISIBILITY_FLAGS);
	const uint32_t onetime_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::ONE_TIME_EVENT_FLAGS);
	const uint32_t room_clear_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::ROOM_CLEAR_FLAGS);
	const uint32_t locked_doors_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::LOCKED_DOOR_SPRITE_FLAGS);
	const uint32_t permanent_switch_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::PERMANENT_SWITCH_FLAGS);
	const uint32_t trees_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::SACRED_TREE_FLAGS);
	const uint32_t sprite_ent_lut_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::SPRITE_GFX_IDX_LOOKUP);
	const uint32_t sprite_dims_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::SPRITE_DIMENSIONS_LOOKUP);
	const uint32_t offsets_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::ROOM_SPRITE_TABLE_OFFSETS);
	const uint32_t enemy_data_begin = Disasm::ReadOffset16(rom, RomOffsets::Sprites::ENEMY_STATS);
	const uint32_t entity_table_begin = rom.read<uint32_t>(RomOffsets::Sprites::ROOM_SPRITE_TABLE);
	const uint32_t entity_table_end = rom.get_section(RomOffsets::Sprites::SPRITE_DATA_SECTION).end;

	const uint32_t visib_size = onetime_begin - visib_begin;
	const uint32_t onetime_size = room_clear_begin - onetime_begin;
	const uint32_t room_clear_size = locked_doors_begin - room_clear_begin;
	const uint32_t locked_doors_size = permanent_switch_begin - locked_doors_begin;
	const uint32_t permanent_switch_size = trees_begin - permanent_switch_begin;
	const uint32_t trees_size = sprite_ent_lut_begin - trees_begin;
	const uint32_t sprite_ent_lut_size = sprite_dims_begin - sprite_ent_lut_begin;
	const uint32_t sprite_dims_size = offsets_begin - sprite_dims_begin;
	const uint32_t offsets_size = enemy_data_begin - offsets_begin;
	const uint32_t enemy_data_size = entity_table_begin - enemy_data_begin;
	const uint32_t entity_table_size = entity_table_end - entity_table_begin;

	auto Decode = [](const auto& data_in, auto& data_out)
	{
		for (auto& d : data_in)
		{
			data_out.emplace_back(d);
		}
	};

	Decode(DeserialiseFixedWidth<4>(rom.read_array<uint8_t>(visib_begin, visib_size)), m_sprite_visibility_flags);
	m_one_time_event_flags = DeserialiseFixedWidth<6>(rom.read_array<uint8_t>(onetime_begin, onetime_size));
	m_room_clear_flags = DeserialiseFixedWidth<4>(rom.read_array<uint8_t>(room_clear_begin, room_clear_size));
	m_locked_door_flags = DeserialiseFixedWidth<4>(rom.read_array<uint8_t>(locked_doors_begin, locked_doors_size));
	m_permanent_switch_flags = DeserialiseFixedWidth<4>(rom.read_array<uint8_t>(permanent_switch_begin, permanent_switch_size));
	m_sacred_tree_flags = DeserialiseFixedWidth<4>(rom.read_array<uint8_t>(trees_begin, trees_size));
	m_item_properties = DeserialiseFixedWidth<4>(rom.read_array<uint8_t>(items_begin, items_size));
	m_enemy_stats = DeserialiseMap<5>(rom.read_array<uint8_t>(enemy_data_begin, enemy_data_size));
	m_sprite_dimensions = DeserialiseMap<2>(rom.read_array<uint8_t>(sprite_dims_begin, sprite_dims_size));
	m_sprite_to_entity_lookup = DeserialiseMap(rom.read_array<uint8_t>(sprite_ent_lut_begin, sprite_ent_lut_size), true);
	m_unknown_entity_lookup = DeserialiseMap(rom.read_array<uint8_t>(unk_begin, unk_size));
	m_sprite_behaviour_offsets = rom.read_array<uint8_t>(behav_offsets_begin, behav_offsets_size);
	m_sprite_behaviours = rom.read_array<uint8_t>(behav_table_begin, behav_table_size);
	DeserialiseRoomEntityTable(rom.read_array<uint8_t>(offsets_begin, offsets_size),
		rom.read_array<uint8_t>(entity_table_begin, entity_table_size));

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
			frm_file << AsmFile::Align(2);
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
			spr_file << AsmFile::Label(m_names[spr.first]);
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

bool SpriteData::AsmSaveSpritePalettes(const filesystem::path& dir)
{
	try
	{
		auto proj1_bytes = SerialisePalArray(m_projectile1_palettes);
		WriteBytes(proj1_bytes, dir / m_proj1_pal_file);
		auto proj2_bytes = SerialisePalArray(m_projectile2_palettes);
		WriteBytes(proj2_bytes, dir / m_proj2_pal_file);
		auto lookup_bytes = SerialisePaletteLUT();
		WriteBytes(lookup_bytes, dir / m_palette_lut_file);

		AsmFile file;
		file.WriteFileHeader(m_palette_data_file, "Sprite Palette Data");
		file << AsmFile::Label(RomOffsets::Sprites::PALETTE_LUT) << AsmFile::IncludeFile(m_palette_lut_file, AsmFile::BINARY);
		file << AsmFile::Label(RomOffsets::Sprites::PALETTE_LO_DATA);
		for (const auto& pal : m_lo_palettes)
		{
			file << AsmFile::IncludeFile(pal->GetFilename(), AsmFile::BINARY);
			pal->Save(dir);
		}
		file << AsmFile::Label(RomOffsets::Sprites::PALETTE_HI_DATA);
		for (const auto& pal : m_hi_palettes)
		{
			file << AsmFile::IncludeFile(pal->GetFilename(), AsmFile::BINARY);
			pal->Save(dir);
		}
		file.WriteFile(dir / m_palette_data_file);

		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool SpriteData::AsmSaveSpriteData(const filesystem::path& dir)
{
	auto Encode4 = [](const auto& data_in)
	{
		std::vector<std::array<uint8_t, 4>> data_out;
		for (auto& d : data_in)
		{
			data_out.push_back(d.GetData());
		}
		return data_out;
	};

	WriteBytes(SerialiseFixedWidth<4>(Encode4(m_sprite_visibility_flags)), dir / m_sprite_visibility_flags_file);
	WriteBytes(SerialiseFixedWidth<6>(m_one_time_event_flags), dir / m_one_time_event_flags_file);
	WriteBytes(SerialiseFixedWidth<4>(m_room_clear_flags), dir / m_room_clear_flags_file);
	WriteBytes(SerialiseFixedWidth<4>(m_locked_door_flags), dir / m_locked_door_sprite_flags_file);
	WriteBytes(SerialiseFixedWidth<4>(m_permanent_switch_flags), dir / m_permanent_switch_flags_file);
	WriteBytes(SerialiseFixedWidth<4>(m_sacred_tree_flags), dir / m_sacred_tree_flags_file);
	WriteBytes(SerialiseFixedWidth<4>(m_item_properties, false), dir / m_item_properties_file);
	WriteBytes(SerialiseMap<5>(m_enemy_stats), dir / m_enemy_stats_file);
	WriteBytes(SerialiseMap<2>(m_sprite_dimensions), dir / m_sprite_dimensions_lookup_file);
	WriteBytes(SerialiseMap(m_sprite_to_entity_lookup, true), dir / m_sprite_gfx_idx_lookup_file);
	WriteBytes(SerialiseMap(m_unknown_entity_lookup), dir / m_unknown_sprite_lookup_file);
	WriteBytes(m_sprite_behaviour_offsets, dir / m_sprite_behaviour_offset_file);
	WriteBytes(m_sprite_behaviours, dir / m_sprite_behaviour_table_file);
	auto result = SerialiseRoomEntityTable();
	WriteBytes(result.first, dir / m_room_sprite_table_file);
	WriteBytes(result.second, dir / m_room_sprite_table_offsets_file);
	return true;
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

bool SpriteData::RomPrepareInjectSpritePalettes(const Rom& rom)
{
	uint32_t proj1_begin = rom.get_section(RomOffsets::Sprites::PALETTE_PROJECTILE_1).begin;
	auto proj1_bytes = std::make_shared<ByteVector>(SerialisePalArray(m_projectile1_palettes));

	uint32_t proj2_begin = rom.get_section(RomOffsets::Sprites::PALETTE_PROJECTILE_2).begin;
	auto proj2_bytes = std::make_shared<ByteVector>(SerialisePalArray(m_projectile2_palettes));

	uint32_t pal_lut_begin = rom.get_section(RomOffsets::Sprites::PALETTE_DATA).begin;
	auto bytes = std::make_shared<ByteVector>(SerialisePaletteLUT());
	uint32_t lo_pals_begin = pal_lut_begin + bytes->size();
	for (const auto& p : m_lo_palettes)
	{
		auto b = p->GetBytes();
		bytes->insert(bytes->end(), b->cbegin(), b->cend());
	}
	uint32_t hi_pals_begin = pal_lut_begin + bytes->size();
	for (const auto& p : m_hi_palettes)
	{
		auto b = p->GetBytes();
		bytes->insert(bytes->end(), b->cbegin(), b->cend());
	}

	m_pending_writes.push_back({ RomOffsets::Sprites::PALETTE_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_LUT, pal_lut_begin));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_LO_DATA, lo_pals_begin));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_HI_DATA, hi_pals_begin));
	m_pending_writes.push_back({ RomOffsets::Sprites::PALETTE_PROJECTILE_1, proj1_bytes });
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_1_MOVEW1, proj1_begin));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_1_MOVEW2, proj1_begin + 2));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_1_MOVEW3, proj1_begin));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_1_MOVEW4, proj1_begin + 2));
	m_pending_writes.push_back({ RomOffsets::Sprites::PALETTE_PROJECTILE_2, proj2_bytes });
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_2_MOVEW1, proj2_begin));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_2_MOVEW2, proj2_begin + 2));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_2_MOVEW3, proj2_begin + 4));
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::PALETTE_PROJECTILE_2_MOVEW4, proj2_begin + 6));

	return true;
}

bool SpriteData::RomPrepareInjectSpriteData(const Rom& rom)
{
	auto Encode4 = [](const auto& data_in)
	{
		std::vector<std::array<uint8_t, 4>> data_out;
		for (auto& d : data_in)
		{
			data_out.push_back(d.GetData());
		}
		return data_out;
	};

	auto room_entities = SerialiseRoomEntityTable();

	uint32_t item_begin = rom.get_section(RomOffsets::Sprites::ITEM_PROPERTIES_SECTION).begin;
	auto item_bytes = std::make_shared<ByteVector>(SerialiseFixedWidth<4>(m_item_properties, false));

	uint32_t unk_begin = rom.get_section(RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP_SECTION).begin;
	auto unk_bytes = std::make_shared<ByteVector>(SerialiseMap(m_unknown_entity_lookup));

	uint32_t behavoff_begin = rom.get_section(RomOffsets::Sprites::SPRITE_BEHAVIOUR_SECTION).begin;
	auto behav_bytes = std::make_shared<ByteVector>(m_sprite_behaviour_offsets);
	uint32_t behavtab_begin = behavoff_begin + behav_bytes->size();
	behav_bytes->insert(behav_bytes->end(), m_sprite_behaviours.cbegin(), m_sprite_behaviours.cend());

	uint32_t visib_begin = rom.get_section(RomOffsets::Sprites::SPRITE_DATA_SECTION).begin;
	auto data_bytes = std::make_shared<ByteVector>(SerialiseFixedWidth<4>(Encode4(m_sprite_visibility_flags)));

	uint32_t onetime_begin = data_bytes->size() + visib_begin;
	auto onetime_bytes = SerialiseFixedWidth<6>(m_one_time_event_flags);
	data_bytes->insert(data_bytes->end(), onetime_bytes.cbegin(), onetime_bytes.cend());

	uint32_t clear_begin = data_bytes->size() + visib_begin;
	auto clear_bytes = SerialiseFixedWidth<4>(m_room_clear_flags);
	data_bytes->insert(data_bytes->end(), clear_bytes.cbegin(), clear_bytes.cend());

	uint32_t door_begin = data_bytes->size() + visib_begin;
	auto door_bytes = SerialiseFixedWidth<4>(m_locked_door_flags);
	data_bytes->insert(data_bytes->end(), door_bytes.cbegin(), door_bytes.cend());

	uint32_t switch_begin = data_bytes->size() + visib_begin;
	auto switch_bytes = SerialiseFixedWidth<4>(m_permanent_switch_flags);
	data_bytes->insert(data_bytes->end(), switch_bytes.cbegin(), switch_bytes.cend());

	uint32_t tree_begin = data_bytes->size() + visib_begin;
	auto tree_bytes = SerialiseFixedWidth<4>(m_sacred_tree_flags);
	data_bytes->insert(data_bytes->end(), tree_bytes.cbegin(), tree_bytes.cend());

	uint32_t sprent_begin = data_bytes->size() + visib_begin;
	auto sprent_bytes = SerialiseMap(m_sprite_to_entity_lookup, true);
	data_bytes->insert(data_bytes->end(), sprent_bytes.cbegin(), sprent_bytes.cend());

	uint32_t hitbox_begin = data_bytes->size() + visib_begin;
	auto hitbox_bytes = SerialiseMap<2>(m_sprite_dimensions);
	data_bytes->insert(data_bytes->end(), hitbox_bytes.cbegin(), hitbox_bytes.cend());
	if ((data_bytes->size() & 1) == 1)
	{
		data_bytes->push_back(0xFF);
	}

	uint32_t offsets_begin = data_bytes->size() + visib_begin;
	data_bytes->insert(data_bytes->end(), room_entities.second.cbegin(), room_entities.second.cend());

	uint32_t enemy_begin = data_bytes->size() + visib_begin;
	auto enemy_bytes = SerialiseMap<5>(m_enemy_stats);
	data_bytes->insert(data_bytes->end(), enemy_bytes.cbegin(), enemy_bytes.cend());

	uint32_t table_begin = data_bytes->size() + visib_begin;
	data_bytes->insert(data_bytes->end(), room_entities.first.cbegin(), room_entities.first.cend());

	m_pending_writes.push_back({ RomOffsets::Sprites::ITEM_PROPERTIES_SECTION, item_bytes });
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomOffsets::Sprites::ITEM_PROPERTIES, item_begin));
	m_pending_writes.push_back({ RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP_SECTION, unk_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::UNKNOWN_SPRITE_LOOKUP, unk_begin));
	m_pending_writes.push_back({ RomOffsets::Sprites::SPRITE_BEHAVIOUR_SECTION, behav_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::SPRITE_BEHAVIOUR_OFFSETS, behavoff_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::SPRITE_BEHAVIOUR_TABLE, behavtab_begin));
	m_pending_writes.push_back({ RomOffsets::Sprites::SPRITE_DATA_SECTION, data_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::SPRITE_VISIBILITY_FLAGS, visib_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::ONE_TIME_EVENT_FLAGS, onetime_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::ROOM_CLEAR_FLAGS, clear_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::LOCKED_DOOR_SPRITE_FLAGS, door_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::PERMANENT_SWITCH_FLAGS, switch_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::SACRED_TREE_FLAGS, tree_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::SPRITE_GFX_IDX_LOOKUP, sprent_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::SPRITE_DIMENSIONS_LOOKUP, hitbox_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::ROOM_SPRITE_TABLE_OFFSETS, offsets_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomOffsets::Sprites::ENEMY_STATS, enemy_begin));
	m_pending_writes.push_back(Asm::WriteAddress32(RomOffsets::Sprites::ROOM_SPRITE_TABLE, table_begin));

	return true;
}
