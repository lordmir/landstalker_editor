#include "GraphicsData.h"
#include "AsmUtils.h"

#include <set>
#include <numeric>

GraphicsData::GraphicsData(const filesystem::path& asm_file)
	: DataManager(asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadFonts())
	{
		throw std::runtime_error(std::string("Unable to load font data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadStrings())
	{
		throw std::runtime_error(std::string("Unable to load string data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadInventoryGraphics())
	{
		throw std::runtime_error(std::string("Unable to load inventory graphics data from \'") + m_inventory_graphics_filename.str() + '\'');
	}
	if (!AsmLoadCompressedStringData())
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from \'") + m_strings_filename.str() + '\'');
	}
	if (!AsmLoadPalettes())
	{
		throw std::runtime_error(std::string("Unable to load palettes from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadTextGraphics())
	{
		throw std::runtime_error(std::string("Unable to load text graphics from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadSwordFx())
	{
		throw std::runtime_error(std::string("Unable to load sword graphics from \'") + m_sword_fx_path.str() + '\'');
	}
	if (!AsmLoadStatusFx())
	{
		throw std::runtime_error(std::string("Unable to load status graphics from \'") + m_status_fx_path.str() + '\'');
	}
	if (!AsmLoadHuffmanData())
	{
		throw std::runtime_error(std::string("Unable to load Huffman data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadHudData())
	{
		throw std::runtime_error(std::string("Unable to load HUD data from \'") + asm_file.str() + '\'');
	}
	if (!AsmLoadEndCreditData())
	{
		throw std::runtime_error(std::string("Unable to load end credit data from \'") + m_end_credits_path.str() + '\'');
	}
	if (!AsmLoadIslandMapData())
	{
		throw std::runtime_error(std::string("Unable to load island map data from \'") + m_island_map_path.str() + '\'');
	}
	if (!AsmLoadTitleScreenData())
	{
		throw std::runtime_error(std::string("Unable to load title screen data from \'") + m_title_path.str() + '\'');
	}
	InitCache();
	UpdateTilesetRecommendedPalettes();
	ResetTilesetDefaultPalettes();
}

GraphicsData::GraphicsData(const Rom& rom)
	: DataManager(rom)
{
	SetDefaultFilenames();
	if (!RomLoadFonts(rom))
	{
		throw std::runtime_error(std::string("Unable to load font data from ROM"));
	}
	if (!RomLoadStrings(rom))
	{
		throw std::runtime_error(std::string("Unable to load string data from ROM"));
	}
	if (!RomLoadInventoryGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to load inventory graphics from ROM"));
	}
	if (!RomLoadCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to load compressed strings from ROM"));
	}
	if (!RomLoadPalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to load palettes from ROM"));
	}
	if (!RomLoadTextGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to load text graphics from ROM"));
	}
	if (!RomLoadSwordFx(rom))
	{
		throw std::runtime_error(std::string("Unable to load sword effects from ROM"));
	}
	if (!RomLoadStatusFx(rom))
	{
		throw std::runtime_error(std::string("Unable to load status graphics from ROM"));
	}
	if (!RomLoadHuffmanData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Huffman data from ROM"));
	}
	if (!RomLoadHudData(rom))
	{
		throw std::runtime_error(std::string("Unable to load HUD data from ROM"));
	}
	if (!RomLoadEndCreditData(rom))
	{
		throw std::runtime_error(std::string("Unable to load end credit data from ROM"));
	}
	if (!RomLoadIslandMapData(rom))
	{
		throw std::runtime_error(std::string("Unable to load island map data from ROM"));
	}
	if (!RomLoadTitleScreenData(rom))
	{
		throw std::runtime_error(std::string("Unable to load title screen data from ROM"));
	}
	InitCache();
	UpdateTilesetRecommendedPalettes();
	ResetTilesetDefaultPalettes();
}

bool GraphicsData::Save(const filesystem::path& dir)
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
	if (!AsmSaveStrings(dir))
	{
		throw std::runtime_error(std::string("Unable to save string data to \'") + m_region_check_strings_filename.str() + '\'');
	}
	if (!AsmSaveGraphics(dir))
	{
		throw std::runtime_error(std::string("Unable to save graphics data to \'") + directory.str() + '\'');
	}
	if (!AsmSaveInventoryGraphics(dir))
	{
		throw std::runtime_error(std::string("Unable to save inventory graphics data to \'") + m_inventory_graphics_filename.str() + '\'');
	}
	if (!AsmSaveCompressedStringData(dir))
	{
		throw std::runtime_error(std::string("Unable to save compressed string data to \'") + m_strings_filename.str() + '\'');
	}
	if (!AsmSaveSwordFx(dir))
	{
		throw std::runtime_error(std::string("Unable to save sword effects data to \'") + m_sword_fx_path.str() + '\'');
	}
	if (!AsmSaveStatusFx(dir))
	{
		throw std::runtime_error(std::string("Unable to save status effects data to \'") + m_status_fx_path.str() + '\'');
	}
	if (!AsmSaveHuffmanData(dir))
	{
		throw std::runtime_error(std::string("Unable to save Huffman data to \'") + directory.str() + '\'');
	}
	if (!AsmSaveEndCreditData(dir))
	{
		throw std::runtime_error(std::string("Unable to save end credit data to \'") + m_end_credits_path.str() + '\'');
	}
	if (!AsmSaveIslandMapData(dir))
	{
		throw std::runtime_error(std::string("Unable to save island map data to \'") + m_island_map_path.str() + '\'');
	}
	if (!AsmSaveTitleScreenData(dir))
	{
		throw std::runtime_error(std::string("Unable to save title screen data to \'") + m_title_path.str() + '\'');
	}
	CommitAllChanges();
	return true;
}

bool GraphicsData::Save()
{
	return Save(GetBasePath());
}

bool GraphicsData::HasBeenModified() const
{
	auto entry_pred = [](const auto& e) {return e != nullptr && e->HasDataChanged(); };
	auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasDataChanged(); };
	if (std::any_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_misc_gfx_by_name.begin(), m_misc_gfx_by_name.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_palettes_by_name.begin(), m_palettes_by_name.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_sword_fx.begin(), m_sword_fx.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_status_fx_frames.begin(), m_status_fx_frames.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_misc_tilemaps.begin(), m_misc_tilemaps.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_island_map_pals.begin(), m_island_map_pals.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_island_map_tiles.begin(), m_island_map_tiles.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_island_map_tilemaps.begin(), m_island_map_tilemaps.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_title_pals.begin(), m_title_pals.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_title_tilemaps.begin(), m_title_tilemaps.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_title_tiles.begin(), m_title_tiles.end(), pair_pred))
	{
		return true;
	}
	if (m_fonts_by_name != m_fonts_by_name_orig)
	{
		return true;
	}
	if (m_system_strings != m_system_strings_orig)
	{
		return true;
	}
	if (m_palettes_by_name_orig != m_palettes_by_name)
	{
		return true;
	}
	if (m_misc_gfx_by_name_orig != m_misc_gfx_by_name)
	{
		return true;
	}
	if (m_sword_fx_orig != m_sword_fx)
	{
		return true;
	}
	if (m_status_fx_orig != m_status_fx)
	{
		return true;
	}
	if (m_status_fx_frames_orig != m_status_fx_frames)
	{
		return true;
	}
	if (m_misc_tilemaps_orig != m_misc_tilemaps)
	{
		return true;
	}
	if (m_huffman_offsets_orig != m_huffman_offsets)
	{
		return true;
	}
	if (m_island_map_pals_orig != m_island_map_pals)
	{
		return true;
	}
	if (m_island_map_tilemaps_orig != m_island_map_tilemaps)
	{
		return true;
	}
	if (m_island_map_tiles_orig != m_island_map_tiles)
	{
		return true;
	}
	if (m_title_pals_orig != m_title_pals)
	{
		return true;
	}
	if (m_title_tilemaps_orig != m_title_tilemaps)
	{
		return true;
	}
	if (m_title_tiles_orig != m_title_tiles)
	{
		return true;
	}
	if (entry_pred(m_end_credits_map))
	{
		return true;
	}
	if (entry_pred(m_end_credits_palette))
	{
		return true;
	}
	if (entry_pred(m_end_credits_tileset))
	{
		return true;
	}
	return false;
}

void GraphicsData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	if (!RomPrepareInjectFonts(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare fonts for ROM injection"));
	}
	if (!RomPrepareInjectInvGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare inventory graphics for ROM injection"));
	}
	if (!RomPrepareInjectCompressedStringData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare compressed strings for ROM injection"));
	}
	if (!RomPrepareInjectPalettes(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare palettes for ROM injection"));
	}
	if (!RomPrepareInjectTextGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare text graphics for ROM injection"));
	}
	if (!RomPrepareInjectSwordFx(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare sword effects for ROM injection"));
	}
	if (!RomPrepareInjectStatusFx(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare status effects for ROM injection"));
	}
	if (!RomPrepareInjectHuffmanData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare Huffman data for ROM injection"));
	}
	if (!RomPrepareInjectHudData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare HUD data for ROM injection"));
	}
	if (!RomPrepareInjectEndCreditData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare end credit data for ROM injection"));
	}
	if (!RomPrepareInjectIslandMapData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare island map data for ROM injection"));
	}
	if (!RomPrepareInjectTitleScreenData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare title screen data for ROM injection"));
	}
}

std::map<std::string, std::shared_ptr<TilesetEntry>> GraphicsData::GetAllTilesets() const
{
	std::map<std::string, std::shared_ptr<TilesetEntry>> result;
	for (auto& t : m_fonts_by_name)
	{
		result.insert(t);
	}
	for (auto& t : m_misc_gfx_by_name)
	{
		result.insert(t);
	}
	for (auto& t : m_sword_fx)
	{
		result.insert(t);
	}
	for (auto& t : m_status_fx_frames)
	{
		result.insert(t);
	}
	for (auto& t : m_island_map_tiles)
	{
		result.insert(t);
	}
	for (auto& t : m_title_tiles)
	{
		result.insert(t);
	}
	result.insert({ m_end_credits_tileset->GetName(), m_end_credits_tileset });
	return result;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetFonts() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_fonts_by_name)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetMiscGraphics() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_misc_gfx_by_name)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetSwordEffects() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_sword_fx)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetStatusEffects() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_status_fx_frames)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::shared_ptr<TilesetEntry> GraphicsData::GetEndCreditLogos() const
{
	return m_end_credits_tileset;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetIslandMapTiles() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_island_map_tiles)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetTitleScreenTiles() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_title_tiles)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::map<std::string, std::shared_ptr<PaletteEntry>> GraphicsData::GetAllPalettes() const
{
	std::map<std::string, std::shared_ptr<PaletteEntry>> result;
	for (auto& t : m_palettes_by_name)
	{
		result.insert(t);
	}
	for (auto& t : m_island_map_pals)
	{
		result.insert(t);
	}
	for (auto& t : m_title_pals)
	{
		result.insert(t);
	}
	result.insert({ m_end_credits_palette->GetName(), m_end_credits_palette });
	return result;
}

void GraphicsData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	std::for_each(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_commit);
	std::for_each(m_palettes_by_name.begin(), m_palettes_by_name.end(), pair_commit);
	std::for_each(m_misc_gfx_by_name.begin(), m_misc_gfx_by_name.end(), pair_commit);
	std::for_each(m_status_fx_frames.begin(), m_status_fx_frames.end(), pair_commit);
	std::for_each(m_sword_fx.begin(), m_sword_fx.end(), pair_commit);
	std::for_each(m_misc_tilemaps.begin(), m_misc_tilemaps.end(), pair_commit);
	std::for_each(m_island_map_pals.begin(), m_island_map_pals.end(), pair_commit);
	std::for_each(m_island_map_tilemaps.begin(), m_island_map_tilemaps.end(), pair_commit);
	std::for_each(m_island_map_tiles.begin(), m_island_map_tiles.end(), pair_commit);
	std::for_each(m_title_pals.begin(), m_title_pals.end(), pair_commit);
	std::for_each(m_title_tilemaps.begin(), m_title_tilemaps.end(), pair_commit);
	std::for_each(m_title_tiles.begin(), m_title_tiles.end(), pair_commit);
	m_fonts_by_name_orig = m_fonts_by_name;
	m_system_strings_orig = m_system_strings;
	m_palettes_by_name_orig = m_palettes_by_name;
	m_misc_gfx_by_name_orig = m_misc_gfx_by_name;
	m_strings_orig = m_strings;
	m_status_fx_orig = m_status_fx;
	m_status_fx_frames_orig = m_status_fx_frames;
	m_sword_fx_orig = m_sword_fx;
	m_misc_tilemaps_orig = m_misc_tilemaps;
	m_huffman_offsets_orig = m_huffman_offsets;
	m_huffman_tables_orig = m_huffman_tables;
	m_island_map_pals_orig = m_island_map_pals;
	m_island_map_tilemaps_orig = m_island_map_tilemaps;
	m_island_map_tiles_orig = m_island_map_tiles;
	m_title_pals_orig = m_title_pals;
	m_title_tilemaps_orig = m_title_tilemaps;
	m_title_tiles_orig = m_title_tiles;
	entry_commit(m_end_credits_map);
	entry_commit(m_end_credits_palette);
	entry_commit(m_end_credits_tileset);
	m_pending_writes.clear();
}

bool GraphicsData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().str());
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::REGION_CHECK, m_region_check_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::INV_SECTION, m_inventory_graphics_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::STRING_SECTION, m_strings_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::STRING_BANK_PTR_DATA, m_string_ptr_filename);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::STATUS_FX_POINTERS, m_status_fx_pointers_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::STATUS_FX_DATA, m_status_fx_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::SWORD_FX_DATA, m_sword_fx_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::HUFFMAN_OFFSETS, m_huffman_offset_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Strings::HUFFMAN_TABLES, m_huffman_table_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::END_CREDITS_DATA, m_end_credits_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::ISLAND_MAP_DATA, m_island_map_path);
		retval = retval && GetFilenameFromAsm(f, RomOffsets::Graphics::TITLE_DATA, m_title_path);
		AsmFile r(GetBasePath() / m_region_check_filename);
		retval = retval && GetFilenameFromAsm(r, RomOffsets::Strings::REGION_CHECK_ROUTINE, m_region_check_routine_filename);
		retval = retval && GetFilenameFromAsm(r, RomOffsets::Strings::REGION_CHECK_STRINGS, m_region_check_strings_filename);
		retval = retval && GetFilenameFromAsm(r, RomOffsets::Graphics::SYS_FONT, m_system_font_filename);
		
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void GraphicsData::SetDefaultFilenames()
{
	if (m_region_check_filename.empty()) m_region_check_filename = RomOffsets::Strings::REGION_CHECK_FILE;
	if (m_region_check_routine_filename.empty()) m_region_check_routine_filename = RomOffsets::Strings::REGION_CHECK_ROUTINE_FILE;
	if (m_region_check_strings_filename.empty()) m_region_check_strings_filename = RomOffsets::Strings::REGION_CHECK_STRINGS_FILE;
	if (m_system_font_filename.empty()) m_system_font_filename = RomOffsets::Graphics::SYS_FONT_FILE;
	if (m_inventory_graphics_filename.empty()) m_inventory_graphics_filename = RomOffsets::Graphics::INV_GRAPHICS_FILE;
	if (m_strings_filename.empty()) m_strings_filename = RomOffsets::Strings::STRINGS_FILE;
	if (m_string_ptr_filename.empty()) m_string_ptr_filename = RomOffsets::Strings::STRING_BANK_PTR_FILE;
	if (m_string_filename_path.empty()) m_string_filename_path = filesystem::path(RomOffsets::Strings::STRING_BANK_FILE).parent_path();
	if (m_status_fx_path.empty()) m_status_fx_path = RomOffsets::Graphics::STATUS_FX_DATA_FILE;
	if (m_status_fx_pointers_path.empty()) m_status_fx_pointers_path = RomOffsets::Graphics::STATUS_FX_POINTER_FILE;
	if (m_sword_fx_path.empty()) m_sword_fx_path = RomOffsets::Graphics::SWORD_FX_DATA_FILE;
	if (m_huffman_offset_path.empty()) m_huffman_offset_path = RomOffsets::Strings::HUFFMAN_OFFSETS_FILE;
	if (m_huffman_table_path.empty()) m_huffman_table_path = RomOffsets::Strings::HUFFMAN_TABLE_FILE;
	if (m_end_credits_path.empty()) m_end_credits_path = RomOffsets::Graphics::END_CREDITS_DATA_FILE;
	if (m_island_map_path.empty()) m_island_map_path = RomOffsets::Graphics::ISLAND_MAP_DATA_FILE;
	if (m_island_map_routine_path.empty()) m_island_map_routine_path = RomOffsets::Graphics::ISLAND_MAP_ROUTINES_FILE;
	if (m_title_path.empty()) m_title_path = RomOffsets::Graphics::TITLE_DATA_FILE;
	if (m_title_routines_1_path.empty()) m_title_routines_1_path = RomOffsets::Graphics::TITLE_ROUTINES_1_FILE;
	if (m_title_routines_2_path.empty()) m_title_routines_2_path = RomOffsets::Graphics::TITLE_ROUTINES_2_FILE;
	if (m_title_routines_3_path.empty()) m_title_routines_3_path = RomOffsets::Graphics::TITLE_ROUTINES_3_FILE;
}

bool GraphicsData::CreateDirectoryStructure(const filesystem::path& dir)
{
	bool retval = true;
	retval = retval && CreateDirectoryTree(dir / m_region_check_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_routine_filename);
	retval = retval && CreateDirectoryTree(dir / m_region_check_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_system_font_filename);
	retval = retval && CreateDirectoryTree(dir / m_inventory_graphics_filename);
	retval = retval && CreateDirectoryTree(dir / m_strings_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_ptr_filename);
	retval = retval && CreateDirectoryTree(dir / m_string_filename_path / "x");
	retval = retval && CreateDirectoryTree(dir / m_status_fx_path);
	retval = retval && CreateDirectoryTree(dir / m_status_fx_pointers_path);
	retval = retval && CreateDirectoryTree(dir / m_sword_fx_path);
	retval = retval && CreateDirectoryTree(dir / m_huffman_offset_path);
	retval = retval && CreateDirectoryTree(dir / m_huffman_table_path);
	retval = retval && CreateDirectoryTree(dir / m_end_credits_path);
	retval = retval && CreateDirectoryTree(dir / m_island_map_path);
	retval = retval && CreateDirectoryTree(dir / m_island_map_routine_path);
	retval = retval && CreateDirectoryTree(dir / m_title_path);
	retval = retval && CreateDirectoryTree(dir / m_title_routines_1_path);
	retval = retval && CreateDirectoryTree(dir / m_title_routines_2_path);
	retval = retval && CreateDirectoryTree(dir / m_title_routines_3_path);
	for (const auto& f : m_fonts_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& g : m_misc_gfx_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / g.second->GetFilename());
	}
	for (const auto& p : m_palettes_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / p.second->GetFilename());
	}
	for (const auto& f : m_sword_fx)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_status_fx_frames)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_misc_tilemaps)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_island_map_pals)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_island_map_tiles)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_island_map_tilemaps)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_title_pals)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_title_tilemaps)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_title_tiles)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	retval = retval && CreateDirectoryTree(dir / m_end_credits_map->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_end_credits_palette->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_end_credits_tileset->GetFilename());
	return retval;
}

