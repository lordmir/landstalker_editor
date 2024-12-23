#include <landstalker/main/include/GraphicsData.h>

#include <set>
#include <numeric>

#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/RomLabels.h>

GraphicsData::GraphicsData(const std::filesystem::path& asm_file)
	: DataManager("Graphics Data", asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadInventoryGraphics())
	{
		throw std::runtime_error(std::string("Unable to load inventory graphics data from \'") + m_inventory_graphics_filename.string() + '\'');
	}
	if (!AsmLoadPalettes())
	{
		throw std::runtime_error(std::string("Unable to load palettes from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadTextGraphics())
	{
		throw std::runtime_error(std::string("Unable to load text graphics from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadSwordFx())
	{
		throw std::runtime_error(std::string("Unable to load sword graphics from \'") + m_sword_fx_path.string() + '\'');
	}
	if (!AsmLoadStatusFx())
	{
		throw std::runtime_error(std::string("Unable to load status graphics from \'") + m_status_fx_path.string() + '\'');
	}
	if (!AsmLoadHudData())
	{
		throw std::runtime_error(std::string("Unable to load HUD data from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadEndCreditData())
	{
		throw std::runtime_error(std::string("Unable to load end credit data from \'") + m_end_credits_path.string() + '\'');
	}
	if (!AsmLoadIslandMapData())
	{
		throw std::runtime_error(std::string("Unable to load island map data from \'") + m_island_map_path.string() + '\'');
	}
	if (!AsmLoadLithographData())
	{
		throw std::runtime_error(std::string("Unable to load lithograph data from \'") + m_lithograph_path.string() + '\'');
	}
	if (!AsmLoadTitleScreenData())
	{
		throw std::runtime_error(std::string("Unable to load title screen data from \'") + m_title_path.string() + '\'');
	}
	if (!AsmLoadSegaLogoData())
	{
		throw std::runtime_error(std::string("Unable to load Sega logo data from \'") + m_sega_logo_path.string() + '\'');
	}
	if (!AsmLoadClimaxLogoData())
	{
		throw std::runtime_error(std::string("Unable to load Climax logo data from \'") + m_climax_logo_path.string() + '\'');
	}
	if (!AsmLoadLoadGameScreenData())
	{
		throw std::runtime_error(std::string("Unable to load Load Game screen data from \'") + m_load_game_path.string() + '\'');
	}
	InitCache();
	UpdateTilesetRecommendedPalettes();
	ResetTilesetDefaultPalettes();
}

GraphicsData::GraphicsData(const Rom& rom)
	: DataManager("Graphics Data", rom)
{
	SetDefaultFilenames();
	if (!RomLoadInventoryGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to load inventory graphics from ROM"));
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
	if (!RomLoadLithographData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Lithograph data from ROM"));
	}
	if (!RomLoadTitleScreenData(rom))
	{
		throw std::runtime_error(std::string("Unable to load title screen data from ROM"));
	}
	if (!RomLoadSegaLogoData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Sega logo data from ROM"));
	}
	if (!RomLoadClimaxLogoData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Climax logo data from ROM"));
	}
	if (!RomLoadGameLoadScreenData(rom))
	{
		throw std::runtime_error(std::string("Unable to load Load Game screen data from ROM"));
	}
	InitCache();
	UpdateTilesetRecommendedPalettes();
	ResetTilesetDefaultPalettes();
}

bool GraphicsData::Save(const std::filesystem::path& dir)
{
	std::filesystem::path directory = dir;
	if (std::filesystem::exists(directory) && std::filesystem::is_regular_file(directory))
	{
		directory = directory.parent_path();
	}
	if (!CreateDirectoryStructure(directory))
	{
		throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.string() + '\'');
	}
	if (!AsmSaveGraphics(dir))
	{
		throw std::runtime_error(std::string("Unable to save graphics data to \'") + directory.string() + '\'');
	}
	if (!AsmSaveInventoryGraphics(dir))
	{
		throw std::runtime_error(std::string("Unable to save inventory graphics data to \'") + m_inventory_graphics_filename.string() + '\'');
	}
	if (!AsmSaveSwordFx(dir))
	{
		throw std::runtime_error(std::string("Unable to save sword effects data to \'") + m_sword_fx_path.string() + '\'');
	}
	if (!AsmSaveStatusFx(dir))
	{
		throw std::runtime_error(std::string("Unable to save status effects data to \'") + m_status_fx_path.string() + '\'');
	}
	if (!AsmSaveEndCreditData(dir))
	{
		throw std::runtime_error(std::string("Unable to save end credit data to \'") + m_end_credits_path.string() + '\'');
	}
	if (!AsmSaveIslandMapData(dir))
	{
		throw std::runtime_error(std::string("Unable to save island map data to \'") + m_island_map_path.string() + '\'');
	}
	if (!AsmSaveLithographData(dir))
	{
		throw std::runtime_error(std::string("Unable to save lithograph data to \'") + m_lithograph_path.string() + '\'');
	}
	if (!AsmSaveTitleScreenData(dir))
	{
		throw std::runtime_error(std::string("Unable to save title screen data to \'") + m_title_path.string() + '\'');
	}
	if (!AsmSaveSegaLogoData(dir))
	{
		throw std::runtime_error(std::string("Unable to save Sega logo data to \'") + m_sega_logo_path.string() + '\'');
	}
	if (!AsmSaveClimaxLogoData(dir))
	{
		throw std::runtime_error(std::string("Unable to save Climax logo data to \'") + m_climax_logo_path.string() + '\'');
	}
	if (!AsmSaveGameLoadData(dir))
	{
		throw std::runtime_error(std::string("Unable to save load game screen data to \'") + m_load_game_path.string() + '\'');
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
	auto entry_pred = [](const auto& e) {return e != nullptr && e->HasSavedDataChanged(); };
	auto pair_pred = [](const auto& e) {return e.second != nullptr && e.second->HasSavedDataChanged(); };
	if (std::any_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_ui_gfx_by_name.begin(), m_ui_gfx_by_name.end(), pair_pred))
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
	if (std::any_of(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), pair_pred))
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
	if (std::any_of(m_load_game_pals.begin(), m_load_game_pals.end(), pair_pred))
	{
		return true;
	}
	if (std::any_of(m_load_game_tiles.begin(), m_load_game_tiles.end(), pair_pred))
	{
		return true;
	}
	if (m_fonts_by_name != m_fonts_by_name_orig)
	{
		return true;
	}
	if (m_palettes_by_name_orig != m_palettes_by_name)
	{
		return true;
	}
	if (m_ui_gfx_by_name_orig != m_ui_gfx_by_name)
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
	if (m_ui_tilemaps_orig != m_ui_tilemaps)
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
	if (m_load_game_pals_orig != m_load_game_pals)
	{
		return true;
	}
	if (m_load_game_tiles_orig != m_load_game_tiles)
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
	if (entry_pred(m_lithograph_map))
	{
		return true;
	}
	if (entry_pred(m_lithograph_tileset))
	{
		return true;
	}
	if (entry_pred(m_lithograph_palette))
	{
		return true;
	}
	if (entry_pred(m_sega_logo_palette))
	{
		return true;
	}
	if (entry_pred(m_sega_logo_tileset))
	{
		return true;
	}
	if (entry_pred(m_climax_logo_map))
	{
		return true;
	}
	if (entry_pred(m_climax_logo_tileset))
	{
		return true;
	}
	if (entry_pred(m_climax_logo_palette))
	{
		return true;
	}
	if (entry_pred(m_load_game_map))
	{
		return true;
	}
	return false;
}

void GraphicsData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	if (!RomPrepareInjectInvGraphics(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare inventory graphics for ROM injection"));
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
	if (!RomPrepareInjectLithographData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare lithograph data for ROM injection"));
	}
	if (!RomPrepareInjectTitleScreenData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare title screen data for ROM injection"));
	}
	if (!RomPrepareInjectSegaLogoData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare Sega logo data for ROM injection"));
	}
	if (!RomPrepareInjectClimaxLogoData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare Climax logo data for ROM injection"));
	}
	if (!RomPrepareInjectGameLoadScreenData(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare load game screen data for ROM injection"));
	}
}

std::map<std::string, std::shared_ptr<TilesetEntry>> GraphicsData::GetAllTilesets() const
{
	std::map<std::string, std::shared_ptr<TilesetEntry>> result;
	for (auto& t : m_fonts_by_name)
	{
		result.insert(t);
	}
	for (auto& t : m_ui_gfx_by_name)
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
	for (auto& t : m_load_game_tiles)
	{
		result.insert(t);
	}
	result.insert({ m_end_credits_tileset->GetName(), m_end_credits_tileset });
	result.insert({ m_lithograph_tileset->GetName(), m_lithograph_tileset });
	result.insert({ m_sega_logo_tileset->GetName(), m_sega_logo_tileset });
	result.insert({ m_climax_logo_tileset->GetName(), m_climax_logo_tileset });
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

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetUIGraphics() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_ui_gfx_by_name)
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

std::shared_ptr<TilesetEntry> GraphicsData::GetEndCreditLogosTiles() const
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

std::shared_ptr<TilesetEntry> GraphicsData::GetLithographTiles() const
{
	return m_lithograph_tileset;
}

std::shared_ptr<TilesetEntry> GraphicsData::GetSegaLogoTiles() const
{
	return m_sega_logo_tileset;
}

std::shared_ptr<TilesetEntry> GraphicsData::GetClimaxLogoTiles() const
{
	return m_climax_logo_tileset;
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

std::vector<std::shared_ptr<TilesetEntry>> GraphicsData::GetGameLoadScreenTiles() const
{
	std::vector<std::shared_ptr<TilesetEntry>> retval;
	for (const auto& t : m_load_game_tiles)
	{
		retval.push_back(t.second);
	}
	return retval;
}

std::map<std::string, std::shared_ptr<Tilemap2DEntry>> GraphicsData::GetAllMaps() const
{
	std::map<std::string, std::shared_ptr<Tilemap2DEntry>> result;
	for (auto& t : m_ui_tilemaps)
	{
		result.insert(t);
	}
	for (auto& t : m_island_map_tilemaps)
	{
		result.insert(t);
	}
	for (auto& t : m_title_tilemaps)
	{
		result.insert(t);
	}
	result.insert({ m_end_credits_map->GetName(), m_end_credits_map });
	result.insert({ m_lithograph_map->GetName(), m_lithograph_map });
	result.insert({ m_climax_logo_map->GetName(), m_climax_logo_map });
	result.insert({ m_load_game_map->GetName(), m_load_game_map });
	return result;
}

std::shared_ptr<Tilemap2DEntry> GraphicsData::GetMap(const std::string& name) const
{
	for (auto& t : m_ui_tilemaps)
	{
		if (t.first == name)
		{
			return t.second;
		}
	}
	for (auto& t : m_island_map_tilemaps)
	{
		if (t.first == name)
		{
			return t.second;
		}
	}
	for (auto& t : m_title_tilemaps)
	{
		if (t.first == name)
		{
			return t.second;
		}
	}
	if (name == m_end_credits_map->GetName())
	{
		return m_end_credits_map;
	}
	if (name == m_lithograph_map->GetName())
	{
		return m_lithograph_map;
	}

	if (name == m_climax_logo_map->GetName())
	{
		return m_climax_logo_map;
	}

	if (name == m_load_game_map->GetName())
	{
		return m_load_game_map;
	}
	return nullptr;
}

std::vector<std::shared_ptr<Tilemap2DEntry>> GraphicsData::GetUIMaps() const
{
	std::vector<std::shared_ptr<Tilemap2DEntry>> result;
	for (auto& t : m_ui_tilemaps)
	{
		result.push_back(t.second);
	}
	return result;
}

std::shared_ptr<Tilemap2DEntry> GraphicsData::GetEndCreditLogosMaps() const
{
	return m_end_credits_map;
}

std::vector<std::shared_ptr<Tilemap2DEntry>> GraphicsData::GetIslandMapMaps() const
{
	std::vector<std::shared_ptr<Tilemap2DEntry>> result;
	for (auto& t : m_island_map_tilemaps)
	{
		result.push_back(t.second);
	}
	return result;
}

std::shared_ptr<Tilemap2DEntry> GraphicsData::GetLithographMap() const
{
	return m_lithograph_map;
}

std::shared_ptr<Tilemap2DEntry> GraphicsData::GetClimaxLogoMap() const
{
	return m_climax_logo_map;
}

std::vector<std::shared_ptr<Tilemap2DEntry>> GraphicsData::GetTitleScreenMap() const
{
	std::vector<std::shared_ptr<Tilemap2DEntry>> result;
	for (auto& t : m_title_tilemaps)
	{
		result.push_back(t.second);
	}
	return result;
}

std::shared_ptr<Tilemap2DEntry> GraphicsData::GetGameLoadScreenMap() const
{
	return m_load_game_map;
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
	for (auto& t : m_load_game_pals)
	{
		result.insert(t);
	}
	result.insert({ m_end_credits_palette->GetName(), m_end_credits_palette });
	result.insert({ m_lithograph_palette->GetName(), m_lithograph_palette });
	result.insert({ m_sega_logo_palette->GetName(), m_sega_logo_palette });
	result.insert({ m_climax_logo_palette->GetName(), m_climax_logo_palette });
	return result;
}

std::map<std::string, std::shared_ptr<PaletteEntry>> GraphicsData::GetSwordPalettes() const
{
	return m_sword_palettes;
}

std::map<std::string, std::shared_ptr<PaletteEntry>> GraphicsData::GetArmourPalettes() const
{
	return m_armour_palettes;
}

std::map<std::string, std::shared_ptr<PaletteEntry>> GraphicsData::GetOtherPalettes() const
{
	std::map<std::string, std::shared_ptr<PaletteEntry>> result;
	for (auto& t : m_misc_palettes)
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
	for (auto& t : m_load_game_pals)
	{
		result.insert(t);
	}
	result.insert({ m_end_credits_palette->GetName(), m_end_credits_palette });
	result.insert({ m_lithograph_palette->GetName(), m_lithograph_palette });
	result.insert({ m_sega_logo_palette->GetName(), m_sega_logo_palette });
	result.insert({ m_climax_logo_palette->GetName(), m_climax_logo_palette });
	return result;
}

std::shared_ptr<PaletteEntry> GraphicsData::GetPlayerPalette() const
{
	return m_palettes_by_name.find(RomLabels::Graphics::PLAYER_PAL)->second;
}

std::shared_ptr<PaletteEntry> GraphicsData::GetHudPalette() const
{
	return m_palettes_by_name.find(RomLabels::Graphics::HUD_PAL)->second;
}

void GraphicsData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };

	std::for_each(m_fonts_by_name.begin(), m_fonts_by_name.end(), pair_commit);
	std::for_each(m_palettes_by_name.begin(), m_palettes_by_name.end(), pair_commit);
	std::for_each(m_ui_gfx_by_name.begin(), m_ui_gfx_by_name.end(), pair_commit);
	std::for_each(m_status_fx_frames.begin(), m_status_fx_frames.end(), pair_commit);
	std::for_each(m_sword_fx.begin(), m_sword_fx.end(), pair_commit);
	std::for_each(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), pair_commit);
	std::for_each(m_island_map_pals.begin(), m_island_map_pals.end(), pair_commit);
	std::for_each(m_island_map_tilemaps.begin(), m_island_map_tilemaps.end(), pair_commit);
	std::for_each(m_island_map_tiles.begin(), m_island_map_tiles.end(), pair_commit);
	std::for_each(m_title_pals.begin(), m_title_pals.end(), pair_commit);
	std::for_each(m_title_tilemaps.begin(), m_title_tilemaps.end(), pair_commit);
	std::for_each(m_title_tiles.begin(), m_title_tiles.end(), pair_commit);
	std::for_each(m_load_game_tiles.begin(), m_load_game_tiles.end(), pair_commit);
	std::for_each(m_load_game_pals.begin(), m_load_game_pals.end(), pair_commit);

	m_fonts_by_name_orig = m_fonts_by_name;
	m_palettes_by_name_orig = m_palettes_by_name;
	m_ui_gfx_by_name_orig = m_ui_gfx_by_name;
	m_status_fx_orig = m_status_fx;
	m_status_fx_frames_orig = m_status_fx_frames;
	m_sword_fx_orig = m_sword_fx;
	m_ui_tilemaps_orig = m_ui_tilemaps;
	m_island_map_pals_orig = m_island_map_pals;
	m_island_map_tilemaps_orig = m_island_map_tilemaps;
	m_island_map_tiles_orig = m_island_map_tiles;
	m_title_pals_orig = m_title_pals;
	m_title_tilemaps_orig = m_title_tilemaps;
	m_title_tiles_orig = m_title_tiles;
	m_load_game_pals_orig = m_load_game_pals;
	m_load_game_tiles_orig = m_load_game_tiles;

	entry_commit(m_end_credits_map);
	entry_commit(m_end_credits_palette);
	entry_commit(m_end_credits_tileset);
	entry_commit(m_lithograph_map);
	entry_commit(m_lithograph_palette);
	entry_commit(m_lithograph_tileset);
	entry_commit(m_sega_logo_palette);
	entry_commit(m_sega_logo_tileset);
	entry_commit(m_climax_logo_map);
	entry_commit(m_climax_logo_palette);
	entry_commit(m_climax_logo_tileset);
	entry_commit(m_load_game_map);
	m_pending_writes.clear();
}

bool GraphicsData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().string());
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::INV_SECTION, m_inventory_graphics_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::STATUS_FX_POINTERS, m_status_fx_pointers_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::STATUS_FX_DATA, m_status_fx_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::SWORD_FX_DATA, m_sword_fx_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::END_CREDITS_DATA, m_end_credits_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::ISLAND_MAP_DATA, m_island_map_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::LITHOGRAPH_DATA, m_lithograph_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::TITLE_DATA, m_title_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::SEGA_LOGO_DATA, m_sega_logo_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::CLIMAX_LOGO_DATA, m_climax_logo_path);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Graphics::GAME_LOAD_DATA, m_load_game_path);
		
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void GraphicsData::SetDefaultFilenames()
{
	if (m_inventory_graphics_filename.empty()) m_inventory_graphics_filename = RomLabels::Graphics::INV_GRAPHICS_FILE;
	if (m_status_fx_path.empty()) m_status_fx_path = RomLabels::Graphics::STATUS_FX_DATA_FILE;
	if (m_status_fx_pointers_path.empty()) m_status_fx_pointers_path = RomLabels::Graphics::STATUS_FX_POINTER_FILE;
	if (m_sword_fx_path.empty()) m_sword_fx_path = RomLabels::Graphics::SWORD_FX_DATA_FILE;
	if (m_end_credits_path.empty()) m_end_credits_path = RomLabels::Graphics::END_CREDITS_DATA_FILE;
	if (m_island_map_path.empty()) m_island_map_path = RomLabels::Graphics::ISLAND_MAP_DATA_FILE;
	if (m_lithograph_path.empty()) m_lithograph_path = RomLabels::Graphics::LITHOGRAPH_DATA_FILE;
	if (m_title_path.empty()) m_title_path = RomLabels::Graphics::TITLE_DATA_FILE;
	if (m_title_routines_1_path.empty()) m_title_routines_1_path = RomLabels::Graphics::TITLE_ROUTINES_1_FILE;
	if (m_title_routines_2_path.empty()) m_title_routines_2_path = RomLabels::Graphics::TITLE_ROUTINES_2_FILE;
	if (m_title_routines_3_path.empty()) m_title_routines_3_path = RomLabels::Graphics::TITLE_ROUTINES_3_FILE;
	if (m_sega_logo_path.empty()) m_sega_logo_path = RomLabels::Graphics::SEGA_LOGO_DATA_FILE;
	if (m_sega_logo_routines_1_path.empty()) m_sega_logo_routines_1_path = RomLabels::Graphics::SEGA_LOGO_ROUTINES1_FILE;
	if (m_sega_logo_routines_2_path.empty()) m_sega_logo_routines_2_path = RomLabels::Graphics::SEGA_LOGO_ROUTINES2_FILE;
	if (m_climax_logo_path.empty()) m_climax_logo_path = RomLabels::Graphics::CLIMAX_LOGO_DATA_FILE;
	if (m_load_game_path.empty()) m_load_game_path = RomLabels::Graphics::GAME_LOAD_DATA_FILE;
	if (m_load_game_routines_1_path.empty()) m_load_game_routines_1_path = RomLabels::Graphics::GAME_LOAD_ROUTINES_1_FILE;
	if (m_load_game_routines_2_path.empty()) m_load_game_routines_2_path = RomLabels::Graphics::GAME_LOAD_ROUTINES_2_FILE;
	if (m_load_game_routines_3_path.empty()) m_load_game_routines_3_path = RomLabels::Graphics::GAME_LOAD_ROUTINES_3_FILE;
}

bool GraphicsData::CreateDirectoryStructure(const std::filesystem::path& dir)
{
	bool retval = true;
	retval = retval && CreateDirectoryTree(dir / m_inventory_graphics_filename);
	retval = retval && CreateDirectoryTree(dir / m_status_fx_path);
	retval = retval && CreateDirectoryTree(dir / m_status_fx_pointers_path);
	retval = retval && CreateDirectoryTree(dir / m_sword_fx_path);
	retval = retval && CreateDirectoryTree(dir / m_end_credits_path);
	retval = retval && CreateDirectoryTree(dir / m_island_map_path);
	retval = retval && CreateDirectoryTree(dir / m_lithograph_path);
	retval = retval && CreateDirectoryTree(dir / m_title_path);
	retval = retval && CreateDirectoryTree(dir / m_title_routines_1_path);
	retval = retval && CreateDirectoryTree(dir / m_title_routines_2_path);
	retval = retval && CreateDirectoryTree(dir / m_title_routines_3_path);
	retval = retval && CreateDirectoryTree(dir / m_sega_logo_path);
	retval = retval && CreateDirectoryTree(dir / m_sega_logo_routines_1_path);
	retval = retval && CreateDirectoryTree(dir / m_sega_logo_routines_1_path);
	retval = retval && CreateDirectoryTree(dir / m_climax_logo_path);
	retval = retval && CreateDirectoryTree(dir / m_load_game_path);
	retval = retval && CreateDirectoryTree(dir / m_load_game_routines_1_path);
	retval = retval && CreateDirectoryTree(dir / m_load_game_routines_2_path);
	retval = retval && CreateDirectoryTree(dir / m_load_game_routines_3_path);
	for (const auto& f : m_fonts_by_name)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& g : m_ui_gfx_by_name)
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
	for (const auto& f : m_ui_tilemaps)
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
	for (const auto& f : m_load_game_pals)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	for (const auto& f : m_load_game_tiles)
	{
		retval = retval && CreateDirectoryTree(dir / f.second->GetFilename());
	}
	retval = retval && CreateDirectoryTree(dir / m_end_credits_map->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_end_credits_palette->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_end_credits_tileset->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_lithograph_map->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_lithograph_palette->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_lithograph_tileset->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_sega_logo_palette->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_sega_logo_tileset->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_climax_logo_map->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_climax_logo_palette->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_climax_logo_tileset->GetFilename());
	retval = retval && CreateDirectoryTree(dir / m_load_game_map->GetFilename());
	return retval;
}

void GraphicsData::InitCache()
{
	m_palettes_by_name_orig = m_palettes_by_name;
	m_fonts_by_name_orig = m_fonts_by_name;
	m_ui_gfx_by_name_orig = m_ui_gfx_by_name;
	m_sword_fx_orig = m_sword_fx;
	m_status_fx_orig = m_status_fx;
	m_status_fx_frames_orig = m_status_fx_frames;
	m_ui_tilemaps_orig = m_ui_tilemaps;
	m_island_map_pals_orig = m_island_map_pals;
	m_island_map_tilemaps_orig = m_island_map_tilemaps;
	m_island_map_tiles_orig = m_island_map_tiles;
	m_title_pals_orig = m_title_pals;
	m_title_tilemaps_orig = m_title_tilemaps;
	m_title_tiles_orig = m_title_tiles;
	m_load_game_pals_orig = m_load_game_pals;
	m_load_game_tiles_orig = m_load_game_tiles;
}

bool GraphicsData::AsmLoadInventoryGraphics()
{
	try
	{
		AsmFile file(GetBasePath() / m_inventory_graphics_filename);
		AsmFile::Label name;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVectorPtr>> gfx;
		while (file.IsGood())
		{
			file >> name >> inc;
			auto path = GetBasePath() / inc.path;
			auto raw_data = std::make_shared<std::vector<uint8_t>>(ReadBytes(path));
			gfx.push_back({ name, inc.path, raw_data });
		}

		assert(gfx.size() == 5 || gfx.size() == 7);

		std::shared_ptr<TilesetEntry> menufont;
		std::shared_ptr<TilesetEntry> menucursor;
		std::shared_ptr<TilesetEntry> arrow;
		std::shared_ptr<TilesetEntry> unused1;
		std::shared_ptr<TilesetEntry> unused2;
		std::shared_ptr<PaletteEntry> pal1;
		std::shared_ptr<PaletteEntry> pal2;

		std::filesystem::path menu_font_filename = std::get<1>(gfx[0]);
		std::string menu_font_extension = str_to_lower(menu_font_filename.extension().string());

		// French and German ROMs have a larger, compressed menu font to accomodate diacritics.
		if (menu_font_extension == ".lz77")
		{
			menufont = TilesetEntry::Create(this, *std::get<2>(gfx[0]), std::get<0>(gfx[0]),
				std::get<1>(gfx[0]), true, 8, 16, 1);
		}
		else
		{
			menufont = TilesetEntry::Create(this, *std::get<2>(gfx[0]), std::get<0>(gfx[0]),
				std::get<1>(gfx[0]), false, 8, 8, 1);
		}

		menucursor = TilesetEntry::Create(this, *std::get<2>(gfx[1]), std::get<0>(gfx[1]),
					std::get<1>(gfx[1]), false, 8, 8, 2, Tileset::BlockType::BLOCK4X4);
		arrow = TilesetEntry::Create(this, *std::get<2>(gfx[2]), std::get<0>(gfx[2]),
			std::get<1>(gfx[2]), false, 8, 8, 2, Tileset::BlockType::BLOCK2X2);

		if (gfx.size() == 5)
		{
			pal1 = PaletteEntry::Create(this, *std::get<2>(gfx[3]), std::get<0>(gfx[3]),
				std::get<1>(gfx[3]), Palette::Type::LOW8);
			pal2 = PaletteEntry::Create(this, *std::get<2>(gfx[4]), std::get<0>(gfx[4]),
				std::get<1>(gfx[4]), Palette::Type::FULL);
		}
		else
		{
			unused1 = TilesetEntry::Create(this, *std::get<2>(gfx[3]), std::get<0>(gfx[3]),
				std::get<1>(gfx[3]), false, 8, 11, 2);
			unused2 = TilesetEntry::Create(this, *std::get<2>(gfx[4]), std::get<0>(gfx[4]),
				std::get<1>(gfx[4]), false, 8, 10, 2);
			pal1 = PaletteEntry::Create(this, *std::get<2>(gfx[5]), std::get<0>(gfx[5]),
				std::get<1>(gfx[5]), Palette::Type::LOW8);
			pal2 = PaletteEntry::Create(this, *std::get<2>(gfx[6]), std::get<0>(gfx[6]),
				std::get<1>(gfx[6]), Palette::Type::FULL);
		}

		if (menufont != nullptr)
		{
			m_fonts_by_name.insert({ menufont->GetName(), menufont });
			m_fonts_internal.insert({ menufont->GetName(), menufont });
		}
		if (menucursor != nullptr)
		{
			m_ui_gfx_by_name.insert({ menucursor->GetName(), menucursor });
			m_ui_gfx_internal.insert({ menucursor->GetName(), menucursor });
		}
		if (arrow != nullptr)
		{
			m_ui_gfx_by_name.insert({ arrow->GetName(), arrow });
			m_ui_gfx_internal.insert({ arrow->GetName(), arrow });
		}
		if (unused1 != nullptr)
		{
			m_ui_gfx_by_name.insert({ unused1->GetName(), unused1 });
			m_ui_gfx_internal.insert({ unused1->GetName(), unused1 });
		}
		if (unused2 != nullptr)
		{
			m_ui_gfx_by_name.insert({ unused2->GetName(), unused2 });
			m_ui_gfx_internal.insert({ unused2->GetName(), unused2 });
		}
		if (pal1 != nullptr)
		{
			m_palettes_by_name.insert({ pal1->GetName(), pal1 });
			m_palettes_internal.insert({ pal1->GetName(), pal1 });
		}
		if (pal2 != nullptr)
		{
			m_palettes_by_name.insert({ pal2->GetName(), pal2 });
			m_palettes_internal.insert({ pal2->GetName(), pal2 });
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
				if (type == Palette::Type::SWORD)
				{
					m_sword_palettes.insert({ e->GetName(), e });
				}
				else if (type == Palette::Type::ARMOUR)
				{
					m_armour_palettes.insert({ e->GetName(), e });
				}
				else
				{
					m_misc_palettes.insert({ e->GetName(), e });
				}
			}
		};
		file.Goto(RomLabels::Graphics::PLAYER_PAL);
		file >> inc;
		auto player_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::PLAYER_PAL, inc.path, Palette::Type::FULL);
		file.Goto(RomLabels::Graphics::HUD_PAL);
		file >> inc;
		auto hud_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::HUD_PAL, inc.path, Palette::Type::HUD);
		file.Goto(RomLabels::Graphics::INV_ITEM_PAL);
		file >> inc;
		auto item_pal = PaletteEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::INV_ITEM_PAL, inc.path, Palette::Type::FULL);
		file.Goto(RomLabels::Graphics::SWORD_PAL_SWAPS);
		file >> inc;
		auto sword_pal_bytes = ReadBytes(GetBasePath() / inc.path);
		extract_pals(sword_pal_bytes, RomLabels::Graphics::SWORD_PAL_SWAPS, Palette::Type::SWORD);
		file.Goto(RomLabels::Graphics::ARMOUR_PAL_SWAPS);
		file >> inc;
		auto armour_pal_bytes = ReadBytes(GetBasePath() / inc.path);
		extract_pals(armour_pal_bytes, RomLabels::Graphics::ARMOUR_PAL_SWAPS, Palette::Type::ARMOUR);

		m_palettes_by_name.insert({ player_pal->GetName(), player_pal });
		m_palettes_by_name.insert({ hud_pal->GetName(), hud_pal });
		m_palettes_by_name.insert({ item_pal->GetName(), item_pal });
		m_palettes_internal.insert({ player_pal->GetName(), player_pal });
		m_palettes_internal.insert({ hud_pal->GetName(), hud_pal });
		m_palettes_internal.insert({ item_pal->GetName(), item_pal });
		m_misc_palettes.insert({ player_pal->GetName(), player_pal });
		m_misc_palettes.insert({ hud_pal->GetName(), hud_pal });
		m_misc_palettes.insert({ item_pal->GetName(), item_pal });
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
		file.Goto(RomLabels::Graphics::DOWN_ARROW);
		file >> inc;
		auto down_arrow = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::DOWN_ARROW, inc.path, false, 8, 8, 4, Tileset::BlockType::BLOCK2X2);
		file.Goto(RomLabels::Graphics::RIGHT_ARROW);
		file >> inc;
		auto right_arrow = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::RIGHT_ARROW, inc.path, false, 8, 8, 4, Tileset::BlockType::BLOCK2X2);
		m_ui_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
		m_ui_gfx_by_name.insert({ right_arrow->GetName(), right_arrow });
		m_ui_gfx_internal.insert({ down_arrow->GetName(), down_arrow });
		m_ui_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
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
		m_ui_tilemaps.insert({ m->GetName(), m });
		m_ui_tilemaps_internal.insert({ RomLabels::Graphics::INV_TILEMAP, m });
		auto names = { RomLabels::Graphics::SWORD_MAGIC, RomLabels::Graphics::SWORD_THUNDER,
					   RomLabels::Graphics::SWORD_GAIA,  RomLabels::Graphics::SWORD_ICE,
					   RomLabels::Graphics::COINFALL };
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
					name, inc.path, true, 8, 8, 4, Tileset::BlockType::BLOCK4X4);
			}
			else
			{
				e = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
					name, inc.path, true, 8, 8, 4, Tileset::BlockType::BLOCK4X6);
			}
			m_sword_fx.insert({ e->GetName(), e });
			if (count < static_cast<int>(names.size()))
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
				inc.path, true, 8, 8, 4, Tileset::BlockType::BLOCK4X4);
			m_status_fx_frames.insert({ e->GetName(), e });
		}
		AsmFile pfile(GetBasePath() / m_status_fx_pointers_path);
		std::vector<std::string> status = { RomLabels::Graphics::STATUS_FX_POISON, RomLabels::Graphics::STATUS_FX_CONFUSION,
					   RomLabels::Graphics::STATUS_FX_PARALYSIS, RomLabels::Graphics::STATUS_FX_CURSE };
		std::vector<std::string> status_frame_names = { RomLabels::Graphics::STATUS_FX_POISON_FRAME, RomLabels::Graphics::STATUS_FX_CONFUSION_FRAME,
					   RomLabels::Graphics::STATUS_FX_PARALYSIS_FRAME, RomLabels::Graphics::STATUS_FX_CURSE_FRAME };
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
		for (std::size_t si = 0; si < status.size(); si++)
		{
			const auto& status_name = status[si];
			const auto& status_frame_name = status_frame_names[si];
			const auto& status_frames = m_status_fx[status_name];
			for (std::size_t fi = 0; fi < status_frames.size(); ++fi)
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

bool GraphicsData::AsmLoadHudData()
{
	try
	{
		AsmFile file(GetAsmFilename());
		AsmFile::IncludeFile inc;
		file.Goto(RomLabels::Graphics::HUD_TILEMAP);
		file >> inc;
		auto map = Tilemap2DEntry::Create(this, ReadBytes(GetBasePath() / inc.path),
			RomLabels::Graphics::HUD_TILEMAP, inc.path, Tilemap2D::Compression::NONE, 0x6B4, 40, 3);
		file.Goto(RomLabels::Graphics::HUD_TILESET);
		file >> inc;
		auto ts = TilesetEntry::Create(this, ReadBytes(GetBasePath() / inc.path), RomLabels::Graphics::HUD_TILESET, inc.path);
		m_ui_tilemaps.insert({ map->GetName(), map });
		m_ui_tilemaps_internal.insert({ map->GetName(), map });
		m_ui_gfx_by_name.insert({ ts->GetName(), ts });
		m_ui_gfx_internal.insert({ ts->GetName(), ts });
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
		std::filesystem::path pal_path = inc.path;
		auto pal_bytes = ReadBytes(GetBasePath() / pal_path);
		file >> lbl >> inc;
		std::string font_name = lbl;
		std::filesystem::path font_path = inc.path;
		auto font_bytes = ReadBytes(GetBasePath() / font_path);
		file >> lbl >> inc;
		std::string logos_name = lbl;
		std::filesystem::path logos_path = inc.path;
		auto logos_bytes = ReadBytes(GetBasePath() / logos_path);
		file >> lbl >> inc;
		std::string map_name = lbl;
		std::filesystem::path map_path = inc.path;
		auto map_bytes = ReadBytes(GetBasePath() / map_path);
		m_end_credits_palette = PaletteEntry::Create(this, pal_bytes, pal_name, pal_path, Palette::Type::END_CREDITS);
		m_end_credits_tileset = TilesetEntry::Create(this, logos_bytes, logos_name, logos_path);
		m_end_credits_map = Tilemap2DEntry::Create(this, map_bytes, map_name, map_path, Tilemap2D::Compression::RLE, 0x100);
		auto font = TilesetEntry::Create(this, font_bytes, font_name, font_path, true, 8, 8, 2);
		m_fonts_by_name.insert({ font->GetName(), font });
		m_fonts_internal.insert({ RomLabels::Graphics::END_CREDITS_FONT, font });
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
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			const auto& path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 8);
		auto fg_tiles = TilesetEntry::Create(this, std::get<2>(entries[0]), std::get<0>(entries[0]), std::get<1>(entries[0]));
		auto fg_map = Tilemap2DEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]), Tilemap2D::Compression::RLE, 0x100);
		auto bg_tiles = TilesetEntry::Create(this, std::get<2>(entries[2]), std::get<0>(entries[2]), std::get<1>(entries[2]));
		auto bg_map = Tilemap2DEntry::Create(this, std::get<2>(entries[3]), std::get<0>(entries[3]), std::get<1>(entries[3]), Tilemap2D::Compression::RLE, 0x100);
		auto dots_tiles = TilesetEntry::Create(this, std::get<2>(entries[4]), std::get<0>(entries[4]), std::get<1>(entries[4]));
		auto friday_tiles = TilesetEntry::Create(this, std::get<2>(entries[5]), std::get<0>(entries[5]), std::get<1>(entries[5]), true, 8, 8, 4, Tileset::BlockType::BLOCK2X2);
		auto fg_pal = PaletteEntry::Create(this, std::get<2>(entries[6]), std::get<0>(entries[6]), std::get<1>(entries[6]), Palette::Type::FULL);
		auto bg_pal = PaletteEntry::Create(this, std::get<2>(entries[7]), std::get<0>(entries[7]), std::get<1>(entries[7]), Palette::Type::FULL);

		m_island_map_tiles.insert({ fg_tiles->GetName(), fg_tiles });
		m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FG_TILES, fg_tiles });
		m_island_map_tiles.insert({ bg_tiles->GetName(), bg_tiles });
		m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_BG_TILES, bg_tiles });
		m_island_map_tiles.insert({ dots_tiles->GetName(), dots_tiles });
		m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_DOTS, dots_tiles });
		m_island_map_tiles.insert({ friday_tiles->GetName(), friday_tiles });
		m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FRIDAY, friday_tiles });

		m_island_map_tilemaps.insert({ fg_map->GetName(), fg_map });
		m_island_map_tilemaps_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FG_MAP, fg_map });
		m_island_map_tilemaps.insert({ bg_map->GetName(), bg_map });
		m_island_map_tilemaps_internal.insert({ RomLabels::Graphics::ISLAND_MAP_BG_MAP, bg_map });

		m_island_map_pals.insert({ fg_pal->GetName(), fg_pal });
		m_island_map_pals_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FG_PAL, fg_pal });
		m_island_map_pals.insert({ bg_pal->GetName(), bg_pal });
		m_island_map_pals_internal.insert({ RomLabels::Graphics::ISLAND_MAP_BG_PAL, bg_pal });
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadLithographData()
{
	try
	{
		AsmFile file(GetBasePath() / m_lithograph_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			const auto& path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 3);
		m_lithograph_palette = PaletteEntry::Create(this, std::get<2>(entries[0]), std::get<0>(entries[0]), std::get<1>(entries[0]), Palette::Type::FULL);
		m_lithograph_tileset = TilesetEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]));
		m_lithograph_map = Tilemap2DEntry::Create(this, std::get<2>(entries[2]), std::get<0>(entries[2]), std::get<1>(entries[2]), Tilemap2D::Compression::RLE, 0x100);

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
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			const auto& path = inc.path;
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
		m_title_tiles_internal.insert({ RomLabels::Graphics::TITLE_1_TILES, title_1_tiles });
		m_title_tiles.insert({ title_2_tiles->GetName(), title_2_tiles });
		m_title_tiles_internal.insert({ RomLabels::Graphics::TITLE_2_TILES, title_2_tiles });
		m_title_tiles.insert({ title_3_tiles->GetName(), title_3_tiles });
		m_title_tiles_internal.insert({ RomLabels::Graphics::TITLE_3_TILES, title_3_tiles });

		m_title_tilemaps.insert({ title_1_map->GetName(), title_1_map });
		m_title_tilemaps_internal.insert({ RomLabels::Graphics::TITLE_1_MAP, title_1_map });
		m_title_tilemaps.insert({ title_2_map->GetName(), title_2_map });
		m_title_tilemaps_internal.insert({ RomLabels::Graphics::TITLE_2_MAP, title_2_map });
		m_title_tilemaps.insert({ title_3_map->GetName(), title_3_map });
		m_title_tilemaps_internal.insert({ RomLabels::Graphics::TITLE_3_MAP, title_3_map });

		m_title_pals.insert({ title_3_pal->GetName(), title_3_pal });
		m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_3_PAL, title_3_pal });
		m_title_pals.insert({ title_3_pal_highlight->GetName(), title_3_pal_highlight });
		m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT, title_3_pal_highlight });
		m_title_pals.insert({ yellow_pal->GetName(), yellow_pal });
		m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_PALETTE_YELLOW, yellow_pal });
		m_title_pals.insert({ blue_pal->GetName(), blue_pal });
		m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_PALETTE_BLUE, blue_pal });

		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadSegaLogoData()
{
	try
	{
		AsmFile file(GetBasePath() / m_sega_logo_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			const auto& path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 4);
		m_sega_logo_routines_1_path = std::get<1>(entries[0]);
		m_sega_logo_palette = PaletteEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]), Palette::Type::SEGA_LOGO);
		m_sega_logo_routines_2_path = std::get<1>(entries[2]);
		m_sega_logo_tileset = TilesetEntry::Create(this, std::get<2>(entries[3]), std::get<0>(entries[3]), std::get<1>(entries[3]));

		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadClimaxLogoData()
{
	try
	{
		AsmFile file(GetBasePath() / m_climax_logo_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			const auto& path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 3);
		m_climax_logo_tileset = TilesetEntry::Create(this, std::get<2>(entries[0]), std::get<0>(entries[0]), std::get<1>(entries[0]));
		m_climax_logo_map = Tilemap2DEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]), Tilemap2D::Compression::RLE, 0x100);
		m_climax_logo_palette = PaletteEntry::Create(this, std::get<2>(entries[2]), std::get<0>(entries[2]), std::get<1>(entries[2]), Palette::Type::CLIMAX_LOGO);

		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmLoadLoadGameScreenData()
{
	try
	{
		AsmFile file(GetBasePath() / m_load_game_path);
		AsmFile::Label lbl;
		AsmFile::IncludeFile inc;
		std::vector<std::tuple<std::string, std::filesystem::path, ByteVector>> entries;
		while (file.IsGood())
		{
			file >> lbl >> inc;
			std::string name = lbl;
			const auto& path = inc.path;
			auto bytes = ReadBytes(GetBasePath() / path);
			entries.push_back({ name, path, bytes });
		}
		assert(entries.size() >= 8);
		m_load_game_routines_1_path = std::get<1>(entries[0]);
		auto pal = PaletteEntry::Create(this, std::get<2>(entries[1]), std::get<0>(entries[1]), std::get<1>(entries[1]), Palette::Type::FULL);
		m_load_game_routines_2_path = std::get<1>(entries[2]);
		auto player_pal = PaletteEntry::Create(this, std::get<2>(entries[3]), std::get<0>(entries[3]), std::get<1>(entries[3]), Palette::Type::FULL);
		auto chars = TilesetEntry::Create(this, std::get<2>(entries[4]), std::get<0>(entries[4]), std::get<1>(entries[4]));
		auto tiles = TilesetEntry::Create(this, std::get<2>(entries[5]), std::get<0>(entries[5]), std::get<1>(entries[5]));
		m_load_game_map = Tilemap2DEntry::Create(this, std::get<2>(entries[6]), std::get<0>(entries[6]), std::get<1>(entries[6]), Tilemap2D::Compression::RLE, 0x100);
		m_load_game_routines_3_path = std::get<1>(entries[7]);

		m_load_game_pals.insert({ pal->GetName(), pal });
		m_load_game_pals_internal.insert({ RomLabels::Graphics::GAME_LOAD_PALETTE, pal });
		m_load_game_pals.insert({ player_pal->GetName(), player_pal });
		m_load_game_pals_internal.insert({ RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE, player_pal });
		m_load_game_tiles.insert({ chars->GetName(), chars });
		m_load_game_tiles_internal.insert({ RomLabels::Graphics::GAME_LOAD_CHARS, chars });
		m_load_game_tiles.insert({ tiles->GetName(), tiles });
		m_load_game_tiles_internal.insert({ RomLabels::Graphics::GAME_LOAD_TILES, tiles });

		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::RomLoadInventoryGraphics(const Rom& rom)
{
	auto load_bytes = [&](const std::string& lea_loc, const std::string& size)
	{
		uint32_t start = Disasm::ReadOffset16(rom, lea_loc);
		uint32_t sz = rom.get_address(size);
		return rom.read_array<uint8_t>(start, sz);
	};
	auto load_pal = [&](const std::string& lea_loc, Palette::Type type)
	{
		uint32_t start = Disasm::ReadOffset16(rom, lea_loc);
		uint32_t sz = Palette::GetSizeBytes(type);
		return rom.read_array<uint8_t>(start, sz);
	};
	std::shared_ptr<TilesetEntry> menu_font;
	if (rom.get_address(RomLabels::Graphics::INV_FONT_SIZE) > 0)
	{
		menu_font = TilesetEntry::Create(this, load_bytes(RomLabels::Graphics::INV_FONT, RomLabels::Graphics::INV_FONT_SIZE),
			RomLabels::Graphics::INV_FONT, RomLabels::Graphics::INV_FONT_FILE, false, 8, 8, 1);
	}
	else
	{
		menu_font = TilesetEntry::Create(this, rom.read_array<uint8_t>(Disasm::ReadOffset16(rom, RomLabels::Graphics::INV_FONT), 65536),
			RomLabels::Graphics::INV_FONT, RomLabels::Graphics::INV_FONT_LARGE_FILE, true, 8, 16, 1);
	}
	auto cursor = TilesetEntry::Create(this, load_bytes(RomLabels::Graphics::INV_CURSOR, RomLabels::Graphics::INV_CURSOR_SIZE),
		RomLabels::Graphics::INV_CURSOR, RomLabels::Graphics::INV_CURSOR_FILE, false, 8, 8, 2, Tileset::BlockType::BLOCK4X4);
	auto arrow = TilesetEntry::Create(this, load_bytes(RomLabels::Graphics::INV_ARROW, RomLabels::Graphics::INV_ARROW_SIZE),
		RomLabels::Graphics::INV_ARROW, RomLabels::Graphics::INV_ARROW_FILE, false, 8, 8, 2, Tileset::BlockType::BLOCK2X2);
	std::shared_ptr<TilesetEntry> unused1;
	if (rom.get_address(RomLabels::Graphics::INV_UNUSED1_SIZE) > 0)
	{
		unused1 = TilesetEntry::Create(this, load_bytes(RomLabels::Graphics::INV_UNUSED1, RomLabels::Graphics::INV_UNUSED1_SIZE),
			RomLabels::Graphics::INV_UNUSED1, RomLabels::Graphics::INV_UNUSED1_FILE, false, 8, 11, 2);
	}
	std::shared_ptr<TilesetEntry> unused2;
	if (rom.get_address(RomLabels::Graphics::INV_UNUSED2_SIZE) > 0)
	{
		unused2 = TilesetEntry::Create(this, load_bytes(RomLabels::Graphics::INV_UNUSED2, RomLabels::Graphics::INV_UNUSED2_SIZE),
			RomLabels::Graphics::INV_UNUSED2, RomLabels::Graphics::INV_UNUSED2_FILE, false, 8, 10, 2);
	}
	auto pal1 = PaletteEntry::Create(this, load_pal(RomLabels::Graphics::INV_PAL1, Palette::Type::LOW8),
		RomLabels::Graphics::INV_PAL1, RomLabels::Graphics::INV_PAL1_FILE, Palette::Type::LOW8);
	auto pal2 = PaletteEntry::Create(this, load_pal(RomLabels::Graphics::INV_PAL2, Palette::Type::FULL),
		RomLabels::Graphics::INV_PAL2, RomLabels::Graphics::INV_PAL2_FILE, Palette::Type::FULL);
	m_fonts_by_name.insert({ menu_font->GetName(), menu_font });
	m_fonts_internal.insert({ RomLabels::Graphics::INV_FONT, menu_font });
	m_ui_gfx_by_name.insert({ cursor->GetName(), cursor });
	m_ui_gfx_internal.insert({ RomLabels::Graphics::INV_CURSOR, cursor });
	m_ui_gfx_by_name.insert({ arrow->GetName(), arrow });
	m_ui_gfx_internal.insert({ RomLabels::Graphics::INV_ARROW, arrow });
	if (unused1 != nullptr)
	{
		m_ui_gfx_by_name.insert({ unused1->GetName(), unused1 });
		m_ui_gfx_internal.insert({ RomLabels::Graphics::INV_UNUSED1, unused1 });
	}
	if (unused2 != nullptr)
	{
		m_ui_gfx_by_name.insert({ unused2->GetName(), unused2 });
		m_ui_gfx_internal.insert({ RomLabels::Graphics::INV_UNUSED2, unused2 });
	}
	m_palettes_by_name.insert({ pal1->GetName(), pal1 });
	m_palettes_internal.insert({ RomLabels::Graphics::INV_PAL1, pal1 });
	m_palettes_by_name.insert({ pal2->GetName(), pal2 });
	m_palettes_internal.insert({ RomLabels::Graphics::INV_PAL2, pal2 });
	return true;
}

bool GraphicsData::RomLoadPalettes(const Rom& rom)
{
	auto extract_pals = [&](const ByteVector& bytes, const std::string& name, std::filesystem::path fname, Palette::Type type)
	{
		int idx = 0;
		for (auto it = bytes.cbegin(); it != bytes.cend(); it += Palette::GetSizeBytes(type))
		{
			auto pal_bytes = ByteVector(it, it + Palette::GetSizeBytes(type));
			auto e = PaletteEntry::Create(this, pal_bytes, name + ":" + std::to_string(idx++), fname, type);
			m_palettes_by_name.insert({ e->GetName(), e });
			m_palettes_internal.insert({ e->GetName(), e });
			if (type == Palette::Type::SWORD)
			{
				m_sword_palettes.insert({ e->GetName(), e });
			}
			else if (type == Palette::Type::ARMOUR)
			{
				m_armour_palettes.insert({ e->GetName(), e });
			}
			else
			{
				m_misc_palettes.insert({ e->GetName(), e });
			}
		}
	};
	uint32_t player_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::PLAYER_PAL);
	uint32_t hud_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::HUD_PAL);
	uint32_t item_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::INV_ITEM_PAL);
	uint32_t sword_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SWORD_PAL_SWAPS);
	uint32_t armour_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ARMOUR_PAL_SWAPS);
	uint32_t armour_end_addr = rom.get_section(RomLabels::Graphics::EQUIP_PAL_SECTION).end;
	uint32_t sword_pal_size = armour_addr - sword_addr;
	uint32_t armour_pal_size = armour_end_addr - armour_addr;
	auto player_bytes = rom.read_array<uint8_t>(player_addr, Palette::GetSizeBytes(Palette::Type::FULL));
	auto hud_bytes = rom.read_array<uint8_t>(hud_addr, Palette::GetSizeBytes(Palette::Type::HUD));
	auto item_bytes = rom.read_array<uint8_t>(item_addr, Palette::GetSizeBytes(Palette::Type::FULL));
	auto sword_bytes = rom.read_array<uint8_t>(sword_addr, sword_pal_size);
	auto armour_bytes = rom.read_array<uint8_t>(armour_addr, armour_pal_size);
	auto player_pal = PaletteEntry::Create(this, player_bytes, RomLabels::Graphics::PLAYER_PAL,
		RomLabels::Graphics::PLAYER_PAL_FILE, Palette::Type::FULL);
	auto hud_pal = PaletteEntry::Create(this, hud_bytes, RomLabels::Graphics::HUD_PAL,
		RomLabels::Graphics::HUD_PAL_FILE, Palette::Type::HUD);
	auto item_pal = PaletteEntry::Create(this, item_bytes, RomLabels::Graphics::INV_ITEM_PAL,
		RomLabels::Graphics::INV_ITEM_PAL_FILE, Palette::Type::FULL);
	extract_pals(sword_bytes, RomLabels::Graphics::SWORD_PAL_SWAPS, RomLabels::Graphics::SWORD_PAL_FILE, Palette::Type::SWORD);
	extract_pals(armour_bytes, RomLabels::Graphics::ARMOUR_PAL_SWAPS, RomLabels::Graphics::ARMOUR_PAL_FILE, Palette::Type::ARMOUR);
	m_palettes_by_name.insert({ player_pal->GetName(), player_pal });
	m_palettes_internal.insert({ player_pal->GetName(), player_pal });
	m_palettes_by_name.insert({ hud_pal->GetName(), hud_pal });
	m_palettes_internal.insert({ hud_pal->GetName(), hud_pal });
	m_palettes_by_name.insert({ item_pal->GetName(), item_pal });
	m_palettes_internal.insert({ item_pal->GetName(), item_pal });
	m_misc_palettes.insert({ player_pal->GetName(), player_pal });
	m_misc_palettes.insert({ hud_pal->GetName(), hud_pal });
	m_misc_palettes.insert({ item_pal->GetName(), item_pal });
	return true;
}

