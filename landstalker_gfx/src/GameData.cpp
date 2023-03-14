#include "GameData.h"
#include <algorithm>

GameData::GameData(const filesystem::path& asm_file)
	: DataManager(asm_file),
	  m_rd(std::make_shared<RoomData>(asm_file)),
	  m_gd(std::make_shared<GraphicsData>(asm_file))
{
	m_data.push_back(m_rd);
	m_data.push_back(m_gd);
}

GameData::GameData(const Rom& rom)
	: DataManager(rom),
	  m_rd(std::make_shared<RoomData>(rom)),
	  m_gd(std::make_shared<GraphicsData>(rom))
{
	m_data.push_back(m_rd);
	m_data.push_back(m_gd);
}

bool GameData::Save(const filesystem::path& dir)
{
	return std::all_of(m_data.begin(), m_data.end(), [&](auto& d)
		{
			return d->Save(dir);
		});
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
