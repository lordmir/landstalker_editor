#ifndef _SCRIPT_DATA_H_
#define _SCRIPT_DATA_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/script/include/Script.h>
#include <landstalker/script/include/ScriptTable.h>
#include <landstalker/script/include/ScriptFunction.h>

class ScriptData : public DataManager
{
public:

    ScriptData(const std::filesystem::path& asm_file);
    ScriptData(const Rom& rom);

    virtual ~ScriptData() {}

    virtual bool Save(const std::filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    static std::wstring GetScriptEntryDisplayName(int script_id);
    static std::wstring GetFlagDisplayName(int script_id);
    static std::wstring GetCutsceneDisplayName(int script_id);

    uint16_t GetStringStart() const;

    std::shared_ptr<Script> GetScript();
    std::shared_ptr<const Script> GetScript() const;

    bool HasTables() const;
    std::shared_ptr<const std::vector<ScriptTable::Action>> GetCharTable() const;
    std::shared_ptr<std::vector<ScriptTable::Action>> GetCharTable();
    std::shared_ptr<const std::vector<ScriptTable::Action>> GetCutsceneTable() const;
    std::shared_ptr<std::vector<ScriptTable::Action>> GetCutsceneTable();
    std::shared_ptr<const std::vector<ScriptTable::Shop>> GetShopTable() const;
    std::shared_ptr<std::vector<ScriptTable::Shop>> GetShopTable();
    std::shared_ptr<const std::vector<ScriptTable::Item>> GetItemTable() const;
    std::shared_ptr<std::vector<ScriptTable::Item>> GetItemTable();

    std::shared_ptr<const ScriptFunctionTable> GetCharFuncs() const;
    std::shared_ptr<ScriptFunctionTable> GetCharFuncs();
    void SetCharFuncs(const ScriptFunctionTable& funcs);
    std::shared_ptr<const ScriptFunctionTable> GetCutsceneFuncs() const;
    std::shared_ptr<ScriptFunctionTable> GetCutsceneFuncs();
    void SetCutsceneFuncs(const ScriptFunctionTable& funcs);
    std::shared_ptr<const ScriptFunctionTable> GetShopFuncs() const;
    std::shared_ptr<ScriptFunctionTable> GetShopFuncs();
    void SetShopFuncs(const ScriptFunctionTable& funcs);
    std::shared_ptr<const ScriptFunctionTable> GetItemFuncs() const;
    std::shared_ptr<ScriptFunctionTable> GetItemFuncs();
    void SetItemFuncs(ScriptFunctionTable& funcs); 
    std::shared_ptr<const ScriptFunctionTable> GetProgressFlagsFuncs() const;
    std::shared_ptr<ScriptFunctionTable> GetProgressFlagsFuncs();
    void SetProgressFlagsFuncs(const ScriptFunctionTable& funcs);
protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const std::filesystem::path& dir);
    void InitCache();

    bool AsmLoadScript();
    bool AsmLoadScriptTables();
    bool AsmLoadScriptFunctions();

    bool RomLoadScript(const Rom& rom);

    bool AsmSaveScript(const std::filesystem::path& dir);
    bool AsmSaveScriptTables(const std::filesystem::path& dir);
    bool AsmSaveScriptFunctions(const std::filesystem::path& dir);

    bool RomPrepareInjectScript(const Rom& rom);

    std::filesystem::path m_defines_filename;
    std::filesystem::path m_script_filename;
    std::filesystem::path m_cutscene_table_filename;
    std::filesystem::path m_char_table_filename;
    std::filesystem::path m_shop_table_filename;
    std::filesystem::path m_item_table_filename;

    std::filesystem::path m_char_funcs_filename;
    std::filesystem::path m_cutscene_funcs_filename;
    std::filesystem::path m_shop_funcs_filename;
    std::filesystem::path m_item_funcs_filename;
    std::filesystem::path m_flag_progress_filename;

    std::map<std::string, std::string> m_defines;
    std::shared_ptr<Script> m_script;
    Script m_script_orig;
    uint16_t m_script_start;
    bool m_is_asm;

    std::shared_ptr<std::vector<ScriptTable::Action>> m_chartable;
    std::shared_ptr<std::vector<ScriptTable::Action>> m_chartable_orig;
    std::shared_ptr<std::vector<ScriptTable::Action>> m_cutscene_table;
    std::shared_ptr<std::vector<ScriptTable::Action>> m_cutscene_table_orig;
    std::shared_ptr<std::vector<ScriptTable::Shop>> m_shoptable;
    std::shared_ptr<std::vector<ScriptTable::Shop>> m_shoptable_orig;
    std::shared_ptr<std::vector<ScriptTable::Item>> m_itemtable;
    std::shared_ptr<std::vector<ScriptTable::Item>> m_itemtable_orig;

    std::shared_ptr<ScriptFunctionTable> m_charfuncs;
    std::shared_ptr<ScriptFunctionTable> m_charfuncs_orig;
    std::shared_ptr<ScriptFunctionTable> m_cutscenefuncs;
    std::shared_ptr<ScriptFunctionTable> m_cutscenefuncs_orig;
    std::shared_ptr<ScriptFunctionTable> m_shopfuncs;
    std::shared_ptr<ScriptFunctionTable> m_shopfuncs_orig;
    std::shared_ptr<ScriptFunctionTable> m_itemfuncs;
    std::shared_ptr<ScriptFunctionTable> m_itemfuncs_orig;
    std::shared_ptr<ScriptFunctionTable> m_flagprogress;
    std::shared_ptr<ScriptFunctionTable> m_flagprogress_orig;
};

#endif // _SCRIPT_DATA_H_