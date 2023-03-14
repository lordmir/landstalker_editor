#ifndef _GRAPHICS_DATA_H_
#define _GRAPHICS_DATA_H_

#include "DataManager.h"

class GraphicsData : public DataManager
{
public:
    GraphicsData(const filesystem::path& asm_file);
    GraphicsData(const Rom& rom);

    virtual ~GraphicsData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

    virtual bool HasBeenModified() const;
    virtual void RefreshPendingWrites(const Rom& rom);
protected:
    virtual void CommitAllChanges();
private:
    bool LoadAsmFilenames();
    void SetDefaultFilenames();
    bool CreateDirectoryStructure(const filesystem::path& dir);
};

#endif // _GRAPHICS_DATA_H_