void GraphicsData::InitCache()
{
	m_palettes_by_name_orig = m_palettes_by_name;
	m_system_strings_orig = m_system_strings;
	m_fonts_by_name_orig = m_fonts_by_name;
	m_misc_gfx_by_name_orig = m_misc_gfx_by_name;
	m_strings_orig = m_strings;
	m_sword_fx_orig = m_sword_fx;
	m_status_fx_orig = m_status_fx;
	m_status_fx_frames_orig = m_status_fx_frames;
	m_misc_tilemaps_orig = m_misc_tilemaps;
	m_huffman_offsets_orig = m_huffman_offsets;
	m_huffman_tables_orig = m_huffman_tables;
	m_island_map_pals_orig = m_island_map_pals;
	m_island_map_tilemaps_orig = m_island_map_tilemaps;
	m_island_map_tiles_orig = m_island_map_tiles;
	m_title_pals_orig = m_title_pals;
	m_title_tilemaps_orig = m_title_tilemaps;
	m_title_tiles_orig = m_title_tiles;
}

bool GraphicsData::AsmLoadFonts()
{
	filesystem::path path = GetBasePath() / m_system_font_filename;
	auto e = TilesetEntry::Create(this, ReadBytes(path), RomOffsets::Graphics::SYS_FONT, m_system_font_filename, false, 8, 8);
	m_fonts_by_name.insert({ RomOffsets::Graphics::SYS_FONT, e });
	m_fonts_internal.insert({ RomOffsets::Graphics::SYS_FONT, e });
	return true;
}

