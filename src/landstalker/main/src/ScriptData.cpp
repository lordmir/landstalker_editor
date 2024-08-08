#include <landstalker/main/include/ScriptData.h>

#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/RomLabels.h>
#include <landstalker/misc/include/Literals.h>

ScriptData::ScriptData(const filesystem::path& asm_file)
	: DataManager(asm_file)
{
	InitCache();
}

ScriptData::ScriptData(const Rom& rom)
	: DataManager(rom)
{
	SetDefaultFilenames();
	InitCache();
}

bool ScriptData::Save(const filesystem::path& dir)
{
	filesystem::path directory = dir;
	if (directory.exists() && directory.is_file())
	{
		directory = directory.parent_path();
	}
	if (!CreateDirectoryStructure(directory))
	{
		throw std::runtime_error(std::string("Unable to create directory structure at \'") + directory.str() + '\'');
	}
	CommitAllChanges();
	return true;
}

bool ScriptData::Save()
{
	return Save(GetBasePath());
}

bool ScriptData::HasBeenModified() const
{
	return false;
}

void ScriptData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
}

void ScriptData::CommitAllChanges()
{
	m_pending_writes.clear();
}

bool ScriptData::LoadAsmFilenames()
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

void ScriptData::SetDefaultFilenames()
{
}

bool ScriptData::CreateDirectoryStructure(const filesystem::path& dir)
{
	bool retval = true;

	return retval;
}

void ScriptData::InitCache()
{
}
