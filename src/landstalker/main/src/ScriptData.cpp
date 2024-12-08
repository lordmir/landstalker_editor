#include <landstalker/main/include/ScriptData.h>

#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/RomLabels.h>
#include <landstalker/misc/include/Literals.h>
#include <landstalker/misc/include/Labels.h>

ScriptData::ScriptData(const std::filesystem::path& asm_file)
	: DataManager(asm_file)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadScript())
	{
		throw std::runtime_error(std::string("Unable to load script from \'") + m_script_filename.string() + '\'');
	}
	m_script_start = 0x4D;
	InitCache();
}

ScriptData::ScriptData(const Rom& rom)
	: DataManager(rom)
{
	SetDefaultFilenames();
	if (!RomLoadScript(rom))
	{
		throw std::runtime_error(std::string("Unable to load script from ROM"));
	}
	InitCache();
}

bool ScriptData::Save(const std::filesystem::path& dir)
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
	if (!AsmSaveScript(dir))
	{
		throw std::runtime_error(std::string("Unable to save script to \'") + m_script_filename.string() + '\'');
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
	if (m_script_orig != *m_script)
	{
		return true;
	}
	return false;
}

void ScriptData::RefreshPendingWrites(const Rom& rom)
{
	DataManager::RefreshPendingWrites(rom);
	if (!RomPrepareInjectScript(rom))
	{
		throw std::runtime_error(std::string("Unable to prepare script for ROM injection"));
	}
}

std::wstring ScriptData::GetScriptEntryDisplayName(int script_id)
{
	std::wstring label = StrWPrintf("Script%04d", script_id);
	if (Labels::Exists(Labels::C_SCRIPT, script_id))
	{
		label += L" " + *Labels::Get(Labels::C_SCRIPT, script_id);
	}
	return label;
}

std::wstring ScriptData::GetFlagDisplayName(int flag)
{
	std::wstring label = StrWPrintf("[Flag %04d]", flag);
	if (Labels::Exists(Labels::C_FLAGS, flag))
	{
		label += L" (" + *Labels::Get(Labels::C_FLAGS, flag) + L")";
	}
	return label;
}

std::wstring ScriptData::GetCutsceneDisplayName(int cutscene)
{
	std::wstring label = StrWPrintf("Cutscene%03d", cutscene);
	if (Labels::Exists(Labels::C_CUTSCENE, cutscene))
	{
		label += L" " + *Labels::Get(Labels::C_CUTSCENE, cutscene);
	}
	return label;
}

uint16_t ScriptData::GetStringStart() const
{
	return m_script_start;
}

std::shared_ptr<Script> ScriptData::GetScript()
{
	return m_script;
}

std::shared_ptr<const Script> ScriptData::GetScript() const
{
	return m_script;
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
		AsmFile f(GetAsmFilename().string());
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::SCRIPT_SECTION, m_script_filename);
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void ScriptData::SetDefaultFilenames()
{
	if (m_script_filename.empty()) m_script_filename = RomLabels::Script::SCRIPT_FILE;
}

bool ScriptData::CreateDirectoryStructure(const std::filesystem::path& dir)
{
	bool retval = true;

	retval = retval && CreateDirectoryTree(dir / m_script_filename);

	return retval;
}

void ScriptData::InitCache()
{
	m_script_orig = *m_script;
}

bool ScriptData::AsmLoadScript()
{
	std::filesystem::path path = GetBasePath() / m_script_filename;
	m_script = std::make_shared<Script>(ReadBytes(path));
	return true;
}

bool ScriptData::RomLoadScript(const Rom& rom)
{
	uint32_t script_begin = rom.get_section(RomLabels::Script::SCRIPT_SECTION).begin;
	uint32_t script_end = Disasm::ReadOffset16(rom, RomLabels::Script::SCRIPT_END);
	uint32_t script_size = script_end - script_begin;
	m_script = std::make_shared<Script>(rom.read_array<uint8_t>(script_begin, script_size));
	m_script_start = rom.read<uint16_t>(RomLabels::Script::SCRIPT_STRINGS_BEGIN);
	return true;
}

bool ScriptData::AsmSaveScript(const std::filesystem::path& dir)
{
	WriteBytes(m_script->ToBytes(), dir / m_script_filename);
	return true;
}

bool ScriptData::RomPrepareInjectScript(const Rom& /*rom*/)
{
	m_pending_writes.push_back({ RomLabels::Script::SCRIPT_SECTION, std::make_shared<ByteVector>(m_script->ToBytes()) });
	return true;
}