bool GraphicsData::AsmLoadStrings()
{
	try
	{
		AsmFile file(GetBasePath() / m_region_check_strings_filename);
		auto read_str = [&](std::string& s)
		{
			char c;
			do
			{
				file >> c;
				if (c != 0)
				{
					s += c;
				}
			} while (c != 0);
		};
		file.Goto(RomOffsets::Strings::REGION_ERROR_LINE1);
		read_str(m_system_strings[0]);
		file.Goto(RomOffsets::Strings::REGION_ERROR_NTSC);
		read_str(m_system_strings[1]);
		file.Goto(RomOffsets::Strings::REGION_ERROR_PAL);
		read_str(m_system_strings[2]);
		file.Goto(RomOffsets::Strings::REGION_ERROR_LINE3);
		read_str(m_system_strings[3]);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadInventoryGraphics()
{
	try
	{
		AsmFile file(GetBasePath() / m_inventory_graphics_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, filesystem::path, ByteVectorPtr>> gfx;
		for(int i = 0; i < 7; ++i)
		{
			file >> name >> inc;
			auto path = GetBasePath() / inc.path;
			auto raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(path));
			gfx.push_back({ name, inc.path, raw_data });
		}
		auto menufont = TilesetEntry::Create(this, *std::get<2>(gfx[0]), std::get<0>(gfx[0]),
			std::get<1>(gfx[0]), false, 8, 8, 1);
		auto menucursor = TilesetEntry::Create(this, *std::get<2>(gfx[1]), std::get<0>(gfx[1]),
			std::get<1>(gfx[1]), false, 8, 8, 2, Tileset::BLOCK4X4);
		auto arrow = TilesetEntry::Create(this, *std::get<2>(gfx[2]), std::get<0>(gfx[2]),
			std::get<1>(gfx[2]), false, 8, 8, 2, Tileset::BLOCK2X2);
		auto unused1 = TilesetEntry::Create(this, *std::get<2>(gfx[3]), std::get<0>(gfx[3]),
			std::get<1>(gfx[3]), false, 8, 11, 2);
		auto unused2 = TilesetEntry::Create(this, *std::get<2>(gfx[4]), std::get<0>(gfx[4]),
			std::get<1>(gfx[4]), false, 8, 10, 2);
		auto pal1 = PaletteEntry::Create(this, *std::get<2>(gfx[5]), std::get<0>(gfx[5]),
			std::get<1>(gfx[5]), Palette::Type::LOW8);
		auto pal2 = PaletteEntry::Create(this, *std::get<2>(gfx[6]), std::get<0>(gfx[6]),
			std::get<1>(gfx[6]), Palette::Type::FULL);
		m_fonts_by_name.insert({ menufont->GetName(), menufont });
		m_misc_gfx_by_name.insert({ menucursor->GetName(), menucursor });
		m_misc_gfx_by_name.insert({ arrow->GetName(), arrow });
		m_misc_gfx_by_name.insert({ unused1->GetName(), unused1 });
		m_misc_gfx_by_name.insert({ unused2->GetName(), unused2 });
		m_palettes_by_name.insert({ pal1->GetName(), pal1 });
		m_palettes_by_name.insert({ pal2->GetName(), pal2 });
		m_fonts_internal.insert({ menufont->GetName(), menufont });
		m_misc_gfx_internal.insert({ menucursor->GetName(), menucursor });
		m_misc_gfx_internal.insert({ arrow->GetName(), arrow });
		m_misc_gfx_internal.insert({ unused1->GetName(), unused1 });
		m_misc_gfx_internal.insert({ unused2->GetName(), unused2 });
		m_palettes_internal.insert({ pal1->GetName(), pal1 });
		m_palettes_internal.insert({ pal2->GetName(), pal2 });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadCompressedStringData()
{
	try
	{
		AsmFile file(GetBasePath() / m_strings_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		file >> name >> inc;
		auto font = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name, inc.path, false, 16, 15, 1);
		m_fonts_by_name.insert({ font->GetName(), font});
		m_fonts_internal.insert({ RomOffsets::Graphics::MAIN_FONT, font });
		while (file.IsGood())
		{
			file >> name >> inc;
			m_string_filename_path = inc.path.parent_path();
			auto bytes = ReadBytes(GetBasePath() / inc.path);
			auto it = bytes.cbegin();
			while (it != bytes.cend() && *it != 0x00)
			{
				m_strings.push_back(ByteVector(it, it + *it));
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

bool GraphicsData::AsmLoadPalettes()
{
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		auto extract_pals = [&](const ByteVector& bytes, const std::string& name, Palette::Type type)
		{
			int idx = 0;
			for (auto it = bytes.cbegin(); it != bytes.cend(); it += Palette::GetSizeBytes(type))
			{
				auto pal_bytes = ByteVector(it, it + Palette::GetSizeBytes(type));
				auto e = PaletteEntry::Create(this, pal_bytes, name + ":" + std::to_string(idx++), inc.path, type);
				m_palettes_by_name.insert({ e->GetName(), e });
				m_palettes_internal.insert({ e->GetName(), e });
			}
		};
		file.Goto(RomOffsets::Graphics::PLAYER_PAL);
		file >> inc;
		auto player_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::PLAYER_PAL, inc.path, Palette::Type::FULL);
		file.Goto(RomOffsets::Graphics::HUD_PAL);
		file >> inc;
		auto hud_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::HUD_PAL, inc.path, Palette::Type::HUD);
		file.Goto(RomOffsets::Graphics::INV_ITEM_PAL);
		file >> inc;
		auto item_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::INV_ITEM_PAL, inc.path, Palette::Type::FULL);
		file.Goto(RomOffsets::Graphics::SWORD_PAL_SWAPS);
		file >> inc;
		auto sword_pal_bytes = ReadBytes(GetBasePath() / inc.path);
		extract_pals(sword_pal_bytes, RomOffsets::Graphics::SWORD_PAL_SWAPS, Palette::Type::SWORD);
		file.Goto(RomOffsets::Graphics::ARMOUR_PAL_SWAPS);
		file >> inc;
		auto armour_pal_bytes = ReadBytes(GetBasePath() / inc.path);
		extract_pals(armour_pal_bytes, RomOffsets::Graphics::ARMOUR_PAL_SWAPS, Palette::Type::ARMOUR);

		m_palettes_by_name.insert({ player_pal->GetName(), player_pal });
		m_palettes_by_name.insert({ hud_pal->GetName(), hud_pal });
		m_palettes_by_name.insert({ item_pal->GetName(), item_pal });
		m_palettes_internal.insert({ player_pal->GetName(), player_pal });
		m_palettes_internal.insert({ hud_pal->GetName(), hud_pal });
		m_palettes_internal.insert({ item_pal->GetName(), item_pal });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadTextGraphics()
{
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Graphics::DOWN_ARROW);
		file >> inc;
		auto down_arrow = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::DOWN_ARROW, inc.path, false, 8, 8, 4, Tileset::BLOCK2X2);
		file.Goto(RomOffsets::Graphics::RIGHT_ARROW);
		file >> inc;
		auto right_arrow = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::RIGHT_ARROW, inc.path, false, 8, 8, 4, Tileset::BLOCK2X2);
		m_misc_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
		m_misc_gfx_by_name.insert({ right_arrow->GetName(), right_arrow });
		m_misc_gfx_internal.insert({ down_arrow->GetName(), down_arrow });
		m_misc_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadSwordFx()
{
	try
	{
		AsmFile file(GetBasePath() / m_sword_fx_path);
		AsmFile::IncludeFile inc;
		AsmFile::Label name;
		file >> name >> inc;
		auto tilemap_bytes = ReadBytes(GetBasePath() / inc.path);
		auto m = Tilemap2DEntry::Create(this, tilemap_bytes, name, inc.path, Tilemap2D::Compression::LZ77, 0x6B4);
		m_misc_tilemaps.insert({ m->GetName(), m });
		m_misc_tilemaps_internal.insert({ RomOffsets::Graphics::INV_TILEMAP, m });
		auto names = { RomOffsets::Graphics::SWORD_MAGIC, RomOffsets::Graphics::SWORD_THUNDER,
					   RomOffsets::Graphics::SWORD_GAIA,  RomOffsets::Graphics::SWORD_ICE,
					   RomOffsets::Graphics::COINFALL };
		auto names_it = names.begin();
		int count = 0;
		while (file.IsGood())
		{
			file >> name >> inc;
			std::shared_ptr<TilesetEntry> e;
			// Gaia FX, Coinfall FX
			if ((count == 2) || (count == 4))
			{
				e = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
					name, inc.path, true, 8, 8, 4, Tileset::BLOCK4X4);
			}
			else
			{
				e = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
					name, inc.path, true, 8, 8, 4, Tileset::BLOCK4X6);
			}
			m_sword_fx.insert({ e->GetName(), e });
			if (count < names.size())
			{
				m_sword_fx_internal.insert({ *names_it++, e });
			}
			else
			{
				m_sword_fx_internal.insert({ e->GetName(), e });
			}
			count++;
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadStatusFx()
{
	try
	{
		AsmFile file(GetBasePath() / m_status_fx_path);
		AsmFile::IncludeFile inc;
		AsmFile::Label name;
		while (file.IsGood())
		{
			file >> name >> inc;
			std::shared_ptr<TilesetEntry> e = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), name,
				inc.path, true, 8, 8, 4, Tileset::BLOCK4X4);
			m_status_fx_frames.insert({ e->GetName(), e });
		}
		AsmFile pfile(GetBasePath() / m_status_fx_pointers_path);
		std::vector<std::string> status = { RomOffsets::Graphics::STATUS_FX_POISON, RomOffsets::Graphics::STATUS_FX_CONFUSION,
					   RomOffsets::Graphics::STATUS_FX_PARALYSIS, RomOffsets::Graphics::STATUS_FX_CURSE };
		std::vector<std::string> status_frame_names = { RomOffsets::Graphics::STATUS_FX_POISON_FRAME, RomOffsets::Graphics::STATUS_FX_CONFUSION_FRAME,
					   RomOffsets::Graphics::STATUS_FX_PARALYSIS_FRAME, RomOffsets::Graphics::STATUS_FX_CURSE_FRAME };
		for (const auto& s : status)
		{
			pfile.Goto(s);
			std::string frame;
			do
			{
				pfile >> frame;
				m_status_fx[s].push_back(frame);
			} while (pfile.IsGood() && !pfile.IsLabel());
		}
		for (int si = 0; si < status.size(); si++)
		{
			const auto& status_name = status[si];
			const auto& status_frame_name = status_frame_names[si];
			const auto& status_frames = m_status_fx[status_name];
			for (int fi = 0; fi < status_frames.size(); ++fi)
			{
				std::string fname = StrPrintf(status_frame_name, fi + 1);
				m_status_fx_internal[status_name].push_back(status_frame_name);
				m_status_fx_frames_internal.insert({ status_frame_name, m_status_fx_frames[status_frames[fi]] });
			}
		}
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadHuffmanData()
{
	filesystem::path path = GetBasePath() / m_huffman_offset_path;
	m_huffman_offsets = ReadBytes(path);
	path = GetBasePath() / m_huffman_table_path;
	m_huffman_tables = ReadBytes(path);
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Graphics::TEXTBOX_2LINE_MAP);
		file >> inc;
		auto tb2 = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::TEXTBOX_2LINE_MAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 6);
		file.Goto(RomOffsets::Graphics::TEXTBOX_3LINE_MAP);
		file >> inc;
		auto tb3 = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::TEXTBOX_3LINE_MAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 8);
		m_misc_tilemaps.insert({ tb2->GetName(), tb2 });
		m_misc_tilemaps_internal.insert({ tb2->GetName(), tb2 });
		m_misc_tilemaps.insert({ tb3->GetName(), tb3 });
		m_misc_tilemaps_internal.insert({ tb3->GetName(), tb3 });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadHudData()
{
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomOffsets::Graphics::HUD_TILEMAP);
		file >> inc;
		auto map = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomOffsets::Graphics::HUD_TILEMAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 3);
		file.Goto(RomOffsets::Graphics::HUD_TILESET);
		file >> inc;
		auto ts = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), RomOffsets::Graphics::HUD_TILESET, inc.path);
		m_misc_tilemaps.insert({ map->GetName(), map });
		m_misc_tilemaps_internal.insert({ map->GetName(), map });
		m_misc_gfx_by_name.insert({ ts->GetName(), ts });
		m_misc_gfx_internal.insert({ ts->GetName(), ts });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadEndCreditData()
{
	try
	{
		AsmFile file(GetBasePath() / m_end_credits_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		file >> lbl >> inc;
		std::string pal_name = lbl;
		auto pal_path = inc.path;
		auto pal_bytes = ReadBytes(GetBasePath() / pal_path);
		file >> lbl >> inc;
		std::string font_name = lbl;
		auto font_path = inc.path;
		auto font_bytes = ReadBytes(GetBasePath() / font_path);
		file >> lbl >> inc;
		std::string logos_name = lbl;
		auto logos_path = inc.path;
		auto logos_bytes = ReadBytes(GetBasePath() / logos_path);
		file >> lbl >> inc;
		std::string map_name = lbl;
		auto map_path = inc.path;
		auto map_bytes = ReadBytes(GetBasePath() / map_path);
		m_end_credits_palette = PaletteEntry::Create(this, pal_bytes, pal_name, pal_path, Palette::Type::END_CREDITS);
		m_end_credits_tileset = TilesetEntry::Create(this, logos_bytes, logos_name, logos_path);
		m_end_credits_map = Tilemap2DEntry::Create(this, map_bytes, map_name, map_path, Tilemap2D::Compression::RLE, 0x100);
		auto font = TilesetEntry::Create(this, font_bytes, font_name, font_path, true, 8, 8, 2);
		m_fonts_by_name.insert({ font->GetName(), font });
		m_fonts_internal.insert({ RomOffsets::Graphics::END_CREDITS_FONT, font });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadIslandMapData()
{
	try
	{
		AsmFile file(GetBasePath() / m_island_map_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, filesystem::path, ByteVector>> entries;
		file >> lbl >> inc;
		m_island_map_routine_path = inc.path;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			auto path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 8);
		auto fg_tiles = TilesetEntry::Create(this, std::get<2>(entries[0]), std::get<0>(entries[0]), std::get<1>(entries[0]));
		auto fg_map = Tilemap2DEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]), Tilemap2D::Compression::RLE, 0x100);
		auto bg_tiles = TilesetEntry::Create(this, std::get<2>(entries[2]), std::get<0>(entries[2]), std::get<1>(entries[2]));
		auto bg_map = Tilemap2DEntry::Create(this, std::get<2>(entries[3]), std::get<0>(entries[3]), std::get<1>(entries[3]), Tilemap2D::Compression::RLE, 0x100);
		auto dots_tiles = TilesetEntry::Create(this, std::get<2>(entries[4]), std::get<0>(entries[4]), std::get<1>(entries[4]));
		auto friday_tiles = TilesetEntry::Create(this, std::get<2>(entries[5]), std::get<0>(entries[5]), std::get<1>(entries[5]), true, 8, 8, 4, Tileset::BLOCK2X2);
		auto fg_pal = PaletteEntry::Create(this, std::get<2>(entries[6]), std::get<0>(entries[6]), std::get<1>(entries[6]), Palette::Type::FULL);
		auto bg_pal = PaletteEntry::Create(this, std::get<2>(entries[7]), std::get<0>(entries[7]), std::get<1>(entries[7]), Palette::Type::FULL);

		m_island_map_tiles.insert({ fg_tiles->GetName(), fg_tiles });
		m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FG_TILES, fg_tiles });
		m_island_map_tiles.insert({ bg_tiles->GetName(), bg_tiles });
		m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_BG_TILES, bg_tiles });
		m_island_map_tiles.insert({ dots_tiles->GetName(), dots_tiles });
		m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_DOTS, dots_tiles });
		m_island_map_tiles.insert({ friday_tiles->GetName(), friday_tiles });
		m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FRIDAY, friday_tiles });

		m_island_map_tilemaps.insert({ fg_map->GetName(), fg_map });
		m_island_map_tilemaps_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FG_MAP, fg_map });
		m_island_map_tilemaps.insert({ bg_map->GetName(), bg_map });
		m_island_map_tilemaps_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_BG_MAP, bg_map });

		m_island_map_pals.insert({ fg_pal->GetName(), fg_pal });
		m_island_map_pals_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FG_PAL, fg_pal });
		m_island_map_pals.insert({ bg_pal->GetName(), bg_pal });
		m_island_map_pals_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_BG_PAL, bg_pal });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadTitleScreenData()
{
	try
	{
		AsmFile file(GetBasePath() / m_title_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			auto path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 13);
		m_title_routines_1_path = std::get<1>(entries[0]);
		auto blue_pal = PaletteEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]), Palette::Type::TITLE_BLUE_FADE);
		m_title_routines_2_path = std::get<1>(entries[2]);
		auto yellow_pal = PaletteEntry::Create(this, std::get<2>(entries[3]), std::get<0>(entries[3]), std::get<1>(entries[3]), Palette::Type::TITLE_YELLOW);
		m_title_routines_3_path = std::get<1>(entries[4]);
		auto title_1_tiles = TilesetEntry::Create(this, std::get<2>(entries[5]), std::get<0>(entries[5]), std::get<1>(entries[5]));
		auto title_2_tiles = TilesetEntry::Create(this, std::get<2>(entries[6]), std::get<0>(entries[6]), std::get<1>(entries[6]));
		auto title_3_tiles = TilesetEntry::Create(this, std::get<2>(entries[7]), std::get<0>(entries[7]), std::get<1>(entries[7]));
		auto title_1_map = Tilemap2DEntry::Create(this, std::get<2>(entries[8]), std::get<0>(entries[8]), std::get<1>(entries[8]), Tilemap2D::Compression::RLE, 0x100);
		auto title_2_map = Tilemap2DEntry::Create(this, std::get<2>(entries[9]), std::get<0>(entries[9]), std::get<1>(entries[9]), Tilemap2D::Compression::RLE, 0x100);
		auto title_3_map = Tilemap2DEntry::Create(this, std::get<2>(entries[10]), std::get<0>(entries[10]), std::get<1>(entries[10]), Tilemap2D::Compression::RLE, 0x100);
		auto title_3_pal = PaletteEntry::Create(this, std::get<2>(entries[11]), std::get<0>(entries[11]), std::get<1>(entries[11]), Palette::Type::FULL);
		auto title_3_pal_highlight = PaletteEntry::Create(this, std::get<2>(entries[12]), std::get<0>(entries[12]), std::get<1>(entries[12]), Palette::Type::FULL);

		m_title_tiles.insert({ title_1_tiles->GetName(), title_1_tiles });
		m_title_tiles_internal.insert({ RomOffsets::Graphics::TITLE_1_TILES, title_1_tiles });
		m_title_tiles.insert({ title_2_tiles->GetName(), title_2_tiles });
		m_title_tiles_internal.insert({ RomOffsets::Graphics::TITLE_2_TILES, title_2_tiles });
		m_title_tiles.insert({ title_3_tiles->GetName(), title_3_tiles });
		m_title_tiles_internal.insert({ RomOffsets::Graphics::TITLE_3_TILES, title_3_tiles });

		m_title_tilemaps.insert({ title_1_map->GetName(), title_1_map });
		m_title_tilemaps_internal.insert({ RomOffsets::Graphics::TITLE_1_MAP, title_1_map });
		m_title_tilemaps.insert({ title_2_map->GetName(), title_2_map });
		m_title_tilemaps_internal.insert({ RomOffsets::Graphics::TITLE_2_MAP, title_2_map });
		m_title_tilemaps.insert({ title_3_map->GetName(), title_3_map });
		m_title_tilemaps_internal.insert({ RomOffsets::Graphics::TITLE_3_MAP, title_3_map });

		m_title_pals.insert({ title_3_pal->GetName(), title_3_pal });
		m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_3_PAL, title_3_pal });
		m_title_pals.insert({ title_3_pal_highlight->GetName(), title_3_pal_highlight });
		m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT, title_3_pal_highlight });
		m_title_pals.insert({ yellow_pal->GetName(), yellow_pal });
		m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_PALETTE_YELLOW, yellow_pal });
		m_title_pals.insert({ blue_pal->GetName(), blue_pal });
		m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_PALETTE_BLUE, blue_pal });

		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::RomLoadFonts(const Rom& rom)
{
	uint32_t sys_font_lea = rom.read<uint32_t>(RomOffsets::Graphics::SYS_FONT);
	uint32_t sys_font_begin = Disasm::LEA_PCRel(sys_font_lea, rom.get_address(RomOffsets::Graphics::SYS_FONT));
	uint32_t sys_font_size = rom.get_address(RomOffsets::Graphics::SYS_FONT_SIZE);
	auto sys_font_bytes = rom.read_array<uint8_t>(sys_font_begin, sys_font_size);
	auto sys_font = TilesetEntry::Create(this, sys_font_bytes, RomOffsets::Graphics::SYS_FONT,
		RomOffsets::Graphics::SYS_FONT_FILE, false);
	sys_font->SetStartAddress(sys_font_begin);
	m_fonts_by_name.insert({ sys_font->GetName(), sys_font });
	m_fonts_internal.insert({ sys_font->GetName(), sys_font });
	return true;
}

bool GraphicsData::RomLoadStrings(const Rom& rom)
{
	uint32_t line1_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_LINE1);
	uint32_t line1_begin = Disasm::LEA_PCRel(line1_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE1));
	m_system_strings[0] = rom.read_string(line1_begin);
	uint32_t line2_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_NTSC);
	uint32_t line2_begin = Disasm::LEA_PCRel(line2_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_NTSC));
	m_system_strings[1] = rom.read_string(line2_begin);
	uint32_t line3_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_PAL);
	uint32_t line3_begin = Disasm::LEA_PCRel(line3_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_PAL));
	m_system_strings[2] = rom.read_string(line3_begin);
	uint32_t line4_lea = rom.read<uint32_t>(RomOffsets::Strings::REGION_ERROR_LINE3);
	uint32_t line4_begin = Disasm::LEA_PCRel(line4_lea, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE3));
	m_system_strings[3] = rom.read_string(line4_begin);
	return true;
}

bool GraphicsData::RomLoadInventoryGraphics(const Rom& rom)
{
	auto load_bytes = [&](const std::string& lea_loc, const std::string& size)
	{
		uint32_t pc = rom.get_address(lea_loc);
		uint32_t lea = rom.read<uint32_t>(pc);
		uint32_t start = Disasm::LEA_PCRel(lea, pc);
		uint32_t sz = rom.get_address(size);
		return rom.read_array<uint8_t>(start, sz);
	};
	auto load_pal = [&](const std::string& lea_loc, Palette::Type type)
	{
		uint32_t pc = rom.get_address(lea_loc);
		uint32_t lea = rom.read<uint32_t>(pc);
		uint32_t start = Disasm::LEA_PCRel(lea, pc);
		uint32_t sz = Palette::GetSizeBytes(type);
		return rom.read_array<uint8_t>(start, sz);
	};
	auto menu_font = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_FONT, RomOffsets::Graphics::INV_FONT_SIZE),
		RomOffsets::Graphics::INV_FONT, RomOffsets::Graphics::INV_FONT_FILE, false, 8, 8, 1);
	auto cursor = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_CURSOR, RomOffsets::Graphics::INV_CURSOR_SIZE),
		RomOffsets::Graphics::INV_CURSOR, RomOffsets::Graphics::INV_CURSOR_FILE, false, 8, 8, 2, Tileset::BLOCK4X4);
	auto arrow = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_ARROW, RomOffsets::Graphics::INV_ARROW_SIZE),
		RomOffsets::Graphics::INV_ARROW, RomOffsets::Graphics::INV_ARROW_FILE, false, 8, 8, 2, Tileset::BLOCK2X2);
	auto unused1 = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_UNUSED1, RomOffsets::Graphics::INV_UNUSED1_SIZE),
		RomOffsets::Graphics::INV_UNUSED1, RomOffsets::Graphics::INV_UNUSED1_FILE, false, 8, 11, 2);
	auto unused2 = TilesetEntry::Create(this, load_bytes(RomOffsets::Graphics::INV_UNUSED2, RomOffsets::Graphics::INV_UNUSED2_SIZE),
		RomOffsets::Graphics::INV_UNUSED2, RomOffsets::Graphics::INV_UNUSED2_FILE, false, 8, 10, 2);
	auto pal1 = PaletteEntry::Create(this, load_pal(RomOffsets::Graphics::INV_PAL1, Palette::Type::LOW8),
		RomOffsets::Graphics::INV_PAL1, RomOffsets::Graphics::INV_PAL1_FILE, Palette::Type::LOW8);
	auto pal2 = PaletteEntry::Create(this, load_pal(RomOffsets::Graphics::INV_PAL2, Palette::Type::FULL),
		RomOffsets::Graphics::INV_PAL2, RomOffsets::Graphics::INV_PAL2_FILE, Palette::Type::FULL);
	m_fonts_by_name.insert({ menu_font->GetName(), menu_font });
	m_fonts_internal.insert({ RomOffsets::Graphics::INV_FONT, menu_font });
	m_misc_gfx_by_name.insert({ cursor->GetName(), cursor });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_CURSOR, cursor });
	m_misc_gfx_by_name.insert({ arrow->GetName(), arrow });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_ARROW, arrow });
	m_misc_gfx_by_name.insert({ unused1->GetName(), unused1 });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_UNUSED1, unused1 });
	m_misc_gfx_by_name.insert({ unused2->GetName(), unused2 });
	m_misc_gfx_internal.insert({ RomOffsets::Graphics::INV_UNUSED2, unused2 });
	m_palettes_by_name.insert({ pal1->GetName(), pal1 });
	m_palettes_internal.insert({ RomOffsets::Graphics::INV_PAL1, pal1 });
	m_palettes_by_name.insert({ pal2->GetName(), pal2 });
	m_palettes_internal.insert({ RomOffsets::Graphics::INV_PAL2, pal2 });
	return true;
}

