#include <landstalker/main/include/StringData.h>

#include <codecvt>

#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/RomLabels.h>
#include <landstalker/misc/include/Literals.h>
#include <landstalker/misc/include/Labels.h>

#include <yaml-cpp/yaml.h>

StringData::StringData(const std::filesystem::path& asm_file)
	: DataManager("String Data", asm_file), m_has_region_check(false)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadCompressedStringData())
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from \'") + m_strings_filename.string() + '\'');
	}
	if (!AsmLoadHuffmanData())
	{
		throw std::runtime_error(std::string("Unable to load Huffman data from \'") + asm_file.string() + '\'');
	}
	m_region = Charset::DeduceRegion(GetCharsetSize());
	if (m_has_region_check)
	{
		if (!AsmLoadSystemFont())
		{
			throw std::runtime_error(std::string("Unable to load system font data from \'") + asm_file.string() + '\'');
		}
		if (!AsmLoadSystemStrings())
		{
			throw std::runtime_error(std::string("Unable to load system string data from \'") + asm_file.string() + '\'');
		}
	}
	if (!AsmLoadStringTables())
	{
		throw std::runtime_error(std::string("Unable to load string tables from \'") + m_string_table_path.string() + '\'');
	}
	if (!AsmLoadIntroStrings())
	{
		throw std::runtime_error(std::string("Unable to load intro string data from \'") + m_string_table_path.string() + '\'');
	}
	if (!AsmLoadEndCreditStrings())
	{
		throw std::runtime_error(std::string("Unable to load end credit strings from \'") + m_end_credit_strings_path.string() + '\'');
	}
	if (!AsmLoadTalkSfx())
	{
		throw std::runtime_error(std::string("Unable to load talk sound effects from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadScriptData())
	{
		throw std::runtime_error(std::string("Unable to load script data from \'") + asm_file.string() + '\'');
	}
	DecompressStrings();
	InitCache();
}

StringData::StringData(const Rom& rom)
	: DataManager("String Data", rom),
	  m_region(rom.get_region()),
	  m_has_region_check(m_region != RomOffsets::Region::JP && m_region != RomOffsets::Region::US_BETA)
{
	SetDefaultFilenames();
	if (m_has_region_check)
	{
		if (!RomLoadSystemFont(rom))
		{
			throw std::runtime_error(std::string("Unable to load system font from ROM"));
		}
		if (!RomLoadSystemStrings(rom))
		{
			throw std::runtime_error(std::string("Unable to load system strings from ROM"));
		}
	}
	if (!RomLoadCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from ROM"));
	}
	if (!RomLoadHuffmanData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Huffman data from ROM"));
	}
	if (!RomLoadStringTables(rom))
	{
		throw std::runtime_error(std::string("Unable to load string tables from ROM"));
	}
	if (!RomLoadIntroStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to load intro strings from ROM"));
	}
	if (!RomLoadEndCreditStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to load end credit strings from ROM"));
	}
	if (!RomLoadTalkSfx(rom))
	{
		throw std::runtime_error(std::string("Unable to load talk sound effects from ROM"));
	}
	if (!RomLoadScriptData(rom))
	{
		throw std::runtime_error(std::string("Unable to load script data from ROM"));
	}
	DecompressStrings();
	InitCache();
}

bool StringData::Save(const std::filesystem::path& dir)
{
	std::filesystem::path directory = dir;
	CompressStrings();
	if (std::filesystem::exists(directory) && std::filesystem::is_regular_file(directory))
	{
		directory = directory.parent_path();
	}
	if (!CreateDirectoryStructure(directory))
	{
		throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.string() + '\'');
	}
	if (!AsmSaveFonts(dir))
	{
		throw std::runtime_error(std::string("Unable to save font data to \'") + directory.string() + '\'');
	}
	if (m_has_region_check)
	{
		if (!AsmSaveSystemText(dir))
		{
			throw std::runtime_error(std::string("Unable to save system text data to \'") + m_region_check_strings_filename.string() + '\'');
		}
	}
	if (!AsmSaveCompressedStringData(dir))
	{
		throw std::runtime_error(std::string("Unable to save compressed string data to \'") + m_strings_filename.string() + '\'');
	}
	if (!AsmSaveHuffmanData(dir))
	{
		throw std::runtime_error(std::string("Unable to save Huffman data to \'") + directory.string() + '\'');
	}
	if (!AsmSaveStringTables(dir))
	{
		throw std::runtime_error(std::string("Unable to save string tables to \'") + directory.string() + '\'');
	}
	if (!AsmSaveIntroStrings(dir))
	{
		throw std::runtime_error(std::string("Unable to save intro strings to \'") + directory.string() + '\'');
	}
	if (!AsmSaveEndCreditStrings(dir))
	{
		throw std::runtime_error(std::string("Unable to save end credit strings to \'") + directory.string() + '\'');
	}
	if (!AsmSaveTalkSfx(dir))
	{
		throw std::runtime_error(std::string("Unable to save talk SFX data to \'") + directory.string() + '\'');
	}
	if (!AsmSaveScriptData(dir))
	{
		throw std::runtime_error(std::string("Unable to save script data to \'") + directory.string() + '\'');
	}
	CommitAllChanges();
	return true;
}

bool StringData::Save()
{
	return Save(GetBasePath());
}

bool StringData::HasBeenModified() const
{
	auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasSavedDataChanged(); };
	if (std::any_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), pair_pred))
	{
		return true;
	}
	if (m_decompressed_strings_orig != m_decompressed_strings)
	{
		return true;
	}
	if (m_compressed_strings_orig != m_compressed_strings)
	{
		return true;
	}
	if (m_fonts_by_name_orig != m_fonts_by_name)
	{
		return true;
	}
	if (m_ui_tilemaps_orig != m_ui_tilemaps)
	{
		return true;
	}
	if (m_system_strings_orig != m_system_strings)
	{
		return true;
	}
	if (m_huffman_offsets_orig != m_huffman_offsets)
	{
		return true;
	}
	if (m_huffman_tables_orig != m_huffman_tables)
	{
		return true;
	}
	if (m_character_names_orig != m_character_names)
	{
		return true;
	}
	if (m_special_character_names_orig != m_special_character_names)
	{
		return true;
	}
	if (m_default_character_name_orig != m_default_character_name)
	{
		return true;
	}
	if (m_item_names_orig != m_item_names)
	{
		return true;
	}
	if (m_menu_strings_orig != m_menu_strings)
	{
		return true;
	}
	if (m_intro_strings_orig != m_intro_strings)
	{
		return true;
	}
	if (m_ending_strings_orig != m_ending_strings)
	{
		return true;
	}
	if (m_save_game_locations_orig != m_save_game_locations)
	{
		return true;
	}
	if (m_island_map_locations_orig != m_island_map_locations)
	{
		return true;
	}
	if (m_room_visit_flags_orig != m_room_visit_flags)
	{
		return true;
	}
	if (m_char_talk_sfx_orig != m_char_talk_sfx)
	{
		return true;
	}
	if (m_sprite_talk_sfx_orig != m_sprite_talk_sfx)
	{
		return true;
	}
	if (m_room_dialogue_table_orig != m_room_dialogue_table)
	{
		return true;
	}
	return false;
}