bool GraphicsData::RomLoadTextGraphics(const Rom& rom)
{
	uint32_t down_arrow_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::DOWN_ARROW);
	uint32_t down_arrow_size = rom.get_section(RomLabels::Graphics::DOWN_ARROW_SECTION).size();
	uint32_t right_arrow_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::RIGHT_ARROW);
	uint32_t right_arrow_size = rom.get_section(RomLabels::Graphics::RIGHT_ARROW_SECTION).size();
	auto down_arrow_bytes = rom.read_array<uint8_t>(down_arrow_addr, down_arrow_size);
	auto right_arrow_bytes = rom.read_array<uint8_t>(right_arrow_addr, right_arrow_size);
	auto down_arrow = TilesetEntry::Create(this, down_arrow_bytes, RomLabels::Graphics::DOWN_ARROW,
		RomLabels::Graphics::DOWN_ARROW_FILE, false, 8, 8, 4, Tileset::BlockType::BLOCK2X2);
	auto right_arrow = TilesetEntry::Create(this, right_arrow_bytes, RomLabels::Graphics::RIGHT_ARROW,
		RomLabels::Graphics::RIGHT_ARROW_FILE, false, 8, 8, 4, Tileset::BlockType::BLOCK2X2);
	down_arrow->SetStartAddress(down_arrow_size);
	right_arrow->SetStartAddress(right_arrow_size);
	m_ui_gfx_by_name.insert({ down_arrow->GetName(), down_arrow });
	m_ui_gfx_by_name.insert({ right_arrow->GetName(), right_arrow });
	m_ui_gfx_internal.insert({ down_arrow->GetName(), down_arrow });
	m_ui_gfx_internal.insert({ right_arrow->GetName(), right_arrow });
	return true;
}

