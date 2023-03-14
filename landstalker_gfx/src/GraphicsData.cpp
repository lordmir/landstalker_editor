#include "GraphicsData.h"

GraphicsData::GraphicsData(const filesystem::path& asm_file)
	: DataManager(asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.str() + '\'');
	}
}

GraphicsData::GraphicsData(const Rom& rom)
	: DataManager(rom)
{
	SetDefaultFilenames();
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
	return false;
}

void GraphicsData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
}

void GraphicsData::CommitAllChanges()
{
	auto entry_commit = [](const auto& e) {return e->Commit(); };
	auto pair_commit = [](const auto& e) {return e.second->Commit(); };
	m_pending_writes.clear();
}

bool GraphicsData::LoadAsmFilenames()
{
	try
	{
		bool retval = true;
		AsmFile f(GetAsmFilename().str());
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void GraphicsData::SetDefaultFilenames()
{
}

bool GraphicsData::CreateDirectoryStructure(const filesystem::path& dir)
{
	bool retval = true;
	return true;
}