bool GraphicsData::RomLoadCompressedStringData(const Rom& rom)
{
	uint32_t font_ptr = rom.read<uint32_t>(RomOffsets::Graphics::MAIN_FONT_PTR);
	uint32_t bank_ptr = rom.read<uint32_t>(RomOffsets::Strings::STRING_BANK_PTR_PTR);
	uint32_t banks_begin = rom.read<uint32_t>(bank_ptr);
	auto font_bytes = rom.read_array<uint8_t>(font_ptr, banks_begin - font_ptr);
	auto string_bytes = rom.read_array<uint8_t>(banks_begin, bank_ptr - banks_begin);
	auto it = string_bytes.cbegin();
	while (it != string_bytes.cend() && *it != 0x00)
	{
		m_strings.push_back(ByteVector(it, it + *it));
		it += *it;
	}
	auto e = TilesetEntry::Create(this, font_bytes, RomOffsets::Graphics::MAIN_FONT,
		RomOffsets::Graphics::MAIN_FONT_FILE, false, 16, 15, 1);
	m_fonts_by_name.insert({ e->GetName(), e });
	m_fonts_internal.insert({ e->GetName(), e });
	return true;
}

bool GraphicsData::RomLoadPalettes(const Rom& rom)
{
	auto extract_pals = [&](const ByteVector& bytes, const std::string& name, filesystem::path fname, Palette::Type type)
	{
		int idx = 0;
		for (auto it = bytes.cbegin(); it != bytes.cend(); it += Palette::GetSizeBytes(type))
		{
			auto pal_bytes = ByteVector(it, it + Palette::GetSizeBytes(type));
			auto e = PaletteEntry::Create(this, pal_bytes, name + ":" + std::to_string(idx++), fname, type);
			m_palettes_by_name.insert({ e->GetName(), e });
			m_palettes_internal.insert({ e->GetName(), e });
		}
	};
	uint32_t player_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::PLAYER_PAL),
		rom.get_address(RomOffsets::Graphics::PLAYER_PAL));
	uint32_t hud_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::HUD_PAL),
		rom.get_address(RomOffsets::Graphics::HUD_PAL));
	uint32_t item_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::INV_ITEM_PAL),
		rom.get_address(RomOffsets::Graphics::INV_ITEM_PAL));
	uint32_t sword_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::SWORD_PAL_SWAPS),
		rom.get_address(RomOffsets::Graphics::SWORD_PAL_SWAPS));
	uint32_t armour_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ARMOUR_PAL_SWAPS),
		rom.get_address(RomOffsets::Graphics::ARMOUR_PAL_SWAPS));
	uint32_t armour_end_addr = rom.get_section(RomOffsets::Graphics::EQUIP_PAL_SECTION).end;
	uint32_t sword_pal_size = armour_addr - sword_addr;
	uint32_t armour_pal_size = armour_end_addr - armour_addr;
	auto player_bytes = rom.read_array<uint8_t>(player_addr, Palette::GetSizeBytes(Palette::Type::FULL));
	auto hud_bytes = rom.read_array<uint8_t>(hud_addr, Palette::GetSizeBytes(Palette::Type::HUD));
	auto item_bytes = rom.read_array<uint8_t>(item_addr, Palette::GetSizeBytes(Palette::Type::FULL));
	auto sword_bytes = rom.read_array<uint8_t>(sword_addr, sword_pal_size);
	auto armour_bytes = rom.read_array<uint8_t>(armour_addr, armour_pal_size);
	auto player_pal = PaletteEntry::Create(this, player_bytes, RomOffsets::Graphics::PLAYER_PAL,
		RomOffsets::Graphics::PLAYER_PAL_FILE, Palette::Type::FULL);
	auto hud_pal = PaletteEntry::Create(this, hud_bytes, RomOffsets::Graphics::HUD_PAL,
		RomOffsets::Graphics::HUD_PAL_FILE, Palette::Type::HUD);
	auto item_pal = PaletteEntry::Create(this, item_bytes, RomOffsets::Graphics::INV_ITEM_PAL,
		RomOffsets::Graphics::INV_ITEM_PAL_FILE, Palette::Type::FULL);
	extract_pals(sword_bytes, RomOffsets::Graphics::SWORD_PAL_SWAPS, RomOffsets::Graphics::SWORD_PAL_FILE, Palette::Type::SWORD);
	extract_pals(armour_bytes, RomOffsets::Graphics::ARMOUR_PAL_SWAPS, RomOffsets::Graphics::ARMOUR_PAL_FILE, Palette::Type::ARMOUR);
	m_palettes_by_name.insert({ player_pal->GetName(), player_pal });
	m_palettes_internal.insert({ player_pal->GetName(), player_pal });
	m_palettes_by_name.insert({ hud_pal->GetName(), hud_pal });
	m_palettes_internal.insert({ hud_pal->GetName(), hud_pal });
	m_palettes_by_name.insert({ item_pal->GetName(), item_pal });
	m_palettes_internal.insert({ item_pal->GetName(), item_pal });
	return true;
}

bool GraphicsData::RomLoadTextGraphics(const Rom& rom)
{
	uint32_t down_arrow_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::DOWN_ARROW),
		rom.get_address(RomOffsets::Graphics::DOWN_ARROW));
	uint32_t down_arrow_size = rom.get_section(RomOffsets::Graphics::DOWN_ARROW_SECTION).size();
	uint32_t right_arrow_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::RIGHT_ARROW),
		rom.get_address(RomOffsets::Graphics::RIGHT_ARROW));
	uint32_t right_arrow_size = rom.get_section(RomOffsets::Graphics::RIGHT_ARROW_SECTION).size();
	auto down_arrow_bytes = rom.read_array<uint8_t>(down_arrow_addr, down_arrow_size);
	auto right_arrow_bytes = rom.read_array<uint8_t>(right_arrow_addr, right_arrow_size);
	auto down_arrow = TilesetEntry::Create(this, down_arrow_bytes, RomOffsets::Graphics::DOWN_ARROW,
		RomOffsets::Graphics::DOWN_ARROW_FILE, false, 8, 8, 4, Tileset::BLOCK2X2);
	auto right_arrow = TilesetEntry::Create(this, right_arrow_bytes, RomOffsets::Graphics::RIGHT_ARROW,
		RomOffsets::Graphics::RIGHT_ARROW_FILE, false, 8, 8, 4, Tileset::BLOCK2X2);
	down_arrow->SetStartAddress(down_arrow_size);
	right_arrow->SetStartAddress(right_arrow_size);
	m_misc_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
	m_misc_gfx_by_name.insert({ right_arrow->GetName(), right_arrow });
	m_misc_gfx_internal.insert({ down_arrow->GetName(), down_arrow });
	m_misc_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
	return true;
}

bool GraphicsData::RomLoadSwordFx(const Rom& rom)
{
	uint32_t inv_tilemap_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::INV_TILEMAP),
		rom.get_address(RomOffsets::Graphics::INV_TILEMAP));
	uint32_t magic_sword_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::SWORD_MAGIC),
		rom.get_address(RomOffsets::Graphics::SWORD_MAGIC));
	uint32_t thunder_sword_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::SWORD_THUNDER),
		rom.get_address(RomOffsets::Graphics::SWORD_THUNDER));
	uint32_t gaia_sword_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::SWORD_GAIA),
		rom.get_address(RomOffsets::Graphics::SWORD_GAIA));
	uint32_t ice_sword_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::SWORD_ICE),
		rom.get_address(RomOffsets::Graphics::SWORD_ICE));
	uint32_t coinfall_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::COINFALL),
		rom.get_address(RomOffsets::Graphics::COINFALL));
	uint32_t end = rom.get_section(RomOffsets::Graphics::SWORD_FX_SECTION).end;
	
	uint32_t inv_tilemap_size = magic_sword_addr - inv_tilemap_addr;
	uint32_t magic_sword_size = thunder_sword_addr - magic_sword_addr;
	uint32_t thunder_sword_size = gaia_sword_addr - thunder_sword_addr;
	uint32_t gaia_sword_size = ice_sword_addr - gaia_sword_addr;
	uint32_t ice_sword_size = coinfall_addr - ice_sword_addr;
	uint32_t coinfall_size = end - coinfall_addr;

	auto inv_tilemap_bytes = rom.read_array<uint8_t>(inv_tilemap_addr, inv_tilemap_size);
	auto magic_sword_bytes = rom.read_array<uint8_t>(magic_sword_addr, magic_sword_size);
	auto thunder_sword_bytes = rom.read_array<uint8_t>(thunder_sword_addr, thunder_sword_size);
	auto gaia_sword_bytes = rom.read_array<uint8_t>(gaia_sword_addr, gaia_sword_size);
	auto ice_sword_bytes = rom.read_array<uint8_t>(ice_sword_addr, ice_sword_size);
	auto coinfall_bytes = rom.read_array<uint8_t>(coinfall_addr, coinfall_size);

	auto inv_tilemap = Tilemap2DEntry::Create(this, inv_tilemap_bytes, RomOffsets::Graphics::INV_TILEMAP,
		RomOffsets::Graphics::INV_TILEMAP_FILE, Tilemap2D::Compression::LZ77, 0);
	auto magic_sword = TilesetEntry::Create(this, magic_sword_bytes, RomOffsets::Graphics::SWORD_MAGIC,
		RomOffsets::Graphics::SWORD_MAGIC_FILE, true, 8, 8, 4, Tileset::BLOCK4X6);
	auto thunder_sword = TilesetEntry::Create(this, thunder_sword_bytes, RomOffsets::Graphics::SWORD_THUNDER,
		RomOffsets::Graphics::SWORD_THUNDER_FILE, true, 8, 8, 4, Tileset::BLOCK4X6);
	auto gaia_sword = TilesetEntry::Create(this, gaia_sword_bytes, RomOffsets::Graphics::SWORD_GAIA,
		RomOffsets::Graphics::SWORD_GAIA_FILE, true, 8, 8, 4, Tileset::BLOCK4X4);
	auto ice_sword = TilesetEntry::Create(this, ice_sword_bytes, RomOffsets::Graphics::SWORD_ICE,
		RomOffsets::Graphics::SWORD_ICE_FILE, true, 8, 8, 4, Tileset::BLOCK4X6);
	auto coinfall = TilesetEntry::Create(this, coinfall_bytes, RomOffsets::Graphics::COINFALL,
		RomOffsets::Graphics::COINFALL_FILE, true, 8, 8, 4, Tileset::BLOCK4X4);

	inv_tilemap->SetStartAddress(inv_tilemap_addr);
	magic_sword->SetStartAddress(magic_sword_addr);
	thunder_sword->SetStartAddress(thunder_sword_addr);
	gaia_sword->SetStartAddress(gaia_sword_addr);
	ice_sword->SetStartAddress(ice_sword_addr);
	coinfall->SetStartAddress(coinfall_addr);

	m_misc_tilemaps.insert({ inv_tilemap->GetName(), inv_tilemap });
	m_misc_tilemaps_internal.insert({ inv_tilemap->GetName(), inv_tilemap });
	m_sword_fx.insert({ magic_sword->GetName(), magic_sword });
	m_sword_fx_internal.insert({ magic_sword->GetName(), magic_sword });
	m_sword_fx.insert({ thunder_sword->GetName(), thunder_sword });
	m_sword_fx_internal.insert({ thunder_sword->GetName(), thunder_sword });
	m_sword_fx.insert({ gaia_sword->GetName(), gaia_sword });
	m_sword_fx_internal.insert({ gaia_sword->GetName(), gaia_sword });
	m_sword_fx.insert({ ice_sword->GetName(), ice_sword });
	m_sword_fx_internal.insert({ ice_sword->GetName(), ice_sword });
	m_sword_fx.insert({ coinfall->GetName(), coinfall });
	m_sword_fx_internal.insert({ coinfall->GetName(), coinfall });
	return true;
}

bool GraphicsData::RomLoadStatusFx(const Rom& rom)
{
	uint32_t poison_ptrs_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::STATUS_FX_POISON),
		rom.get_address(RomOffsets::Graphics::STATUS_FX_POISON));
	uint32_t confusion_ptrs_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::STATUS_FX_CONFUSION),
		rom.get_address(RomOffsets::Graphics::STATUS_FX_CONFUSION));
	uint32_t paralysis_ptrs_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::STATUS_FX_PARALYSIS),
		rom.get_address(RomOffsets::Graphics::STATUS_FX_PARALYSIS));
	uint32_t curse_ptrs_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::STATUS_FX_CURSE),
		rom.get_address(RomOffsets::Graphics::STATUS_FX_CURSE));
	std::vector<uint32_t> poison_frame_ptrs;
	std::vector<uint32_t> confusion_frame_ptrs;
	std::vector<uint32_t> paralysis_frame_ptrs;
	std::vector<uint32_t> curse_frame_ptrs;
	
	uint32_t ptrs_end = 0xFFFFFF;
	auto get_ptrs = [&](uint32_t addr, uint32_t& end, std::vector<uint32_t>& ptrs)
	{
		int i = 0;
		while (addr < end)
		{
			uint32_t p = rom.inc_read<uint32_t>(addr);
			if (p < ptrs_end)
			{
				ptrs_end = p;
			}
			ptrs.push_back(p);
		}
	};
	get_ptrs(poison_ptrs_addr, confusion_ptrs_addr, poison_frame_ptrs);
	get_ptrs(confusion_ptrs_addr, paralysis_ptrs_addr, confusion_frame_ptrs);
	get_ptrs(paralysis_ptrs_addr, curse_ptrs_addr, paralysis_frame_ptrs);
	get_ptrs(curse_ptrs_addr, ptrs_end, curse_frame_ptrs);
	uint32_t end = rom.get_section(RomOffsets::Graphics::STATUS_FX_SECTION).end;
	auto read_frames = [&](const std::vector<uint32_t>& ptrs, const std::string status_name,
		const std::string& frame_name, const std::string& filename)
	{
		int i = 1;
		for (const auto& addr : ptrs)
		{
			std::string name = StrPrintf(frame_name, i);
			std::string fname = StrPrintf(filename, i++);
			auto b = rom.read_array<uint8_t>(addr, end - addr);
			auto e = TilesetEntry::Create(this, b, name, fname, true, 8, 8, 4, Tileset::BLOCK4X4);
			e->SetStartAddress(addr);
			m_status_fx_frames.insert({ name, e });
			m_status_fx_frames_internal.insert({ name, e });
			m_status_fx[status_name].push_back(name);
			m_status_fx_internal[status_name].push_back(name);
		}
	};
	read_frames(poison_frame_ptrs, RomOffsets::Graphics::STATUS_FX_POISON,
		RomOffsets::Graphics::STATUS_FX_POISON_FRAME, RomOffsets::Graphics::STATUS_FX_POISON_FILE);
	read_frames(confusion_frame_ptrs, RomOffsets::Graphics::STATUS_FX_CONFUSION,
		RomOffsets::Graphics::STATUS_FX_CONFUSION_FRAME, RomOffsets::Graphics::STATUS_FX_CONFUSION_FILE);
	read_frames(paralysis_frame_ptrs, RomOffsets::Graphics::STATUS_FX_PARALYSIS,
		RomOffsets::Graphics::STATUS_FX_PARALYSIS_FRAME, RomOffsets::Graphics::STATUS_FX_PARALYSIS_FILE);
	read_frames(curse_frame_ptrs, RomOffsets::Graphics::STATUS_FX_CURSE,
		RomOffsets::Graphics::STATUS_FX_CURSE_FRAME, RomOffsets::Graphics::STATUS_FX_CURSE_FILE);
	return true;
}