bool GraphicsData::RomLoadSwordFx(const Rom& rom)
{
	uint32_t inv_tilemap_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::INV_TILEMAP);
	uint32_t magic_sword_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SWORD_MAGIC);
	uint32_t thunder_sword_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SWORD_THUNDER);
	uint32_t gaia_sword_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SWORD_GAIA);
	uint32_t ice_sword_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SWORD_ICE);
	uint32_t coinfall_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::COINFALL);
	uint32_t end = rom.get_section(RomLabels::Graphics::SWORD_FX_SECTION).end;
	
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

	auto inv_tilemap = Tilemap2DEntry::Create(this, inv_tilemap_bytes, RomLabels::Graphics::INV_TILEMAP,
		RomLabels::Graphics::INV_TILEMAP_FILE, Tilemap2D::Compression::LZ77, 0x6B4);
	auto magic_sword = TilesetEntry::Create(this, magic_sword_bytes, RomLabels::Graphics::SWORD_MAGIC,
		RomLabels::Graphics::SWORD_MAGIC_FILE, true, 8, 8, 4, Tileset::BlockType::BLOCK4X6);
	auto thunder_sword = TilesetEntry::Create(this, thunder_sword_bytes, RomLabels::Graphics::SWORD_THUNDER,
		RomLabels::Graphics::SWORD_THUNDER_FILE, true, 8, 8, 4, Tileset::BlockType::BLOCK4X6);
	auto gaia_sword = TilesetEntry::Create(this, gaia_sword_bytes, RomLabels::Graphics::SWORD_GAIA,
		RomLabels::Graphics::SWORD_GAIA_FILE, true, 8, 8, 4, Tileset::BlockType::BLOCK4X4);
	auto ice_sword = TilesetEntry::Create(this, ice_sword_bytes, RomLabels::Graphics::SWORD_ICE,
		RomLabels::Graphics::SWORD_ICE_FILE, true, 8, 8, 4, Tileset::BlockType::BLOCK4X6);
	auto coinfall = TilesetEntry::Create(this, coinfall_bytes, RomLabels::Graphics::COINFALL,
		RomLabels::Graphics::COINFALL_FILE, true, 8, 8, 4, Tileset::BlockType::BLOCK4X4);

	inv_tilemap->SetStartAddress(inv_tilemap_addr);
	magic_sword->SetStartAddress(magic_sword_addr);
	thunder_sword->SetStartAddress(thunder_sword_addr);
	gaia_sword->SetStartAddress(gaia_sword_addr);
	ice_sword->SetStartAddress(ice_sword_addr);
	coinfall->SetStartAddress(coinfall_addr);

	m_ui_tilemaps.insert({ inv_tilemap->GetName(), inv_tilemap });
	m_ui_tilemaps_internal.insert({ inv_tilemap->GetName(), inv_tilemap });
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
	uint32_t poison_ptrs_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::STATUS_FX_POISON);
	uint32_t confusion_ptrs_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::STATUS_FX_CONFUSION);
	uint32_t paralysis_ptrs_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::STATUS_FX_PARALYSIS);
	uint32_t curse_ptrs_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::STATUS_FX_CURSE);
	std::vector<uint32_t> poison_frame_ptrs;
	std::vector<uint32_t> confusion_frame_ptrs;
	std::vector<uint32_t> paralysis_frame_ptrs;
	std::vector<uint32_t> curse_frame_ptrs;
	
	uint32_t ptrs_end = 0xFFFFFF;
	auto get_ptrs = [&](uint32_t addr, uint32_t& end, std::vector<uint32_t>& ptrs)
	{
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
	uint32_t end = rom.get_section(RomLabels::Graphics::STATUS_FX_SECTION).end;
	auto read_frames = [&](const std::vector<uint32_t>& ptrs, const std::string status_name,
		const std::string& frame_name, const std::string& filename)
	{
		int i = 1;
		for (const auto& addr : ptrs)
		{
			std::string name = StrPrintf(frame_name, i);
			std::string fname = StrPrintf(filename, i++);
			auto b = rom.read_array<uint8_t>(addr, end - addr);
			auto e = TilesetEntry::Create(this, b, name, fname, true, 8, 8, 4, Tileset::BlockType::BLOCK4X4);
			e->SetStartAddress(addr);
			m_status_fx_frames.insert({ name, e });
			m_status_fx_frames_internal.insert({ name, e });
			m_status_fx[status_name].push_back(name);
			m_status_fx_internal[status_name].push_back(name);
		}
	};
	read_frames(poison_frame_ptrs, RomLabels::Graphics::STATUS_FX_POISON,
		RomLabels::Graphics::STATUS_FX_POISON_FRAME, RomLabels::Graphics::STATUS_FX_POISON_FILE);
	read_frames(confusion_frame_ptrs, RomLabels::Graphics::STATUS_FX_CONFUSION,
		RomLabels::Graphics::STATUS_FX_CONFUSION_FRAME, RomLabels::Graphics::STATUS_FX_CONFUSION_FILE);
	read_frames(paralysis_frame_ptrs, RomLabels::Graphics::STATUS_FX_PARALYSIS,
		RomLabels::Graphics::STATUS_FX_PARALYSIS_FRAME, RomLabels::Graphics::STATUS_FX_PARALYSIS_FILE);
	read_frames(curse_frame_ptrs, RomLabels::Graphics::STATUS_FX_CURSE,
		RomLabels::Graphics::STATUS_FX_CURSE_FRAME, RomLabels::Graphics::STATUS_FX_CURSE_FILE);
	return true;
}