void StringData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	CompressStrings();
	if (m_has_region_check)
	{
		if (!RomPrepareInjectSystemText(rom))
		{
			throw std::runtime_error(std::string("Unable to prepare system text data for ROM injection"));
		}
	}
	if (!RomPrepareInjectCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare compressed strings for ROM injection"));
	}
	if (!RomPrepareInjectHuffmanData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare Huffman data for ROM injection"));
	}
	if (!RomPrepareInjectStringTables(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare string tables for ROM injection"));
	}
	if (!RomPrepareInjectIntroStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare intro strings for ROM injection"));
	}
	if (!RomPrepareInjectEndCreditStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare end credit strings for ROM injection"));
	}
	if (!RomPrepareInjectTalkSfx(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare talk sound effects for ROM injection"));
	}
	if (!RomPrepareInjectScriptData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare script data for ROM injection"));
	}
}

std::wstring StringData::GetItemDisplayName(int item) const
{
	if (Labels::Get(Labels::C_ENTITIES, item + 0xC0))
	{
		return *Labels::Get(Labels::C_ENTITIES, item + 0xC0);
	}
	else if (item < static_cast<int>(m_item_names.size()))
	{
		return m_item_names.at(item);
	}
	return StrWPrintf(L"Item%02d", item);
}

std::wstring StringData::GetCharacterDisplayName(int character) const
{
	if (Labels::Get(Labels::C_CHARACTER, character))
	{
		return *Labels::Get(Labels::C_CHARACTER, character);
	}
	else if (character < static_cast<int>(m_character_names.size()))
	{
		return m_character_names.at(character);
	}
	return m_default_character_name;
}

std::wstring StringData::GetGlobalCharacterDisplayName(int character) const
{
	if (Labels::Get(Labels::C_GLOBAL_CHARACTER, character))
	{
		return *Labels::Get(Labels::C_GLOBAL_CHARACTER, character);
	}
	else if (character < static_cast<int>(m_special_character_names.size()))
	{
		return m_special_character_names.at(character);
	}
	return StrWPrintf(L"%ls (%d)", m_default_character_name.c_str(), character);
}

std::map<std::string, std::shared_ptr<TilesetEntry>> StringData::GetAllTilesets() const
{
	return m_fonts_by_name;
}

std::vector<std::shared_ptr<TilesetEntry>> StringData::GetFonts() const
{
	std::vector<std::shared_ptr<TilesetEntry>> result;
	for (auto& t : m_fonts_by_name)
	{
		result.push_back(t.second);
	}
	return result;
}

std::map<std::string, std::shared_ptr<Tilemap2DEntry>> StringData::GetAllMaps() const
{
	return m_ui_tilemaps;
}

std::vector<std::shared_ptr<Tilemap2DEntry>> StringData::GetTextboxMaps() const
{
	std::vector<std::shared_ptr<Tilemap2DEntry>> result;
	for (auto& t : m_ui_tilemaps)
	{
		result.push_back(t.second);
	}
	return result;
}

std::size_t StringData::GetMainStringCount() const
{
	return m_decompressed_strings.size();
}

const LSString::StringType& StringData::GetMainString(std::size_t index) const
{
	assert(index < m_decompressed_strings.size());
	return m_decompressed_strings[index];
}

LSString::StringType StringData::GetOrigMainString(std::size_t index) const
{
	assert(index < m_decompressed_strings.size());
	if (index >= m_decompressed_strings_orig.size())
	{
		return LSString::StringType();
	}
	return m_decompressed_strings_orig[index];
}

void StringData::SetMainString(std::size_t index, const LSString::StringType& value)
{
	assert(index < m_decompressed_strings.size());
	m_decompressed_strings[index] = value;
}

void StringData::InsertMainString(std::size_t index, const LSString::StringType& value)
{
	assert(index <= m_decompressed_strings.size());
	m_decompressed_strings.insert(m_decompressed_strings.begin() + index, value);
}

bool StringData::HasMainStringChanged(std::size_t index) const
{
	assert(index < m_decompressed_strings.size());
	if (index >= m_decompressed_strings_orig.size())
	{
		return true;
	}
	return m_decompressed_strings_orig[index] != m_decompressed_strings[index];
}

std::size_t StringData::GetStringCount(Type type) const
{
	switch (type)
	{
	case Type::MAIN:
		return GetMainStringCount();
	case Type::INTRO:
		return GetIntroStringCount();
	case Type::NAMES:
		return GetCharNameCount();
	case Type::SPECIAL_NAMES:
		return GetSpecialCharNameCount();
	case Type::DEFAULT_NAME:
		return 1;
	case Type::ITEM_NAMES:
		return GetItemNameCount();
	case Type::MENU:
		return GetMenuStrCount();
	case Type::END_CREDITS:
		return GetEndCreditStringCount();
	case Type::SYSTEM:
		return GetSystemStringCount();
	default:
		return 0;
	}
}

const LSString::StringType& StringData::GetString(Type type, std::size_t index) const
{
	switch (type)
	{
	case Type::MAIN:
		return GetMainString(index);
	case Type::NAMES:
		return GetCharName(index);
	case Type::SPECIAL_NAMES:
		return GetSpecialCharName(index);
	case Type::DEFAULT_NAME:
		return GetDefaultCharName();
	case Type::ITEM_NAMES:
		return GetItemName(index);
	case Type::MENU:
		return GetMenuStr(index);
	case Type::SYSTEM:
		return GetSystemString(index);
	case Type::INTRO:
	case Type::END_CREDITS:
	default:
		throw std::runtime_error("Unexpected string type");
	}
}

LSString::StringType StringData::GetOrigString(Type type, std::size_t index) const
{
	switch (type)
	{
	case Type::MAIN:
		return GetOrigMainString(index);
	case Type::NAMES:
		return GetOrigCharName(index);
	case Type::SPECIAL_NAMES:
		return GetOrigSpecialCharName(index);
	case Type::DEFAULT_NAME:
		return GetOrigDefaultCharName();
	case Type::ITEM_NAMES:
		return GetOrigItemName(index);
	case Type::MENU:
		return GetOrigMenuStr(index);
	case Type::SYSTEM:
		return GetOrigSystemString(index);
	case Type::INTRO:
	case Type::END_CREDITS:
	default:
		throw std::runtime_error("Unexpected string type");
	}
}

void StringData::SetString(Type type, std::size_t index, const LSString::StringType& value)
{
	switch (type)
	{
	case Type::MAIN:
		SetMainString(index, value);
		break;
	case Type::NAMES:
		SetCharName(index, value);
		break;
	case Type::SPECIAL_NAMES:
		SetSpecialCharName(index, value);
		break;
	case Type::DEFAULT_NAME:
		SetDefaultCharName(value);
		break;
	case Type::ITEM_NAMES:
		SetItemName(index, value);
		break;
	case Type::MENU:
		SetMenuStr(index, value);
		break;
	case Type::SYSTEM:
		SetSystemString(index, value);
		break;
	case Type::INTRO:
	case Type::END_CREDITS:
	default:
		break;
	}
}

void StringData::InsertString(Type type, std::size_t index, const LSString::StringType& value)
{
	switch (type)
	{
	case Type::MAIN:
		assert(index <= m_decompressed_strings.size());
		m_decompressed_strings.insert(m_decompressed_strings.begin() + index, value);
		break;
	case Type::NAMES:
		assert(index <= m_character_names.size());
		m_character_names.insert(m_character_names.begin() + index, value);
		break;
	case Type::SPECIAL_NAMES:
		assert(index <= m_special_character_names.size());
		m_special_character_names.insert(m_special_character_names.begin() + index, value);
		m_char_talk_sfx.insert(m_char_talk_sfx.begin() + index, 0);
		break;
	case Type::ITEM_NAMES:
		assert(index <= m_item_names.size());
		m_item_names.insert(m_item_names.begin() + index, value);
		break;
	case Type::MENU:
		assert(index <= m_menu_strings.size());
		m_menu_strings.insert(m_menu_strings.begin() + index, value);
		break;
	case Type::INTRO:
		assert(index <= m_intro_strings.size());
		m_intro_strings.insert(m_intro_strings.begin() + index, IntroString(0, 0, 0, 0, 0, value, L""));
		break;
	case Type::END_CREDITS:
		assert(index <= m_ending_strings.size());
		m_ending_strings.insert(m_ending_strings.begin() + index, EndCreditString(0, 0, value));
		break;
	case Type::SYSTEM:
	case Type::DEFAULT_NAME:
	default:
		break;
	}
}

void StringData::DeleteString(Type type, std::size_t index)
{
	switch (type)
	{
	case Type::MAIN:
		assert(index < m_decompressed_strings.size());
		m_decompressed_strings.erase(m_decompressed_strings.begin() + index);
		break;
	case Type::NAMES:
		assert(index < m_character_names.size());
		m_character_names.erase(m_character_names.begin() + index);
		break;
	case Type::SPECIAL_NAMES:
		assert(index < m_special_character_names.size());
		m_special_character_names.erase(m_special_character_names.begin() + index);
		m_char_talk_sfx.erase(m_char_talk_sfx.begin() + index);
		break;
	case Type::ITEM_NAMES:
		assert(index < m_item_names.size());
		m_item_names.erase(m_item_names.begin() + index);
		break;
	case Type::MENU:
		assert(index < m_menu_strings.size());
		m_menu_strings.erase(m_menu_strings.begin() + index);
		break;
	case Type::INTRO:
		assert(index < m_intro_strings.size());
		m_intro_strings.erase(m_intro_strings.begin() + index);
		break;
	case Type::END_CREDITS:
		assert(index < m_ending_strings.size());
		m_ending_strings.erase(m_ending_strings.begin() + index);
		break;
	case Type::SYSTEM:
	case Type::DEFAULT_NAME:
	default:
		break;
	}
}

void StringData::SwapStrings(Type type, std::size_t i1, std::size_t i2)
{
	switch (type)
	{
	case Type::MAIN:
		assert(i1 < m_decompressed_strings.size() && i2 < m_decompressed_strings.size());
		std::iter_swap(m_decompressed_strings.begin() + i1, m_decompressed_strings.begin() + i2);
		break;
	case Type::NAMES:
		assert(i1 < m_character_names.size() && i2 < m_character_names.size());
		std::iter_swap(m_character_names.begin() + i1, m_character_names.begin() + i2);
		break;
	case Type::SPECIAL_NAMES:
		assert(i1 < m_special_character_names.size() && i2 < m_special_character_names.size());
		std::iter_swap(m_special_character_names.begin() + i1, m_special_character_names.begin() + i2);
		std::iter_swap(m_char_talk_sfx.begin() + i1, m_char_talk_sfx.begin() + i2);
		break;
	case Type::ITEM_NAMES:
		assert(i1 < m_item_names.size() && i2 < m_item_names.size());
		std::iter_swap(m_item_names.begin() + i1, m_item_names.begin() + i2);
		break;
	case Type::MENU:
		assert(i1 < m_menu_strings.size() && i2 < m_menu_strings.size());
		std::iter_swap(m_menu_strings.begin() + i1, m_menu_strings.begin() + i2);
		break;
	case Type::INTRO:
		assert(i1 < m_intro_strings.size() && i2 < m_intro_strings.size());
		std::iter_swap(m_intro_strings.begin() + i1, m_intro_strings.begin() + i2);
		break;
	case Type::END_CREDITS:
		assert(i1 < m_ending_strings.size() && i2 < m_ending_strings.size());
		std::iter_swap(m_ending_strings.begin() + i1, m_ending_strings.begin() + i2);
		break;
	case Type::SYSTEM:
		assert(i1 < m_system_strings.size() && i2 < m_system_strings.size());
		std::iter_swap(m_system_strings.begin() + i1, m_system_strings.begin() + i2);
		break;
	case Type::DEFAULT_NAME:
	default:
		break;
	}
}

bool StringData::HasStringChanged(Type type, std::size_t index) const
{
	switch (type)
	{
	case Type::MAIN:
		return HasMainStringChanged(index);
	case Type::INTRO:
		return HasIntroStringChanged(index);
	case Type::NAMES:
		return HasCharNameChanged(index);
	case Type::SPECIAL_NAMES:
		return HasSpecialCharNameChanged(index);
	case Type::DEFAULT_NAME:
		return HasDefaultCharNameChanged();
	case Type::ITEM_NAMES:
		return HasItemNameChanged(index);
	case Type::MENU:
		return HasMenuStrChanged(index);
	case Type::END_CREDITS:
		return HasEndCreditStringChanged(index);
	case Type::SYSTEM:
		return HasSystemStringChanged(index);
	default:
		return false;
	}
}

std::size_t StringData::GetSystemStringCount() const
{
	return m_has_region_check ? m_system_strings.size() : 0;
}

const LSString::StringType& StringData::GetSystemString(std::size_t index) const
{
	assert(index < m_system_strings.size());
	return m_system_strings[index];
}

LSString::StringType StringData::GetOrigSystemString(std::size_t index) const
{
	assert(index < m_system_strings.size());
	if (index >= m_system_strings_orig.size())
	{
		return LSString::StringType();
	}
	return m_system_strings_orig[index];
}

void StringData::SetSystemString(std::size_t index, const LSString::StringType& value)
{
	assert(index < m_system_strings.size());
	m_system_strings[index] = value;
}

bool StringData::HasSystemStringChanged(std::size_t index) const
{
	assert(index <= m_system_strings.size());
	if (index >= m_system_strings_orig.size())
	{
		return true;
	}
	return m_system_strings_orig[index] != m_system_strings[index];
}

std::size_t StringData::GetCharNameCount() const
{
	return m_character_names.size();
}

const LSString::StringType& StringData::GetCharName(std::size_t index) const
{
	assert(index < m_character_names.size());
	return m_character_names[index];
}

LSString::StringType StringData::GetOrigCharName(std::size_t index) const
{
	assert(index < m_character_names.size());
	if (index >= m_character_names_orig.size())
	{
		return LSString::StringType();
	}
	return m_character_names_orig[index];
}

void StringData::SetCharName(std::size_t index, const LSString::StringType& value)
{
	assert(index < m_character_names.size());
	m_character_names[index] = value;
}

bool StringData::HasCharNameChanged(std::size_t index) const
{
	assert(index <= m_character_names.size());
	if (index >= m_character_names_orig.size())
	{
		return true;
	}
	return m_character_names_orig[index] != m_character_names[index];
}

std::size_t StringData::GetSpecialCharNameCount() const
{
	return m_special_character_names.size();
}

const LSString::StringType& StringData::GetSpecialCharName(std::size_t index) const
{
	assert(index < m_special_character_names.size());
	return m_special_character_names[index];
}

LSString::StringType StringData::GetOrigSpecialCharName(std::size_t index) const
{
	assert(index < m_special_character_names.size());
	if (index >= m_special_character_names_orig.size())
	{
		return LSString::StringType();
	}
	return m_special_character_names_orig[index];
}

void StringData::SetSpecialCharName(std::size_t index, const LSString::StringType& value)
{
	assert(index < m_special_character_names.size());
	m_special_character_names[index] = value;
}

bool StringData::HasSpecialCharNameChanged(std::size_t index) const
{
	assert(index <= m_special_character_names.size());
	if (index >= m_special_character_names_orig.size())
	{
		return true;
	}
	return m_special_character_names_orig[index] != m_special_character_names[index];
}

uint8_t StringData::GetSpecialCharTalkSfx(std::size_t index) const
{
	assert(index < m_char_talk_sfx.size());
	return m_char_talk_sfx.at(index);
}

void StringData::SetSpecialCharTalkSfx(std::size_t index, uint8_t sound)
{
	assert(index < m_char_talk_sfx.size());
	m_char_talk_sfx[index] = sound;
}

std::string StringData::ExportSpecialCharTalkSfxYaml() const
{
	std::wostringstream ss;
	ss << "SpecialCharTalkSfx:" << std::endl;
	for (std::size_t i = 0; i < m_char_talk_sfx.size(); ++i)
	{
		ss << L"  " << i << ": " << StrWPrintf(L"0x%02X", m_char_talk_sfx.at(i)) << "  # "
		   << m_special_character_names.at(i) << ": "
		   << Labels::Get(Labels::C_SOUNDS, m_char_talk_sfx.at(i)).value_or(std::to_wstring(i)) << std::endl;
	}
	return wstr_to_utf8(ss.str());
}

void StringData::ImportSpecialCharTalkSfxYaml(std::string yaml)
{
	try
	{
		YAML::Node node = YAML::Load(yaml);
		std::fill(m_char_talk_sfx.begin(), m_char_talk_sfx.end(), 0_u8);
		auto charlist = node.begin()->second;
		for (const auto& val : charlist)
		{
			uint8_t character = val.first.as<uint8_t>();
			uint8_t sound =  val.second.as<uint8_t>();
			if (character < m_char_talk_sfx.size())
			{
				m_char_talk_sfx[character] = sound;
			}
		}
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(e.what());
	}
}

const LSString::StringType& StringData::GetDefaultCharName() const
{
	return m_default_character_name;
}

const LSString::StringType& StringData::GetOrigDefaultCharName() const
{
	return m_default_character_name_orig;
}

void StringData::SetDefaultCharName(const LSString::StringType& value)
{
	m_default_character_name = value;
}

bool StringData::HasDefaultCharNameChanged() const
{
	return m_default_character_name_orig != m_default_character_name;
}

std::size_t StringData::GetItemNameCount() const
{
	return m_item_names.size();
}

const LSString::StringType& StringData::GetItemName(std::size_t index) const
{
	assert(index < m_item_names.size());
	return m_item_names[index];
}

LSString::StringType StringData::GetOrigItemName(std::size_t index) const
{
	assert(index < m_item_names.size());
	if (index >= m_item_names_orig.size())
	{
		return LSString::StringType();
	}
	return m_item_names_orig[index];
}

void StringData::SetItemName(std::size_t index, const LSString::StringType& value)
{
	assert(index < m_item_names.size());
	m_item_names[index] = value;
}

bool StringData::HasItemNameChanged(std::size_t index) const
{
	assert(index <= m_item_names.size());
	if (index >= m_item_names_orig.size())
	{
		return true;
	}
	return m_item_names_orig[index] != m_item_names[index];
}

std::size_t StringData::GetMenuStrCount() const
{
	return m_menu_strings.size();
}

const LSString::StringType& StringData::GetMenuStr(std::size_t index) const
{
	assert(index < m_menu_strings.size());
	return m_menu_strings[index];
}

LSString::StringType StringData::GetOrigMenuStr(std::size_t index) const
{
	assert(index < m_menu_strings.size());
	if (index >= m_menu_strings_orig.size())
	{
		return LSString::StringType();
	}
	return m_menu_strings_orig[index];
}

void StringData::SetMenuStr(std::size_t index, const LSString::StringType& value)
{
	assert(index < m_menu_strings.size());
	m_menu_strings[index] = value;
}

bool StringData::HasMenuStrChanged(std::size_t index) const
{
	assert(index <= m_menu_strings.size());
	if (index >= m_menu_strings_orig.size())
	{
		return true;
	}
	return m_menu_strings_orig[index] != m_menu_strings[index];
}

std::size_t StringData::GetIntroStringCount() const
{
	return m_intro_strings.size();
}

const IntroString& StringData::GetIntroString(std::size_t index) const
{
	assert(index < m_intro_strings.size());
	return m_intro_strings[index];
}

IntroString StringData::GetOrigIntroString(std::size_t index) const
{
	assert(index < m_intro_strings.size());
	if (index >= m_intro_strings_orig.size())
	{
		return IntroString();
	}
	return m_intro_strings_orig[index];
}

void StringData::SetIntroString(std::size_t index, const IntroString& value)
{
	assert(index < m_intro_strings.size());
	m_intro_strings[index] = value;
}

bool StringData::HasIntroStringChanged(std::size_t index) const
{
	assert(index <= m_intro_strings.size());
	if (index >= m_intro_strings_orig.size())
	{
		return true;
	}
	return m_intro_strings_orig[index] != m_intro_strings[index];
}

std::size_t StringData::GetEndCreditStringCount() const
{
	return m_ending_strings.size();
}

const EndCreditString& StringData::GetEndCreditString(std::size_t index) const
{
	assert(index < m_ending_strings.size());
	return m_ending_strings[index];
}

EndCreditString StringData::GetOrigEndCreditString(std::size_t index) const
{
	assert(index < m_ending_strings.size());
	if (index >= m_ending_strings_orig.size())
	{
		return EndCreditString();
	}
	return m_ending_strings_orig[index];
}

void StringData::SetEndCreditString(std::size_t index, const EndCreditString& value)
{
	assert(index < m_ending_strings.size());
	m_ending_strings[index] = value;
}

bool StringData::HasEndCreditStringChanged(std::size_t index) const
{
	assert(index <= m_ending_strings.size());
	if (index >= m_ending_strings_orig.size())
	{
		return true;
	}
	return m_ending_strings_orig[index] != m_ending_strings[index];
}

uint16_t StringData::GetRoomVisitFlag(uint16_t room) const
{
	if (room < m_room_visit_flags.size())
	{
		return m_room_visit_flags.at(room);
	}
	return 0xFFFF;
}

void StringData::SetRoomVisitFlag(uint16_t room, uint16_t flag)
{
	if (room < m_room_visit_flags.size())
	{
		m_room_visit_flags.resize(room + 1);
	}
	m_room_visit_flags[room] = flag;
}

std::vector<uint16_t> StringData::GetRoomCharacters(uint16_t room) const
{
	return m_room_dialogue_table.GetRoomCharacters(room);
}

void StringData::SetRoomCharacters(uint16_t room, const std::vector<uint16_t>& characters)
{
	m_room_dialogue_table.SetRoomCharacters(room, characters);
}

uint8_t StringData::GetSaveLocation(uint16_t room)
{
	auto loc = m_save_game_locations.find(room);
	if (loc != m_save_game_locations.cend())
	{
		return loc->second.second;
	}
	return 0xFF;
}

void StringData::SetSaveLocation(uint16_t room, uint8_t name)
{
	auto loc = m_save_game_locations.find(room);
	if (loc != m_save_game_locations.cend())
	{
		if (name == 0xFF)
		{
			m_save_game_locations.erase(room);
		}
		else
		{
			m_save_game_locations[room].second = name;
		}
	}
	else
	{
		m_save_game_locations.insert({ room, {0_u8, name} });
	}
}

uint8_t StringData::GetMapLocation(uint16_t room)
{
	auto loc = m_island_map_locations.find(room);
	if (loc != m_island_map_locations.cend())
	{
		return loc->second.first;
	}
	return 0xFF;
}

uint8_t StringData::GetMapPosition(uint16_t room)
{
	auto loc = m_island_map_locations.find(room);
	if (loc != m_island_map_locations.cend())
	{
		return loc->second.second;
	}
	return 0xFF;
}

void StringData::SetMapLocation(uint16_t room, uint8_t name, uint8_t position)
{
	if (position == 0xFF)
	{
		return;
	}
	auto loc = m_island_map_locations.find(room);
	if (loc != m_island_map_locations.cend())
	{
		if (name == 0xFF)
		{
			m_island_map_locations.erase(room);
		}
		else
		{
			m_island_map_locations[room].first = name;
			m_island_map_locations[room].second = position;
		}
	}
	else
	{
		m_island_map_locations.insert({ room, {name, position} });
	}
}

uint8_t StringData::GetEntityTalkSound(uint8_t entity_id) const
{
	if (m_sprite_talk_sfx.count(entity_id) > 0)
	{
		return m_sprite_talk_sfx.at(entity_id);
	}
	else
	{
		return 0;
	}
}

void StringData::SetEntityTalkSound(uint8_t entity_id, uint8_t sound)
{
	if (sound == 0)
	{
		if (m_sprite_talk_sfx.count(entity_id) > 0)
		{
			m_sprite_talk_sfx.erase(entity_id);
		}
	}
	else
	{
		m_sprite_talk_sfx[entity_id] = sound;
	}
}

uint8_t StringData::GetCharacterTalkSound(uint8_t character_id) const
{
	if (character_id < static_cast<int>(m_char_talk_sfx.size()))
	{
		return m_char_talk_sfx.at(character_id);
	}
	else
	{
		return 0;
	}
}

void StringData::SetCharacterTalkSound(uint8_t character_id, uint8_t sound)
{
	if (character_id < static_cast<int>(m_char_talk_sfx.size()))
	{
		m_char_talk_sfx.at(character_id) = sound;
	}
}

void StringData::CommitAllChanges()
{
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	std::for_each(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_commit);
	std::for_each(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), pair_commit);
	m_fonts_by_name_orig = m_fonts_by_name;
	m_ui_tilemaps_orig = m_ui_tilemaps;
	m_system_strings_orig = m_system_strings;
	m_decompressed_strings_orig = m_decompressed_strings;
	m_compressed_strings_orig = m_compressed_strings;
	m_huffman_offsets_orig = m_huffman_offsets;
	m_huffman_tables_orig = m_huffman_tables;
	m_character_names_orig = m_character_names;
	m_special_character_names_orig = m_special_character_names;
	m_default_character_name_orig = m_default_character_name;
	m_item_names_orig = m_item_names;
	m_menu_strings_orig = m_menu_strings;
	m_save_game_locations_orig = m_save_game_locations;
	m_island_map_locations_orig = m_island_map_locations;
	m_intro_strings_orig = m_intro_strings;
	m_room_visit_flags_orig = m_room_visit_flags;
	m_ending_strings_orig = m_ending_strings;
	m_char_talk_sfx_orig = m_char_talk_sfx;
	m_sprite_talk_sfx_orig = m_sprite_talk_sfx;
	m_room_dialogue_table_orig = m_room_dialogue_table;
	m_pending_writes.clear();
}

