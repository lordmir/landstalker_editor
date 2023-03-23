#ifndef _SPRITE_DATA_H_
#define _SPRITE_DATA_H_

#include "DataManager.h"

class SpriteData : public DataManager
{
public:
	SpriteData(const filesystem::path& asm_file);
	SpriteData(const Rom& rom);

	virtual ~SpriteData();

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
	void InitCache();
};

#endif // _SPRITE_DATA_H_