bool GraphicsData::RomLoadHudData(const Rom& rom)
{
	uint32_t map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::HUD_TILEMAP);
	uint32_t ts_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::HUD_TILESET);
	uint32_t end = rom.get_section(RomLabels::Graphics::HUD_SECTION).end;

	uint32_t map_size = ts_addr - map_addr;
	uint32_t ts_size = end - ts_addr;

	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);
	auto ts_bytes = rom.read_array<uint8_t>(ts_addr, ts_size);

	auto map = Tilemap2DEntry::Create(this, map_bytes, RomLabels::Graphics::HUD_TILEMAP,
		RomLabels::Graphics::HUD_TILEMAP_FILE, Tilemap2D::Compression::NONE, 0x6B4, 40, 3);
	auto ts = TilesetEntry::Create(this, ts_bytes, RomLabels::Graphics::HUD_TILESET,
		RomLabels::Graphics::HUD_TILESET_FILE);

	m_ui_tilemaps.insert({ map->GetName(), map });
	m_ui_tilemaps_internal.insert({ map->GetName(), map });
	m_ui_gfx_by_name.insert({ ts->GetName(), ts });
	m_ui_gfx_internal.insert({ ts->GetName(), ts });

	return true;
}

bool GraphicsData::RomLoadEndCreditData(const Rom& rom)
{
	uint32_t pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::END_CREDITS_PAL);
	uint32_t font_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::END_CREDITS_FONT);
	uint32_t logos_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::END_CREDITS_LOGOS);
	uint32_t map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::END_CREDITS_MAP);
	uint32_t end = rom.get_section(RomLabels::Graphics::END_CREDITS_DATA).end;

	uint32_t pal_size = font_addr - pal_addr;
	uint32_t font_size = logos_addr - font_addr;
	uint32_t logos_size = map_addr - logos_addr;
	uint32_t map_size = end - map_addr;

	auto pal_bytes = rom.read_array<uint8_t>(pal_addr, pal_size);
	auto font_bytes = rom.read_array<uint8_t>(font_addr, font_size);
	auto logos_bytes = rom.read_array<uint8_t>(logos_addr, logos_size);
	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);

	m_end_credits_palette = PaletteEntry::Create(this, pal_bytes, RomLabels::Graphics::END_CREDITS_PAL,
		RomLabels::Graphics::END_CREDITS_PAL_FILE, Palette::Type::END_CREDITS);
	m_end_credits_tileset = TilesetEntry::Create(this, logos_bytes, RomLabels::Graphics::END_CREDITS_LOGOS,
		RomLabels::Graphics::END_CREDITS_LOGOS_FILE);
	m_end_credits_map = Tilemap2DEntry::Create(this, map_bytes, RomLabels::Graphics::END_CREDITS_MAP,
		RomLabels::Graphics::END_CREDITS_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto font = TilesetEntry::Create(this, font_bytes, RomLabels::Graphics::END_CREDITS_FONT,
		RomLabels::Graphics::END_CREDITS_FONT_FILE, true, 8, 8, 2);
	m_fonts_by_name.insert({ font->GetName(), font });
	m_fonts_internal.insert({ RomLabels::Graphics::END_CREDITS_FONT, font });

	return true;
}