bool StringData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().string());

		if (f.IsGood() && f.LabelExists(RomLabels::Strings::REGION_CHECK))
		{
			m_has_region_check = true;
			retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::REGION_CHECK, m_region_check_filename);
			AsmFile r(GetBasePath() / m_region_check_filename);
			if (r.IsGood())
			{
				retval = retval && GetFilenameFromAsm(r, RomLabels::Strings::REGION_CHECK_ROUTINE, m_region_check_routine_filename);
				retval = retval && GetFilenameFromAsm(r, RomLabels::Strings::REGION_CHECK_STRINGS, m_region_check_strings_filename);
				retval = retval && GetFilenameFromAsm(r, RomLabels::Graphics::SYS_FONT, m_system_font_filename);
			}
		}

		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::STRING_SECTION, m_strings_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::STRING_BANK_PTR_DATA, m_string_ptr_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::HUFFMAN_OFFSETS, m_huffman_offset_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::HUFFMAN_TABLES, m_huffman_table_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::STRING_TABLE_DATA, m_string_table_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::SAVE_GAME_LOCATIONS, m_save_loc_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::ISLAND_MAP_LOCATIONS, m_map_loc_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::INTRO_STRING_DATA, m_intro_string_data_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Rooms::ROOM_VISIT_FLAGS, m_room_visit_flags_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::END_CREDIT_STRINGS, m_end_credit_strings_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::CHARACTER_TALK_SFX, m_char_talk_sfx_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::SPRITE_TALK_SFX, m_sprite_talk_sfx_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Strings::ROOM_CHARACTER_TABLE, m_room_dialogue_table_path);
		
		AsmFile s(GetBasePath() / m_string_table_path);
		retval = retval && GetFilenameFromAsm(s, RomLabels::Strings::CHAR_NAME_TABLE, m_char_table_path);
		retval = retval && GetFilenameFromAsm(s, RomLabels::Strings::SPECIAL_CHAR_NAME_TABLE, m_schar_table_path);
		retval = retval && GetFilenameFromAsm(s, RomLabels::Strings::DEFAULT_NAME, m_dchar_table_path);
		retval = retval && GetFilenameFromAsm(s, RomLabels::Strings::ITEM_NAME_TABLE, m_item_table_path);
		retval = retval && GetFilenameFromAsm(s, RomLabels::Strings::MENU_STRING_TABLE, m_menu_table_path);
		AsmFile i(GetBasePath() / m_intro_string_data_path);
		retval = retval && GetFilenameFromAsm(i, RomLabels::Strings::INTRO_STRING_PTRS, m_intro_string_ptrtable_path);
		int idx = 1;
		while (i.IsGood())
		{
			auto label = StrPrintf(RomLabels::Strings::INTRO_STRING, idx++);
			std::filesystem::path path;
			retval = retval && GetFilenameFromAsm(i, label, path);
			m_intro_strings_path.push_back(path);
		}
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void StringData::SetDefaultFilenames()
{
	if (m_region_check_filename.empty()) m_region_check_filename = RomLabels::Strings::REGION_CHECK_FILE;
	if (m_region_check_routine_filename.empty()) m_region_check_routine_filename = RomLabels::Strings::REGION_CHECK_ROUTINE_FILE;
	if (m_region_check_strings_filename.empty()) m_region_check_strings_filename = RomLabels::Strings::REGION_CHECK_STRINGS_FILE;
	if (m_system_font_filename.empty()) m_system_font_filename = RomLabels::Graphics::SYS_FONT_FILE;
	if (m_strings_filename.empty()) m_strings_filename = RomLabels::Strings::STRINGS_FILE;
	if (m_string_ptr_filename.empty()) m_string_ptr_filename = RomLabels::Strings::STRING_BANK_PTR_FILE;
	if (m_string_filename_path.empty()) m_string_filename_path = std::filesystem::path(RomLabels::Strings::STRING_BANK_FILE).parent_path();
	if (m_huffman_offset_path.empty()) m_huffman_offset_path = RomLabels::Strings::HUFFMAN_OFFSETS_FILE;
	if (m_huffman_table_path.empty()) m_huffman_table_path = RomLabels::Strings::HUFFMAN_TABLE_FILE;
	if (m_string_table_path.empty()) m_string_table_path = RomLabels::Strings::STRING_TABLE_DATA_FILE;
	if (m_save_loc_path.empty()) m_save_loc_path = RomLabels::Strings::SAVE_GAME_LOCATIONS_FILE;
	if (m_map_loc_path.empty()) m_map_loc_path = RomLabels::Strings::ISLAND_MAP_LOCATIONS_FILE;
	if (m_char_table_path.empty()) m_char_table_path = RomLabels::Strings::CHAR_NAME_TABLE_FILE;
	if (m_schar_table_path.empty()) m_schar_table_path = RomLabels::Strings::SPECIAL_CHAR_NAME_TABLE_FILE;
	if (m_dchar_table_path.empty()) m_dchar_table_path = RomLabels::Strings::DEFAULT_NAME_FILE;
	if (m_item_table_path.empty()) m_item_table_path = RomLabels::Strings::ITEM_NAME_TABLE_FILE;
	if (m_menu_table_path.empty()) m_menu_table_path = RomLabels::Strings::MENU_STRING_TABLE_FILE;
	if (m_intro_string_data_path.empty()) m_intro_string_data_path = RomLabels::Strings::INTRO_STRING_DATA_FILE;
	if (m_intro_string_ptrtable_path.empty()) m_intro_string_ptrtable_path = RomLabels::Strings::INTRO_STRING_PTRS_FILE;
	if (m_room_visit_flags_path.empty()) m_room_visit_flags_path = RomLabels::Rooms::ROOM_VISIT_FLAGS_FILE;
	if (m_end_credit_strings_path.empty()) m_end_credit_strings_path = RomLabels::Strings::END_CREDIT_STRINGS_FILE;
	if (m_char_talk_sfx_path.empty()) m_char_talk_sfx_path = RomLabels::Strings::CHARACTER_TALK_SFX_FILE;
	if (m_sprite_talk_sfx_path.empty()) m_sprite_talk_sfx_path = RomLabels::Strings::SPRITE_TALK_SFX_FILE;
	if (m_room_dialogue_table_path.empty()) m_room_dialogue_table_path = RomLabels::Strings::ROOM_DIALOGUE_TABLE_FILE;
}

