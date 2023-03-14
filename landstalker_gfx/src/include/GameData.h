#ifndef _GAME_DATA_H_
#define _GAME_DATA_H_

#include "DataManager.h"
#include "RoomData.h"
#include "GraphicsData.h"

#include <vector>
#include <memory>

class GameData : public DataManager
{
public:
    GameData(const filesystem::path& asm_file);
    GameData(const Rom& rom);

    virtual ~GameData() {}

    virtual bool Save(const filesystem::path& dir);
    virtual bool Save();

    virtual PendingWrites GetPendingWrites() const;
    virtual bool WillFitInRom(const Rom& rom) const;
    virtual bool HasBeenModified() const;
    virtual bool InjectIntoRom(Rom& rom);
    virtual void RefreshPendingWrites(const Rom& rom);

    std::shared_ptr<RoomData> GetRoomData() const { return m_rd; }
    std::shared_ptr<GraphicsData> GetGraphicsData() const { return m_gd; }

private:
    std::shared_ptr<RoomData> m_rd;
    std::shared_ptr<GraphicsData> m_gd;

    std::vector<std::shared_ptr<DataManager>> m_data;
};

#endif // _GAME_DATA_H_