bool GraphicsData::RomLoadIslandMapData(const Rom& rom)
{
	uint32_t fg_tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FG_TILES);
	uint32_t fg_map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FG_MAP);
	uint32_t bg_tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_BG_TILES);
	uint32_t bg_map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_BG_MAP);
	uint32_t dots_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_DOTS);
	uint32_t friday_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FRIDAY);
	uint32_t fg_pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FG_PAL);

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

	auto fg_tiles = TilesetEntry::Create(this, fg_tiles_bytes, RomLabels::Graphics::ISLAND_MAP_FG_TILES, RomLabels::Graphics::ISLAND_MAP_FG_TILES_FILE);
	auto fg_map = Tilemap2DEntry::Create(this, fg_map_bytes, RomLabels::Graphics::ISLAND_MAP_FG_MAP, RomLabels::Graphics::ISLAND_MAP_FG_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto bg_tiles = TilesetEntry::Create(this, bg_tiles_bytes, RomLabels::Graphics::ISLAND_MAP_BG_TILES, RomLabels::Graphics::ISLAND_MAP_BG_TILES_FILE);
	auto bg_map = Tilemap2DEntry::Create(this, bg_map_bytes, RomLabels::Graphics::ISLAND_MAP_BG_MAP, RomLabels::Graphics::ISLAND_MAP_BG_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto dots_tiles = TilesetEntry::Create(this, dots_bytes, RomLabels::Graphics::ISLAND_MAP_DOTS, RomLabels::Graphics::ISLAND_MAP_DOTS_FILE);
	auto friday_tiles = TilesetEntry::Create(this, friday_bytes, RomLabels::Graphics::ISLAND_MAP_FRIDAY, RomLabels::Graphics::ISLAND_MAP_FRIDAY_FILE, true, 8, 8, 4, Tileset::BlockType::BLOCK2X2);
	auto fg_pal = PaletteEntry::Create(this, fg_pal_bytes, RomLabels::Graphics::ISLAND_MAP_FG_PAL, RomLabels::Graphics::ISLAND_MAP_FG_PAL_FILE, Palette::Type::FULL);
	auto bg_pal = PaletteEntry::Create(this, bg_pal_bytes, RomLabels::Graphics::ISLAND_MAP_BG_PAL, RomLabels::Graphics::ISLAND_MAP_BG_PAL_FILE, Palette::Type::FULL);

	m_island_map_tiles.insert({ fg_tiles->GetName(), fg_tiles });
	m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FG_TILES, fg_tiles });
	m_island_map_tiles.insert({ bg_tiles->GetName(), bg_tiles });
	m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_BG_TILES, bg_tiles });
	m_island_map_tiles.insert({ dots_tiles->GetName(), dots_tiles });
	m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_DOTS, dots_tiles });
	m_island_map_tiles.insert({ friday_tiles->GetName(), friday_tiles });
	m_island_map_tiles_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FRIDAY, friday_tiles });

	m_island_map_tilemaps.insert({ fg_map->GetName(), fg_map });
	m_island_map_tilemaps_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FG_MAP, fg_map });
	m_island_map_tilemaps.insert({ bg_map->GetName(), bg_map });
	m_island_map_tilemaps_internal.insert({ RomLabels::Graphics::ISLAND_MAP_BG_MAP, bg_map });

	m_island_map_pals.insert({ fg_pal->GetName(), fg_pal });
	m_island_map_pals_internal.insert({ RomLabels::Graphics::ISLAND_MAP_FG_PAL, fg_pal });
	m_island_map_pals.insert({ bg_pal->GetName(), bg_pal });
	m_island_map_pals_internal.insert({ RomLabels::Graphics::ISLAND_MAP_BG_PAL, bg_pal });

	return true;
}

bool GraphicsData::RomLoadLithographData(const Rom& rom)
{
	uint32_t pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::LITHOGRAPH_PAL);
	uint32_t tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::LITHOGRAPH_TILES);
	uint32_t map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::LITHOGRAPH_MAP);
	uint32_t end = rom.get_section(RomLabels::Graphics::LITHOGRAPH_DATA).end;

	uint32_t pal_size = Palette::GetSizeBytes(Palette::Type::FULL);
	uint32_t tiles_size = map_addr - tiles_addr;
	uint32_t map_size = end - map_addr;

	auto pal_bytes = rom.read_array<uint8_t>(pal_addr, pal_size);
	auto tiles_bytes = rom.read_array<uint8_t>(tiles_addr, tiles_size);
	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);

	m_lithograph_palette = PaletteEntry::Create(this, pal_bytes, RomLabels::Graphics::LITHOGRAPH_PAL,
		RomLabels::Graphics::LITHOGRAPH_PAL_FILE, Palette::Type::FULL);
	m_lithograph_tileset = TilesetEntry::Create(this, tiles_bytes, RomLabels::Graphics::LITHOGRAPH_TILES,
		RomLabels::Graphics::LITHOGRAPH_TILES_FILE);
	m_lithograph_map = Tilemap2DEntry::Create(this, map_bytes, RomLabels::Graphics::LITHOGRAPH_MAP,
		RomLabels::Graphics::LITHOGRAPH_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);

	return true;
}

bool GraphicsData::RomLoadTitleScreenData(const Rom& rom)
{
	uint32_t blue_pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_PALETTE_BLUE_LEA);
	uint32_t yellow_pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_PALETTE_YELLOW_LEA);
	uint32_t title_1_tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_1_TILES);
	uint32_t title_2_tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_2_TILES);
	uint32_t title_3_tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_3_TILES);
	uint32_t title_1_map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_1_MAP);
	uint32_t title_2_map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_2_MAP);
	uint32_t title_3_map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_3_MAP);
	uint32_t title_3_pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_3_PAL);
	uint32_t title_3_pal_highlight_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT);

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

	auto blue_pal = PaletteEntry::Create(this, blue_pal_bytes, RomLabels::Graphics::TITLE_PALETTE_BLUE,
		RomLabels::Graphics::TITLE_PALETTE_BLUE_FILE, Palette::Type::TITLE_BLUE_FADE);
	auto yellow_pal = PaletteEntry::Create(this, yellow_pal_bytes, RomLabels::Graphics::TITLE_PALETTE_YELLOW,
		RomLabels::Graphics::TITLE_PALETTE_YELLOW_FILE, Palette::Type::TITLE_YELLOW);
	auto title_1_tiles = TilesetEntry::Create(this, title_1_tiles_bytes, RomLabels::Graphics::TITLE_1_TILES,
		RomLabels::Graphics::TITLE_1_TILES_FILE);
	auto title_2_tiles = TilesetEntry::Create(this, title_2_tiles_bytes, RomLabels::Graphics::TITLE_2_TILES,
		RomLabels::Graphics::TITLE_2_TILES_FILE);
	auto title_3_tiles = TilesetEntry::Create(this, title_3_tiles_bytes, RomLabels::Graphics::TITLE_3_TILES,
		RomLabels::Graphics::TITLE_3_TILES_FILE);
	auto title_1_map = Tilemap2DEntry::Create(this, title_1_map_bytes, RomLabels::Graphics::TITLE_1_MAP,
		RomLabels::Graphics::TITLE_1_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto title_2_map = Tilemap2DEntry::Create(this, title_2_map_bytes, RomLabels::Graphics::TITLE_2_MAP,
		RomLabels::Graphics::TITLE_2_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto title_3_map = Tilemap2DEntry::Create(this, title_3_map_bytes, RomLabels::Graphics::TITLE_3_MAP,
		RomLabels::Graphics::TITLE_3_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);
	auto title_3_pal = PaletteEntry::Create(this, title_3_pal_bytes, RomLabels::Graphics::TITLE_3_PAL,
		RomLabels::Graphics::TITLE_3_PAL_FILE, Palette::Type::FULL);
	auto title_3_pal_highlight = PaletteEntry::Create(this, title_3_pal_highlight_bytes, RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT,
		RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT_FILE, Palette::Type::FULL);

	m_title_tiles.insert({ title_1_tiles->GetName(), title_1_tiles });
	m_title_tiles_internal.insert({ RomLabels::Graphics::TITLE_1_TILES, title_1_tiles });
	m_title_tiles.insert({ title_2_tiles->GetName(), title_2_tiles });
	m_title_tiles_internal.insert({ RomLabels::Graphics::TITLE_2_TILES, title_2_tiles });
	m_title_tiles.insert({ title_3_tiles->GetName(), title_3_tiles });
	m_title_tiles_internal.insert({ RomLabels::Graphics::TITLE_3_TILES, title_3_tiles });

	m_title_tilemaps.insert({ title_1_map->GetName(), title_1_map });
	m_title_tilemaps_internal.insert({ RomLabels::Graphics::TITLE_1_MAP, title_1_map });
	m_title_tilemaps.insert({ title_2_map->GetName(), title_2_map });
	m_title_tilemaps_internal.insert({ RomLabels::Graphics::TITLE_2_MAP, title_2_map });
	m_title_tilemaps.insert({ title_3_map->GetName(), title_3_map });
	m_title_tilemaps_internal.insert({ RomLabels::Graphics::TITLE_3_MAP, title_3_map });

	m_title_pals.insert({ title_3_pal->GetName(), title_3_pal });
	m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_3_PAL, title_3_pal });
	m_title_pals.insert({ title_3_pal_highlight->GetName(), title_3_pal_highlight });
	m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT, title_3_pal_highlight });
	m_title_pals.insert({ yellow_pal->GetName(), yellow_pal });
	m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_PALETTE_YELLOW, yellow_pal });
	m_title_pals.insert({ blue_pal->GetName(), blue_pal });
	m_title_pals_internal.insert({ RomLabels::Graphics::TITLE_PALETTE_BLUE, blue_pal });

	return true;
}