bool StringData::CreateDirectoryStructure(const std::filesystem::path& dir)
{
	bool retval = true;

	retval = retval && CreateDirectoryTree(dir / m_region_check_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_routine_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_system_font_filename);
	retval = retval && CreateDirectoryTree(dir / m_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_ptr_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_filename_path / "x");
	retval = retval && CreateDirectoryTree(dir / m_huffman_offset_path);
	retval = retval && CreateDirectoryTree(dir / m_huffman_table_path);
	retval = retval && CreateDirectoryTree(dir / m_string_table_path);
	retval = retval && CreateDirectoryTree(dir / m_save_loc_path);
	retval = retval && CreateDirectoryTree(dir / m_map_loc_path);
	retval = retval && CreateDirectoryTree(dir / m_char_table_path);
	retval = retval && CreateDirectoryTree(dir / m_schar_table_path);
	retval = retval && CreateDirectoryTree(dir / m_dchar_table_path);
	retval = retval && CreateDirectoryTree(dir / m_item_table_path);
	retval = retval && CreateDirectoryTree(dir / m_menu_table_path);
	retval = retval && CreateDirectoryTree(dir / m_intro_string_data_path);
	retval = retval && CreateDirectoryTree(dir / m_intro_string_ptrtable_path);
	retval = retval && CreateDirectoryTree(dir / m_room_visit_flags_path);
	retval = retval && CreateDirectoryTree(dir / m_end_credit_strings_path);
	retval = retval && CreateDirectoryTree(dir / m_char_talk_sfx_path);
	retval = retval && CreateDirectoryTree(dir / m_sprite_talk_sfx_path);
	retval = retval && CreateDirectoryTree(dir / m_room_dialogue_table_path);
	for (const auto& f : m_fonts_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& g : m_ui_tilemaps)
	{
		retval = retval && CreateDirectoryTree(dir / g.second->GetFilename());
	}
	for (const auto& p : m_intro_strings_path)
	{
		retval = retval && CreateDirectoryTree(dir / p);
	}

	return retval;
}

void StringData::InitCache()
{
	m_ui_tilemaps_orig = m_ui_tilemaps;
	m_fonts_by_name_orig = m_fonts_by_name;
	m_system_strings_orig = m_system_strings;
	m_compressed_strings_orig = m_compressed_strings;
	m_decompressed_strings_orig = m_decompressed_strings;
	m_huffman_offsets_orig = m_huffman_offsets;
	m_huffman_tables_orig = m_huffman_tables;
	m_decompressed_strings_orig = m_decompressed_strings;
	m_save_game_locations_orig = m_save_game_locations;
	m_island_map_locations_orig = m_island_map_locations;
	m_character_names_orig = m_character_names;
	m_special_character_names_orig = m_special_character_names;
	m_default_character_name_orig = m_default_character_name;
	m_item_names_orig = m_item_names;
	m_menu_strings_orig = m_menu_strings;
	m_intro_strings_orig = m_intro_strings;
	m_room_visit_flags_orig = m_room_visit_flags;
	m_ending_strings_orig = m_ending_strings;
	m_char_talk_sfx_orig = m_char_talk_sfx;
	m_sprite_talk_sfx_orig = m_sprite_talk_sfx;
	m_room_dialogue_table_orig = m_room_dialogue_table;
}

