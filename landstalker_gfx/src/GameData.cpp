#include "GameData.h"
#include <algorithm>

GameData::GameData(const filesystem::path& asm_file)
	: DataManager(asm_file),
	  m_rd(std::make_shared<RoomData>(asm_file)),
	  m_gd(std::make_shared<GraphicsData>(asm_file))
{
	m_data.push_back(m_rd);
	m_data.push_back(m_gd);
	CacheData();
}

GameData::GameData(const Rom& rom)
	: DataManager(rom),
	  m_rd(std::make_shared<RoomData>(rom)),
	  m_gd(std::make_shared<GraphicsData>(rom))
{
	m_data.push_back(m_rd);
	m_data.push_back(m_gd);
	CacheData();
}

bool GameData::Save(const filesystem::path& dir)
{
	auto success = std::all_of(m_data.begin(), m_data.end(), [&](auto& d)
		{
			return d->Save(dir);
		});
	return success;
}

bool GameData::Save()
{
	return std::all_of(m_data.begin(), m_data.end(), [](auto& d)
		{
			return d->Save();
		});
}

PendingWrites GameData::GetPendingWrites() const
{
	PendingWrites retval;
	std::for_each(m_data.begin(), m_data.end(), [&](auto& d)
		{
			auto w = d->GetPendingWrites();
			retval.insert(retval.end(), w.cbegin(), w.cend());
		});
	return retval;
}

bool GameData::WillFitInRom(const Rom& rom) const
{
	return std::all_of(m_data.begin(), m_data.end(), [&](auto& d)
		{
			return d->WillFitInRom(rom);
		});
}

bool GameData::HasBeenModified() const
{
	return std::any_of(m_data.begin(), m_data.end(), [](auto& d)
		{
			return d->HasBeenModified();
		});
}

bool GameData::InjectIntoRom(Rom& rom)
{
	return std::all_of(m_data.begin(), m_data.end(), [&](auto& d)
		{
			return d->InjectIntoRom(rom);
		});
}

void GameData::RefreshPendingWrites(const Rom& rom)
{
	std::for_each(m_data.begin(), m_data.end(), [&](auto& d)
		{
			d->RefreshPendingWrites(rom);
		});
}

const std::map<std::string, std::shared_ptr<PaletteEntry>>& GameData::GetAllPalettes() const
{
	return m_palettes;
}

const std::map<std::string, std::shared_ptr<TilesetEntry>>& GameData::GetAllTilesets() const
{
	return m_tilesets;
}

const std::map<std::string, std::shared_ptr<AnimatedTilesetEntry>>& GameData::GetAllAnimatedTilesets() const
{
	return m_anim_tilesets;
}

std::shared_ptr<PaletteEntry> GameData::GetPalette(const std::string& name) const
{
	return m_palettes.at(name);
}

std::shared_ptr<TilesetEntry> GameData::GetTileset(const std::string& name) const
{
	return m_tilesets.at(name);
}

std::shared_ptr<AnimatedTilesetEntry> GameData::GetAnimatedTileset(const std::string& name) const
{
	return m_anim_tilesets.at(name);
}

void GameData::CacheData()
{
	auto room_pals = m_rd->GetAllPalettes();
	m_palettes.insert(room_pals.cbegin(), room_pals.cend());
	auto gfx_pals = m_gd->GetAllPalettes();
	m_palettes.insert(gfx_pals.cbegin(), gfx_pals.cend());

	auto room_ts = m_rd->GetAllTilesets();
	m_tilesets.insert(room_ts.cbegin(), room_ts.cend());
	auto gfx_ts = m_gd->GetAllTilesets();
	m_tilesets.insert(gfx_ts.cbegin(), gfx_ts.cend());

	auto anim_ts = m_rd->GetAllAnimatedTilesets();
	m_anim_tilesets.insert(anim_ts.cbegin(), anim_ts.cend());
}
