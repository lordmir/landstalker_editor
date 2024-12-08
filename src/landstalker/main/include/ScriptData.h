#ifndef _SCRIPT_DATA_H_
#define _SCRIPT_DATA_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/script/include/Script.h>

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
protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const std::filesystem::path& dir);
    void InitCache();

    bool AsmLoadScript();

    bool RomLoadScript(const Rom& rom);

    bool AsmSaveScript(const std::filesystem::path& dir);

    bool RomPrepareInjectScript(const Rom& rom);

    std::filesystem::path m_script_filename;

    std::shared_ptr<Script> m_script;
    Script m_script_orig;
    uint16_t m_script_start;
};

#endif // _SCRIPT_DATA_H_