bool StringData::DecompressStrings()
{
	auto huff_trees = std::make_shared<HuffmanTrees>(m_huffman_offsets.data(), m_huffman_offsets.size(), m_huffman_tables.data(), m_huffman_tables.size(), m_huffman_offsets.size() / 2);
	const auto& charset = Charset::GetDefaultCharset(m_region);
	auto eos_marker = Charset::GetEOSChar(m_region);
	const auto& diacritic_map = Charset::GetDiacriticMap(m_region);
	auto decoder = HuffmanString(huff_trees, charset, eos_marker, diacritic_map);
	m_decompressed_strings.clear();
	for (const auto& s : m_compressed_strings)
	{
		decoder.Decode(s.data(), s[0]);
		m_decompressed_strings.push_back(decoder.Str());
	}
	return true;
}

bool StringData::CompressStrings()
{
	if (m_decompressed_strings_orig != m_decompressed_strings)
	{
		auto huff_trees = std::make_shared<HuffmanTrees>();
		const auto& charset = Charset::GetDefaultCharset(m_region);
		auto eos_marker = Charset::GetEOSChar(m_region);
		const auto& diacritic_map = Charset::GetDiacriticMap(m_region);
		m_compressed_strings.clear();
		std::vector<std::shared_ptr<LSString>> strs;
		for (const auto& s : m_decompressed_strings)
		{
			strs.push_back(std::make_shared<HuffmanString>(s, huff_trees, charset, eos_marker, diacritic_map));
		}
		huff_trees->RecalculateTrees(strs);
		huff_trees->EncodeTrees(m_huffman_offsets, m_huffman_tables);
		for (const auto& s : strs)
		{
			m_compressed_strings.push_back(ByteVector(256));
			auto sz = s->Encode(m_compressed_strings.back().data(), m_compressed_strings.back().size());
			m_compressed_strings.back().resize(sz);
		}
	}
	return true;
}

bool StringData::DecodeStrings(const std::vector<uint8_t>& bytes, std::vector<LSString::StringType>& strings)
{
	const auto& charset = Charset::GetDefaultCharset(m_region);
	const auto& diacritic_map = Charset::GetDiacriticMap(m_region);
	auto decoder = LSString(charset, diacritic_map);
	uint8_t const* p = bytes.data();
	uint8_t next = *p;
	while ((p + next) < (bytes.data() + bytes.size()) && next != 0xFF)
	{
		decoder.Decode(p, next + 1);
		strings.push_back(decoder.Str());
		p += next + 1;
		next = *p;
	}
	return true;
}

bool StringData::DecodeString(const std::vector<uint8_t>& bytes, LSString::StringType& string)
{
	const auto& charset = Charset::GetDefaultCharset(m_region);
	const auto& diacritic_map = Charset::GetDiacriticMap(m_region);
	auto decoder = LSString(charset, diacritic_map);
	decoder.Decode(bytes.data(), bytes.size());
	string = decoder.Str();
	return true;
}

bool StringData::EncodeStrings(const std::vector<LSString::StringType>& strings, std::vector<uint8_t>& bytes)
{
	const auto& charset = Charset::GetDefaultCharset(m_region);
	const auto& diacritic_map = Charset::GetDiacriticMap(m_region);
	auto encoder = LSString(charset, diacritic_map);
	bytes.resize(65536);
	std::size_t offset = 0;
	for (const auto& s : strings)
	{
		encoder.Deserialise(s);
		offset += encoder.Encode(bytes.data() + offset, bytes.size() - offset);
	}
	bytes.resize(offset);
	return true;
}

bool StringData::EncodeString(const LSString::StringType& string, std::vector<uint8_t>& bytes)
{
	const auto& charset = Charset::GetDefaultCharset(m_region);
	const auto& diacritic_map = Charset::GetDiacriticMap(m_region);
	auto encoder = LSString(charset, diacritic_map);
	bytes.resize(256);
	encoder.Deserialise(string);
	bytes.resize(encoder.Encode(bytes.data(), bytes.size()));
	return true;
}

std::map<uint16_t, std::pair<uint8_t, uint8_t>> StringData::DeserialiseLocationMap(const std::vector<uint8_t>& bytes)
{
	std::map<uint16_t, std::pair<uint8_t, uint8_t>> result;
	for (std::size_t i = 0; i < bytes.size(); i += 4)
	{
		uint8_t p1 = bytes[i];
		uint8_t p2 = bytes[i + 1];
		uint16_t room = bytes[i + 2] << 8 | bytes[i + 3];
		result[room] = { p1, p2 };
		if (room == 0xFFFF)
		{
			break;
		}
	}
	return result;
}

std::map<uint8_t, uint8_t> StringData::DeserialiseSfxMap(const ByteVector& bytes)
{
	std::map<uint8_t, uint8_t> result;

	assert((bytes.size() & 1) == 0);
	for (std::size_t i = 0; i < bytes.size(); i += 2)
	{
		if (bytes[i] == 0xFF || bytes[i + 1] == 0xFF)
		{
			break;
		}
		result.insert({ bytes[i], bytes[i + 1] });
	}

	return result;
}

ByteVector StringData::SerialiseSfxMap(const std::map<uint8_t, uint8_t>& map)
{
	ByteVector result;

	result.reserve(map.size() * 2 + 2);
	for ( const auto& elem : map )
	{
		result.push_back(elem.first);
		result.push_back(elem.second);
	}
	result.push_back(0xFF);
	result.push_back(0xFF);

	return result;
}

std::vector<uint8_t> StringData::SerialiseLocationMap(const std::map<uint16_t, std::pair<uint8_t, uint8_t>>& locs)
{
	std::vector<uint8_t> result;
	result.reserve(locs.size() * 4);
	for (const auto& v : locs)
	{
		result.push_back(v.second.first);
		result.push_back(v.second.second);
		result.push_back(v.first >> 8);
		result.push_back(v.first & 0xFF);
	}
	return result;
}

void StringData::DeserialiseVisitFlags(const std::vector<uint8_t>& bytes)
{
	for (std::size_t i = 0; i < bytes.size(); i += 2)
	{
		uint16_t room = bytes[i] << 8 | bytes[i + 1];
		if (room > 0x7FFF)
		{
			break;
		}
		m_room_visit_flags.push_back(room);
	}
}

std::vector<uint8_t> StringData::SerialiseVisitFlags()
{
	std::vector<uint8_t> result;
	result.reserve(m_room_visit_flags.size() * 2);
	for (const auto& v : m_room_visit_flags)
	{
		result.push_back(v >> 8);
		result.push_back(v & 0xFF);
	}
	return result;
}

uint32_t StringData::GetCharsetSize() const
{
	const auto& font = m_fonts_internal.at(RomLabels::Graphics::MAIN_FONT);
	const auto& orig_font = m_fonts_by_name.at(font->GetName());
	return orig_font->GetData()->GetTileCount();
}

bool StringData::AsmLoadSystemFont()
{
	std::filesystem::path path = GetBasePath() / m_system_font_filename;
	auto e = TilesetEntry::Create(this, ReadBytes(path), RomLabels::Graphics::SYS_FONT, m_system_font_filename, false, 8, 8);
	m_fonts_by_name.insert({ RomLabels::Graphics::SYS_FONT, e });
	m_fonts_internal.insert({ RomLabels::Graphics::SYS_FONT, e });
	return true;
}

