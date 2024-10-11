#ifndef _SCRIPT_DATA_H_
#define _SCRIPT_DATA_H_

#include <landstalker/main/include/DataManager.h>
#include <landstalker/main/include/DataTypes.h>
#include <landstalker/script/include/Script.h>

class ScriptData : public DataManager
{
public:

    ScriptData(const filesystem::path& asm_file);
    ScriptData(const Rom& rom);

    virtual ~ScriptData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);

    uint16_t GetStringStart() const;

    Script& GetScript();
    const Script& GetScript() const;
protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const filesystem::path& dir);
    void InitCache();

    bool AsmLoadScript();

    bool RomLoadScript(const Rom& rom);

    bool AsmSaveScript(const filesystem::path& dir);

    bool RomPrepareInjectScript(const Rom& rom);

    filesystem::path m_script_filename;

    Script m_script;
    Script m_script_orig;
};

#endif // _SCRIPT_DATA_H_