bool GraphicsData::RomLoadSegaLogoData(const Rom& rom)
{
	uint32_t pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SEGA_LOGO_PAL_LEA);
	uint32_t tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::SEGA_LOGO_TILES);
	uint32_t end = rom.get_section(RomLabels::Graphics::SEGA_LOGO_DATA).end;

	uint32_t pal_size = Palette::GetSizeBytes(Palette::Type::SEGA_LOGO);
	uint32_t tiles_size = end - tiles_addr;

	auto pal_bytes = rom.read_array<uint8_t>(pal_addr, pal_size);
	auto tiles_bytes = rom.read_array<uint8_t>(tiles_addr, tiles_size);

	m_sega_logo_palette = PaletteEntry::Create(this, pal_bytes, RomLabels::Graphics::SEGA_LOGO_PAL,
		RomLabels::Graphics::SEGA_LOGO_PAL_FILE, Palette::Type::SEGA_LOGO);
	m_sega_logo_tileset = TilesetEntry::Create(this, tiles_bytes, RomLabels::Graphics::SEGA_LOGO_TILES,
		RomLabels::Graphics::SEGA_LOGO_TILES_FILE);

	return true;
}

bool GraphicsData::RomLoadClimaxLogoData(const Rom& rom)
{
	uint32_t pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::CLIMAX_LOGO_PAL);
	uint32_t tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::CLIMAX_LOGO_TILES);
	uint32_t map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::CLIMAX_LOGO_MAP);

	uint32_t tiles_size = map_addr - tiles_addr;
	uint32_t map_size = pal_addr - map_addr;
	uint32_t pal_size = Palette::GetSizeBytes(Palette::Type::CLIMAX_LOGO);

	auto pal_bytes = rom.read_array<uint8_t>(pal_addr, pal_size);
	auto tiles_bytes = rom.read_array<uint8_t>(tiles_addr, tiles_size);
	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);

	m_climax_logo_palette = PaletteEntry::Create(this, pal_bytes, RomLabels::Graphics::CLIMAX_LOGO_PAL,
		RomLabels::Graphics::CLIMAX_LOGO_PAL_FILE, Palette::Type::CLIMAX_LOGO);
	m_climax_logo_tileset = TilesetEntry::Create(this, tiles_bytes, RomLabels::Graphics::CLIMAX_LOGO_TILES,
		RomLabels::Graphics::CLIMAX_LOGO_TILES_FILE);
	m_climax_logo_map = Tilemap2DEntry::Create(this, map_bytes, RomLabels::Graphics::CLIMAX_LOGO_MAP,
		RomLabels::Graphics::CLIMAX_LOGO_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);

	return true;
}

bool GraphicsData::RomLoadGameLoadScreenData(const Rom& rom)
{
	uint32_t pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::GAME_LOAD_PALETTE_LEA);
	uint32_t player_pal_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE);
	uint32_t chars_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::GAME_LOAD_CHARS);
	uint32_t tiles_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::GAME_LOAD_TILES);
	uint32_t map_addr = Disasm::ReadOffset16(rom, RomLabels::Graphics::GAME_LOAD_MAP);
	uint32_t end = rom.get_section(RomLabels::Graphics::GAME_LOAD_DATA).end;

	uint32_t pal_size = Palette::GetSizeBytes(Palette::Type::FULL);
	uint32_t player_pal_size = Palette::GetSizeBytes(Palette::Type::FULL);
	uint32_t chars_size = tiles_addr - chars_addr;
	uint32_t tiles_size = map_addr - tiles_addr;
	uint32_t map_size = end - map_addr;

	auto pal_bytes = rom.read_array<uint8_t>(pal_addr, pal_size);
	auto player_pal_bytes = rom.read_array<uint8_t>(player_pal_addr, player_pal_size);
	auto chars_bytes = rom.read_array<uint8_t>(chars_addr, chars_size);
	auto tiles_bytes = rom.read_array<uint8_t>(tiles_addr, tiles_size);
	auto map_bytes = rom.read_array<uint8_t>(map_addr, map_size);

	auto pal = PaletteEntry::Create(this, pal_bytes, RomLabels::Graphics::GAME_LOAD_PALETTE,
		RomLabels::Graphics::GAME_LOAD_PALETTE_FILE, Palette::Type::FULL);
	auto player_pal = PaletteEntry::Create(this, player_pal_bytes, RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE,
		RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE_FILE, Palette::Type::FULL);
	auto chars = TilesetEntry::Create(this, chars_bytes, RomLabels::Graphics::GAME_LOAD_CHARS,
		RomLabels::Graphics::GAME_LOAD_CHARS_FILE);
	auto tiles = TilesetEntry::Create(this, tiles_bytes, RomLabels::Graphics::GAME_LOAD_TILES,
		RomLabels::Graphics::GAME_LOAD_TILES_FILE);
	m_load_game_map = Tilemap2DEntry::Create(this, map_bytes, RomLabels::Graphics::GAME_LOAD_MAP,
		RomLabels::Graphics::GAME_LOAD_MAP_FILE, Tilemap2D::Compression::RLE, 0x100);

	m_load_game_pals.insert({ pal->GetName(), pal });
	m_load_game_pals_internal.insert({ RomLabels::Graphics::GAME_LOAD_PALETTE, pal });
	m_load_game_pals.insert({ player_pal->GetName(), player_pal });
	m_load_game_pals_internal.insert({ RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE, player_pal });
	m_load_game_tiles.insert({ chars->GetName(), chars });
	m_load_game_tiles_internal.insert({ RomLabels::Graphics::GAME_LOAD_CHARS, chars });
	m_load_game_tiles.insert({ tiles->GetName(), tiles });
	m_load_game_tiles_internal.insert({ RomLabels::Graphics::GAME_LOAD_TILES, tiles });

	return true;
}

bool GraphicsData::AsmSaveGraphics(const std::filesystem::path& dir)
{
	std::unordered_map<std::string, ByteVector> combined;
	bool retval = std::all_of(m_fonts_by_name.begin(), m_fonts_by_name.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_ui_gfx_by_name.begin(), m_ui_gfx_by_name.end(), [&](auto& f)
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
	retval = retval && std::all_of(m_ui_tilemaps.begin(), m_ui_tilemaps.end(), [&](auto& f)
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
	retval = retval && std::all_of(m_load_game_pals.begin(), m_load_game_pals.end(), [&](auto& f)
		{
			return f.second->Save(dir);
		});
	retval = retval && std::all_of(m_load_game_tiles.begin(), m_load_game_tiles.end(), [&](auto& f)
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
				auto it = combined.find(f.second->GetFilename().string());
				if (it == combined.cend())
				{
					combined.insert({ f.second->GetFilename().string(), *f.second->GetBytes()});
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
	m_lithograph_map->Save(dir);
	m_lithograph_palette->Save(dir);
	m_lithograph_tileset->Save(dir);
	m_sega_logo_palette->Save(dir);
	m_sega_logo_tileset->Save(dir);
	m_climax_logo_map->Save(dir);
	m_climax_logo_palette->Save(dir);
	m_climax_logo_tileset->Save(dir);
	m_load_game_map->Save(dir);
	return retval;
}

bool GraphicsData::AsmSaveInventoryGraphics(const std::filesystem::path& dir)
{

	try
	{
		AsmFile ifile;
		auto write_inc = [&](const std::string& name, const auto& container)
		{
			auto item = container.find(name);
			if (item != container.cend())
			{
				ifile << AsmFile::Label(name) << AsmFile::IncludeFile(item->second->GetFilename(), AsmFile::FileType::BINARY);
			}
		};
		ifile.WriteFileHeader(m_inventory_graphics_filename, "Inventory Graphics Data");
		write_inc(RomLabels::Graphics::INV_FONT, m_fonts_internal);
		ifile << AsmFile::Align(2);
		write_inc(RomLabels::Graphics::INV_CURSOR, m_ui_gfx_internal);
		write_inc(RomLabels::Graphics::INV_ARROW, m_ui_gfx_internal);
		write_inc(RomLabels::Graphics::INV_UNUSED1, m_ui_gfx_internal);
		write_inc(RomLabels::Graphics::INV_UNUSED2, m_ui_gfx_internal);
		write_inc(RomLabels::Graphics::INV_PAL1, m_palettes_internal);
		write_inc(RomLabels::Graphics::INV_PAL2, m_palettes_internal);
		ifile.WriteFile(dir / m_inventory_graphics_filename);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveSwordFx(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_sword_fx_path, "Magic Sword Effects");
		const auto& invmap = m_ui_tilemaps_internal[RomLabels::Graphics::INV_TILEMAP];
		file << AsmFile::Label(invmap->GetName()) << AsmFile::IncludeFile(invmap->GetFilename(), AsmFile::FileType::BINARY);
		for (const auto& t : m_sword_fx)
		{
			file << AsmFile::Label(t.first) << AsmFile::IncludeFile(t.second->GetFilename(), AsmFile::FileType::BINARY);
		}
		file.WriteFile(dir / m_sword_fx_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveStatusFx(const std::filesystem::path& dir)
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
			file << AsmFile::Label(frame.first) << AsmFile::IncludeFile(frame.second->GetFilename(), AsmFile::FileType::BINARY);
		}
		file.WriteFile(dir / m_status_fx_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveEndCreditData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_end_credits_path, "End Credits Data File");
		file << AsmFile::Label(m_end_credits_palette->GetName())
			<< AsmFile::IncludeFile(m_end_credits_palette->GetFilename(), AsmFile::FileType::BINARY);
		const auto& font = m_fonts_internal[RomLabels::Graphics::END_CREDITS_FONT];
		file << AsmFile::Label(font->GetName()) << AsmFile::IncludeFile(font->GetFilename(), AsmFile::FileType::BINARY);
		file << AsmFile::Label(m_end_credits_tileset->GetName())
			<< AsmFile::IncludeFile(m_end_credits_tileset->GetFilename(), AsmFile::FileType::BINARY);
		file << AsmFile::Label(m_end_credits_map->GetName())
			<< AsmFile::IncludeFile(m_end_credits_map->GetFilename(), AsmFile::FileType::BINARY);
		file.WriteFile(dir / m_end_credits_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveIslandMapData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_island_map_path, "Island Map Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::FileType::BINARY);
		};
		write_include(m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_FG_TILES]);
		write_include(m_island_map_tilemaps_internal[RomLabels::Graphics::ISLAND_MAP_FG_MAP]);
		write_include(m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_BG_TILES]);
		write_include(m_island_map_tilemaps_internal[RomLabels::Graphics::ISLAND_MAP_BG_MAP]);
		write_include(m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_DOTS]);
		write_include(m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_FRIDAY]);
		file << AsmFile::Align(2);
		write_include(m_island_map_pals_internal[RomLabels::Graphics::ISLAND_MAP_FG_PAL]);
		write_include(m_island_map_pals_internal[RomLabels::Graphics::ISLAND_MAP_BG_PAL]);

		file.WriteFile(dir / m_island_map_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveLithographData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_lithograph_path, "Lithograph Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::FileType::BINARY);
		};
		write_include(m_lithograph_palette);
		write_include(m_lithograph_tileset);
		write_include(m_lithograph_map);
		file << AsmFile::Align(2);

		file.WriteFile(dir / m_lithograph_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveTitleScreenData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_title_path, "Title Screen Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::FileType::BINARY);
		};
		file << AsmFile::Label(RomLabels::Graphics::TITLE_ROUTINES_1) << AsmFile::IncludeFile(m_title_routines_1_path, AsmFile::FileType::ASSEMBLER);
		write_include(m_title_pals_internal[RomLabels::Graphics::TITLE_PALETTE_BLUE]);
		file << AsmFile::Label(RomLabels::Graphics::TITLE_ROUTINES_2) << AsmFile::IncludeFile(m_title_routines_2_path, AsmFile::FileType::ASSEMBLER);
		write_include(m_title_pals_internal[RomLabels::Graphics::TITLE_PALETTE_YELLOW]);
		file << AsmFile::Label(RomLabels::Graphics::TITLE_ROUTINES_3) << AsmFile::IncludeFile(m_title_routines_3_path, AsmFile::FileType::ASSEMBLER);
		write_include(m_title_tiles_internal[RomLabels::Graphics::TITLE_1_TILES]);
		write_include(m_title_tiles_internal[RomLabels::Graphics::TITLE_2_TILES]);
		write_include(m_title_tiles_internal[RomLabels::Graphics::TITLE_3_TILES]);
		write_include(m_title_tilemaps_internal[RomLabels::Graphics::TITLE_1_MAP]);
		write_include(m_title_tilemaps_internal[RomLabels::Graphics::TITLE_2_MAP]);
		write_include(m_title_tilemaps_internal[RomLabels::Graphics::TITLE_3_MAP]);
		file << AsmFile::Align(2);
		write_include(m_title_pals_internal[RomLabels::Graphics::TITLE_3_PAL]);
		write_include(m_title_pals_internal[RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT]);

		file.WriteFile(dir / m_title_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveSegaLogoData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_sega_logo_path, "Sega Logo Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::FileType::BINARY);
		};
		file << AsmFile::Label(RomLabels::Graphics::SEGA_LOGO_ROUTINES1)
			 << AsmFile::IncludeFile(RomLabels::Graphics::SEGA_LOGO_ROUTINES1_FILE, AsmFile::FileType::ASSEMBLER);
		write_include(m_sega_logo_palette);
		file << AsmFile::Label(RomLabels::Graphics::SEGA_LOGO_ROUTINES2)
			 << AsmFile::IncludeFile(RomLabels::Graphics::SEGA_LOGO_ROUTINES2_FILE, AsmFile::FileType::ASSEMBLER);
		write_include(m_sega_logo_tileset);
		file << AsmFile::Align(2);

		file.WriteFile(dir / m_sega_logo_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveClimaxLogoData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_climax_logo_path, "Climax Logo Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::FileType::BINARY);
		};
		write_include(m_climax_logo_tileset);
		write_include(m_climax_logo_map);
		file << AsmFile::Align(2);
		write_include(m_climax_logo_palette);

		file.WriteFile(dir / m_climax_logo_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::AsmSaveGameLoadData(const std::filesystem::path& dir)
{
	try
	{
		AsmFile file;
		file.WriteFileHeader(m_load_game_path, "Load Game Screen Data File");
		auto write_include = [&](const auto& data)
		{
			file << AsmFile::Label(data->GetName()) << AsmFile::IncludeFile(data->GetFilename(), AsmFile::FileType::BINARY);
		};
		file << AsmFile::Label(RomLabels::Graphics::GAME_LOAD_ROUTINES_1)
			<< AsmFile::IncludeFile(RomLabels::Graphics::GAME_LOAD_ROUTINES_1_FILE, AsmFile::FileType::ASSEMBLER);
		write_include(m_load_game_pals_internal[RomLabels::Graphics::GAME_LOAD_PALETTE]);
		file << AsmFile::Label(RomLabels::Graphics::GAME_LOAD_ROUTINES_2)
			<< AsmFile::IncludeFile(RomLabels::Graphics::GAME_LOAD_ROUTINES_2_FILE, AsmFile::FileType::ASSEMBLER);
		write_include(m_load_game_pals_internal[RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE]);
		write_include(m_load_game_tiles_internal[RomLabels::Graphics::GAME_LOAD_CHARS]);
		write_include(m_load_game_tiles_internal[RomLabels::Graphics::GAME_LOAD_TILES]);
		write_include(m_load_game_map);
		file << AsmFile::Align(2);
		file << AsmFile::Label(RomLabels::Graphics::GAME_LOAD_ROUTINES_3)
			<< AsmFile::IncludeFile(RomLabels::Graphics::GAME_LOAD_ROUTINES_3_FILE, AsmFile::FileType::ASSEMBLER);

		file.WriteFile(dir / m_load_game_path);
		return true;
	}
	catch (const std::exception&)
	{
	}
	return false;
}

bool GraphicsData::RomPrepareInjectInvGraphics(const Rom& rom)
{
	ByteVectorPtr bytes = std::make_shared<ByteVector>();
	uint32_t base = rom.get_section(RomLabels::Graphics::INV_SECTION).begin;
	uint32_t inv_font_addr = 0, inv_cursor_addr = 0, inv_arrow_addr = 0, inv_unused1_addr = 0,
		inv_unused2_addr = 0, inv_pal1_addr = 0, inv_pal2_addr = 0;
	ConstByteVectorPtr inv_font, inv_cursor, inv_arrow, inv_unused1, inv_unused2,
		inv_pal1, inv_pal2;

	if (m_fonts_internal.find(RomLabels::Graphics::INV_FONT) != m_fonts_internal.cend())
	{
		inv_font_addr = base + bytes->size();
		inv_font = m_fonts_internal[RomLabels::Graphics::INV_FONT]->GetBytes();
		bytes->insert(bytes->end(), inv_font->cbegin(), inv_font->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_FONT, inv_font_addr));
		if (bytes->size() & 1)
		{
			bytes->push_back(0xFF);
		}
	}

	if (m_ui_gfx_internal.find(RomLabels::Graphics::INV_CURSOR) != m_ui_gfx_internal.cend())
	{
		inv_cursor_addr = base + bytes->size();
		inv_cursor = m_ui_gfx_internal[RomLabels::Graphics::INV_CURSOR]->GetBytes();
		bytes->insert(bytes->end(), inv_cursor->cbegin(), inv_cursor->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_CURSOR, inv_cursor_addr));
	}

	if (m_ui_gfx_internal.find(RomLabels::Graphics::INV_ARROW) != m_ui_gfx_internal.cend())
	{
		inv_arrow_addr = base + bytes->size();
		inv_arrow = m_ui_gfx_internal[RomLabels::Graphics::INV_ARROW]->GetBytes();
		bytes->insert(bytes->end(), inv_arrow->cbegin(), inv_arrow->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_ARROW, inv_arrow_addr));
	}

	if (m_ui_gfx_internal.find(RomLabels::Graphics::INV_UNUSED1) != m_ui_gfx_internal.cend())
	{
		inv_unused1_addr = base + bytes->size();
		inv_unused1 = m_ui_gfx_internal[RomLabels::Graphics::INV_UNUSED1]->GetBytes();
		bytes->insert(bytes->end(), inv_unused1->cbegin(), inv_unused1->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_UNUSED1, inv_unused1_addr));
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_UNUSED1_PLUS6, inv_unused1_addr+12));
	}

	if (m_ui_gfx_internal.find(RomLabels::Graphics::INV_UNUSED2) != m_ui_gfx_internal.cend())
	{
		inv_unused2_addr = base + bytes->size();
		inv_unused2 = m_ui_gfx_internal[RomLabels::Graphics::INV_UNUSED2]->GetBytes();
		bytes->insert(bytes->end(), inv_unused2->cbegin(), inv_unused2->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_UNUSED2, inv_unused2_addr));
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_UNUSED2_PLUS4, inv_unused2_addr+8));
	}

	if (m_palettes_internal.find(RomLabels::Graphics::INV_PAL1) != m_palettes_internal.cend())
	{
		inv_pal1_addr = base + bytes->size();
		inv_pal1 = m_palettes_internal[RomLabels::Graphics::INV_PAL1]->GetBytes();
		bytes->insert(bytes->end(), inv_pal1->cbegin(), inv_pal1->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_PAL1, inv_pal1_addr));
	}

	if (m_palettes_internal.find(RomLabels::Graphics::INV_PAL2) != m_palettes_internal.cend())
	{
		inv_pal2_addr = base + bytes->size();
		inv_pal2 = m_palettes_internal[RomLabels::Graphics::INV_PAL2]->GetBytes();
		bytes->insert(bytes->end(), inv_pal2->cbegin(), inv_pal2->cend());
		m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_PAL2, inv_pal2_addr));
	}

	m_pending_writes.push_back({ RomLabels::Graphics::INV_SECTION, bytes });

	return true;
}