bool StringData::AsmLoadSystemStrings()
{
	try
	{
		AsmFile file(GetBasePath() / m_region_check_strings_filename);
		auto read_str = [&](std::string& s)
		{
			char c = '\0';
			do
			{
				file >> c;
				if (c != 0)
				{
					s += c;
				}
			} while (c != 0);
		};

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string line;
		file.Goto(RomLabels::Strings::REGION_ERROR_LINE1);
		read_str(line);
		m_system_strings[0] = converter.from_bytes(line);
		line.clear();
		file.Goto(RomLabels::Strings::REGION_ERROR_NTSC);
		read_str(line);
		m_system_strings[1] = converter.from_bytes(line);
		line.clear();
		file.Goto(RomLabels::Strings::REGION_ERROR_PAL);
		read_str(line);
		m_system_strings[2] = converter.from_bytes(line);
		line.clear();
		file.Goto(RomLabels::Strings::REGION_ERROR_LINE3);
		read_str(line);
		m_system_strings[3] = converter.from_bytes(line);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmLoadCompressedStringData()
{
	try
	{
		AsmFile file(GetBasePath() / m_strings_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		file >> name >> inc;
		auto font = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, false, 16, 15, 1);
		m_fonts_by_name.insert({ font->GetName(), font });
		m_fonts_internal.insert({ RomLabels::Graphics::MAIN_FONT, font });
		while (file.IsGood())
		{
			file >> name >> inc;
			m_string_filename_path = inc.path.parent_path();
			auto bytes = ReadBytes(GetBasePath() / inc.path);
			auto it = bytes.cbegin();
			while (it != bytes.cend() && *it != 0x00)
			{
				m_compressed_strings.push_back(ByteVector(it, it + *it));
				it += *it;
			}
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmLoadHuffmanData()
{
	std::filesystem::path path = GetBasePath() / m_huffman_offset_path;
	m_huffman_offsets = ReadBytes(path);
	path = GetBasePath() / m_huffman_table_path;
	m_huffman_tables = ReadBytes(path);
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomLabels::Graphics::TEXTBOX_2LINE_MAP);
		file >> inc;
		auto tb2 = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::TEXTBOX_2LINE_MAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 6);
		file.Goto(RomLabels::Graphics::TEXTBOX_3LINE_MAP);
		file >> inc;
		auto tb3 = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::TEXTBOX_3LINE_MAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 8);
		m_ui_tilemaps.insert({ tb2->GetName(), tb2 });
		m_ui_tilemaps_internal.insert({ tb2->GetName(), tb2 });
		m_ui_tilemaps.insert({ tb3->GetName(), tb3 });
		m_ui_tilemaps_internal.insert({ tb3->GetName(), tb3 });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmLoadStringTables()
{
	m_save_game_locations = DeserialiseLocationMap(ReadBytes(GetBasePath() / m_save_loc_path));
	m_island_map_locations = DeserialiseLocationMap(ReadBytes(GetBasePath() / m_map_loc_path));
	bool retval = DecodeStrings(ReadBytes(GetBasePath() / m_char_table_path), m_character_names);
	retval = retval && DecodeStrings(ReadBytes(GetBasePath() / m_schar_table_path), m_special_character_names);
	retval = retval && DecodeString(ReadBytes(GetBasePath() / m_dchar_table_path), m_default_character_name);
	retval = retval && DecodeStrings(ReadBytes(GetBasePath() / m_item_table_path), m_item_names);
	retval = retval && DecodeStrings(ReadBytes(GetBasePath() / m_menu_table_path), m_menu_strings);
	return retval;
}

bool StringData::AsmLoadIntroStrings()
{
	for (const auto& path : m_intro_strings_path)
	{
		auto bytes = ReadBytes(GetBasePath() / path);
		IntroString s;
		s.Decode(bytes.data(), bytes.size());
		m_intro_strings.push_back(s);
	}
	DeserialiseVisitFlags(ReadBytes(GetBasePath() / m_room_visit_flags_path));
	return true;
}

bool StringData::AsmLoadEndCreditStrings()
{
	auto bytes = ReadBytes(GetBasePath() / m_end_credit_strings_path);

	size_t offset = 0;
	while ((offset < (bytes.size() - 1)) && (bytes[offset] != 0xFF || bytes[offset + 1] != 0xFF))
	{
		m_ending_strings.emplace_back(EndCreditString());
		offset += m_ending_strings.back().Decode(bytes.data() + offset, bytes.size() - offset);

	}
	return true;
}

bool StringData::AsmLoadTalkSfx()
{
	m_char_talk_sfx = ReadBytes(GetBasePath() / m_char_talk_sfx_path);
	m_sprite_talk_sfx = DeserialiseSfxMap(ReadBytes(GetBasePath() / m_sprite_talk_sfx_path));
	while (m_char_talk_sfx.back() == 0xFF)
	{
		m_char_talk_sfx.pop_back();
	}

	return true;
}

bool StringData::AsmLoadScriptData()
{
	auto bytes = ReadBytes(GetBasePath() / m_room_dialogue_table_path);
	assert(bytes.size() % 2 == 0);
	std::vector<uint16_t> words;
	words.reserve(bytes.size() / 2);
	for (std::size_t i = 0; i < bytes.size(); i += 2)
	{
		words.push_back((bytes[i] << 8) | bytes[i + 1]);
	}
	m_room_dialogue_table = RoomDialogueTable(words);
	return true;
}

bool StringData::RomLoadSystemFont(const Rom& rom)
{
	uint32_t sys_font_begin = Disasm::ReadOffset16(rom, RomLabels::Graphics::SYS_FONT);
	uint32_t sys_font_size = rom.get_address(RomLabels::Graphics::SYS_FONT_SIZE);
	auto sys_font_bytes = rom.read_array<uint8_t>(sys_font_begin, sys_font_size);
	auto sys_font = TilesetEntry::Create(this, sys_font_bytes, RomLabels::Graphics::SYS_FONT,
		RomLabels::Graphics::SYS_FONT_FILE, false);
	sys_font->SetStartAddress(sys_font_begin);
	m_fonts_by_name.insert({ sys_font->GetName(), sys_font });
	m_fonts_internal.insert({ sys_font->GetName(), sys_font });
	return true;
}

bool StringData::RomLoadSystemStrings(const Rom& rom)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	uint32_t line1_begin = Disasm::ReadOffset16(rom, RomLabels::Strings::REGION_ERROR_LINE1);
	m_system_strings[0] = converter.from_bytes(rom.read_string(line1_begin));
	uint32_t line2_begin = Disasm::ReadOffset16(rom, RomLabels::Strings::REGION_ERROR_NTSC);
	m_system_strings[1] = converter.from_bytes(rom.read_string(line2_begin));
	uint32_t line3_begin = Disasm::ReadOffset16(rom, RomLabels::Strings::REGION_ERROR_PAL);
	m_system_strings[2] = converter.from_bytes(rom.read_string(line3_begin));
	uint32_t line4_begin = Disasm::ReadOffset16(rom, RomLabels::Strings::REGION_ERROR_LINE3);
	m_system_strings[3] = converter.from_bytes(rom.read_string(line4_begin));
	return true;
}

bool StringData::RomLoadCompressedStringData(const Rom& rom)
{
	uint32_t font_ptr = rom.read<uint32_t>(RomLabels::Graphics::MAIN_FONT_PTR);
	uint32_t bank_ptr = rom.read<uint32_t>(RomLabels::Strings::STRING_BANK_PTR_PTR);
	uint32_t banks_begin = rom.read<uint32_t>(bank_ptr);
	auto font_bytes = rom.read_array<uint8_t>(font_ptr, banks_begin - font_ptr);
	auto string_bytes = rom.read_array<uint8_t>(banks_begin, bank_ptr - banks_begin);
	auto it = string_bytes.cbegin();
	while (it != string_bytes.cend() && *it != 0x00 && *it != 0xFF)
	{
		m_compressed_strings.push_back(ByteVector(it, it + *it));
		it += *it;
	}
	auto e = TilesetEntry::Create(this, font_bytes, RomLabels::Graphics::MAIN_FONT,
		RomLabels::Graphics::MAIN_FONT_FILE, false, 16, 15, 1);
	m_fonts_by_name.insert({ e->GetName(), e });
	m_fonts_internal.insert({ e->GetName(), e });
	return true;
}

bool StringData::RomLoadHuffmanData(const Rom& rom)
{
	uint32_t textbox_2l_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TEXTBOX_2LINE_MAP);
	uint32_t textbox_3l_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TEXTBOX_3LINE_MAP);
	uint32_t huff_offsets_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::HUFFMAN_OFFSETS);
	uint32_t huff_tables_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::HUFFMAN_TABLES);
	uint32_t end = rom.get_section(RomLabels::Strings::HUFFMAN_SECTION).end;

	uint32_t textbox_3l_size = textbox_2l_addr - textbox_3l_addr;
	uint32_t textbox_2l_size = huff_offsets_addr - textbox_2l_addr;
	uint32_t huff_offsets_size = huff_tables_addr - huff_offsets_addr;
	uint32_t huff_tables_size = end - huff_tables_addr;

	auto textbox_3l_bytes = rom.read_array<uint8_t>(textbox_3l_addr, textbox_3l_size);
	auto textbox_2l_bytes = rom.read_array<uint8_t>(textbox_2l_addr, textbox_2l_size);
	m_huffman_offsets = rom.read_array<uint8_t>(huff_offsets_addr, huff_offsets_size);
	m_huffman_tables = rom.read_array<uint8_t>(huff_tables_addr, huff_tables_size);

	auto textbox_3l = Tilemap2DEntry::Create(this, textbox_3l_bytes, RomLabels::Graphics::TEXTBOX_3LINE_MAP,
		RomLabels::Graphics::TEXTBOX_3LINE_MAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 8);
	auto textbox_2l = Tilemap2DEntry::Create(this, textbox_2l_bytes, RomLabels::Graphics::TEXTBOX_2LINE_MAP,
		RomLabels::Graphics::TEXTBOX_2LINE_MAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 6);

	m_ui_tilemaps.insert({ textbox_3l->GetName(), textbox_3l });
	m_ui_tilemaps_internal.insert({ textbox_3l->GetName(), textbox_3l });
	m_ui_tilemaps.insert({ textbox_2l->GetName(), textbox_2l });
	m_ui_tilemaps_internal.insert({ textbox_2l->GetName(), textbox_2l });

	return true;
}

bool StringData::RomLoadStringTables(const Rom& rom)
{
	uint32_t save_loc_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::SAVE_GAME_LOCATIONS_LEA);
	uint32_t map_loc_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::ISLAND_MAP_LOCATIONS);
	uint32_t chars_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::CHAR_NAME_TABLE);
	uint32_t schars_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::SPECIAL_CHAR_NAME_TABLE);
	uint32_t dchars_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::DEFAULT_NAME);
	uint32_t items_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::ITEM_NAME_TABLE);
	uint32_t menu_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::MENU_STRING_TABLE);
	uint32_t end = rom.get_section(RomLabels::Strings::STRING_TABLE_DATA).end;

	uint32_t save_loc_size = rom.get_section(RomLabels::Strings::SAVE_GAME_LOCATIONS).size();
	uint32_t map_loc_size = chars_addr - map_loc_addr;
	uint32_t chars_size = schars_addr - chars_addr;
	uint32_t schars_size = dchars_addr - schars_addr;
	uint32_t dchars_size = items_addr - dchars_addr;
	uint32_t items_size = menu_addr - items_addr;
	uint32_t menu_size = end - menu_addr;

	auto save_loc_bytes = rom.read_array<uint8_t>(save_loc_addr, save_loc_size);
	auto map_loc_bytes = rom.read_array<uint8_t>(map_loc_addr, map_loc_size);
	auto chars_bytes = rom.read_array<uint8_t>(chars_addr, chars_size);
	auto schars_bytes = rom.read_array<uint8_t>(schars_addr, schars_size);
	auto dchars_bytes = rom.read_array<uint8_t>(dchars_addr, dchars_size);
	auto items_bytes = rom.read_array<uint8_t>(items_addr, items_size);
	auto menu_bytes = rom.read_array<uint8_t>(menu_addr, menu_size);

	m_save_game_locations = DeserialiseLocationMap(save_loc_bytes);
	m_island_map_locations = DeserialiseLocationMap(map_loc_bytes);
	bool retval = DecodeStrings(chars_bytes, m_character_names);
	retval = retval && DecodeStrings(schars_bytes, m_special_character_names);
	retval = retval && DecodeString(dchars_bytes, m_default_character_name);
	retval = retval && DecodeStrings(items_bytes, m_item_names);
	retval = retval && DecodeStrings(menu_bytes, m_menu_strings);

	return retval;
}

bool StringData::RomLoadIntroStrings(const Rom& rom)
{
	uint32_t ptrs_addr = Disasm::ReadOffset16(rom, RomLabels::Strings::INTRO_STRING_PTRS);
	uint32_t room_flags_addr = rom.read<uint32_t>(RomLabels::Rooms::ROOM_VISIT_FLAGS);
	uint32_t room_flags_end = rom.get_section(RomLabels::Strings::INTRO_STRING_DATA).end;
	uint32_t end_addr = rom.size();
	std::vector<uint32_t> ptrs;
	std::vector<uint32_t> sizes;
	while (ptrs_addr < end_addr)
	{
		ptrs.push_back(rom.inc_read<uint32_t>(ptrs_addr));
		end_addr = std::min(ptrs.back(), end_addr);
		if (ptrs.size() > 1)
		{
			sizes.push_back(ptrs.back() - ptrs[ptrs.size() - 2]);
		}
	}
	sizes.push_back(room_flags_addr - ptrs.back());
	for (std::size_t i = 0; i < ptrs.size(); ++i)
	{
		auto bytes = rom.read_array<uint8_t>(ptrs[i], sizes[i]);
		IntroString s;
		s.Decode(bytes.data(), bytes.size());
		m_intro_strings.push_back(s);
		m_intro_strings_path.push_back(StrPrintf(RomLabels::Strings::INTRO_STRING_FILE, i + 1));
	}
	DeserialiseVisitFlags(rom.read_array<uint8_t>(room_flags_addr, room_flags_end - room_flags_addr));
	return true;
}

