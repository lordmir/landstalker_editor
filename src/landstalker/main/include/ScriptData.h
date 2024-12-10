#ifndef _SCRIPT_DATA_H_
#define _SCRIPT_DATA_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/script/include/Script.h>
#include <landstalker/script/include/ScriptTable.h>

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

protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const std::filesystem::path& dir);
    void InitCache();

    bool AsmLoadScript();
    bool AsmLoadScriptTables();

    bool RomLoadScript(const Rom& rom);

    bool AsmSaveScript(const std::filesystem::path& dir);
    bool AsmSaveScriptTables(const std::filesystem::path& dir);

    bool RomPrepareInjectScript(const Rom& rom);

    std::filesystem::path m_script_filename;
    std::filesystem::path m_cutscene_table_filename;
    std::filesystem::path m_char_table_filename;
    std::filesystem::path m_shop_table_filename;
    std::filesystem::path m_item_table_filename;

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
};

#endif // _SCRIPT_DATA_H_