bool GraphicsData::RomPrepareInjectPalettes(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::MISC_PAL_SECTION).begin;
	uint32_t player_begin = begin;
	auto misc_pal_bytes = std::make_shared<ByteVector>();
	auto player_pal = m_palettes_internal[RomLabels::Graphics::PLAYER_PAL]->GetBytes();
	misc_pal_bytes->insert(misc_pal_bytes->end(), player_pal->cbegin(), player_pal->cend());

	uint32_t hud_begin = begin + misc_pal_bytes->size();
	auto hud_pal = m_palettes_internal[RomLabels::Graphics::HUD_PAL]->GetBytes();
	misc_pal_bytes->insert(misc_pal_bytes->end(), hud_pal->cbegin(), hud_pal->cend());

	uint32_t item_pal_begin = rom.get_section(RomLabels::Graphics::INV_ITEM_PAL_SECTION).begin;
	auto item_pal = m_palettes_internal[RomLabels::Graphics::INV_ITEM_PAL]->GetBytes();

	uint32_t equip_pal_begin = rom.get_section(RomLabels::Graphics::EQUIP_PAL_SECTION).begin;
	auto sword_pal_bytes = std::make_shared<ByteVector>();
	auto armour_pal_bytes = std::make_shared<ByteVector>();
	for (const auto& p : m_palettes_internal)
	{
		if (p.first.find(RomLabels::Graphics::SWORD_PAL_SWAPS) != std::string::npos)
		{
			auto bytes = p.second->GetBytes();
			sword_pal_bytes->insert(sword_pal_bytes->end(), bytes->cbegin(), bytes->cend());
		}
		else if (p.first.find(RomLabels::Graphics::ARMOUR_PAL_SWAPS) != std::string::npos)
		{
			auto bytes = p.second->GetBytes();
			armour_pal_bytes->insert(armour_pal_bytes->end(), bytes->cbegin(), bytes->cend());
		}
	}
	uint32_t armour_pal_begin = equip_pal_begin + sword_pal_bytes->size();
	sword_pal_bytes->insert(sword_pal_bytes->end(), armour_pal_bytes->cbegin(), armour_pal_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::MISC_PAL_SECTION, misc_pal_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::PLAYER_PAL, player_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::HUD_PAL, hud_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::HUD_PAL_LEA2, hud_begin));
	m_pending_writes.push_back({ RomLabels::Graphics::INV_ITEM_PAL_SECTION, item_pal });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_ITEM_PAL, item_pal_begin));
	m_pending_writes.push_back({ RomLabels::Graphics::EQUIP_PAL_SECTION, sword_pal_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_PAL_SWAPS, equip_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ARMOUR_PAL_SWAPS, armour_pal_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectTextGraphics(const Rom& rom)
{
	uint32_t down_arrow_begin = rom.get_section(RomLabels::Graphics::DOWN_ARROW_SECTION).begin;
	auto down_arrow_bytes = m_ui_gfx_internal[RomLabels::Graphics::DOWN_ARROW]->GetBytes();
	uint32_t right_arrow_begin = rom.get_section(RomLabels::Graphics::RIGHT_ARROW_SECTION).begin;
	auto right_arrow_bytes = m_ui_gfx_internal[RomLabels::Graphics::RIGHT_ARROW]->GetBytes();

	m_pending_writes.push_back({ RomLabels::Graphics::DOWN_ARROW_SECTION, down_arrow_bytes });
	m_pending_writes.push_back({ RomLabels::Graphics::RIGHT_ARROW_SECTION, right_arrow_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::DOWN_ARROW, down_arrow_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::RIGHT_ARROW, right_arrow_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectSwordFx(const Rom& rom)
{
	uint32_t sword_fx_begin = rom.get_section(RomLabels::Graphics::SWORD_FX_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t inv_tilemap_begin = sword_fx_begin;
	auto inv_tilemap_bytes = m_ui_tilemaps_internal[RomLabels::Graphics::INV_TILEMAP]->GetBytes();
	bytes->insert(bytes->end(), inv_tilemap_bytes->cbegin(), inv_tilemap_bytes->cend());

	uint32_t magic_sword_begin = sword_fx_begin + bytes->size();
	auto magic_sword_bytes = m_sword_fx_internal[RomLabels::Graphics::SWORD_MAGIC]->GetBytes();
	bytes->insert(bytes->end(), magic_sword_bytes->cbegin(), magic_sword_bytes->cend());

	uint32_t thunder_sword_begin = sword_fx_begin + bytes->size();
	auto thunder_sword_bytes = m_sword_fx_internal[RomLabels::Graphics::SWORD_THUNDER]->GetBytes();
	bytes->insert(bytes->end(), thunder_sword_bytes->cbegin(), thunder_sword_bytes->cend());

	uint32_t gaia_sword_begin = sword_fx_begin + bytes->size();
	auto gaia_sword_bytes = m_sword_fx_internal[RomLabels::Graphics::SWORD_GAIA]->GetBytes();
	bytes->insert(bytes->end(), gaia_sword_bytes->cbegin(), gaia_sword_bytes->cend());

	uint32_t ice_sword_begin = sword_fx_begin + bytes->size();
	auto ice_sword_bytes = m_sword_fx_internal[RomLabels::Graphics::SWORD_ICE]->GetBytes();
	bytes->insert(bytes->end(), ice_sword_bytes->cbegin(), ice_sword_bytes->cend());

	uint32_t coinfall_begin = sword_fx_begin + bytes->size();
	auto coinfall_bytes = m_sword_fx_internal[RomLabels::Graphics::COINFALL]->GetBytes();
	bytes->insert(bytes->end(), coinfall_bytes->cbegin(), coinfall_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::SWORD_FX_SECTION, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::INV_TILEMAP, inv_tilemap_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_MAGIC, magic_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_MAGIC_LEA, magic_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_THUNDER, thunder_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_THUNDER_LEA, thunder_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_GAIA, gaia_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_ICE, ice_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SWORD_ICE_LEA, ice_sword_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::COINFALL, coinfall_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectStatusFx(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::STATUS_FX_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();
	uint32_t pointer_table_size = 0;
	std::unordered_map<std::string, uint32_t> pointers;

	uint32_t poison_begin = begin;
	pointer_table_size += m_status_fx[RomLabels::Graphics::STATUS_FX_POISON].size() * sizeof(uint32_t);
	uint32_t confusion_begin = begin + pointer_table_size;
	pointer_table_size += m_status_fx[RomLabels::Graphics::STATUS_FX_CONFUSION].size() * sizeof(uint32_t);
	uint32_t paralysis_begin = begin + pointer_table_size;
	pointer_table_size += m_status_fx[RomLabels::Graphics::STATUS_FX_PARALYSIS].size() * sizeof(uint32_t);
	uint32_t curse_begin = begin + pointer_table_size;
	pointer_table_size += m_status_fx[RomLabels::Graphics::STATUS_FX_CURSE].size() * sizeof(uint32_t);

	bytes->resize(pointer_table_size);
	for (const auto& f : m_status_fx_frames)
	{
		auto fb = f.second->GetBytes();
		pointers[f.first] = bytes->size() + begin;
		bytes->insert(bytes->end(), fb->cbegin(), fb->cend());
	}
	auto it = bytes->begin();
	for (const auto& s : { RomLabels::Graphics::STATUS_FX_POISON,    RomLabels::Graphics::STATUS_FX_CONFUSION,
		                   RomLabels::Graphics::STATUS_FX_PARALYSIS, RomLabels::Graphics::STATUS_FX_CURSE })
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

	m_pending_writes.push_back({ RomLabels::Graphics::STATUS_FX_SECTION, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::STATUS_FX_POISON, poison_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::STATUS_FX_CONFUSION, confusion_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::STATUS_FX_PARALYSIS, paralysis_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::STATUS_FX_CURSE, curse_begin));
	return true;
}

bool GraphicsData::RomPrepareInjectHudData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::HUD_SECTION).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t map_begin = begin;
	auto map_bytes = m_ui_tilemaps_internal[RomLabels::Graphics::HUD_TILEMAP]->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	uint32_t ts_begin = begin + bytes->size();
	auto ts_bytes = m_ui_gfx_internal[RomLabels::Graphics::HUD_TILESET]->GetBytes();
	bytes->insert(bytes->end(), ts_bytes->cbegin(), ts_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::HUD_SECTION, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::HUD_TILEMAP, map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::HUD_TILESET, ts_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectEndCreditData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::END_CREDITS_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t pal_begin = begin;
	auto pal_bytes = m_end_credits_palette->GetBytes();
	bytes->insert(bytes->end(), pal_bytes->cbegin(), pal_bytes->cend());

	uint32_t font_begin = begin + bytes->size();
	auto font_bytes = m_fonts_internal[RomLabels::Graphics::END_CREDITS_FONT]->GetBytes();
	bytes->insert(bytes->end(), font_bytes->cbegin(), font_bytes->cend());

	uint32_t logos_begin = begin + bytes->size();
	auto logos_bytes = m_end_credits_tileset->GetBytes();
	bytes->insert(bytes->end(), logos_bytes->cbegin(), logos_bytes->cend());

	uint32_t map_begin = begin + bytes->size();
	auto map_bytes = m_end_credits_map->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::END_CREDITS_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::END_CREDITS_PAL, pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::END_CREDITS_FONT, font_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::END_CREDITS_LOGOS, logos_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::END_CREDITS_MAP, map_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectIslandMapData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::ISLAND_MAP_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t fg_tiles_begin = begin;
	auto fg_tiles_bytes = m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_FG_TILES]->GetBytes();
	bytes->insert(bytes->end(), fg_tiles_bytes->cbegin(), fg_tiles_bytes->cend());

	uint32_t fg_map_begin = begin + bytes->size();
	auto fg_map_bytes = m_island_map_tilemaps[RomLabels::Graphics::ISLAND_MAP_FG_MAP]->GetBytes();
	bytes->insert(bytes->end(), fg_map_bytes->cbegin(), fg_map_bytes->cend());

	uint32_t bg_tiles_begin = begin + bytes->size();
	auto bg_tiles_bytes = m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_BG_TILES]->GetBytes();
	bytes->insert(bytes->end(), bg_tiles_bytes->cbegin(), bg_tiles_bytes->cend());

	uint32_t bg_map_begin = begin + bytes->size();
	auto bg_map_bytes = m_island_map_tilemaps[RomLabels::Graphics::ISLAND_MAP_BG_MAP]->GetBytes();
	bytes->insert(bytes->end(), bg_map_bytes->cbegin(), bg_map_bytes->cend());

	uint32_t dots_begin = begin + bytes->size();
	auto dots_bytes = m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_DOTS]->GetBytes();
	bytes->insert(bytes->end(), dots_bytes->cbegin(), dots_bytes->cend());

	uint32_t friday_begin = begin + bytes->size();
	auto friday_bytes = m_island_map_tiles_internal[RomLabels::Graphics::ISLAND_MAP_FRIDAY]->GetBytes();
	bytes->insert(bytes->end(), friday_bytes->cbegin(), friday_bytes->cend());
	
	if (((bytes->size() + begin) & 1) == 1)
	{
		bytes->push_back(0xFF);
	}

	uint32_t pal_begin = begin + bytes->size();
	auto fg_pal_bytes = m_island_map_pals_internal[RomLabels::Graphics::ISLAND_MAP_FG_PAL]->GetBytes();
	auto bg_pal_bytes = m_island_map_pals_internal[RomLabels::Graphics::ISLAND_MAP_BG_PAL]->GetBytes();
	bytes->insert(bytes->end(), fg_pal_bytes->cbegin(), fg_pal_bytes->cend());
	bytes->insert(bytes->end(), bg_pal_bytes->cbegin(), bg_pal_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::ISLAND_MAP_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FG_TILES, fg_tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FG_MAP, fg_map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_BG_TILES, bg_tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_BG_MAP, bg_map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_DOTS, dots_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FRIDAY, friday_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::ISLAND_MAP_FG_PAL, pal_begin));
	
	return true;
}

bool GraphicsData::RomPrepareInjectLithographData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::LITHOGRAPH_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t pal_begin = begin;
	auto pal_bytes = m_lithograph_palette->GetBytes();
	bytes->insert(bytes->end(), pal_bytes->cbegin(), pal_bytes->cend());

	uint32_t tiles_begin = begin + bytes->size();
	auto tiles_bytes = m_lithograph_tileset->GetBytes();
	bytes->insert(bytes->end(), tiles_bytes->cbegin(), tiles_bytes->cend());

	uint32_t map_begin = begin + bytes->size();
	auto map_bytes = m_lithograph_map->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::LITHOGRAPH_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::LITHOGRAPH_PAL, pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::LITHOGRAPH_TILES, tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::LITHOGRAPH_MAP, map_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectTitleScreenData(const Rom& rom)
{
	uint32_t blue_pal_begin = rom.get_section(RomLabels::Graphics::TITLE_PALETTE_BLUE).begin;
	auto blue_pal = m_title_pals_internal[RomLabels::Graphics::TITLE_PALETTE_BLUE]->GetBytes();

	uint32_t yellow_pal_begin = rom.get_section(RomLabels::Graphics::TITLE_PALETTE_YELLOW).begin;
	auto yellow_pal = m_title_pals_internal[RomLabels::Graphics::TITLE_PALETTE_YELLOW]->GetBytes();

	uint32_t begin = rom.get_section(RomLabels::Graphics::TITLE_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t title_1_tiles_begin = begin;
	auto title_1_tiles_bytes = m_title_tiles_internal[RomLabels::Graphics::TITLE_1_TILES]->GetBytes();
	bytes->insert(bytes->end(), title_1_tiles_bytes->cbegin(), title_1_tiles_bytes->cend());

	uint32_t title_2_tiles_begin = begin + bytes->size();
	auto title_2_tiles_bytes = m_title_tiles_internal[RomLabels::Graphics::TITLE_2_TILES]->GetBytes();
	bytes->insert(bytes->end(), title_2_tiles_bytes->cbegin(), title_2_tiles_bytes->cend());

	uint32_t title_3_tiles_begin = begin + bytes->size();
	auto title_3_tiles_bytes = m_title_tiles_internal[RomLabels::Graphics::TITLE_3_TILES]->GetBytes();
	bytes->insert(bytes->end(), title_3_tiles_bytes->cbegin(), title_3_tiles_bytes->cend());

	uint32_t title_1_map_begin = begin + bytes->size();
	auto title_1_map_bytes = m_title_tilemaps_internal[RomLabels::Graphics::TITLE_1_MAP]->GetBytes();
	bytes->insert(bytes->end(), title_1_map_bytes->cbegin(), title_1_map_bytes->cend());

	uint32_t title_2_map_begin = begin + bytes->size();
	auto title_2_map_bytes = m_title_tilemaps_internal[RomLabels::Graphics::TITLE_2_MAP]->GetBytes();
	bytes->insert(bytes->end(), title_2_map_bytes->cbegin(), title_2_map_bytes->cend());

	uint32_t title_3_map_begin = begin + bytes->size();
	auto title_3_map_bytes = m_title_tilemaps_internal[RomLabels::Graphics::TITLE_3_MAP]->GetBytes();
	bytes->insert(bytes->end(), title_3_map_bytes->cbegin(), title_3_map_bytes->cend());

	if (((bytes->size() + begin) & 1) == 1)
	{
		bytes->push_back(0xFF);
	}

	uint32_t title_3_pal_begin = begin + bytes->size();
	auto title_3_pal_bytes = m_title_pals_internal[RomLabels::Graphics::TITLE_3_PAL]->GetBytes();
	bytes->insert(bytes->end(), title_3_pal_bytes->cbegin(), title_3_pal_bytes->cend());

	uint32_t title_3_pal_highlight_begin = begin + bytes->size();
	auto title_3_pal_highlight_bytes = m_title_pals_internal[RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT]->GetBytes();
	bytes->insert(bytes->end(), title_3_pal_highlight_bytes->cbegin(), title_3_pal_highlight_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::TITLE_PALETTE_BLUE, blue_pal });
	m_pending_writes.push_back({ RomLabels::Graphics::TITLE_PALETTE_YELLOW, yellow_pal });
	m_pending_writes.push_back({ RomLabels::Graphics::TITLE_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_PALETTE_BLUE_LEA, blue_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_PALETTE_YELLOW_LEA, yellow_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_1_TILES, title_1_tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_2_TILES, title_2_tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_TILES, title_3_tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_1_MAP, title_1_map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_2_MAP, title_2_map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_MAP, title_3_map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_PAL, title_3_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_PAL_LEA2, title_3_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_PAL_LEA3, title_3_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_PAL_LEA4, title_3_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::TITLE_3_PAL_HIGHLIGHT, title_3_pal_highlight_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectSegaLogoData(const Rom& rom)
{
	uint32_t pal_begin = rom.get_section(RomLabels::Graphics::SEGA_LOGO_PAL).begin;
	auto pal_bytes = m_sega_logo_palette->GetBytes();
	
	uint32_t tiles_begin = rom.get_section(RomLabels::Graphics::SEGA_LOGO_DATA).begin;
	auto tiles_bytes = m_sega_logo_tileset->GetBytes();

	m_pending_writes.push_back({ RomLabels::Graphics::SEGA_LOGO_PAL, pal_bytes });
	m_pending_writes.push_back({ RomLabels::Graphics::SEGA_LOGO_DATA, tiles_bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SEGA_LOGO_PAL_LEA, pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::SEGA_LOGO_TILES, tiles_begin));
	
	return true;
}

bool GraphicsData::RomPrepareInjectClimaxLogoData(const Rom& rom)
{
	uint32_t begin = rom.get_section(RomLabels::Graphics::CLIMAX_LOGO_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t tiles_begin = begin;
	auto tiles_bytes = m_climax_logo_tileset->GetBytes();
	bytes->insert(bytes->end(), tiles_bytes->cbegin(), tiles_bytes->cend());

	uint32_t map_begin = begin + bytes->size();
	auto map_bytes = m_climax_logo_map->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	if (((bytes->size() + begin) & 1) == 1)
	{
		bytes->push_back(0xFF);
	}

	uint32_t pal_begin = begin + bytes->size();
	auto pal_bytes = m_climax_logo_palette->GetBytes();
	bytes->insert(bytes->end(), pal_bytes->cbegin(), pal_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::CLIMAX_LOGO_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::CLIMAX_LOGO_TILES, tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::CLIMAX_LOGO_MAP, map_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::CLIMAX_LOGO_PAL, pal_begin));

	return true;
}

bool GraphicsData::RomPrepareInjectGameLoadScreenData(const Rom& rom)
{
	auto pal_bytes = m_load_game_pals_internal[RomLabels::Graphics::GAME_LOAD_PALETTE]->GetBytes();

	uint32_t begin = rom.get_section(RomLabels::Graphics::GAME_LOAD_DATA).begin;
	auto bytes = std::make_shared<ByteVector>();

	uint32_t player_pal_begin = begin;
	auto player_pal_bytes = m_load_game_pals_internal[RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE]->GetBytes();
	bytes->insert(bytes->end(), player_pal_bytes->cbegin(), player_pal_bytes->cend());

	uint32_t chars_begin = begin + bytes->size();
	auto chars_bytes = m_load_game_tiles_internal[RomLabels::Graphics::GAME_LOAD_CHARS]->GetBytes();
	bytes->insert(bytes->end(), chars_bytes->cbegin(), chars_bytes->cend());

	uint32_t tiles_begin = begin + bytes->size();
	auto tiles_bytes = m_load_game_tiles_internal[RomLabels::Graphics::GAME_LOAD_TILES]->GetBytes();
	bytes->insert(bytes->end(), tiles_bytes->cbegin(), tiles_bytes->cend());

	uint32_t map_begin = begin + bytes->size();
	auto map_bytes = m_load_game_map->GetBytes();
	bytes->insert(bytes->end(), map_bytes->cbegin(), map_bytes->cend());

	m_pending_writes.push_back({ RomLabels::Graphics::GAME_LOAD_PALETTE, pal_bytes });
	m_pending_writes.push_back({ RomLabels::Graphics::GAME_LOAD_DATA, bytes });
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::GAME_LOAD_PLAYER_PALETTE, player_pal_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::GAME_LOAD_CHARS, chars_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::GAME_LOAD_TILES, tiles_begin));
	m_pending_writes.push_back(Asm::WriteOffset16(rom, RomLabels::Graphics::GAME_LOAD_MAP, map_begin));

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
	std::vector<std::string> loadgame_palettes;
	for (const auto& p : m_load_game_pals)
	{
		loadgame_palettes.push_back(p.first);
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
	set_pals(m_ui_gfx_by_name, palettes);
	set_pals(m_status_fx_frames, palettes);
	set_pals(m_sword_fx, palettes);
	set_pals(m_island_map_tiles, map_palettes);
	set_pals(m_title_tiles, title_palettes);
	set_pals(m_load_game_tiles, loadgame_palettes);
	m_end_credits_tileset->SetAllPalettes(palettes);
	m_end_credits_tileset->SetRecommendedPalettes({ m_end_credits_palette->GetName() });
	m_lithograph_tileset->SetAllPalettes(palettes);
	m_lithograph_tileset->SetRecommendedPalettes({ m_lithograph_palette->GetName() });
	m_sega_logo_tileset->SetAllPalettes(palettes);
	m_sega_logo_tileset->SetRecommendedPalettes({ m_sega_logo_palette->GetName() });
	m_climax_logo_tileset->SetAllPalettes(palettes);
	m_climax_logo_tileset->SetRecommendedPalettes({ m_climax_logo_palette->GetName() });
}

void GraphicsData::ResetTilesetDefaultPalettes()
{
	auto set_pal = [&](auto& pal)
	{
		if (pal->GetRecommendedPalettes().size() == 0)
		{
			pal->SetDefaultPalette(m_palettes_by_name.begin()->second->GetName());
		}
		else
		{
			pal->SetDefaultPalette(pal->GetRecommendedPalettes().front());
		}
	};
	auto set_pals = [&](auto& container)
	{
		for (const auto& e : container)
		{
			set_pal(e.second);
		}
	};
	set_pals(m_fonts_by_name);
	set_pals(m_ui_gfx_by_name);
	set_pals(m_status_fx_frames);
	set_pals(m_sword_fx);
	set_pals(m_island_map_tiles);
	set_pals(m_title_tiles);
	set_pals(m_load_game_tiles);
	set_pal(m_end_credits_tileset);
	set_pal(m_lithograph_tileset);
	set_pal(m_sega_logo_tileset);
	set_pal(m_climax_logo_tileset);

	m_end_credits_tileset->SetDefaultPalette(m_end_credits_palette->GetName());
}