bool StringData::RomLoadEndCreditStrings(const Rom& rom)
{
	uint32_t addr = Disasm::ReadOffset16(rom, RomLabels::Strings::END_CREDIT_STRINGS);
	uint32_t end = rom.get_section(RomLabels::Strings::END_CREDIT_STRING_SECTION).end;
	auto bytes = rom.read_array<uint8_t>(addr, end - addr);
	size_t offset = 0;
	while ((offset < (bytes.size() - 1)) && (bytes[offset] != 0xFF || bytes[offset + 1] != 0xFF))
	{
		m_ending_strings.emplace_back(EndCreditString());
		offset += m_ending_strings.back().Decode(bytes.data() + offset, bytes.size() - offset);
	}
	return true;
}

bool StringData::RomLoadTalkSfx(const Rom& rom)
{
	uint32_t addr = Disasm::ReadOffset8(rom, RomLabels::Strings::CHARACTER_TALK_SFX);

	uint8_t sfx;
	for (;;)
	{
		sfx = rom.inc_read<uint8_t>(addr);
		if (sfx == 0xFF)
		{
			break;
		}
		m_char_talk_sfx.push_back(sfx);
	}

	addr = Disasm::ReadOffset16(rom, RomLabels::Strings::SPRITE_TALK_SFX);

	uint8_t sprite;
	for (;;)
	{
		sprite = rom.inc_read<uint8_t>(addr);
		sfx = rom.inc_read<uint8_t>(addr);
		if ((sprite == 0xFF) || (sfx == 0xFF))
		{
			break;
		}
		m_sprite_talk_sfx.insert({ sprite, sfx });
	}
	
	return true;
}

bool StringData::RomLoadScriptData(const Rom& rom)
{
	uint32_t begin = Disasm::ReadOffset16(rom, RomLabels::Strings::ROOM_CHARACTER_TABLE);
	uint32_t end = rom.get_section(RomLabels::Strings::ROOM_CHARACTER_TABLE_SECTION).end;
	m_room_dialogue_table = RoomDialogueTable(rom.read_array<uint16_t>(begin, (end - begin)/sizeof(uint16_t)));

	return true;
}

bool StringData::AsmSaveFonts(const std::filesystem::path& dir)
{
	bool retval = std::all_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	return retval;
}

bool StringData::AsmSaveSystemText(const std::filesystem::path& dir)
{
	try
	{
		AsmFile sfile, ifile;
		ByteVector str[4];
		ifile.WriteFileHeader(m_region_check_filename, "Region Check");
		ifile << AsmFile::Label(RomLabels::Strings::REGION_CHECK_ROUTINE) << AsmFile::IncludeFile(m_region_check_routine_filename, AsmFile::FileType::ASSEMBLER);
		ifile << AsmFile::Label(RomLabels::Strings::REGION_CHECK_STRINGS) << AsmFile::IncludeFile(m_region_check_strings_filename, AsmFile::FileType::ASSEMBLER);
		ifile << AsmFile::Align(2) << AsmFile::Label(RomLabels::Graphics::SYS_FONT) << AsmFile::IncludeFile(m_system_font_filename, AsmFile::FileType::BINARY);
		ifile.WriteFile(dir / m_region_check_filename);
		sfile.WriteFileHeader(m_region_check_strings_filename, "Region Check System Strings");
		for (int i = 0; i < 4; ++i)
		{
			std::transform(m_system_strings[i].cbegin(), m_system_strings[i].cend(), std::back_inserter(str[i]), [](const auto& c) {
				return static_cast<uint8_t>(c);
				});
			str[i].push_back(0);
		}
		sfile << AsmFile::Label(RomLabels::Strings::REGION_ERROR_LINE1) << str[0];
		sfile << AsmFile::Label(RomLabels::Strings::REGION_ERROR_NTSC) << str[1];
		sfile << AsmFile::Label(RomLabels::Strings::REGION_ERROR_PAL) << str[2];
		sfile << AsmFile::Label(RomLabels::Strings::REGION_ERROR_LINE3) << str[3];
		sfile.WriteFile(dir / m_region_check_strings_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmSaveCompressedStringData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile sfile, pfile;
		sfile.WriteFileHeader(m_strings_filename, "Compressed Strings");
		pfile.WriteFileHeader(m_string_ptr_filename, "Compressed String Bank Pointers");
		const auto& font = m_fonts_internal[RomLabels::Graphics::MAIN_FONT];
		sfile << AsmFile::Label(font->GetName()) << AsmFile::IncludeFile(font->GetFilename(), AsmFile::FileType::BINARY);
		pfile << AsmFile::Label(RomLabels::Strings::STRING_BANK_PTR);
		int f = 0;
		for (std::size_t i = 0; i < m_compressed_strings.size(); i += 256)
		{
			ByteVector bytes;
			for (std::size_t j = 0; j < 256 && (i + j) < m_compressed_strings.size(); ++j)
			{
				const auto& s = m_compressed_strings[i + j];
				bytes.insert(bytes.end(), s.cbegin(), s.cend());
			}
			std::filesystem::path fname = StrPrintf(RomLabels::Strings::STRING_BANK_FILE, f + 1);
			std::string pname = StrPrintf(RomLabels::Strings::STRING_BANK, f);
			WriteBytes(bytes, dir / m_string_filename_path / fname.filename());
			pfile << pname;
			sfile << AsmFile::Label(pname) << AsmFile::IncludeFile(fname, AsmFile::FileType::BINARY);
			f++;
		}
		sfile.WriteFile(dir / m_strings_filename);
		pfile.WriteFile(dir / m_string_ptr_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmSaveHuffmanData(const std::filesystem::path& dir)
{
	WriteBytes(m_huffman_offsets, dir / m_huffman_offset_path);
	WriteBytes(m_huffman_tables, dir / m_huffman_table_path);
	return true;
}

bool StringData::AsmSaveStringTables(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_string_table_path, "String Tables");
		file << AsmFile::Label(RomLabels::Strings::CHAR_NAME_TABLE) << AsmFile::IncludeFile(m_char_table_path, AsmFile::FileType::BINARY);
		file << AsmFile::Align(2);
		file << AsmFile::Label(RomLabels::Strings::SPECIAL_CHAR_NAME_TABLE) << AsmFile::IncludeFile(m_schar_table_path, AsmFile::FileType::BINARY);
		file << AsmFile::Align(2);
		file << AsmFile::Label(RomLabels::Strings::DEFAULT_NAME) << AsmFile::IncludeFile(m_dchar_table_path, AsmFile::FileType::BINARY);
		file << AsmFile::Align(2);
		file << AsmFile::Label(RomLabels::Strings::ITEM_NAME_TABLE) << AsmFile::IncludeFile(m_item_table_path, AsmFile::FileType::BINARY);
		file << AsmFile::Align(2);
		file << AsmFile::Label(RomLabels::Strings::MENU_STRING_TABLE) << AsmFile::IncludeFile(m_menu_table_path, AsmFile::FileType::BINARY);
		file << AsmFile::Align(2);
		file.WriteFile(dir / m_string_table_path);
		ByteVector cbytes, sbytes, dbytes, ibytes, mbytes;
		EncodeStrings(m_character_names, cbytes);
		EncodeStrings(m_special_character_names, sbytes);
		EncodeString(m_default_character_name, dbytes);
		EncodeStrings(m_item_names, ibytes);
		EncodeStrings(m_menu_strings, mbytes);
		WriteBytes(cbytes, dir / m_char_table_path);
		WriteBytes(sbytes, dir / m_schar_table_path);
		WriteBytes(dbytes, dir / m_dchar_table_path);
		WriteBytes(ibytes, dir / m_item_table_path);
		WriteBytes(mbytes, dir / m_menu_table_path);
		WriteBytes(SerialiseLocationMap(m_save_game_locations), dir / m_save_loc_path);
		WriteBytes(SerialiseLocationMap(m_island_map_locations), dir / m_map_loc_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmSaveIntroStrings(const std::filesystem::path& dir)
{
	try
	{
		AsmFile sfile, pfile;
		sfile.WriteFileHeader(m_intro_string_data_path, "Intro Strings Data");
		pfile.WriteFileHeader(m_intro_string_ptrtable_path, "Intro String Pointers");
		sfile << AsmFile::Label(RomLabels::Strings::INTRO_STRING_PTRS)
		      << AsmFile::IncludeFile(m_intro_string_ptrtable_path, AsmFile::FileType::ASSEMBLER);
		std::size_t i = 0;
		for (const auto& s : m_intro_strings)
		{
			std::filesystem::path path;
			if (i < m_intro_strings_path.size())
			{
				path = m_intro_strings_path[i];
			}
			else
			{
				path = StrPrintf(RomLabels::Strings::INTRO_STRING_FILE, i + 1);
			}
			auto lbl = StrPrintf(RomLabels::Strings::INTRO_STRING, i + 1);
			pfile << lbl;
			sfile << AsmFile::Label(lbl) << AsmFile::IncludeFile(path, AsmFile::FileType::BINARY);
			sfile << AsmFile::Align(2);
			ByteVector b(256);
			b.resize(s.Encode(b.data(), b.size()));
			WriteBytes(b, dir / path);
			i++;
		}
		WriteBytes(SerialiseVisitFlags(), dir / m_room_visit_flags_path);
		sfile.WriteFile(dir / m_intro_string_data_path);
		pfile.WriteFile(dir / m_intro_string_ptrtable_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool StringData::AsmSaveEndCreditStrings(const std::filesystem::path& dir)
{
	ByteVector b(65536);
	std::size_t offset = 0;
	for (const auto& s : m_ending_strings)
	{
		offset += s.Encode(b.data() + offset, b.size() - offset);
	}
	b.resize(offset);
	WriteBytes(b, dir / m_end_credit_strings_path);
	return true;
}

bool StringData::AsmSaveTalkSfx(const std::filesystem::path& dir)
{
	ByteVector bytes(m_char_talk_sfx);
	bytes.push_back(0xFF);
	WriteBytes(bytes, dir / m_char_talk_sfx_path);
	WriteBytes(SerialiseSfxMap(m_sprite_talk_sfx), dir / m_sprite_talk_sfx_path);
	return true;
}

bool StringData::AsmSaveScriptData(const std::filesystem::path& dir)
{
	ByteVector bytes;
	auto words = m_room_dialogue_table.GetData();
	bytes.reserve(words.size() * 2);
	for (const auto& word : words)
	{
		bytes.push_back(word >> 8);
		bytes.push_back(word & 0xFF);
	}
	WriteBytes(bytes, dir / m_room_dialogue_table_path);
	return true;
}

bool StringData::RomPrepareInjectSystemText(const Rom& rom)
{
	auto system_font_bytes = m_fonts_by_name[RomLabels::Graphics::SYS_FONT]->GetBytes();
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	auto write_string = [](ByteVectorPtr& bytes, const std::string& in)
	{
		for (char c : in)
		{
			bytes->push_back(c);
		}
		bytes->push_back(0);
	};
	uint32_t addrs[5]{0};
	uint32_t begin = rom.get_section(RomLabels::Strings::REGION_CHECK_DATA_SECTION).begin;
	for (int i = 0; i < 4; ++i)
	{
		addrs[i] = begin + bytes->size();
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		write_string(bytes, converter.to_bytes(m_system_strings[i]));
	}
	if ((bytes->size() & 1) == 1)
	{
		bytes->push_back(0xFF);
	}
	addrs[4] = begin + bytes->size();
	bytes->insert(bytes->end(), system_font_bytes->cbegin(), system_font_bytes->cend());
	m_pending_writes.push_back({ RomLabels::Strings::REGION_CHECK_DATA_SECTION, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::REGION_ERROR_LINE1, addrs[0]));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::REGION_ERROR_NTSC, addrs[1]));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::REGION_ERROR_PAL, addrs[2]));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::REGION_ERROR_LINE3, addrs[3]));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SYS_FONT, addrs[4]));

	return true;
}

