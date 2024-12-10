#include <landstalker/main/include/DataManager.h>

PendingWrites DataManager::GetPendingWrites() const
{
	return m_pending_writes;
}

bool DataManager::WillFitInRom(const Rom& rom) const
{
	for (const auto& w : m_pending_writes)
	{
		if (rom.section_exists(w.first))
		{
			auto sec = rom.get_section(w.first);
			if (w.second->size() > sec.size())
			{
				return false;
			}
		}
		else if (rom.address_exists(w.first))
		{
			if (w.second->size() > sizeof(uint32_t))
			{
				return false;
			}
		}
		else
		{
			throw std::runtime_error("Section \"" + w.first + "\" does not exist!");
		}
	}

	return true;
}

bool DataManager::HasBeenModified() const
{
	return false;
}

bool DataManager::InjectIntoRom(Rom& rom)
{
	for (const auto& w : m_pending_writes)
	{
		uint32_t addr;
		if (rom.section_exists(w.first))
		{
			addr = rom.get_section(w.first).begin;
		}
		else if (rom.address_exists(w.first))
		{
			addr = rom.get_address(w.first);
		}
		else
		{
			throw std::runtime_error("Section \"" + w.first + "\" does not exist!");
		}
		rom.write_array(*w.second, addr);
		Debug(StrPrintf("ROM WRITE @ 0x%06X - %06d bytes (finish 0x%06X)", addr, w.second->size(), addr + w.second->size()));
	}
	CommitAllChanges();
	m_pending_writes.clear();
	return true;
}

bool DataManager::AbandomRomInjection()
{
	m_pending_writes.clear();
	return true;
}

bool DataManager::Save(const std::filesystem::path& /*dir*/)
{
	bool success = true;
	if (success)
	{
		CommitAllChanges();
	}
	return true;
}

bool DataManager::Save()
{
	if (m_base_path.empty())
	{
		return false;
	}
	return Save(m_base_path);
}

void DataManager::RefreshPendingWrites(const Rom& /*rom*/)
{
	m_pending_writes.clear();
}

void DataManager::CommitAllChanges()
{
}

bool DataManager::GetFilenameFromAsm(AsmFile& file, const std::string& label, std::filesystem::path& path)
{
	if (file.Goto(label) == false)
	{
		throw std::runtime_error("Could not locate label \"" + label + "\" in file \"" + file.GetFilename() + "\"");
	}
	file >> path;
	return std::filesystem::exists(std::filesystem::path(m_base_path / path));
}