bool GraphicsData::RomLoadHuffmanData(const Rom& rom)
{
	uint32_t textbox_2l_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TEXTBOX_2LINE_MAP),
		rom.get_address(RomOffsets::Graphics::TEXTBOX_2LINE_MAP));
	uint32_t textbox_3l_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TEXTBOX_3LINE_MAP),
		rom.get_address(RomOffsets::Graphics::TEXTBOX_3LINE_MAP));
	uint32_t huff_offsets_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Strings::HUFFMAN_OFFSETS),
		rom.get_address(RomOffsets::Strings::HUFFMAN_OFFSETS));
	uint32_t huff_tables_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Strings::HUFFMAN_TABLES),
		rom.get_address(RomOffsets::Strings::HUFFMAN_TABLES));
	uint32_t end = rom.get_section(RomOffsets::Strings::HUFFMAN_SECTION).end;

	uint32_t textbox_3l_size = textbox_2l_addr - textbox_3l_addr;
	uint32_t textbox_2l_size = huff_offsets_addr - textbox_2l_addr;
	uint32_t huff_offsets_size = huff_tables_addr - huff_offsets_addr;
	uint32_t huff_tables_size = end - huff_tables_addr;

	auto textbox_3l_bytes = rom.read_array<uint8_t>(textbox_3l_addr, textbox_3l_size);
	auto textbox_2l_bytes = rom.read_array<uint8_t>(textbox_2l_addr, textbox_2l_size);
	m_huffman_offsets = rom.read_array<uint8_t>(huff_offsets_addr, huff_offsets_size);
	m_huffman_tables = rom.read_array<uint8_t>(huff_tables_addr, huff_tables_size);

	auto textbox_3l = Tilemap2DEntry::Create(this, textbox_3l_bytes, RomOffsets::Graphics::TEXTBOX_3LINE_MAP,
		RomOffsets::Graphics::TEXTBOX_3LINE_MAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 8);
	auto textbox_2l = Tilemap2DEntry::Create(this, textbox_2l_bytes, RomOffsets::Graphics::TEXTBOX_2LINE_MAP,
		RomOffsets::Graphics::TEXTBOX_2LINE_MAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 6);

	m_misc_tilemaps.insert({ textbox_3l->GetName(), textbox_3l });
	m_misc_tilemaps_internal.insert({ textbox_3l->GetName(), textbox_3l });
	m_misc_tilemaps.insert({ textbox_2l->GetName(), textbox_2l });
	m_misc_tilemaps_internal.insert({ textbox_2l->GetName(), textbox_2l });

	return true;
}

bool GraphicsData::RomLoadHudData(const Rom& rom)
{
	uint32_t map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::HUD_TILEMAP),
		rom.get_address(RomOffsets::Graphics::HUD_TILEMAP));
	uint32_t ts_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::HUD_TILESET),
		rom.get_address(RomOffsets::Graphics::HUD_TILESET));
	uint32_t end = rom.get_section(RomOffsets::Graphics::HUD_SECTION).end;

	uint32_t map_size = ts_addr - map_addr;
	uint32_t ts_size = end - ts_addr;

	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);
	auto ts_bytes = rom.read_array<uint8_t>(ts_addr, ts_size);

	auto map = Tilemap2DEntry::Create(this, map_bytes, RomOffsets::Graphics::HUD_TILEMAP,
		RomOffsets::Graphics::HUD_TILEMAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 3);
	auto ts = TilesetEntry::Create(this, ts_bytes, RomOffsets::Graphics::HUD_TILESET,
		RomOffsets::Graphics::HUD_TILESET_FILE);

	m_misc_tilemaps.insert({ map->GetName(), map });
	m_misc_tilemaps_internal.insert({ map->GetName(), map });
	m_misc_gfx_by_name.insert({ ts->GetName(), ts });
	m_misc_gfx_internal.insert({ ts->GetName(), ts });

	return true;
}

bool GraphicsData::RomLoadEndCreditData(const Rom& rom)
{
	uint32_t pal_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::END_CREDITS_PAL),
		rom.get_address(RomOffsets::Graphics::END_CREDITS_PAL));
	uint32_t font_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::END_CREDITS_FONT),
		rom.get_address(RomOffsets::Graphics::END_CREDITS_FONT));
	uint32_t logos_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::END_CREDITS_LOGOS),
		rom.get_address(RomOffsets::Graphics::END_CREDITS_LOGOS));
	uint32_t map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::END_CREDITS_MAP),
		rom.get_address(RomOffsets::Graphics::END_CREDITS_MAP));
	uint32_t end = rom.get_section(RomOffsets::Graphics::END_CREDITS_DATA).end;

	uint32_t pal_size = font_addr - pal_addr;
	uint32_t font_size = logos_addr - font_addr;
	uint32_t logos_size = map_addr - logos_addr;
	uint32_t map_size = end - map_addr;

	auto pal_bytes = rom.read_array<uint8_t>(pal_addr, pal_size);
	auto font_bytes = rom.read_array<uint8_t>(font_addr, font_size);
	auto logos_bytes = rom.read_array<uint8_t>(logos_addr, logos_size);
	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);

	m_end_credits_palette = PaletteEntry::Create(this, pal_bytes, RomOffsets::Graphics::END_CREDITS_PAL,
		RomOffsets::Graphics::END_CREDITS_PAL_FILE, Palette::Type::END_CREDITS);
	m_end_credits_tileset = TilesetEntry::Create(this, logos_bytes, RomOffsets::Graphics::END_CREDITS_LOGOS,
		RomOffsets::Graphics::END_CREDITS_LOGOS_FILE);
	m_end_credits_map = Tilemap2DEntry::Create(this, map_bytes, RomOffsets::Graphics::END_CREDITS_MAP,
		RomOffsets::Graphics::END_CREDITS_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto font = TilesetEntry::Create(this, font_bytes, RomOffsets::Graphics::END_CREDITS_FONT,
		RomOffsets::Graphics::END_CREDITS_FONT_FILE, true, 8, 8, 2);
	m_fonts_by_name.insert({ font->GetName(), font });
	m_fonts_internal.insert({ RomOffsets::Graphics::END_CREDITS_FONT, font });

	return true;
}

bool GraphicsData::RomLoadIslandMapData(const Rom& rom)
{
	uint32_t fg_tiles_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_FG_TILES),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FG_TILES));
	uint32_t fg_map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_FG_MAP),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FG_MAP));
	uint32_t bg_tiles_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_BG_TILES),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_BG_TILES));
	uint32_t bg_map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_BG_MAP),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_BG_MAP));
	uint32_t dots_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_DOTS),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_DOTS));
	uint32_t friday_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_FRIDAY),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FRIDAY));
	uint32_t fg_pal_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::ISLAND_MAP_FG_PAL),
		rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FG_PAL));

	uint32_t fg_tiles_size = fg_map_addr - fg_tiles_addr;
	uint32_t fg_map_size = bg_tiles_addr - fg_map_addr;
	uint32_t bg_tiles_size = bg_map_addr - bg_tiles_addr;
	uint32_t bg_map_size = dots_addr - bg_map_addr;
	uint32_t dots_size = friday_addr - dots_addr;
	uint32_t friday_size = fg_pal_addr - friday_addr;
	uint32_t fg_pal_size = Palette::GetSizeBytes(Palette::Type::FULL);
	uint32_t bg_pal_addr = fg_pal_addr + fg_pal_size;
	uint32_t bg_pal_size = Palette::GetSizeBytes(Palette::Type::FULL);

	auto fg_tiles_bytes = rom.read_array<uint8_t>(fg_tiles_addr, fg_tiles_size);
	auto fg_map_bytes = rom.read_array<uint8_t>(fg_map_addr, fg_map_size);
	auto bg_tiles_bytes = rom.read_array<uint8_t>(bg_tiles_addr, bg_tiles_size);
	auto bg_map_bytes = rom.read_array<uint8_t>(bg_map_addr, bg_map_size);
	auto dots_bytes = rom.read_array<uint8_t>(dots_addr, dots_size);
	auto friday_bytes = rom.read_array<uint8_t>(friday_addr, friday_size);
	auto fg_pal_bytes = rom.read_array<uint8_t>(fg_pal_addr, fg_pal_size);
	auto bg_pal_bytes = rom.read_array<uint8_t>(bg_pal_addr, bg_pal_size);

	auto fg_tiles = TilesetEntry::Create(this, fg_tiles_bytes, RomOffsets::Graphics::ISLAND_MAP_FG_TILES, RomOffsets::Graphics::ISLAND_MAP_FG_TILES_FILE);
	auto fg_map = Tilemap2DEntry::Create(this, fg_map_bytes, RomOffsets::Graphics::ISLAND_MAP_FG_MAP, RomOffsets::Graphics::ISLAND_MAP_FG_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto bg_tiles = TilesetEntry::Create(this, bg_tiles_bytes, RomOffsets::Graphics::ISLAND_MAP_BG_TILES, RomOffsets::Graphics::ISLAND_MAP_BG_TILES_FILE);
	auto bg_map = Tilemap2DEntry::Create(this, bg_map_bytes, RomOffsets::Graphics::ISLAND_MAP_BG_MAP, RomOffsets::Graphics::ISLAND_MAP_BG_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto dots_tiles = TilesetEntry::Create(this, dots_bytes, RomOffsets::Graphics::ISLAND_MAP_DOTS, RomOffsets::Graphics::ISLAND_MAP_DOTS_FILE);
	auto friday_tiles = TilesetEntry::Create(this, friday_bytes, RomOffsets::Graphics::ISLAND_MAP_FRIDAY, RomOffsets::Graphics::ISLAND_MAP_FRIDAY_FILE, true, 8, 8, 4, Tileset::BLOCK2X2);
	auto fg_pal = PaletteEntry::Create(this, fg_pal_bytes, RomOffsets::Graphics::ISLAND_MAP_FG_PAL, RomOffsets::Graphics::ISLAND_MAP_FG_PAL_FILE, Palette::Type::FULL);
	auto bg_pal = PaletteEntry::Create(this, bg_pal_bytes, RomOffsets::Graphics::ISLAND_MAP_BG_PAL, RomOffsets::Graphics::ISLAND_MAP_BG_PAL_FILE, Palette::Type::FULL);

	m_island_map_tiles.insert({ fg_tiles->GetName(), fg_tiles });
	m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FG_TILES, fg_tiles });
	m_island_map_tiles.insert({ bg_tiles->GetName(), bg_tiles });
	m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_BG_TILES, bg_tiles });
	m_island_map_tiles.insert({ dots_tiles->GetName(), dots_tiles });
	m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_DOTS, dots_tiles });
	m_island_map_tiles.insert({ friday_tiles->GetName(), friday_tiles });
	m_island_map_tiles_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FRIDAY, friday_tiles });

	m_island_map_tilemaps.insert({ fg_map->GetName(), fg_map });
	m_island_map_tilemaps_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FG_MAP, fg_map });
	m_island_map_tilemaps.insert({ bg_map->GetName(), bg_map });
	m_island_map_tilemaps_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_BG_MAP, bg_map });

	m_island_map_pals.insert({ fg_pal->GetName(), fg_pal });
	m_island_map_pals_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_FG_PAL, fg_pal });
	m_island_map_pals.insert({ bg_pal->GetName(), bg_pal });
	m_island_map_pals_internal.insert({ RomOffsets::Graphics::ISLAND_MAP_BG_PAL, bg_pal });

	return true;
}