bool StringData::RomPrepareInjectCompressedStringData(const Rom& rom)
{
	std::vector<uint32_t> bank_ptrs;
	uint32_t bank_ptrs_ptr;
	ByteVectorPtr bytes = std::make_shared<ByteVector>(*m_fonts_internal[RomLabels::Graphics::MAIN_FONT]->GetBytes());
	uint32_t begin = rom.get_section(RomLabels::Strings::STRING_SECTION).begin;
	for (std::size_t i = 0; i < m_compressed_strings.size(); i += 256)
	{
		bank_ptrs.push_back(begin + bytes->size());
		for (std::size_t j = 0; j < 256 && (i + j) < m_compressed_strings.size(); ++j)
		{
			const auto& s = m_compressed_strings[i + j];
			bytes->insert(bytes->end(), s.cbegin(), s.cend());
		}
	}
	while ((bytes->size() & 3) != 0)
	{
		bytes->push_back(0);
	}
	bank_ptrs_ptr = bytes->size() + begin;
	for (auto p : bank_ptrs)
	{
		auto b = Split<uint8_t, uint32_t>(p);
		bytes->insert(bytes->end(), b.cbegin(), b.cend());
	}
	m_pending_writes.push_back({ RomLabels::Strings::STRING_SECTION, bytes });
	m_pending_writes.push_back(Asm::WriteAddress32(RomLabels::Strings::STRING_BANK_PTR_PTR, bank_ptrs_ptr));
	m_pending_writes.push_back(Asm::WriteAddress32(RomLabels::Graphics::MAIN_FONT_PTR, begin));
	return true;
}

bool StringData::RomPrepareInjectHuffmanData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Strings::HUFFMAN_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t textbox_3l_begin = begin;
	auto textbox_3l_bytes = m_ui_tilemaps_internal[RomLabels::Graphics::TEXTBOX_3LINE_MAP]->GetBytes();
	bytes->insert(bytes->end(), textbox_3l_bytes->cbegin(), textbox_3l_bytes->cend());

	uint32_t textbox_2l_begin = begin + bytes->size();
	auto textbox_2l_bytes = m_ui_tilemaps_internal[RomLabels::Graphics::TEXTBOX_2LINE_MAP]->GetBytes();
	bytes->insert(bytes->end(), textbox_2l_bytes->cbegin(), textbox_2l_bytes->cend());

	uint32_t huffman_offsets_begin = begin + bytes->size();
	bytes->insert(bytes->end(), m_huffman_offsets.cbegin(), m_huffman_offsets.cend());

	uint32_t huffman_tables_begin = begin + bytes->size();
	bytes->insert(bytes->end(), m_huffman_tables.cbegin(), m_huffman_tables.cend());

	m_pending_writes.push_back({ RomLabels::Strings::HUFFMAN_SECTION, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TEXTBOX_3LINE_MAP, textbox_3l_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TEXTBOX_2LINE_MAP, textbox_2l_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::HUFFMAN_OFFSETS, huffman_offsets_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::HUFFMAN_TABLES, huffman_tables_begin));

	return true;
}

bool StringData::RomPrepareInjectStringTables(const Rom& rom)
{
	uint32_t save_begin = rom.get_section(RomLabels::Strings::SAVE_GAME_LOCATIONS).begin;
	auto save_bytes = std::make_shared<ByteVector>(SerialiseLocationMap(m_save_game_locations));
	
	uint32_t begin = rom.get_section(RomLabels::Strings::STRING_TABLE_DATA).begin;
	uint32_t map_begin = begin;
	auto bytes = std::make_shared<ByteVector>(SerialiseLocationMap(m_island_map_locations));

	uint32_t chars_begin = begin + bytes->size();
	ByteVector chars_bytes;
	EncodeStrings(m_character_names, chars_bytes);
	bytes->insert(bytes->end(), chars_bytes.cbegin(), chars_bytes.cend());

	uint32_t schars_begin = begin + bytes->size();
	ByteVector schars_bytes;
	EncodeStrings(m_special_character_names, schars_bytes);
	bytes->insert(bytes->end(), schars_bytes.cbegin(), schars_bytes.cend());

	uint32_t dchars_begin = begin + bytes->size();
	ByteVector dchars_bytes;
	EncodeString(m_default_character_name, dchars_bytes);
	bytes->insert(bytes->end(), dchars_bytes.cbegin(), dchars_bytes.cend());

	uint32_t items_begin = begin + bytes->size();
	ByteVector items_bytes;
	EncodeStrings(m_item_names, items_bytes);
	bytes->insert(bytes->end(), items_bytes.cbegin(), items_bytes.cend());

	uint32_t menu_begin = begin + bytes->size();
	ByteVector menu_bytes;
	EncodeStrings(m_menu_strings, menu_bytes);
	bytes->insert(bytes->end(), menu_bytes.cbegin(), menu_bytes.cend());
	
	m_pending_writes.push_back({ RomLabels::Strings::SAVE_GAME_LOCATIONS, save_bytes });
	m_pending_writes.push_back({ RomLabels::Strings::STRING_TABLE_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::SAVE_GAME_LOCATIONS_LEA, save_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::ISLAND_MAP_LOCATIONS, map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::CHAR_NAME_TABLE, chars_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::SPECIAL_CHAR_NAME_TABLE, schars_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::DEFAULT_NAME, dchars_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::ITEM_NAME_TABLE, items_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::MENU_STRING_TABLE, menu_begin));

	return true;
}

bool StringData::RomPrepareInjectIntroStrings(const Rom& rom)
{
	ByteVectorPtr bytes = std::make_shared<ByteVector>(sizeof(uint32_t) * m_intro_strings.size());
	uint32_t begin = rom.get_section(RomLabels::Strings::INTRO_STRING_DATA).begin;
	int ptr_offset = 0;
	for (const auto& s : m_intro_strings)
	{
		uint32_t addr = bytes->size() + begin;
		bytes->at(ptr_offset++) = (addr >> 24) & 0xFF;
		bytes->at(ptr_offset++) = (addr >> 16) & 0xFF;
		bytes->at(ptr_offset++) = (addr >> 8) & 0xFF;
		bytes->at(ptr_offset++) = addr & 0xFF;
		ByteVector b(256);
		b.resize(s.Encode(b.data(), b.size()));
		bytes->insert(bytes->end(), b.cbegin(), b.cend());
		if (((bytes->size() + begin) & 1) == 1)
		{
			bytes->push_back(0xFF);
		}
	}
	uint32_t visit_begin = begin + bytes->size();
	auto visit_bytes = SerialiseVisitFlags();
	bytes->insert(bytes->end(), visit_bytes.cbegin(), visit_bytes.cend());

	if (bytes->size() < rom.get_section(RomLabels::Strings::INTRO_STRING_DATA).size())
	{
		bytes->push_back(0xFF);
	}

	m_pending_writes.push_back({ RomLabels::Strings::INTRO_STRING_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::INTRO_STRING_PTRS, begin));
	m_pending_writes.push_back(Asm::WriteAddress32(RomLabels::Rooms::ROOM_VISIT_FLAGS, visit_begin));

	return true;
}

bool StringData::RomPrepareInjectEndCreditStrings(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Strings::END_CREDIT_STRING_SECTION).begin;
	ByteVectorPtr b = std::make_shared<ByteVector>(65536);
	std::size_t offset = 0;
	for (const auto& s : m_ending_strings)
	{
		offset += s.Encode(b->data() + offset, b->size() - offset);
	}
	b->resize(offset);
	m_pending_writes.push_back({ RomLabels::Strings::END_CREDIT_STRING_SECTION, b });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::END_CREDIT_STRINGS, begin));
	return true;
}

bool StringData::RomPrepareInjectTalkSfx(const Rom& rom)
{
	uint32_t char_begin = rom.get_section(RomLabels::Strings::CHARACTER_TALK_SFX_SECTION).begin;
	uint32_t sprite_begin = rom.get_section(RomLabels::Strings::SPRITE_TALK_SFX_SECTION).begin;

	ByteVectorPtr char_bytes = std::make_shared<ByteVector>(m_char_talk_sfx);
	char_bytes->push_back(0xFF);
	ByteVectorPtr sprite_bytes = std::make_shared<ByteVector>(SerialiseSfxMap(m_sprite_talk_sfx));

	m_pending_writes.push_back({ RomLabels::Strings::CHARACTER_TALK_SFX_SECTION, char_bytes });
	m_pending_writes.push_back({ RomLabels::Strings::SPRITE_TALK_SFX_SECTION, sprite_bytes });
	m_pending_writes.push_back(Asm::WriteOffset8(rom, RomLabels::Strings::CHARACTER_TALK_SFX, char_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::SPRITE_TALK_SFX, sprite_begin));

	return true;
}

bool StringData::RomPrepareInjectScriptData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Strings::ROOM_CHARACTER_TABLE_SECTION).begin;
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	auto words = m_room_dialogue_table.GetData();
	bytes->reserve(words.size() * 2);
	for (const auto& word : words)
	{
		bytes->push_back(word >> 8);
		bytes->push_back(word & 0xFF);
	}
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Strings::ROOM_CHARACTER_TABLE, begin));
	m_pending_writes.push_back({ RomLabels::Strings::ROOM_CHARACTER_TABLE_SECTION, bytes });
	return true;
}
