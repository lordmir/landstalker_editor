#include <landstalker/main/include/ScriptData.h>

#include <landstalker/main/include/AsmUtils.h>
#include <landstalker/main/include/RomLabels.h>
#include <landstalker/misc/include/Literals.h>
#include <landstalker/misc/include/Labels.h>

ScriptData::ScriptData(const std::filesystem::path& asm_file)
	: DataManager("Script Data", asm_file),
	  m_is_asm(true)
{
	if (!LoadAsmFilenames())
	{
		throw std::runtime_error(std::string("Unable to load file data from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadScript())
	{
		throw std::runtime_error(std::string("Unable to load script from \'") + m_script_filename.string() + '\'');
	}
	if (!AsmLoadScriptTables())
	{
		throw std::runtime_error(std::string("Unable to load script tables from \'") + asm_file.string() + '\'');
	}
	if (!AsmLoadScriptFunctions())
	{
		throw std::runtime_error(std::string("Unable to load script functions from \'") + asm_file.string() + '\'');
	}
	m_script_start = 0x4D;
	InitCache();
}

ScriptData::ScriptData(const Rom& rom)
	: DataManager("Script Data", rom),
	  m_is_asm(false)
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
	if (!AsmSaveScriptTables(dir))
	{
		throw std::runtime_error(std::string("Unable to save script tables to \'") + directory.string() + '\'');
	}
	if (!AsmSaveScriptFunctions(dir))
	{
		throw std::runtime_error(std::string("Unable to save script functions to \'") + directory.string() + '\'');
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
	if (m_is_asm && *m_chartable != *m_chartable_orig)
	{
		return true;
	}
	if (m_is_asm && *m_cutscene_table != *m_cutscene_table_orig)
	{
		return true;
	}
	if (m_is_asm && *m_shoptable != *m_shoptable_orig)
	{
		return true;
	}
	if (m_is_asm && *m_itemtable != *m_itemtable_orig)
	{
		return true;
	}
	if (m_is_asm && *m_charfuncs != *m_charfuncs_orig)
	{
		return true;
	}
	if (m_is_asm && *m_cutscenefuncs != *m_cutscenefuncs_orig)
	{
		return true;
	}
	if (m_is_asm && *m_shopfuncs != *m_shopfuncs_orig)
	{
		return true;
	}
	if (m_is_asm && *m_itemfuncs != *m_itemfuncs_orig)
	{
		return true;
	}
	if (m_is_asm && *m_flagprogress != *m_flagprogress_orig)
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

bool ScriptData::HasTables() const
{
	return m_is_asm;
}

std::shared_ptr<const std::vector<ScriptTable::Action>> ScriptData::GetCharTable() const
{
	return m_chartable;
}

std::shared_ptr<std::vector<ScriptTable::Action>> ScriptData::GetCharTable()
{
	return m_chartable;
}

std::shared_ptr<const std::vector<ScriptTable::Action>> ScriptData::GetCutsceneTable() const
{
	return m_cutscene_table;
}

std::shared_ptr<std::vector<ScriptTable::Action>> ScriptData::GetCutsceneTable()
{
	return m_cutscene_table;
}

std::shared_ptr<const std::vector<ScriptTable::Shop>> ScriptData::GetShopTable() const
{
	return m_shoptable;
}

std::shared_ptr<std::vector<ScriptTable::Shop>> ScriptData::GetShopTable()
{
	return m_shoptable;
}

std::shared_ptr<const std::vector<ScriptTable::Item>> ScriptData::GetItemTable() const
{
	return m_itemtable;
}

std::shared_ptr<std::vector<ScriptTable::Item>> ScriptData::GetItemTable()
{
	return m_itemtable;
}

std::shared_ptr<const ScriptFunctionTable> ScriptData::GetCharFuncs() const
{
	return m_charfuncs;
}

std::shared_ptr<ScriptFunctionTable> ScriptData::GetCharFuncs()
{
	return m_charfuncs;
}

void ScriptData::SetCharFuncs(const ScriptFunctionTable& funcs)
{
	*m_charfuncs = funcs;
}

std::shared_ptr<const ScriptFunctionTable> ScriptData::GetCutsceneFuncs() const
{
	return m_cutscenefuncs;
}

std::shared_ptr<ScriptFunctionTable> ScriptData::GetCutsceneFuncs()
{
	return m_cutscenefuncs;
}

void ScriptData::SetCutsceneFuncs(const ScriptFunctionTable& funcs)
{
	*m_cutscenefuncs = funcs;
}

std::shared_ptr<const ScriptFunctionTable> ScriptData::GetShopFuncs() const
{
	return m_shopfuncs;
}

std::shared_ptr<ScriptFunctionTable> ScriptData::GetShopFuncs()
{
	return m_shopfuncs;
}

void ScriptData::SetShopFuncs(const ScriptFunctionTable& funcs)
{
	*m_shopfuncs = funcs;
}

std::shared_ptr<const ScriptFunctionTable> ScriptData::GetItemFuncs() const
{
	return m_itemfuncs;
}

std::shared_ptr<ScriptFunctionTable> ScriptData::GetItemFuncs()
{
	return m_itemfuncs;
}

void ScriptData::SetItemFuncs(ScriptFunctionTable& funcs)
{
	*m_itemfuncs = funcs;
}

std::shared_ptr<const ScriptFunctionTable> ScriptData::GetProgressFlagsFuncs() const
{
	return m_flagprogress;
}

std::shared_ptr<ScriptFunctionTable> ScriptData::GetProgressFlagsFuncs()
{
	return m_flagprogress;
}

void ScriptData::SetProgressFlagsFuncs(const ScriptFunctionTable& funcs)
{
	*m_flagprogress = funcs;
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
		retval = retval && GetFilenameFromAsm(f, RomLabels::DEFINES_SECTION, m_defines_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::SCRIPT_SECTION, m_script_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::CUTSCENE_TABLE_SECTION, m_cutscene_table_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::CHAR_TABLE_SECTION, m_char_table_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::SHOP_TABLE_SECTION, m_shop_table_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::ITEM_TABLE_SECTION, m_item_table_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::CHAR_FUNCS_SECTION, m_char_funcs_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::CUTSCENE_FUNCS_SECTION, m_cutscene_funcs_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::SHOP_FUNCS_SECTION, m_shop_funcs_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::ITEM_FUNCS_SECTION, m_item_funcs_filename);
		retval = retval && GetFilenameFromAsm(f, RomLabels::Script::FLAG_PROGRESS_SECTION, m_flag_progress_filename);
		return retval;
	}
	catch (...)
	{
	}
	return false;
}

void ScriptData::SetDefaultFilenames()
{
	if (m_defines_filename.empty()) m_defines_filename = RomLabels::DEFINES_FILE;
	if (m_script_filename.empty()) m_script_filename = RomLabels::Script::SCRIPT_FILE;
	if (m_cutscene_table_filename.empty()) m_cutscene_table_filename = RomLabels::Script::CUTSCENE_TABLE_FILE;
	if (m_char_table_filename.empty()) m_char_table_filename = RomLabels::Script::CHAR_TABLE_FILE;
	if (m_shop_table_filename.empty()) m_shop_table_filename = RomLabels::Script::SHOP_TABLE_FILE;
	if (m_item_table_filename.empty()) m_item_table_filename = RomLabels::Script::ITEM_TABLE_FILE;
	if (m_char_funcs_filename.empty()) m_char_funcs_filename = RomLabels::Script::CHAR_FUNCS_FILE;
	if (m_cutscene_funcs_filename.empty()) m_cutscene_funcs_filename = RomLabels::Script::CUTSCENE_FUNCS_FILE;
	if (m_shop_funcs_filename.empty()) m_shop_funcs_filename = RomLabels::Script::SHOP_FUNCS_FILE;
	if (m_item_funcs_filename.empty()) m_item_funcs_filename = RomLabels::Script::ITEM_FUNCS_FILE;
	if (m_flag_progress_filename.empty()) m_flag_progress_filename = RomLabels::Script::FLAG_PROGRESS_FILE;
}

bool ScriptData::CreateDirectoryStructure(const std::filesystem::path& dir)
{
	bool retval = true;

	retval = retval && CreateDirectoryTree(dir / m_script_filename);
	if (m_is_asm)
	{
		retval = retval && CreateDirectoryTree(dir / m_cutscene_table_filename);
		retval = retval && CreateDirectoryTree(dir / m_char_table_filename);
		retval = retval && CreateDirectoryTree(dir / m_shop_table_filename);
		retval = retval && CreateDirectoryTree(dir / m_item_table_filename);
		retval = retval && CreateDirectoryTree(dir / m_char_funcs_filename);
		retval = retval && CreateDirectoryTree(dir / m_cutscene_funcs_filename);
		retval = retval && CreateDirectoryTree(dir / m_shop_funcs_filename);
		retval = retval && CreateDirectoryTree(dir / m_item_funcs_filename);
		retval = retval && CreateDirectoryTree(dir / m_flag_progress_filename);
	}

	return retval;
}

void ScriptData::InitCache()
{
	m_script_orig = *m_script;
	if (m_is_asm)
	{
		m_cutscene_table_orig = std::make_shared<std::vector<ScriptTable::Action>>(*m_cutscene_table);
		m_chartable_orig = std::make_shared<std::vector<ScriptTable::Action>>(*m_chartable);
		m_shoptable_orig = std::make_shared<std::vector<ScriptTable::Shop>>(*m_shoptable);
		m_itemtable_orig = std::make_shared<std::vector<ScriptTable::Item>>(*m_itemtable);
		m_charfuncs_orig = std::make_shared<ScriptFunctionTable>(*m_charfuncs);
		m_cutscenefuncs_orig = std::make_shared<ScriptFunctionTable>(*m_cutscenefuncs);
		m_shopfuncs_orig = std::make_shared<ScriptFunctionTable>(*m_shopfuncs);
		m_itemfuncs_orig = std::make_shared<ScriptFunctionTable>(*m_itemfuncs);
		m_flagprogress_orig = std::make_shared<ScriptFunctionTable>(*m_flagprogress);
	}
}

bool ScriptData::AsmLoadScript()
{
	std::filesystem::path path = GetBasePath() / m_script_filename;
	m_script = std::make_shared<Script>(ReadBytes(path));
	return true;
}

bool ScriptData::AsmLoadScriptTables()
{
	m_defines = AsmFile::ParseDefines((GetBasePath() / m_defines_filename).string());
	m_cutscene_table = ScriptTable::ReadTable((GetBasePath() / m_cutscene_table_filename).string(), m_defines);
	m_chartable = ScriptTable::ReadTable((GetBasePath() / m_char_table_filename).string(), m_defines);
	m_shoptable = ScriptTable::ReadShopTable((GetBasePath() / m_shop_table_filename).string(), m_defines);
	m_itemtable = ScriptTable::ReadItemTable((GetBasePath() / m_item_table_filename).string(), m_defines);
	return true;
}

bool ScriptData::AsmLoadScriptFunctions()
{
	m_charfuncs = std::make_shared<ScriptFunctionTable>(AsmFile(GetBasePath() / m_char_funcs_filename, m_defines));
	m_cutscenefuncs = std::make_shared<ScriptFunctionTable>(AsmFile(GetBasePath() / m_cutscene_funcs_filename, m_defines));
	m_shopfuncs = std::make_shared<ScriptFunctionTable>(AsmFile(GetBasePath() / m_shop_funcs_filename, m_defines));
	m_itemfuncs = std::make_shared<ScriptFunctionTable>(AsmFile(GetBasePath() / m_item_funcs_filename, m_defines));
	m_flagprogress = std::make_shared<ScriptFunctionTable>(AsmFile(GetBasePath() / m_flag_progress_filename, m_defines));
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

bool ScriptData::AsmSaveScriptTables(const std::filesystem::path& dir)
{
	bool retval = true;
	retval = retval && ScriptTable::WriteTable(dir, m_cutscene_table_filename, "Cutscene Script Table", m_cutscene_table);
	retval = retval && ScriptTable::WriteTable(dir, m_char_table_filename, "Character Script Table", m_chartable);
	retval = retval && ScriptTable::WriteShopTable(dir, m_shop_table_filename, "Shop Script Table", m_shoptable);
	retval = retval && ScriptTable::WriteItemTable(dir, m_item_table_filename, "Item Script Table", m_itemtable);
	return retval;
}

bool ScriptData::AsmSaveScriptFunctions(const std::filesystem::path& dir)
{
	bool retval = true;
	AsmFile char_funcs_asm;
	AsmFile cutscene_funcs_asm;
	AsmFile shop_funcs_asm;
	AsmFile item_funcs_asm;
	AsmFile flag_progress_asm;

	char_funcs_asm.WriteFileHeader(m_char_funcs_filename, "Character Script Functions");
	retval = retval && m_charfuncs->WriteAsm(char_funcs_asm);
	retval = retval && char_funcs_asm.WriteFile(dir / m_char_funcs_filename);

	cutscene_funcs_asm.WriteFileHeader(m_cutscene_funcs_filename, "Cutscene Script Functions");
	retval = retval && m_cutscenefuncs->WriteAsm(cutscene_funcs_asm);
	retval = retval && cutscene_funcs_asm.WriteFile(dir / m_cutscene_funcs_filename);

	shop_funcs_asm.WriteFileHeader(m_shop_funcs_filename, "Shop Script Functions");
	retval = retval && m_shopfuncs->WriteAsm(shop_funcs_asm);
	retval = retval && shop_funcs_asm.WriteFile(dir / m_shop_funcs_filename);

	item_funcs_asm.WriteFileHeader(m_item_funcs_filename, "Special Shop Item Script Functions");
	retval = retval && m_itemfuncs->WriteAsm(item_funcs_asm);
	retval = retval && item_funcs_asm.WriteFile(dir / m_item_funcs_filename);

	flag_progress_asm.WriteFileHeader(m_flag_progress_filename, "Quest Progress Flag Mapping");
	retval = retval && m_flagprogress->WriteAsm(flag_progress_asm);
	retval = retval && flag_progress_asm.WriteFile(dir / m_flag_progress_filename);

	return retval;
}

bool ScriptData::RomPrepareInjectScript(const Rom& /*rom*/)
{
	m_pending_writes.push_back({ RomLabels::Script::SCRIPT_SECTION, std::make_shared<ByteVector>(m_script->ToBytes()) });
	return true;
}