bool GraphicsData::RomLoadTitleScreenData(const Rom& rom)
{
	uint32_t blue_pal_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_PALETTE_BLUE_LEA),
		rom.get_address(RomOffsets::Graphics::TITLE_PALETTE_BLUE_LEA));
	uint32_t yellow_pal_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_PALETTE_YELLOW_LEA),
		rom.get_address(RomOffsets::Graphics::TITLE_PALETTE_YELLOW_LEA));
	uint32_t title_1_tiles_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_1_TILES),
		rom.get_address(RomOffsets::Graphics::TITLE_1_TILES));
	uint32_t title_2_tiles_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_2_TILES),
		rom.get_address(RomOffsets::Graphics::TITLE_2_TILES));
	uint32_t title_3_tiles_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_3_TILES),
		rom.get_address(RomOffsets::Graphics::TITLE_3_TILES));
	uint32_t title_1_map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_1_MAP),
		rom.get_address(RomOffsets::Graphics::TITLE_1_MAP));
	uint32_t title_2_map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_2_MAP),
		rom.get_address(RomOffsets::Graphics::TITLE_2_MAP));
	uint32_t title_3_map_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_3_MAP),
		rom.get_address(RomOffsets::Graphics::TITLE_3_MAP));
	uint32_t title_3_pal_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_3_PAL),
		rom.get_address(RomOffsets::Graphics::TITLE_3_PAL));
	uint32_t title_3_pal_highlight_addr = Disasm::LEA_PCRel(rom.read<uint32_t>(RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT),
		rom.get_address(RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT));

	uint32_t blue_pal_size = rom.read<uint16_t>(blue_pal_addr) * 2 + 4;
	uint32_t yellow_pal_size = Palette::GetSizeBytes(Palette::Type::TITLE_YELLOW);
	uint32_t title_1_tiles_size = title_2_tiles_addr - title_1_tiles_addr;
	uint32_t title_2_tiles_size = title_3_tiles_addr - title_2_tiles_addr;
	uint32_t title_3_tiles_size = title_1_map_addr - title_3_tiles_addr;
	uint32_t title_1_map_size = title_2_map_addr - title_1_map_addr;
	uint32_t title_2_map_size = title_3_map_addr - title_2_map_addr;
	uint32_t title_3_map_size = title_3_pal_addr - title_3_map_addr;
	uint32_t title_3_pal_size = Palette::GetSizeBytes(Palette::Type::FULL);
	uint32_t title_3_pal_highlight_size = Palette::GetSizeBytes(Palette::Type::FULL);

	auto blue_pal_bytes = rom.read_array<uint8_t>(blue_pal_addr, blue_pal_size);
	auto yellow_pal_bytes = rom.read_array<uint8_t>(yellow_pal_addr, yellow_pal_size);
	auto title_1_tiles_bytes = rom.read_array<uint8_t>(title_1_tiles_addr, title_1_tiles_size);
	auto title_2_tiles_bytes = rom.read_array<uint8_t>(title_2_tiles_addr, title_2_tiles_size);
	auto title_3_tiles_bytes = rom.read_array<uint8_t>(title_3_tiles_addr, title_3_tiles_size);
	auto title_1_map_bytes = rom.read_array<uint8_t>(title_1_map_addr, title_1_map_size);
	auto title_2_map_bytes = rom.read_array<uint8_t>(title_2_map_addr, title_2_map_size);
	auto title_3_map_bytes = rom.read_array<uint8_t>(title_3_map_addr, title_3_map_size);
	auto title_3_pal_bytes = rom.read_array<uint8_t>(title_3_pal_addr, title_3_pal_size);
	auto title_3_pal_highlight_bytes = rom.read_array<uint8_t>(title_3_pal_highlight_addr, title_3_pal_highlight_size);

	auto blue_pal = PaletteEntry::Create(this, blue_pal_bytes, RomOffsets::Graphics::TITLE_PALETTE_BLUE,
		RomOffsets::Graphics::TITLE_PALETTE_BLUE_FILE, Palette::Type::TITLE_BLUE_FADE);
	auto yellow_pal = PaletteEntry::Create(this, yellow_pal_bytes, RomOffsets::Graphics::TITLE_PALETTE_YELLOW,
		RomOffsets::Graphics::TITLE_PALETTE_YELLOW_FILE, Palette::Type::TITLE_YELLOW);
	auto title_1_tiles = TilesetEntry::Create(this, title_1_tiles_bytes, RomOffsets::Graphics::TITLE_1_TILES,
		RomOffsets::Graphics::TITLE_1_TILES_FILE);
	auto title_2_tiles = TilesetEntry::Create(this, title_2_tiles_bytes, RomOffsets::Graphics::TITLE_2_TILES,
		RomOffsets::Graphics::TITLE_2_TILES_FILE);
	auto title_3_tiles = TilesetEntry::Create(this, title_3_tiles_bytes, RomOffsets::Graphics::TITLE_3_TILES,
		RomOffsets::Graphics::TITLE_3_TILES_FILE);
	auto title_1_map = Tilemap2DEntry::Create(this, title_1_map_bytes, RomOffsets::Graphics::TITLE_1_MAP,
		RomOffsets::Graphics::TITLE_1_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto title_2_map = Tilemap2DEntry::Create(this, title_2_map_bytes, RomOffsets::Graphics::TITLE_2_MAP,
		RomOffsets::Graphics::TITLE_2_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto title_3_map = Tilemap2DEntry::Create(this, title_3_map_bytes, RomOffsets::Graphics::TITLE_3_MAP,
		RomOffsets::Graphics::TITLE_3_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto title_3_pal = PaletteEntry::Create(this, title_3_pal_bytes, RomOffsets::Graphics::TITLE_3_PAL,
		RomOffsets::Graphics::TITLE_3_PAL_FILE, Palette::Type::FULL);
	auto title_3_pal_highlight = PaletteEntry::Create(this, title_3_pal_highlight_bytes, RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT,
		RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT_FILE, Palette::Type::FULL);

	m_title_tiles.insert({ title_1_tiles->GetName(), title_1_tiles });
	m_title_tiles_internal.insert({ RomOffsets::Graphics::TITLE_1_TILES, title_1_tiles });
	m_title_tiles.insert({ title_2_tiles->GetName(), title_2_tiles });
	m_title_tiles_internal.insert({ RomOffsets::Graphics::TITLE_2_TILES, title_2_tiles });
	m_title_tiles.insert({ title_3_tiles->GetName(), title_3_tiles });
	m_title_tiles_internal.insert({ RomOffsets::Graphics::TITLE_3_TILES, title_3_tiles });

	m_title_tilemaps.insert({ title_1_map->GetName(), title_1_map });
	m_title_tilemaps_internal.insert({ RomOffsets::Graphics::TITLE_1_MAP, title_1_map });
	m_title_tilemaps.insert({ title_2_map->GetName(), title_2_map });
	m_title_tilemaps_internal.insert({ RomOffsets::Graphics::TITLE_2_MAP, title_2_map });
	m_title_tilemaps.insert({ title_3_map->GetName(), title_3_map });
	m_title_tilemaps_internal.insert({ RomOffsets::Graphics::TITLE_3_MAP, title_3_map });

	m_title_pals.insert({ title_3_pal->GetName(), title_3_pal });
	m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_3_PAL, title_3_pal });
	m_title_pals.insert({ title_3_pal_highlight->GetName(), title_3_pal_highlight });
	m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT, title_3_pal_highlight });
	m_title_pals.insert({ yellow_pal->GetName(), yellow_pal });
	m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_PALETTE_YELLOW, yellow_pal });
	m_title_pals.insert({ blue_pal->GetName(), blue_pal });
	m_title_pals_internal.insert({ RomOffsets::Graphics::TITLE_PALETTE_BLUE, blue_pal });

	return true;
}

bool GraphicsData::AsmSaveGraphics(const filesystem::path& dir)
{
	std::unordered_map<std::string, ByteVector> combined;
	bool retval = std::all_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_misc_gfx_by_name.begin(), m_misc_gfx_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_sword_fx.begin(), m_sword_fx.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_status_fx_frames.begin(), m_status_fx_frames.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_misc_tilemaps.begin(), m_misc_tilemaps.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_island_map_pals.begin(), m_island_map_pals.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_island_map_tilemaps.begin(), m_island_map_tilemaps.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_island_map_tiles.begin(), m_island_map_tiles.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_title_pals.begin(), m_title_pals.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_title_tilemaps.begin(), m_title_tilemaps.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_title_tiles.begin(), m_title_tiles.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_palettes_by_name.begin(), m_palettes_by_name.end(), [&](auto& f)
		{
			if (f.first.find(':') == std::string::npos)
			{
				return f.second->Save(dir);
			}
			else
			{
				auto it = combined.find(f.second->GetFilename().str());
				if (it == combined.cend())
				{
					combined.insert({ f.second->GetFilename().str(), *f.second->GetBytes()});
				}
				else
				{
					auto bytes = f.second->GetBytes();
					it->second.insert(it->second.end(), bytes->begin(), bytes->end());
				}
				return true;
			}
		});
	for (const auto& f : combined)
	{
		WriteBytes(f.second, dir / f.first);
	}
	m_end_credits_map->Save(dir);
	m_end_credits_palette->Save(dir);
	m_end_credits_tileset->Save(dir);
	return retval;
}

bool GraphicsData::AsmSaveStrings(const filesystem::path& dir)
{
	try
	{
		AsmFile sfile, ifile;
		ByteVector str[4];
		ifile.WriteFileHeader(m_region_check_filename, "Region Check");
		ifile << AsmFile::Label(RomOffsets::Strings::REGION_CHECK_ROUTINE) << AsmFile::IncludeFile(m_region_check_routine_filename, AsmFile::FileType::ASSEMBLER);
		ifile << AsmFile::Label(RomOffsets::Strings::REGION_CHECK_STRINGS) << AsmFile::IncludeFile(m_region_check_strings_filename, AsmFile::FileType::ASSEMBLER);
		ifile << AsmFile::Align(2) << AsmFile::Label(RomOffsets::Graphics::SYS_FONT) << AsmFile::IncludeFile(m_system_font_filename, AsmFile::FileType::BINARY);
		ifile.WriteFile(dir / m_region_check_filename);
		sfile.WriteFileHeader(m_region_check_strings_filename, "Region Check System Strings");
		for (int i = 0; i < 4; ++i)
		{
			str[i].insert(str[i].end(), m_system_strings[i].cbegin(), m_system_strings[i].cend());
			str[i].push_back(0);
		}
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_LINE1) << str[0];
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_NTSC) << str[1];
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_PAL) << str[2];
		sfile << AsmFile::Label(RomOffsets::Strings::REGION_ERROR_LINE3) << str[3];
		sfile.WriteFile(dir / m_region_check_strings_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveInventoryGraphics(const filesystem::path& dir)
{

	try
	{
		AsmFile ifile;
		auto write_inc = [&](const std::string& name, const auto& container)
		{
			auto e = container.find(name)->second;
			ifile << AsmFile::Label(name) << AsmFile::IncludeFile(e->GetFilename(), AsmFile::FileType::BINARY);
		};
		ifile.WriteFileHeader(m_inventory_graphics_filename, "Inventory Graphics Data");
		write_inc(RomOffsets::Graphics::INV_FONT, m_fonts_internal);
		write_inc(RomOffsets::Graphics::INV_CURSOR, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_ARROW, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_UNUSED1, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_UNUSED2, m_misc_gfx_internal);
		write_inc(RomOffsets::Graphics::INV_PAL1, m_palettes_internal);
		write_inc(RomOffsets::Graphics::INV_PAL2, m_palettes_internal);
		ifile.WriteFile(dir / m_inventory_graphics_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveCompressedStringData(const filesystem::path& dir)
{
	try
	{
		AsmFile sfile, pfile;
		sfile.WriteFileHeader(m_strings_filename, "Compressed Strings");
		pfile.WriteFileHeader(m_string_ptr_filename, "Compressed String Bank Pointers");
		auto font = m_fonts_internal[RomOffsets::Graphics::MAIN_FONT];
		sfile << AsmFile::Label(font->GetName()) << AsmFile::IncludeFile(font->GetFilename(),AsmFile::FileType::BINARY);
		pfile << AsmFile::Label(RomOffsets::Strings::STRING_BANK_PTR);
		int f = 0;
		for (int i = 0; i < m_strings.size(); i += 256)
		{
			ByteVector bytes;
			for (int j = 0; j < 256 && (i + j) < m_strings.size(); ++j)
			{
				const auto& s = m_strings[i + j];
				bytes.insert(bytes.end(), s.cbegin(), s.cend());
			}
			filesystem::path fname = StrPrintf(RomOffsets::Strings::STRING_BANK_FILE, f + 1);
			std::string pname = StrPrintf(RomOffsets::Strings::STRING_BANK, f);
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

bool GraphicsData::AsmSaveSwordFx(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_sword_fx_path, "Magic Sword Effects");
		auto invmap = m_misc_tilemaps_internal[RomOffsets::Graphics::INV_TILEMAP];
		file << AsmFile::Label(invmap->GetName()) << AsmFile::IncludeFile(invmap->GetFilename(), AsmFile::BINARY);
		for (const auto& t : m_sword_fx)
		{
			file << AsmFile::Label(t.first) << AsmFile::IncludeFile(t.second->GetFilename(), AsmFile::BINARY);
		}
		file.WriteFile(dir / m_sword_fx_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveStatusFx(const filesystem::path& dir)
{
	try
	{
		AsmFile pfile;
		pfile.WriteFileHeader(m_status_fx_pointers_path, "Status Effects Pointers File");
		for (const auto& status : m_status_fx)
		{
			pfile << AsmFile::Label(status.first);
			for (const auto& frame : status.second)
			{
				pfile << frame;
			}
		}
		pfile.WriteFile(dir / m_status_fx_pointers_path);
		AsmFile file;
		file.WriteFileHeader(m_status_fx_path, "Status Effects Data File");
		for (const auto& frame : m_status_fx_frames)
		{
			file << AsmFile::Label(frame.first) << AsmFile::IncludeFile(frame.second->GetFilename(), AsmFile::BINARY);
		}
		file.WriteFile(dir / m_status_fx_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveHuffmanData(const filesystem::path& dir)
{
	WriteBytes(m_huffman_offsets, dir / m_huffman_offset_path);
	WriteBytes(m_huffman_tables, dir / m_huffman_table_path);
	return true;
}

bool GraphicsData::AsmSaveEndCreditData(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_end_credits_path, "End Credits Data File");
		file << AsmFile::Label(m_end_credits_palette->GetName())
			<< AsmFile::IncludeFile(m_end_credits_palette->GetFilename(), AsmFile::BINARY);
		const auto& font = m_fonts_internal[RomOffsets::Graphics::END_CREDITS_FONT];
		file << AsmFile::Label(font->GetName()) << AsmFile::IncludeFile(font->GetFilename(), AsmFile::BINARY);
		file << AsmFile::Label(m_end_credits_tileset->GetName())
			<< AsmFile::IncludeFile(m_end_credits_tileset->GetFilename(), AsmFile::BINARY);
		file << AsmFile::Label(m_end_credits_map->GetName())
			<< AsmFile::IncludeFile(m_end_credits_map->GetFilename(), AsmFile::BINARY);
		file.WriteFile(dir / m_end_credits_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveIslandMapData(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_island_map_path, "Island Map Data File");
		file << AsmFile::Label(RomOffsets::Graphics::ISLAND_MAP_ROUTINES) << AsmFile::IncludeFile(m_island_map_routine_path, AsmFile::ASSEMBLER);
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::BINARY);
		};
		write_include(m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_FG_TILES]);
		write_include(m_island_map_tilemaps_internal[RomOffsets::Graphics::ISLAND_MAP_FG_MAP]);
		write_include(m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_BG_TILES]);
		write_include(m_island_map_tilemaps_internal[RomOffsets::Graphics::ISLAND_MAP_BG_MAP]);
		write_include(m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_DOTS]);
		write_include(m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_FRIDAY]);
		file << AsmFile::Align(2);
		write_include(m_island_map_pals_internal[RomOffsets::Graphics::ISLAND_MAP_FG_PAL]);
		write_include(m_island_map_pals_internal[RomOffsets::Graphics::ISLAND_MAP_BG_PAL]);

		file.WriteFile(dir / m_island_map_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveTitleScreenData(const filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_title_path, "Title Screen Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::BINARY);
		};
		file << AsmFile::Label(RomOffsets::Graphics::TITLE_ROUTINES_1) << AsmFile::IncludeFile(m_title_routines_1_path, AsmFile::ASSEMBLER);
		write_include(m_title_pals_internal[RomOffsets::Graphics::TITLE_PALETTE_BLUE]);
		file << AsmFile::Label(RomOffsets::Graphics::TITLE_ROUTINES_2) << AsmFile::IncludeFile(m_title_routines_2_path, AsmFile::ASSEMBLER);
		write_include(m_title_pals_internal[RomOffsets::Graphics::TITLE_PALETTE_YELLOW]);
		file << AsmFile::Label(RomOffsets::Graphics::TITLE_ROUTINES_3) << AsmFile::IncludeFile(m_title_routines_3_path, AsmFile::ASSEMBLER);
		write_include(m_title_tiles_internal[RomOffsets::Graphics::TITLE_1_TILES]);
		write_include(m_title_tiles_internal[RomOffsets::Graphics::TITLE_2_TILES]);
		write_include(m_title_tiles_internal[RomOffsets::Graphics::TITLE_3_TILES]);
		write_include(m_title_tilemaps_internal[RomOffsets::Graphics::TITLE_1_MAP]);
		write_include(m_title_tilemaps_internal[RomOffsets::Graphics::TITLE_2_MAP]);
		write_include(m_title_tilemaps_internal[RomOffsets::Graphics::TITLE_3_MAP]);
		file << AsmFile::Align(2);
		write_include(m_title_pals_internal[RomOffsets::Graphics::TITLE_3_PAL]);
		write_include(m_title_pals_internal[RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT]);

		file.WriteFile(dir / m_title_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::RomPrepareInjectFonts(const Rom& rom)
{
	auto system_font_bytes = m_fonts_by_name[RomOffsets::Graphics::SYS_FONT]->GetBytes();
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	auto write_string = [](ByteVectorPtr& bytes, const std::string& in)
	{
		for (char c : in)
		{
			bytes->push_back(c);
		}
		bytes->push_back(0);
	};
	uint32_t addrs[5];
	uint32_t begin = rom.get_section(RomOffsets::Strings::REGION_CHECK_DATA_SECTION).begin;
	for (int i = 0; i < 4; ++i)
	{
		addrs[i] = begin + bytes->size();
		write_string(bytes, m_system_strings[i]);
	}
	if ((bytes->size() & 1) == 1)
	{
		bytes->push_back(0xFF);
	}
	addrs[4] = begin + bytes->size();
	bytes->insert(bytes->end(), system_font_bytes->cbegin(), system_font_bytes->cend());
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_CHECK_DATA_SECTION, bytes });
	uint32_t line1_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE1), addrs[0]);
	uint32_t line2_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_NTSC), addrs[1]);
	uint32_t line3_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_PAL), addrs[2]);
	uint32_t line4_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Strings::REGION_ERROR_LINE3), addrs[3]);
	uint32_t font_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::SYS_FONT), addrs[4]);

	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_LINE1, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line1_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_NTSC, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line2_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_PAL, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line3_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::REGION_ERROR_LINE3, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(line4_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::SYS_FONT, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(font_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectInvGraphics(const Rom& rom)
{
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	uint32_t base = rom.get_section(RomOffsets::Graphics::INV_SECTION).begin;
	uint32_t addrs[7];
	addrs[0] = base + bytes->size();
	auto inv_font = m_fonts_internal[RomOffsets::Graphics::INV_FONT]->GetBytes();
	bytes->insert(bytes->end(), inv_font->cbegin(), inv_font->cend());
	addrs[1] = base + bytes->size();
	auto inv_cursor = m_misc_gfx_internal[RomOffsets::Graphics::INV_CURSOR]->GetBytes();
	bytes->insert(bytes->end(), inv_cursor->cbegin(), inv_cursor->cend());
	addrs[2] = base + bytes->size();
	auto inv_arrow = m_misc_gfx_internal[RomOffsets::Graphics::INV_ARROW]->GetBytes();
	bytes->insert(bytes->end(), inv_arrow->cbegin(), inv_arrow->cend());
	addrs[3] = base + bytes->size();
	auto inv_unused1 = m_misc_gfx_internal[RomOffsets::Graphics::INV_UNUSED1]->GetBytes();
	bytes->insert(bytes->end(), inv_unused1->cbegin(), inv_unused1->cend());
	addrs[4] = base + bytes->size();
	auto inv_unused2 = m_misc_gfx_internal[RomOffsets::Graphics::INV_UNUSED2]->GetBytes();
	bytes->insert(bytes->end(), inv_unused2->cbegin(), inv_unused2->cend());
	addrs[5] = base + bytes->size();
	auto inv_pal1 = m_palettes_internal[RomOffsets::Graphics::INV_PAL1]->GetBytes();
	bytes->insert(bytes->end(), inv_pal1->cbegin(), inv_pal1->cend());
	addrs[6] = base + bytes->size();
	auto inv_pal2 = m_palettes_internal[RomOffsets::Graphics::INV_PAL2]->GetBytes();
	bytes->insert(bytes->end(), inv_pal2->cbegin(), inv_pal2->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::INV_SECTION, bytes });

	uint32_t font_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_FONT), addrs[0]);
	uint32_t cursor_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_CURSOR), addrs[1]);
	uint32_t arrow_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_ARROW), addrs[2]);
	uint32_t unused1_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED1), addrs[3]);
	uint32_t unused1p6_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED1_PLUS6), addrs[3] + 12);
	uint32_t unused2_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED2), addrs[4]);
	uint32_t unused2p4_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::INV_UNUSED2_PLUS4), addrs[4] + 8);
	uint32_t pal1_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_PAL1), addrs[5]);
	uint32_t pal2_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_PAL2), addrs[6]);

	m_pending_writes.push_back({ RomOffsets::Graphics::INV_FONT, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(font_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_CURSOR, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(cursor_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_ARROW, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(arrow_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED1, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused1_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED1_PLUS6, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused1p6_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED2, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused2_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_UNUSED2_PLUS4, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(unused2p4_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_PAL1, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(pal1_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_PAL2, std::make_shared<std::vector<uint8_t>>(Split<uint8_t>(pal2_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectCompressedStringData(const Rom& rom)
{
	std::vector<uint32_t> bank_ptrs;
	uint32_t bank_ptrs_ptr;
	ByteVectorPtr bytes = std::make_shared<ByteVector>(*m_fonts_internal[RomOffsets::Graphics::MAIN_FONT]->GetBytes());
	uint32_t begin = rom.get_section(RomOffsets::Strings::STRING_SECTION).begin;
	for (int i = 0; i < m_strings.size(); i += 256)
	{
		bank_ptrs.push_back(begin + bytes->size());
		for (int j = 0; j < 256 && (i + j) < m_strings.size(); ++j)
		{
			const auto& s = m_strings[i + j];
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
	m_pending_writes.push_back({ RomOffsets::Strings::STRING_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Strings::STRING_BANK_PTR_PTR, std::make_shared<ByteVector>(Split<uint8_t>(bank_ptrs_ptr)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::MAIN_FONT_PTR, std::make_shared<ByteVector>(Split<uint8_t>(begin)) });
	return true;
}

bool GraphicsData::RomPrepareInjectPalettes(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Graphics::MISC_PAL_SECTION).begin;
	uint32_t player_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::PLAYER_PAL), begin);
	auto bytes = std::make_shared<ByteVector>();
	auto player_pal = m_palettes_internal[RomOffsets::Graphics::PLAYER_PAL]->GetBytes();
	bytes->insert(bytes->end(), player_pal->cbegin(), player_pal->cend());
	uint32_t hud_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::HUD_PAL), begin + bytes->size());
	uint32_t hud_lea2 = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::HUD_PAL_LEA2), begin + bytes->size());
	auto hud_pal = m_palettes_internal[RomOffsets::Graphics::HUD_PAL]->GetBytes();
	bytes->insert(bytes->end(), hud_pal->cbegin(), hud_pal->cend());
	
	uint32_t item_pal_begin = rom.get_section(RomOffsets::Graphics::INV_ITEM_PAL_SECTION).begin;
	uint32_t item_pal_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_ITEM_PAL), item_pal_begin);
	auto item_pal = m_palettes_internal[RomOffsets::Graphics::INV_ITEM_PAL]->GetBytes();

	uint32_t equip_pal_begin = rom.get_section(RomOffsets::Graphics::EQUIP_PAL_SECTION).begin;
	uint32_t sword_pal_lea = Asm::LEA_PCRel(AReg::A2, rom.get_address(RomOffsets::Graphics::SWORD_PAL_SWAPS), equip_pal_begin);
	auto sword_pal_bytes = std::make_shared<ByteVector>();
	auto armour_pal_bytes = std::make_shared<ByteVector>();
	for (const auto& p : m_palettes_internal)
	{
		if (p.first.find(RomOffsets::Graphics::SWORD_PAL_SWAPS) != std::string::npos)
		{
			auto bytes = p.second->GetBytes();
			sword_pal_bytes->insert(sword_pal_bytes->end(), bytes->cbegin(), bytes->cend());
		}
		else if (p.first.find(RomOffsets::Graphics::ARMOUR_PAL_SWAPS) != std::string::npos)
		{
			auto bytes = p.second->GetBytes();
			armour_pal_bytes->insert(armour_pal_bytes->end(), bytes->cbegin(), bytes->cend());
		}
	}
	uint32_t armour_pal_begin = equip_pal_begin + sword_pal_bytes->size();
	uint32_t armour_pal_lea = Asm::LEA_PCRel(AReg::A2, rom.get_address(RomOffsets::Graphics::ARMOUR_PAL_SWAPS), armour_pal_begin);
	sword_pal_bytes->insert(sword_pal_bytes->end(), armour_pal_bytes->cbegin(), armour_pal_bytes->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::MISC_PAL_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::PLAYER_PAL, std::make_shared<ByteVector>(Split<uint8_t>(player_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_PAL, std::make_shared<ByteVector>(Split<uint8_t>(hud_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_PAL_LEA2, std::make_shared<ByteVector>(Split<uint8_t>(hud_lea2)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_ITEM_PAL_SECTION, item_pal });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_ITEM_PAL, std::make_shared<ByteVector>(Split<uint8_t>(item_pal_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::EQUIP_PAL_SECTION, sword_pal_bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::SWORD_PAL_SWAPS, std::make_shared<ByteVector>(Split<uint8_t>(sword_pal_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ARMOUR_PAL_SWAPS, std::make_shared<ByteVector>(Split<uint8_t>(armour_pal_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectTextGraphics(const Rom& rom)
{
	uint32_t down_arrow_begin = rom.get_section(RomOffsets::Graphics::DOWN_ARROW_SECTION).begin;
	uint32_t down_arrow_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::DOWN_ARROW), down_arrow_begin);
	auto down_arrow_bytes = m_misc_gfx_internal[RomOffsets::Graphics::DOWN_ARROW]->GetBytes();
	uint32_t right_arrow_begin = rom.get_section(RomOffsets::Graphics::RIGHT_ARROW_SECTION).begin;
	uint32_t right_arrow_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::RIGHT_ARROW), right_arrow_begin);
	auto right_arrow_bytes = m_misc_gfx_internal[RomOffsets::Graphics::RIGHT_ARROW]->GetBytes();

	m_pending_writes.push_back({ RomOffsets::Graphics::DOWN_ARROW_SECTION, down_arrow_bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::RIGHT_ARROW_SECTION, right_arrow_bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::DOWN_ARROW, std::make_shared<ByteVector>(Split<uint8_t>(down_arrow_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::RIGHT_ARROW, std::make_shared<ByteVector>(Split<uint8_t>(right_arrow_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectSwordFx(const Rom& rom)
{
	uint32_t sword_fx_begin = rom.get_section(RomOffsets::Graphics::SWORD_FX_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t inv_tilemap_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::INV_TILEMAP), sword_fx_begin);
	auto inv_tilemap_bytes = m_misc_tilemaps_internal[RomOffsets::Graphics::INV_TILEMAP]->GetBytes();
	bytes->insert(bytes->end(), inv_tilemap_bytes->cbegin(), inv_tilemap_bytes->cend());

	uint32_t magic_sword_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::SWORD_MAGIC), sword_fx_begin + bytes->size());
	auto magic_sword_bytes = m_sword_fx_internal[RomOffsets::Graphics::SWORD_MAGIC]->GetBytes();
	bytes->insert(bytes->end(), magic_sword_bytes->cbegin(), magic_sword_bytes->cend());

	uint32_t thunder_sword_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::SWORD_THUNDER), sword_fx_begin + bytes->size());
	auto thunder_sword_bytes = m_sword_fx_internal[RomOffsets::Graphics::SWORD_THUNDER]->GetBytes();
	bytes->insert(bytes->end(), thunder_sword_bytes->cbegin(), thunder_sword_bytes->cend());

	uint32_t gaia_sword_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::SWORD_GAIA), sword_fx_begin + bytes->size());
	auto gaia_sword_bytes = m_sword_fx_internal[RomOffsets::Graphics::SWORD_GAIA]->GetBytes();
	bytes->insert(bytes->end(), gaia_sword_bytes->cbegin(), gaia_sword_bytes->cend());

	uint32_t ice_sword_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::SWORD_ICE), sword_fx_begin + bytes->size());
	auto ice_sword_bytes = m_sword_fx_internal[RomOffsets::Graphics::SWORD_ICE]->GetBytes();
	bytes->insert(bytes->end(), ice_sword_bytes->cbegin(), ice_sword_bytes->cend());

	uint32_t coinfall_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::COINFALL), sword_fx_begin + bytes->size());
	auto coinfall_bytes = m_sword_fx_internal[RomOffsets::Graphics::COINFALL]->GetBytes();
	bytes->insert(bytes->end(), coinfall_bytes->cbegin(), coinfall_bytes->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::SWORD_FX_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::INV_TILEMAP, std::make_shared<ByteVector>(Split<uint8_t>(inv_tilemap_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::SWORD_MAGIC, std::make_shared<ByteVector>(Split<uint8_t>(magic_sword_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::SWORD_THUNDER, std::make_shared<ByteVector>(Split<uint8_t>(thunder_sword_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::SWORD_GAIA, std::make_shared<ByteVector>(Split<uint8_t>(gaia_sword_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::SWORD_ICE, std::make_shared<ByteVector>(Split<uint8_t>(ice_sword_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::COINFALL, std::make_shared<ByteVector>(Split<uint8_t>(coinfall_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectStatusFx(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Graphics::STATUS_FX_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();
	uint32_t pointer_table_size = 0;
	std::unordered_map<std::string, uint32_t> pointers;

	uint32_t poison_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::STATUS_FX_POISON), begin);
	pointer_table_size += m_status_fx[RomOffsets::Graphics::STATUS_FX_POISON].size() * sizeof(uint32_t);
	uint32_t confusion_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::STATUS_FX_CONFUSION), begin + pointer_table_size);
	pointer_table_size += m_status_fx[RomOffsets::Graphics::STATUS_FX_CONFUSION].size() * sizeof(uint32_t);
	uint32_t paralysis_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::STATUS_FX_PARALYSIS), begin + pointer_table_size);
	pointer_table_size += m_status_fx[RomOffsets::Graphics::STATUS_FX_PARALYSIS].size() * sizeof(uint32_t);
	uint32_t curse_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::STATUS_FX_CURSE), begin + pointer_table_size);
	pointer_table_size += m_status_fx[RomOffsets::Graphics::STATUS_FX_CURSE].size() * sizeof(uint32_t);

	uint32_t addr = pointer_table_size;
	bytes->resize(pointer_table_size);
	for (const auto& f : m_status_fx_frames)
	{
		auto fb = f.second->GetBytes();
		pointers[f.first] = bytes->size() + begin;
		bytes->insert(bytes->end(), fb->cbegin(), fb->cend());
	}
	auto it = bytes->begin();
	for (const auto& s : { RomOffsets::Graphics::STATUS_FX_POISON,    RomOffsets::Graphics::STATUS_FX_CONFUSION,
		                   RomOffsets::Graphics::STATUS_FX_PARALYSIS, RomOffsets::Graphics::STATUS_FX_CURSE })
	{
		const auto& frames = m_status_fx[s];
		for (const auto& p : frames)
		{
			uint32_t addr = pointers[p];
			*it++ = (addr >> 24) & 0xFF;
			*it++ = (addr >> 16) & 0xFF;
			*it++ = (addr >> 8) & 0xFF;
			*it++ = addr & 0xFF;
		}
	}

	m_pending_writes.push_back({ RomOffsets::Graphics::STATUS_FX_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::STATUS_FX_POISON, std::make_shared<ByteVector>(Split<uint8_t>(poison_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::STATUS_FX_CONFUSION, std::make_shared<ByteVector>(Split<uint8_t>(confusion_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::STATUS_FX_PARALYSIS, std::make_shared<ByteVector>(Split<uint8_t>(paralysis_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::STATUS_FX_CURSE, std::make_shared<ByteVector>(Split<uint8_t>(curse_lea)) });
	return true;
}

bool GraphicsData::RomPrepareInjectHuffmanData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Strings::HUFFMAN_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t textbox_3l_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TEXTBOX_3LINE_MAP), begin);
	auto textbox_3l_bytes = m_misc_tilemaps_internal[RomOffsets::Graphics::TEXTBOX_3LINE_MAP]->GetBytes();
	bytes->insert(bytes->end(), textbox_3l_bytes->cbegin(), textbox_3l_bytes->cend());

	uint32_t textbox_2l_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TEXTBOX_2LINE_MAP), begin + bytes->size());
	auto textbox_2l_bytes = m_misc_tilemaps_internal[RomOffsets::Graphics::TEXTBOX_2LINE_MAP]->GetBytes();
	bytes->insert(bytes->end(), textbox_2l_bytes->cbegin(), textbox_2l_bytes->cend());

	uint32_t huffman_offsets_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Strings::HUFFMAN_OFFSETS), begin + bytes->size());
	bytes->insert(bytes->end(), m_huffman_offsets.cbegin(), m_huffman_offsets.cend());

	uint32_t huffman_tables_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Strings::HUFFMAN_TABLES), begin + bytes->size());
	bytes->insert(bytes->end(), m_huffman_tables.cbegin(), m_huffman_tables.cend());

	m_pending_writes.push_back({ RomOffsets::Strings::HUFFMAN_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::TEXTBOX_3LINE_MAP, std::make_shared<ByteVector>(Split<uint8_t>(textbox_3l_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TEXTBOX_2LINE_MAP, std::make_shared<ByteVector>(Split<uint8_t>(textbox_2l_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::HUFFMAN_OFFSETS, std::make_shared<ByteVector>(Split<uint8_t>(huffman_offsets_lea)) });
	m_pending_writes.push_back({ RomOffsets::Strings::HUFFMAN_TABLES, std::make_shared<ByteVector>(Split<uint8_t>(huffman_tables_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectHudData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Graphics::HUD_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::HUD_TILEMAP), begin);
	auto map_bytes = m_misc_tilemaps_internal[RomOffsets::Graphics::HUD_TILEMAP]->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	uint32_t ts_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::HUD_TILESET), begin + bytes->size());
	auto ts_bytes = m_misc_gfx_internal[RomOffsets::Graphics::HUD_TILESET]->GetBytes();
	bytes->insert(bytes->end(), ts_bytes->cbegin(), ts_bytes->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_SECTION, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_TILEMAP, std::make_shared<ByteVector>(Split<uint8_t>(map_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::HUD_TILESET, std::make_shared<ByteVector>(Split<uint8_t>(ts_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectEndCreditData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Graphics::END_CREDITS_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t pal_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::END_CREDITS_PAL), begin);
	auto pal_bytes = m_end_credits_palette->GetBytes();
	bytes->insert(bytes->end(), pal_bytes->cbegin(), pal_bytes->cend());

	uint32_t font_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::END_CREDITS_FONT), begin + bytes->size());
	auto font_bytes = m_fonts_internal[RomOffsets::Graphics::END_CREDITS_FONT]->GetBytes();
	bytes->insert(bytes->end(), font_bytes->cbegin(), font_bytes->cend());

	uint32_t logos_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::END_CREDITS_LOGOS), begin + bytes->size());
	auto logos_bytes = m_end_credits_tileset->GetBytes();
	bytes->insert(bytes->end(), logos_bytes->cbegin(), logos_bytes->cend());

	uint32_t map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::END_CREDITS_MAP), begin + bytes->size());
	auto map_bytes = m_end_credits_map->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::END_CREDITS_DATA, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::END_CREDITS_PAL, std::make_shared<ByteVector>(Split<uint8_t>(pal_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::END_CREDITS_FONT, std::make_shared<ByteVector>(Split<uint8_t>(font_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::END_CREDITS_LOGOS, std::make_shared<ByteVector>(Split<uint8_t>(logos_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::END_CREDITS_MAP, std::make_shared<ByteVector>(Split<uint8_t>(map_lea)) });

	return true;
}

bool GraphicsData::RomPrepareInjectIslandMapData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomOffsets::Graphics::ISLAND_MAP_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t fg_tiles_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FG_TILES), begin);
	auto fg_tiles_bytes = m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_FG_TILES]->GetBytes();
	bytes->insert(bytes->end(), fg_tiles_bytes->cbegin(), fg_tiles_bytes->cend());

	uint32_t fg_map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FG_MAP), begin + bytes->size());
	auto fg_map_bytes = m_island_map_tilemaps[RomOffsets::Graphics::ISLAND_MAP_FG_MAP]->GetBytes();
	bytes->insert(bytes->end(), fg_map_bytes->cbegin(), fg_map_bytes->cend());

	uint32_t bg_tiles_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_BG_TILES), begin + bytes->size());
	auto bg_tiles_bytes = m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_BG_TILES]->GetBytes();
	bytes->insert(bytes->end(), bg_tiles_bytes->cbegin(), bg_tiles_bytes->cend());

	uint32_t bg_map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_BG_MAP), begin + bytes->size());
	auto bg_map_bytes = m_island_map_tilemaps[RomOffsets::Graphics::ISLAND_MAP_BG_MAP]->GetBytes();
	bytes->insert(bytes->end(), bg_map_bytes->cbegin(), bg_map_bytes->cend());

	uint32_t dots_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_DOTS), begin + bytes->size());
	auto dots_bytes = m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_DOTS]->GetBytes();
	bytes->insert(bytes->end(), dots_bytes->cbegin(), dots_bytes->cend());

	uint32_t friday_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FRIDAY), begin + bytes->size());
	auto friday_bytes = m_island_map_tiles_internal[RomOffsets::Graphics::ISLAND_MAP_FRIDAY]->GetBytes();
	bytes->insert(bytes->end(), friday_bytes->cbegin(), friday_bytes->cend());
	
	if (((bytes->size() + begin) & 1) == 1)
	{
		bytes->push_back(0xFF);
	}

	uint32_t pal_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::ISLAND_MAP_FG_PAL), begin + bytes->size());
	auto fg_pal_bytes = m_island_map_pals_internal[RomOffsets::Graphics::ISLAND_MAP_FG_PAL]->GetBytes();
	auto bg_pal_bytes = m_island_map_pals_internal[RomOffsets::Graphics::ISLAND_MAP_BG_PAL]->GetBytes();
	bytes->insert(bytes->end(), fg_pal_bytes->cbegin(), fg_pal_bytes->cend());
	bytes->insert(bytes->end(), bg_pal_bytes->cbegin(), bg_pal_bytes->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_DATA, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_FG_TILES, std::make_shared<ByteVector>(Split<uint8_t>(fg_tiles_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_FG_MAP, std::make_shared<ByteVector>(Split<uint8_t>(fg_map_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_BG_TILES, std::make_shared<ByteVector>(Split<uint8_t>(bg_tiles_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_BG_MAP, std::make_shared<ByteVector>(Split<uint8_t>(bg_map_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_DOTS, std::make_shared<ByteVector>(Split<uint8_t>(dots_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_FRIDAY, std::make_shared<ByteVector>(Split<uint8_t>(friday_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::ISLAND_MAP_FG_PAL, std::make_shared<ByteVector>(Split<uint8_t>(pal_lea)) });
	
	return true;
}

bool GraphicsData::RomPrepareInjectTitleScreenData(const Rom& rom)
{
	uint32_t blue_pal_begin = rom.get_section(RomOffsets::Graphics::TITLE_PALETTE_BLUE).begin;
	uint32_t blue_pal_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::TITLE_PALETTE_BLUE_LEA), blue_pal_begin);
	auto blue_pal = m_title_pals_internal[RomOffsets::Graphics::TITLE_PALETTE_BLUE]->GetBytes();

	uint32_t yellow_pal_begin = rom.get_section(RomOffsets::Graphics::TITLE_PALETTE_YELLOW).begin;
	uint32_t yellow_pal_lea = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::TITLE_PALETTE_YELLOW_LEA), yellow_pal_begin);
	auto yellow_pal = m_title_pals_internal[RomOffsets::Graphics::TITLE_PALETTE_YELLOW]->GetBytes();

	uint32_t begin = rom.get_section(RomOffsets::Graphics::TITLE_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t title_1_tiles_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_1_TILES), begin);
	auto title_1_tiles_bytes = m_title_tiles_internal[RomOffsets::Graphics::TITLE_1_TILES]->GetBytes();
	bytes->insert(bytes->end(), title_1_tiles_bytes->cbegin(), title_1_tiles_bytes->cend());

	uint32_t title_2_tiles_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_2_TILES), begin + bytes->size());
	auto title_2_tiles_bytes = m_title_tiles_internal[RomOffsets::Graphics::TITLE_2_TILES]->GetBytes();
	bytes->insert(bytes->end(), title_2_tiles_bytes->cbegin(), title_2_tiles_bytes->cend());

	uint32_t title_3_tiles_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_3_TILES), begin + bytes->size());
	auto title_3_tiles_bytes = m_title_tiles_internal[RomOffsets::Graphics::TITLE_3_TILES]->GetBytes();
	bytes->insert(bytes->end(), title_3_tiles_bytes->cbegin(), title_3_tiles_bytes->cend());

	uint32_t title_1_map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_1_MAP), begin + bytes->size());
	auto title_1_map_bytes = m_title_tilemaps_internal[RomOffsets::Graphics::TITLE_1_MAP]->GetBytes();
	bytes->insert(bytes->end(), title_1_map_bytes->cbegin(), title_1_map_bytes->cend());

	uint32_t title_2_map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_2_MAP), begin + bytes->size());
	auto title_2_map_bytes = m_title_tilemaps_internal[RomOffsets::Graphics::TITLE_2_MAP]->GetBytes();
	bytes->insert(bytes->end(), title_2_map_bytes->cbegin(), title_2_map_bytes->cend());

	uint32_t title_3_map_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_3_MAP), begin + bytes->size());
	auto title_3_map_bytes = m_title_tilemaps_internal[RomOffsets::Graphics::TITLE_3_MAP]->GetBytes();
	bytes->insert(bytes->end(), title_3_map_bytes->cbegin(), title_3_map_bytes->cend());

	if (((bytes->size() + begin) & 1) == 1)
	{
		bytes->push_back(0xFF);
	}

	uint32_t title_3_pal_lea1 = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_3_PAL), begin + bytes->size());
	uint32_t title_3_pal_lea2 = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_3_PAL_LEA2), begin + bytes->size());
	uint32_t title_3_pal_lea3 = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::TITLE_3_PAL_LEA3), begin + bytes->size());
	uint32_t title_3_pal_lea4 = Asm::LEA_PCRel(AReg::A1, rom.get_address(RomOffsets::Graphics::TITLE_3_PAL_LEA4), begin + bytes->size());
	auto title_3_pal_bytes = m_title_pals_internal[RomOffsets::Graphics::TITLE_3_PAL]->GetBytes();
	bytes->insert(bytes->end(), title_3_pal_bytes->cbegin(), title_3_pal_bytes->cend());

	uint32_t title_3_pal_highlight_lea = Asm::LEA_PCRel(AReg::A0, rom.get_address(RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT), begin + bytes->size());
	auto title_3_pal_highlight_bytes = m_title_pals_internal[RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT]->GetBytes();
	bytes->insert(bytes->end(), title_3_pal_highlight_bytes->cbegin(), title_3_pal_highlight_bytes->cend());

	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_PALETTE_BLUE, blue_pal });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_PALETTE_YELLOW, yellow_pal });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_DATA, bytes });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_PALETTE_BLUE_LEA, std::make_shared<ByteVector>(Split<uint8_t>(blue_pal_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_PALETTE_YELLOW_LEA, std::make_shared<ByteVector>(Split<uint8_t>(yellow_pal_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_1_TILES, std::make_shared<ByteVector>(Split<uint8_t>(title_1_tiles_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_2_TILES, std::make_shared<ByteVector>(Split<uint8_t>(title_2_tiles_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_TILES, std::make_shared<ByteVector>(Split<uint8_t>(title_3_tiles_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_1_MAP, std::make_shared<ByteVector>(Split<uint8_t>(title_1_map_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_2_MAP, std::make_shared<ByteVector>(Split<uint8_t>(title_2_map_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_MAP, std::make_shared<ByteVector>(Split<uint8_t>(title_3_map_lea)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_PAL, std::make_shared<ByteVector>(Split<uint8_t>(title_3_pal_lea1)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_PAL_LEA2, std::make_shared<ByteVector>(Split<uint8_t>(title_3_pal_lea2)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_PAL_LEA3, std::make_shared<ByteVector>(Split<uint8_t>(title_3_pal_lea3)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_PAL_LEA4, std::make_shared<ByteVector>(Split<uint8_t>(title_3_pal_lea4)) });
	m_pending_writes.push_back({ RomOffsets::Graphics::TITLE_3_PAL_HIGHLIGHT, std::make_shared<ByteVector>(Split<uint8_t>(title_3_pal_highlight_lea)) });

	return true;
}

void GraphicsData::UpdateTilesetRecommendedPalettes()
{
	std::vector<std::string> palettes;
	for (const auto& p : GetAllPalettes())
	{
		palettes.push_back(p.first);
	}
	std::vector<std::string> map_palettes;
	for (const auto& p : m_island_map_pals)
	{
		map_palettes.push_back(p.first);
	}
	std::vector<std::string> title_palettes;
	for (const auto& p : m_title_pals)
	{
		title_palettes.push_back(p.first);
	}
	auto set_pals = [&](auto& container, const auto& rec)
	{
		for (auto& e : container)
		{
			e.second->SetAllPalettes(palettes);
			e.second->SetRecommendedPalettes(rec);
		}
	};
	set_pals(m_fonts_by_name, palettes);
	set_pals(m_misc_gfx_by_name, palettes);
	set_pals(m_status_fx_frames, palettes);
	set_pals(m_sword_fx, palettes);
	set_pals(m_island_map_tiles, map_palettes);
	set_pals(m_title_tiles, title_palettes);
	m_end_credits_tileset->SetAllPalettes(palettes);
	m_end_credits_tileset->SetRecommendedPalettes({ m_end_credits_palette->GetName() });
}

void GraphicsData::ResetTilesetDefaultPalettes()
{
	auto set_pals = [&](auto& container)
	{
		for (const auto& e : container)
		{
			if (e.second->GetRecommendedPalettes().size() == 0)
			{
				e.second->SetDefaultPalette(m_palettes_by_name.begin()->second->GetName());
			}
			else
			{
				e.second->SetDefaultPalette(e.second->GetRecommendedPalettes().front());
			}
		}
	};
	set_pals(m_fonts_by_name);
	set_pals(m_misc_gfx_by_name);
	set_pals(m_status_fx_frames);
	set_pals(m_sword_fx);
	set_pals(m_island_map_tiles);
	set_pals(m_title_tiles);
	m_end_credits_tileset->SetDefaultPalette(m_end_credits_palette->GetName());
}
